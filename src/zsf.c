#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"
#include "zsf.h"

// The zsf_calculate loop can take advantage of shared values (e.g. a
// reciprocal volume) between steps and the derivative parameters. Most
// compilers cannot seem to recognize the ~20% speedup that can be gained this
// way, so we have to force it.
#ifdef _MSC_VER
#  define forceinline __forceinline
#elif defined(__GNUC__)
#  define forceinline inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#  if __has_attribute(__always_inline__)
#    define forceinline inline __attribute__((__always_inline__))
#  else
#    define forceinline inline
#  endif
#else
#  define forceinline inline
#endif

#ifdef ZSF_USE_FAST_TANH
inline double TANH(const double x) {
  const double ax = fabs(x);
  const double x2 = x * x;

  const double z1 =
      (x *
       (2.45550750702956 + 2.45550750702956 * ax +
        (0.893229853513558 + 0.821226666969744 * ax) * x2) /
       (2.44506634652299 + (2.44506634652299 + x2) * fabs(x + 0.814642734961073 * x * ax)));

  return fmin(z1, 1.0);
}
#else
#  define TANH tanh
#endif

#define ERROR_CODES(X)                                                                             \
  X(ZSF_SUCCESS, "Success")                                                                        \
  X(ZSF_SHIP_TOO_BIG, "The ship is too large for the lock")                                        \
  X(ZSF_ERR_REMAINING_HEAD_DIFF, "Remaining head difference when opening doors")                   \
  X(ZSF_ERR_SAL_LOCK_OUT_OF_BOUNDS, "The salinity of the lock exceeds that of the boundaries")

#define ERROR_ENUM(ID, TEXT) ID,
enum error_ids { ERROR_CODES(ERROR_ENUM) ZSF_NUM_ERRORS };
#undef ERROR_ENUM

#define ERROR_TEXT(ID, TEXT)                                                                       \
  case ID:                                                                                         \
    return TEXT;
const char *ZSF_CALLCONV zsf_error_msg(int code) {
  switch (code) { ERROR_CODES(ERROR_TEXT) }
  return "Unknown error";
}
#undef ERROR_TEXT
#undef ERROR_CODES

typedef struct derived_parameters_t {
  double g;
  int is_high_tide;
  int is_low_tide;
  double volume_lock_at_sea;
  double volume_lock_at_lake;
  double t_cycle;
  double t_open_avg;
  double t_open;
  double t_open_lake;
  double t_open_sea;
  double flushing_discharge;
  double density_average;
} derived_parameters_t;

const char *ZSF_CALLCONV zsf_version() { return ZSF_GIT_DESCRIBE; }

static forceinline void calculate_derived_parameters(const zsf_param_t *p,
                                                     derived_parameters_t *o) {
  // Gravitational constant
  o->g = 9.81;

  // Calculate derived parameters
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Tide signal
  o->is_high_tide = p->head_sea >= p->head_lake;
  o->is_low_tide = 1 - o->is_high_tide;

  // Volumes
  o->volume_lock_at_sea = p->lock_length * p->lock_width * (p->head_sea - p->lock_bottom);
  o->volume_lock_at_lake = p->lock_length * p->lock_width * (p->head_lake - p->lock_bottom);

  // Door open times
  o->t_cycle = 24.0 * 3600.0 / p->num_cycles;
  o->t_open_avg = 0.5 * o->t_cycle - (p->leveling_time + 2.0 * 0.5 * p->door_time_to_open);
  o->t_open = p->calibration_coefficient * o->t_open_avg;
  o->t_open_lake = p->symmetry_coefficient * o->t_open;
  o->t_open_sea = (2.0 - p->symmetry_coefficient) * o->t_open;

  // Flushing discharge
  o->flushing_discharge =
      o->is_low_tide ? p->flushing_discharge_low_tide : p->flushing_discharge_high_tide;

  // Average density (for lock exchange)
  o->density_average = 0.5 * (sal_2_density(p->sal_lake, p->temperature, p->rtol, p->atol) +
                              sal_2_density(p->sal_sea, p->temperature, p->rtol, p->atol));
}

static int check_parameters_state(const zsf_param_t *p, const derived_parameters_t *o,
                                  const zsf_phase_state_t *state) {

  if (fmax(p->ship_volume_lake_to_sea, p->ship_volume_sea_to_lake) >
      fmin(o->volume_lock_at_lake, o->volume_lock_at_sea)) {
    return ZSF_SHIP_TOO_BIG;
  }
  if ((state->sal_lock > fmax(p->sal_lake, p->sal_sea)) ||
      (state->sal_lock < fmin(p->sal_lake, p->sal_sea))) {
    return ZSF_ERR_SAL_LOCK_OUT_OF_BOUNDS;
  }

  return ZSF_SUCCESS;
}

void ZSF_CALLCONV zsf_param_default(zsf_param_t *p) {
  /* */
  memset(p, 0, sizeof(zsf_param_t));

  // Lock properties
  p->lock_length = 100.0;
  p->lock_width = 10.0;
  p->lock_bottom = -5.0;

  // Operation properties
  p->num_cycles = 12.0;
  p->door_time_to_open = 300.0;
  p->leveling_time = 300.0;
  p->symmetry_coefficient = 1.0;
  p->ship_volume_sea_to_lake = 0.0;
  p->ship_volume_lake_to_sea = 0.0;
  p->calibration_coefficient = 1.0;

  // Salt intrusion counter-measures
  p->flushing_discharge_high_tide = 0.0;
  p->flushing_discharge_low_tide = 0.0;
  p->density_current_factor_sea = 1.0;
  p->density_current_factor_lake = 1.0;

  // Initial condition
  p->sal_lock = ZSF_NAN;

  // Boundary conditions
  p->head_sea = 0.0;
  p->sal_sea = 30.0;
  p->head_lake = 0.0;
  p->sal_lake = 1.0;
  p->temperature = 15.0;

  // Convergence criterion
  p->rtol = 1E-5;
  p->atol = 1E-8;
}

static forceinline void step_phase_1(const zsf_param_t *p, const derived_parameters_t *o,
                                     double t_level, zsf_phase_state_t *state,
                                     zsf_phase_transports_t *results) {
  // Phase 1: Leveling lock to lake side
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //              Low Tide                                       High Tide
  //
  //      Lake                          Sea              Lake                          Sea
  //                |             |                                |--\   ↓   /--|------------
  //    ------------|             |                    ------------|   \_____/   |
  //                |--\   ↑   /--|------------                    |             |
  //                →   \_____/   |                                ←             |
  //    ____________|_____________|____________        ____________|_____________|____________
  //
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  double saltmass_lock_4 = state->saltmass_lock;
  double sal_lock_4 = state->sal_lock;
  double volume_ship_in_lock_4 = state->volume_ship_in_lock;

  // Leveling
  double vol_lake_in = fmax(state->head_lock - p->head_lake, 0.0) * p->lock_width * p->lock_length;
  double vol_lake_out = fmax(p->head_lake - state->head_lock, 0.0) * p->lock_width * p->lock_length;
  double mt_lake_1 = vol_lake_out * p->sal_lake - vol_lake_in * sal_lock_4;

  // Update the results
  results->mass_transport_lake = mt_lake_1;
  results->volume_lake_out = vol_lake_out;
  results->volume_lake_in = vol_lake_in;
  results->discharge_lake_out = vol_lake_out / t_level;
  results->discharge_lake_in = vol_lake_in / t_level;
  results->salinity_lake_in = sal_lock_4;

  results->mass_transport_sea = 0.0;
  results->volume_sea_out = 0.0;
  results->volume_sea_in = 0.0;
  results->discharge_sea_out = 0.0;
  results->discharge_sea_in = 0.0;
  results->salinity_sea_in = sal_lock_4;

  // Update state variables of the lock
  double saltmass_lock_1 = saltmass_lock_4 + mt_lake_1;
  state->saltmass_lock = saltmass_lock_1;
  state->sal_lock = saltmass_lock_1 / (o->volume_lock_at_lake - volume_ship_in_lock_4);
  state->head_lock = p->head_lake;
  // state->volume_ship_in_lock = state->volume_ship_in_lock;  /* Unchanged */
}

static forceinline void step_phase_2(const zsf_param_t *p, const derived_parameters_t *o,
                                     double t_open_lake, zsf_phase_state_t *state,
                                     zsf_phase_transports_t *results) {
  // Phase 2: Gate opening at lake side
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //              Low Tide                                       High Tide
  //
  //      Lake                          Sea             Lake                          Sea
  //                              |                                              |------------
  //    --------\  <->  /---------|                    --------\  <->  /---------|
  //             \_____/          |------------                 \_____/          |
  //                '             → flushing                       '             → flushing
  //    ____________'_____________|____________        ____________'_____________|____________
  //
  // Consists of three subphases:
  // a. Ships exiting lock
  // b. Lock exchange + flushing
  // c. Ships entering lock
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  double saltmass_lock_1 = state->saltmass_lock;
  double sal_lock_1 = state->sal_lock;
  double volume_ship_in_lock_1 = state->volume_ship_in_lock;

  // Subphase a. Ships exiting the lock chamber towards the lake
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double mt_lake_2_ship_exit = volume_ship_in_lock_1 * p->sal_lake;

  // Update state variables of the lock
  double saltmass_lock_2a = saltmass_lock_1 + mt_lake_2_ship_exit;
  double sal_lock_2a = saltmass_lock_2a / o->volume_lock_at_lake;

  // Subphase b. Flushing compensated lock exchange
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double velocity_flushing =
      o->flushing_discharge / (p->lock_width * (p->head_lake - p->lock_bottom));

  double sal_diff = sal_lock_2a - p->sal_lake;
  double velocity_exchange_raw =
      0.5 * sqrt(o->g * 0.8 * sal_diff / o->density_average * (p->head_lake - p->lock_bottom));
  double velocity_exchange_eta = p->density_current_factor_lake * velocity_exchange_raw;
  double frac_lock_exchange =
      fmax((velocity_exchange_eta - velocity_flushing) / velocity_exchange_eta, 0.0);
  double t_lock_exchange = 2 * p->lock_length / velocity_exchange_eta;
  double volume_exchange_2 =
      frac_lock_exchange * o->volume_lock_at_lake * TANH(t_open_lake / t_lock_exchange);

  double mt_lake_2_lock_exchange = (p->sal_lake - sal_lock_2a) * volume_exchange_2;

  // Flushing itself (taking lock exchange into account)
  double volume_flush = o->flushing_discharge * t_open_lake;

  // Max volume that will lead to the lock being refreshed (before we
  // reach steady state where we are flushing to the sea with salinity of
  // lake)
  double max_volume_flush_refresh = o->volume_lock_at_lake - volume_exchange_2;

  double volume_flush_refresh = fmin(volume_flush, max_volume_flush_refresh);
  double volume_flush_passthrough = fmax(volume_flush - max_volume_flush_refresh, 0.0);

  double mt_lake_2_flushing =
      volume_flush_refresh * p->sal_lake + volume_flush_passthrough * p->sal_lake;
  double mt_sea_2_flushing =
      volume_flush_refresh * sal_lock_2a + volume_flush_passthrough * p->sal_lake;

  // Update state variables of the lock
  double saltmass_lock_2b =
      saltmass_lock_2a + mt_lake_2_flushing + mt_lake_2_lock_exchange - mt_sea_2_flushing;
  double sal_lock_2b = saltmass_lock_2b / o->volume_lock_at_lake;

  // Subphase c. Ship entering the lock chamber from the lake
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double mt_lake_2_ship_enter = -1 * p->ship_volume_lake_to_sea * sal_lock_2b;

#ifndef NDEBUG
  // These variables are only needed for the assertion later on
  // Update state variables of the lock
  double saltmass_lock_2c = saltmass_lock_2b + mt_lake_2_ship_enter;
  double sal_lock_2c = saltmass_lock_2c / (o->volume_lock_at_lake - p->ship_volume_lake_to_sea);
#endif

  // Totals for Phase 2
  // ~~~~~~~~~~~~~~~~~~
  // Total mass transports over both gates
  double mt_lake_2 =
      mt_lake_2_ship_exit + mt_lake_2_ship_enter + mt_lake_2_flushing + mt_lake_2_lock_exchange;
  double mt_sea_2 = mt_sea_2_flushing;

  // Update state variables of the lock
  double saltmass_lock_2 = saltmass_lock_1 + mt_lake_2 - mt_sea_2;
  double sal_lock_2 = saltmass_lock_2 / (o->volume_lock_at_lake - p->ship_volume_lake_to_sea);

  assert(fabs(saltmass_lock_2 - saltmass_lock_2c) < 1E-8);
  assert(fabs(sal_lock_2 - sal_lock_2c) < 1E-8);

  assert((sal_lock_2 >= p->sal_lake) & (sal_lock_2 <= p->sal_sea));

  // Update the results
  results->mass_transport_lake = mt_lake_2;
  results->volume_lake_out =
      volume_ship_in_lock_1 + volume_exchange_2 + o->flushing_discharge * t_open_lake;
  results->volume_lake_in = volume_exchange_2 + p->ship_volume_lake_to_sea;
  results->discharge_lake_out = results->volume_lake_out / t_open_lake;
  results->discharge_lake_in = results->volume_lake_in / t_open_lake;
  results->salinity_lake_in =
      -1 * (mt_lake_2 - results->volume_lake_out * p->sal_lake) / results->volume_lake_in;

  results->mass_transport_sea = mt_sea_2;
  results->volume_sea_out = 0.0;
  results->volume_sea_in = o->flushing_discharge * t_open_lake;
  results->discharge_sea_out = 0.0;
  results->discharge_sea_in = o->flushing_discharge;
  results->salinity_sea_in =
      (results->volume_sea_in > 0.0) ? mt_sea_2 / results->volume_sea_in : sal_lock_1;

  // Update state variables of the lock
  state->saltmass_lock = saltmass_lock_2;
  state->sal_lock = sal_lock_2;
  // state->head_lock = state->head_lock;  /* Unchanged */
  state->volume_ship_in_lock = p->ship_volume_lake_to_sea;
}

static forceinline void step_phase_3(const zsf_param_t *p, const derived_parameters_t *o,
                                     double t_level, zsf_phase_state_t *state,
                                     zsf_phase_transports_t *results) {
  // Phase 3: Leveling lock to sea side
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //              Low Tide                                       High Tide
  //
  //      Lake                          Sea              Lake                          Sea
  //                |             |                                |             |------------
  //    ------------|--\   ↓   /--|                    ------------|--\   ↑   /--|
  //                |   \_____/   |------------                    |   \_____/   |
  //                |             →                                |             ←
  //    ____________|_____________|____________        ____________|_____________|____________
  //
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  double saltmass_lock_2 = state->saltmass_lock;
  double sal_lock_2 = state->sal_lock;
  double volume_ship_in_lock_2 = state->volume_ship_in_lock;

  // Leveling
  double vol_sea_in = fmax(state->head_lock - p->head_sea, 0.0) * p->lock_width * p->lock_length;
  double vol_sea_out = fmax(p->head_sea - state->head_lock, 0.0) * p->lock_width * p->lock_length;
  double mt_sea_3 = vol_sea_in * sal_lock_2 - vol_sea_out * p->sal_sea;

  // Update the results
  results->mass_transport_lake = 0.0;
  results->volume_lake_out = 0.0;
  results->volume_lake_in = 0.0;
  results->discharge_lake_out = 0.0;
  results->discharge_lake_in = 0.0;
  results->salinity_lake_in = sal_lock_2;

  results->mass_transport_sea = mt_sea_3;
  results->volume_sea_out = vol_sea_out;
  results->volume_sea_in = vol_sea_in;
  results->discharge_sea_out = vol_sea_out / t_level;
  results->discharge_sea_in = vol_sea_in / t_level;
  results->salinity_sea_in = sal_lock_2;

  // Update state variables of the lock
  double saltmass_lock_3 = saltmass_lock_2 - mt_sea_3;
  state->saltmass_lock = saltmass_lock_3;
  state->sal_lock = saltmass_lock_3 / (o->volume_lock_at_sea - volume_ship_in_lock_2);
  state->head_lock = p->head_sea;
  // state->volume_ship_in_lock = state->volume_ship_in_lock;  /* Unchanged */
}

static forceinline void step_phase_4(const zsf_param_t *p, const derived_parameters_t *o,
                                     double t_open_sea, zsf_phase_state_t *state,
                                     zsf_phase_transports_t *results) {
  // Phase 4: Gate opening at sea side
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //              Low Tide                                       High Tide
  //
  //      Lake                          Sea              Lake                          Sea
  //                |                                              |---------\  <->  /--------
  //    ------------|                                  ------------|          \_____/
  //                |---------\  <->  /--------                    |             '
  //                |          \_____/                             |             '
  //    ____________|_____________'____________        ____________|_____________'____________
  //
  // Consists of three subphases:
  // a. Ships exiting lock
  // b. Lock exchange + flushing
  // c. Ships entering lock
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  double saltmass_lock_3 = state->saltmass_lock;
  double sal_lock_3 = state->sal_lock;
  double volume_ship_in_lock_3 = state->volume_ship_in_lock;

  // Subphase a. Ships exiting the lock chamber towards the sea
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double mt_sea_4_ship_exit = -1 * volume_ship_in_lock_3 * p->sal_sea;

  // Update state variables of the lock
  double saltmass_lock_4a = saltmass_lock_3 - mt_sea_4_ship_exit;
  double sal_lock_4a = saltmass_lock_4a / o->volume_lock_at_sea;

  // Subphase b. Flushing compensated lock exchange
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double velocity_flushing =
      o->flushing_discharge / (p->lock_width * (p->head_sea - p->lock_bottom));

  double sal_diff = p->sal_sea - sal_lock_4a;
  double velocity_exchange_raw =
      0.5 * sqrt(o->g * 0.8 * sal_diff / o->density_average * (p->head_sea - p->lock_bottom));
  double velocity_exchange_eta = p->density_current_factor_sea * velocity_exchange_raw;

  // The equilibrium depth of the boundary layer between the salt (sal_sea)
  // and fresh (sal_lake) water when flushing for a very long time.
  double head_equilibrium = cbrt(2.0 * pow(o->flushing_discharge, 2.0) * o->density_average /
                                 (o->g * 0.8 * (p->sal_sea - p->sal_lake)));

  head_equilibrium = fmin(head_equilibrium, p->head_sea - p->lock_bottom);

  double frac_lock_exchange =
      (p->head_sea - p->lock_bottom - head_equilibrium) / (p->head_sea - p->lock_bottom);

  // If we flush so much that the density current never enters the lock, we
  // might get division by zero. Avoid by branching such that we can still
  // use fast math (which typically does not work with non-finite values).
  double volume_exchange_4 = 0.0;

  if (velocity_exchange_eta > velocity_flushing) {
    double t_lock_exchange =
        2 * p->lock_length * frac_lock_exchange / (velocity_exchange_eta - velocity_flushing);
    volume_exchange_4 =
        frac_lock_exchange * o->volume_lock_at_sea * TANH(t_open_sea / t_lock_exchange);
  }

  double mt_sea_4_lock_exchange = (sal_lock_4a - p->sal_sea) * volume_exchange_4;

  // Flushing itself (taking lock exchange into account)
  double volume_flush = o->flushing_discharge * t_open_sea;

  // Max volume that will lead to the lock being refreshed (before we
  // reach steady state where we are flushing to the sea with salinity of
  // lake)
  double max_volume_flush_refresh = o->volume_lock_at_sea - volume_exchange_4;

  double volume_flush_refresh = fmin(volume_flush, max_volume_flush_refresh);
  double volume_flush_passthrough = fmax(volume_flush - max_volume_flush_refresh, 0.0);

  double mt_lake_4_flushing =
      volume_flush_refresh * p->sal_lake + volume_flush_passthrough * p->sal_lake;
  double mt_sea_4_flushing =
      volume_flush_refresh * sal_lock_4a + volume_flush_passthrough * p->sal_lake;

  // Update state variables of the lock
  double saltmass_lock_4b =
      saltmass_lock_4a + mt_lake_4_flushing - mt_sea_4_lock_exchange - mt_sea_4_flushing;
  double sal_lock_4b = saltmass_lock_4b / o->volume_lock_at_sea;

  // Subphase c. Ship entering the lock chamber from the sea
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double mt_sea_4_ship_enter = p->ship_volume_sea_to_lake * sal_lock_4b;

#ifndef NDEBUG
  // These variables are only needed for the assertion later on
  // Update state variables of the lock
  double saltmass_lock_4c = saltmass_lock_4b - mt_sea_4_ship_enter;
  double sal_lock_4c = saltmass_lock_4c / (o->volume_lock_at_sea - p->ship_volume_sea_to_lake);
#endif

  // Totals for Phase 4
  // ~~~~~~~~~~~~~~~~~~
  // Total mass transports over both gates
  double mt_sea_4 =
      mt_sea_4_ship_exit + mt_sea_4_ship_enter + mt_sea_4_flushing + mt_sea_4_lock_exchange;
  double mt_lake_4 = mt_lake_4_flushing;

  // Update state variables of the lock
  double saltmass_lock_4 = saltmass_lock_3 + mt_lake_4 - mt_sea_4;
  double sal_lock_4 = saltmass_lock_4 / (o->volume_lock_at_sea - p->ship_volume_sea_to_lake);

  assert(fabs(saltmass_lock_4 - saltmass_lock_4c) < 1E-8);
  assert(fabs(sal_lock_4 - sal_lock_4c) < 1E-8);

  assert((sal_lock_4 >= p->sal_lake) & (sal_lock_4 <= p->sal_sea));

  // Update the results
  results->mass_transport_lake = mt_lake_4;
  results->volume_lake_out = o->flushing_discharge * t_open_sea;
  results->volume_lake_in = 0.0;
  results->discharge_lake_out = o->flushing_discharge;
  results->discharge_lake_in = 0.0;
  results->salinity_lake_in = sal_lock_3;

  results->mass_transport_sea = mt_sea_4;
  results->volume_sea_out = volume_exchange_4 + volume_ship_in_lock_3;
  results->volume_sea_in =
      volume_exchange_4 + p->ship_volume_sea_to_lake + o->flushing_discharge * t_open_sea;
  results->discharge_sea_out = results->volume_sea_out / t_open_sea;
  results->discharge_sea_in = results->volume_sea_in / t_open_sea;
  results->salinity_sea_in =
      (mt_sea_4 - results->volume_sea_out * p->sal_sea) / results->volume_sea_in;

  // Update state variables of the lock
  state->saltmass_lock = saltmass_lock_4;
  state->sal_lock = sal_lock_4;
  // state->head_lock = state->head_lock;  /* Unchanged */
  state->volume_ship_in_lock = p->ship_volume_sea_to_lake;
}

int ZSF_CALLCONV zsf_initialize_state(const zsf_param_t *p, zsf_phase_state_t *state,
                                      double sal_lock, double head_lock) {
  state->sal_lock = sal_lock;
  state->saltmass_lock = sal_lock * (p->lock_length * p->lock_width * (head_lock - p->lock_bottom));
  state->head_lock = head_lock;
  state->volume_ship_in_lock = 0.0;

  return ZSF_SUCCESS;
}

int ZSF_CALLCONV zsf_step_phase_1(const zsf_param_t *p, double t_level, zsf_phase_state_t *state,
                                  zsf_phase_transports_t *results) {
  // Get the derived parameters
  derived_parameters_t o;
  calculate_derived_parameters(p, &o);

  int err = check_parameters_state(p, &o, state);
  if (err) {
    return err;
  }

  step_phase_1(p, &o, t_level, state, results);

  return ZSF_SUCCESS;
}

int ZSF_CALLCONV zsf_step_phase_2(const zsf_param_t *p, double t_open_lake,
                                  zsf_phase_state_t *state, zsf_phase_transports_t *results) {
  // Get the derived parameters
  derived_parameters_t o;
  calculate_derived_parameters(p, &o);

  int err = check_parameters_state(p, &o, state);
  if (err) {
    return err;
  }
  if (fabs(state->head_lock - p->head_lake) > 1E-8) {
    return ZSF_ERR_REMAINING_HEAD_DIFF;
  }

  step_phase_2(p, &o, t_open_lake, state, results);

  return ZSF_SUCCESS;
}

int ZSF_CALLCONV zsf_step_phase_3(const zsf_param_t *p, double t_level, zsf_phase_state_t *state,
                                  zsf_phase_transports_t *results) {
  // Get the derived parameters
  derived_parameters_t o;
  calculate_derived_parameters(p, &o);

  int err = check_parameters_state(p, &o, state);
  if (err) {
    return err;
  }

  step_phase_3(p, &o, t_level, state, results);

  return ZSF_SUCCESS;
}

int ZSF_CALLCONV zsf_step_phase_4(const zsf_param_t *p, double t_open_sea, zsf_phase_state_t *state,
                                  zsf_phase_transports_t *results) {
  // Get the derived parameters
  derived_parameters_t o;
  calculate_derived_parameters(p, &o);

  int err = check_parameters_state(p, &o, state);
  if (err) {
    return err;
  }
  if (fabs(state->head_lock - p->head_sea) > 1E-8) {
    return ZSF_ERR_REMAINING_HEAD_DIFF;
  }

  step_phase_4(p, &o, t_open_sea, state, results);

  return ZSF_SUCCESS;
}
int ZSF_CALLCONV zsf_calc_steady(const zsf_param_t *p, zsf_results_t *results,
                                 zsf_aux_results_t *aux_results) {

  derived_parameters_t o;
  calculate_derived_parameters(p, &o);

  // Start salinity and salt mass
  zsf_phase_state_t state;

  double sal_lock_4 = p->sal_lock;
  if (sal_lock_4 == ZSF_NAN)
    sal_lock_4 = 0.5 * (p->sal_sea + p->sal_lake);

  state.volume_ship_in_lock = p->ship_volume_sea_to_lake;
  state.saltmass_lock = sal_lock_4 * (o.volume_lock_at_sea - state.volume_ship_in_lock);
  state.head_lock = p->head_sea;
  state.sal_lock = sal_lock_4;

  int err = check_parameters_state(p, &o, &state);
  if (err) {
    return err;
  }

  while (1) {
    // Backup old salinity value for convergence check
    double sal_lock_4_prev = sal_lock_4;

    zsf_phase_transports_t tp1;
    step_phase_1(p, &o, p->leveling_time, &state, &tp1);
    double sal_lock_1 = state.sal_lock;

    zsf_phase_transports_t tp2;
    step_phase_2(p, &o, o.t_open_lake, &state, &tp2);
    double sal_lock_2 = state.sal_lock;

    zsf_phase_transports_t tp3;
    step_phase_3(p, &o, p->leveling_time, &state, &tp3);
    double sal_lock_3 = state.sal_lock;

    zsf_phase_transports_t tp4;
    step_phase_4(p, &o, o.t_open_sea, &state, &tp4);

    sal_lock_4 = state.sal_lock;

    // Convergence check
    // ~~~~~~~~~~~~~~~~~
    if (is_close(sal_lock_4, sal_lock_4_prev, p->rtol, p->atol)) {
      // Cycle-averaged discharges and salinities
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Lake side
      double mt_lake = tp1.mass_transport_lake + tp2.mass_transport_lake + tp3.mass_transport_lake +
                       tp4.mass_transport_lake;

      double vol_lake_out =
          tp1.volume_lake_out + tp2.volume_lake_out + tp3.volume_lake_out + tp4.volume_lake_out;
      double disch_lake_out = vol_lake_out / o.t_cycle;

      double vol_lake_in =
          tp1.volume_lake_in + tp2.volume_lake_in + tp3.volume_lake_in + tp4.volume_lake_in;
      double disch_lake_in = vol_lake_in / o.t_cycle;

      double salt_load_lake = mt_lake / o.t_cycle;
      double sal_lake_in = -1 * (mt_lake - vol_lake_out * p->sal_lake) / vol_lake_in;

      // Sea side
      double mt_sea = tp1.mass_transport_sea + tp2.mass_transport_sea + tp3.mass_transport_sea +
                      tp4.mass_transport_sea;

      double vol_sea_out =
          tp1.volume_sea_out + tp2.volume_sea_out + tp3.volume_sea_out + tp4.volume_sea_out;
      double disch_sea_out = vol_sea_out / o.t_cycle;

      double vol_sea_in =
          tp1.volume_sea_in + tp2.volume_sea_in + tp3.volume_sea_in + tp4.volume_sea_in;
      double disch_sea_in = vol_sea_in / o.t_cycle;

      double salt_load_sea = mt_sea / o.t_cycle;
      double sal_sea_in = (mt_sea + vol_sea_out * p->sal_sea) / vol_sea_in;

      // Put the main results in the output stucture
      results->mass_transport_lake = mt_lake;
      results->salt_load_lake = salt_load_lake;
      results->discharge_lake_out = disch_lake_out;
      results->discharge_lake_in = disch_lake_in;
      results->salinity_lake_in = sal_lake_in;

      results->mass_transport_sea = mt_sea;
      results->salt_load_sea = salt_load_sea;
      results->discharge_sea_out = disch_sea_out;
      results->discharge_sea_in = disch_sea_in;
      results->salinity_sea_in = sal_sea_in;

      // Additional results. Only interesting when one wants to get a closer
      // understanding of what is going on, what happens in each phase, etc.
      if (aux_results != NULL) {
        // Equivalent full lock exchanges
        aux_results->z_fraction =
            0.5 * (mt_lake + mt_sea) /
            (0.5 * (o.volume_lock_at_lake + o.volume_lock_at_sea) * (p->sal_sea - p->sal_lake));

        // Dimensionless door open time
        double sal_diff = p->sal_sea - p->sal_lake;
        double head_avg = 0.5 * (p->head_sea + p->head_lake);
        double velocity_exchange =
            0.5 * sqrt(o.g * 0.8 * sal_diff / o.density_average * (head_avg - p->lock_bottom));
        double t_lock_exchange = 2 * p->lock_length / velocity_exchange;

        aux_results->dimensionless_door_open_time = t_lock_exchange / o.t_open;

        // Salinities after each phase
        aux_results->salinity_lock_1 = sal_lock_1;
        aux_results->salinity_lock_2 = sal_lock_2;
        aux_results->salinity_lock_3 = sal_lock_3;
        aux_results->salinity_lock_4 = sal_lock_4;

        // Mass transports in each phase
        aux_results->mass_transport_lake_1 = tp1.mass_transport_lake;
        aux_results->mass_transport_lake_2 = tp2.mass_transport_lake;
        aux_results->mass_transport_lake_4 = tp4.mass_transport_lake;

        aux_results->mass_transport_sea_2 = tp2.mass_transport_sea;
        aux_results->mass_transport_sea_3 = tp3.mass_transport_sea;
        aux_results->mass_transport_sea_4 = tp4.mass_transport_sea;

        // Volumes from/to lake and sea
        aux_results->volume_lake_in = vol_lake_in;
        aux_results->volume_lake_out = vol_lake_out;
        aux_results->volume_sea_in = vol_sea_in;
        aux_results->volume_sea_out = vol_sea_out;

        // Dependent parameters
        aux_results->volume_lock_at_lake = o.volume_lock_at_lake;
        aux_results->volume_lock_at_sea = o.volume_lock_at_sea;

        aux_results->t_cycle = o.t_cycle;
        aux_results->t_open = o.t_open;
        aux_results->t_open_lake = o.t_open_lake;
        aux_results->t_open_sea = o.t_open_sea;
      }

      break;
    }
  }

  return ZSF_SUCCESS;
}
