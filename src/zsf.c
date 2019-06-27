#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "zsf.h"

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

void ZSF_CALLCONV zsf_param_default(zsf_param_t *p) {
  /* */
  memset(p, 0, sizeof(zsf_param_t));

  // Lock properties
  p->lock_length = 100.0;
  p->lock_width = 10.0;
  p->lock_bottom = -5.0;

  // Operation properties
  p->num_cycles = 12.0;
  p->door_time_to_open = 5.0;
  p->leveling_time = 5.0;
  p->symmetry_coefficient = 1.0;
  p->ship_volume_sea_to_lake = 0.0;
  p->ship_volume_lake_to_sea = 0.0;
  p->calibration_coefficient = 1.0;

  // Salt intrusion counter-measures
  p->flushing_discharge_high_tide = 0.0;
  p->flushing_discharge_low_tide = 0.0;
  p->density_current_factor_sea = 1.0;
  p->density_current_factor_lake = 1.0;

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

void ZSF_CALLCONV zsf_calculate(zsf_param_t *p, zsf_results_t *results,
                                zsf_aux_results_t *aux_results) {
  // Gravitational constant
  double g = 9.81;

  // Calculate derived parameters
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Tide signal
  int is_high_tide = p->head_sea >= p->head_lake;
  int is_low_tide = 1 - is_high_tide;

  // Volumes
  double volume_lock_at_sea = p->lock_length * p->lock_width * (p->head_sea - p->lock_bottom);
  double volume_lock_at_lake = p->lock_length * p->lock_width * (p->head_lake - p->lock_bottom);

  double leveling_volume = fabs((p->head_sea - p->head_lake) * p->lock_width * p->lock_length);

  double volume_leveling_low_tide = (double)is_low_tide * leveling_volume;
  double volume_leveling_high_tide = (double)is_high_tide * leveling_volume;

  // Door open times
  double t_cycle = 24.0 * 3600.0 / p->num_cycles;
  double t_open_avg = 0.5 * t_cycle - (p->leveling_time + 2.0 * 0.5 * p->door_time_to_open) * 60.0;
  double t_open = p->calibration_coefficient * t_open_avg;
  double t_open_lake = p->symmetry_coefficient * t_open;
  double t_open_sea = (2.0 - p->symmetry_coefficient) * t_open;

  // Flushing discharge
  double flushing_discharge =
      is_low_tide ? p->flushing_discharge_low_tide : p->flushing_discharge_high_tide;

  // Start salinity and salt mass guess
  double sal_lock_4 = 0.5 * (p->sal_sea + p->sal_lake);
  double saltmass_lock_4 = sal_lock_4 * (volume_lock_at_sea - p->ship_volume_sea_to_lake);

  // Average density (for lock exchange)
  double density_average = 0.5 * (sal_2_density(p->sal_lake, p->temperature, p->rtol, p->atol) +
                                  sal_2_density(p->sal_sea, p->temperature, p->rtol, p->atol));

  while (1) {
    // Per phase we calculate the mass transports ("mt_" variables) and
    // volume transports ("vt_" variables) over the lock gates/openings. A
    // positive values means "from lake to lock" or "from lock to sea".

    // Backup old salinity value for convergence check
    double sal_lock_4_prev = sal_lock_4;

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
    // NOTE: If `volume_leveling_low_tide` is zero, then
    // `volume_leveling_high_tide` is not (and vice-versa).

    // Leveling
    double mt_lake_1 =
        volume_leveling_low_tide * p->sal_lake - volume_leveling_high_tide * sal_lock_4;

    // Update state variables of the lock
    double saltmass_lock_1 = saltmass_lock_4 + mt_lake_1;
    double sal_lock_1 = saltmass_lock_1 / (volume_lock_at_lake - p->ship_volume_sea_to_lake);

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

    // Subphase a. Ships exiting the lock chamber towards the lake
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double mt_lake_2_ship_exit = p->ship_volume_sea_to_lake * p->sal_lake;

    // Update state variables of the lock
    double saltmass_lock_2a = saltmass_lock_1 + mt_lake_2_ship_exit;
    double sal_lock_2a = saltmass_lock_2a / volume_lock_at_lake;

    // Subphase b. Flushing compensated lock exchange
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double velocity_flushing =
        flushing_discharge / (p->lock_width * (p->head_lake - p->lock_bottom));

    double sal_diff = sal_lock_2a - p->sal_lake;
    double velocity_exchange_raw =
        0.5 * sqrt(g * 0.8 * sal_diff / density_average * (p->head_lake - p->lock_bottom));
    double velocity_exchange_eta = p->density_current_factor_lake * velocity_exchange_raw;
    double frac_lock_exchange =
        fmax((velocity_exchange_eta - velocity_flushing) / velocity_exchange_eta, 0.0);
    double t_lock_exchange = 2 * p->lock_length / velocity_exchange_eta;
    double volume_exchange_2 =
        frac_lock_exchange * volume_lock_at_lake * TANH(t_open_lake / t_lock_exchange);

    double mt_lake_2_lock_exchange = (p->sal_lake - sal_lock_2a) * volume_exchange_2;

    // Flushing itself (taking lock exchange into account)
    double volume_flush = flushing_discharge * t_open_lake;

    // Max volume that will lead to the lock being refreshed (before we
    // reach steady state where we are flushing to the sea with salinity of
    // lake)
    double max_volume_flush_refresh = volume_lock_at_lake - volume_exchange_2;

    double volume_flush_refresh = fmin(volume_flush, max_volume_flush_refresh);
    double volume_flush_passthrough = fmax(volume_flush - max_volume_flush_refresh, 0.0);

    double mt_lake_2_flushing =
        volume_flush_refresh * p->sal_lake + volume_flush_passthrough * p->sal_lake;
    double mt_sea_2_flushing =
        volume_flush_refresh * sal_lock_2a + volume_flush_passthrough * p->sal_lake;

    // Update state variables of the lock
    double saltmass_lock_2b =
        saltmass_lock_2a + mt_lake_2_flushing + mt_lake_2_lock_exchange - mt_sea_2_flushing;
    double sal_lock_2b = saltmass_lock_2b / volume_lock_at_lake;

    // Subphase c. Ship entering the lock chamber from the lake
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double mt_lake_2_ship_enter = -1 * p->ship_volume_lake_to_sea * sal_lock_2b;

    // Update state variables of the lock
    double saltmass_lock_2c = saltmass_lock_2b + mt_lake_2_ship_enter;
    double sal_lock_2c = saltmass_lock_2c / (volume_lock_at_lake - p->ship_volume_lake_to_sea);

    // Totals for Phase 2
    // ~~~~~~~~~~~~~~~~~~
    // Total mass transports over both gates
    double mt_lake_2 =
        mt_lake_2_ship_exit + mt_lake_2_ship_enter + mt_lake_2_flushing + mt_lake_2_lock_exchange;
    double mt_sea_2 = mt_sea_2_flushing;

    // Update state variables of the lock
    double saltmass_lock_2 = saltmass_lock_1 + mt_lake_2 - mt_sea_2;
    double sal_lock_2 = saltmass_lock_2 / (volume_lock_at_lake - p->ship_volume_lake_to_sea);

    assert(fabs(saltmass_lock_2 - saltmass_lock_2c) < 1E-8);
    assert(fabs(sal_lock_2 - sal_lock_2c) < 1E-8);

    assert((sal_lock_2 >= p->sal_lake) & (sal_lock_2 <= p->sal_sea));

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

    // Leveling
    double mt_sea_3 =
        volume_leveling_low_tide * sal_lock_2 - volume_leveling_high_tide * p->sal_sea;

    // Update state variables of the lock
    double saltmass_lock_3 = saltmass_lock_2 - mt_sea_3;
    double sal_lock_3 = saltmass_lock_3 / (volume_lock_at_sea - p->ship_volume_lake_to_sea);

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

    // Subphase a. Ships exiting the lock chamber towards the sea
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double mt_sea_4_ship_exit = -1 * p->ship_volume_lake_to_sea * p->sal_sea;

    // Update state variables of the lock
    double saltmass_lock_4a = saltmass_lock_3 - mt_sea_4_ship_exit;
    double sal_lock_4a = saltmass_lock_4a / volume_lock_at_sea;

    // Subphase b. Flushing compensated lock exchange
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    velocity_flushing = flushing_discharge / (p->lock_width * (p->head_sea - p->lock_bottom));

    sal_diff = p->sal_sea - sal_lock_4a;
    velocity_exchange_raw =
        0.5 * sqrt(g * 0.8 * sal_diff / density_average * (p->head_sea - p->lock_bottom));
    velocity_exchange_eta = p->density_current_factor_sea * velocity_exchange_raw;

    // The equilibrium depth of the boundary layer between the salt (sal_sea)
    // and fresh (sal_lake) water when flushing for a very long time.
    double head_equilibrium = cbrt(2.0 * pow(flushing_discharge, 2.0) * density_average /
                                   (g * 0.8 * (p->sal_sea - p->sal_lake)));

    head_equilibrium = fmin(head_equilibrium, p->head_sea - p->lock_bottom);

    frac_lock_exchange =
        (p->head_sea - p->lock_bottom - head_equilibrium) / (p->head_sea - p->lock_bottom);

    // If we flush so much that the density current never enters the lock, we
    // might get division by zero. Avoid by branching such that we can still
    // use fast math (which typically does not work with non-finite values).
    double volume_exchange_4 = 0.0;

    if (velocity_exchange_eta > velocity_flushing) {
      t_lock_exchange =
          2 * p->lock_length * frac_lock_exchange / (velocity_exchange_eta - velocity_flushing);
      volume_exchange_4 =
          frac_lock_exchange * volume_lock_at_sea * TANH(t_open_sea / t_lock_exchange);
    }

    double mt_sea_4_lock_exchange = (sal_lock_4a - p->sal_sea) * volume_exchange_4;

    // Flushing itself (taking lock exchange into account)
    volume_flush = flushing_discharge * t_open_sea;

    // Max volume that will lead to the lock being refreshed (before we
    // reach steady state where we are flushing to the sea with salinity of
    // lake)
    max_volume_flush_refresh = volume_lock_at_sea - volume_exchange_4;

    volume_flush_refresh = fmin(volume_flush, max_volume_flush_refresh);
    volume_flush_passthrough = fmax(volume_flush - max_volume_flush_refresh, 0.0);

    double mt_lake_4_flushing =
        volume_flush_refresh * p->sal_lake + volume_flush_passthrough * p->sal_lake;
    double mt_sea_4_flushing =
        volume_flush_refresh * sal_lock_4a + volume_flush_passthrough * p->sal_lake;

    // Update state variables of the lock
    double saltmass_lock_4b =
        saltmass_lock_4a + mt_lake_4_flushing - mt_sea_4_lock_exchange - mt_sea_4_flushing;
    double sal_lock_4b = saltmass_lock_4b / volume_lock_at_sea;

    // Subphase c. Ship entering the lock chamber from the sea
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    double mt_sea_4_ship_enter = p->ship_volume_sea_to_lake * sal_lock_4b;

    // Update state variables of the lock
    double saltmass_lock_4c = saltmass_lock_4b - mt_sea_4_ship_enter;
    double sal_lock_4c = saltmass_lock_4c / (volume_lock_at_sea - p->ship_volume_sea_to_lake);

    // Totals for Phase 4
    // ~~~~~~~~~~~~~~~~~~
    // Total mass transports over both gates
    double mt_sea_4 =
        mt_sea_4_ship_exit + mt_sea_4_ship_enter + mt_sea_4_flushing + mt_sea_4_lock_exchange;
    double mt_lake_4 = mt_lake_4_flushing;

    // Update state variables of the lock
    saltmass_lock_4 = saltmass_lock_3 + mt_lake_4 - mt_sea_4;
    sal_lock_4 = saltmass_lock_4 / (volume_lock_at_sea - p->ship_volume_sea_to_lake);

    assert(fabs(saltmass_lock_4 - saltmass_lock_4c) < 1E-8);
    assert(fabs(sal_lock_4 - sal_lock_4c) < 1E-8);

    assert((sal_lock_4 >= p->sal_lake) & (sal_lock_4 <= p->sal_sea));

    // Convergence check
    // ~~~~~~~~~~~~~~~~~
    if (is_close(sal_lock_4, sal_lock_4_prev, p->rtol, p->atol)) {
      // Cycle-averaged discharges and salinities
      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Lake side
      double mt_lake = mt_lake_1 + mt_lake_2 + mt_lake_4;

      double vol_lake_out = (volume_leveling_low_tide + p->ship_volume_sea_to_lake +
                             volume_exchange_2 + flushing_discharge * 2 * t_open);
      double disch_lake_out = vol_lake_out / t_cycle;

      double vol_lake_in =
          (volume_leveling_high_tide + volume_exchange_2 + p->ship_volume_lake_to_sea);
      double disch_lake_in = vol_lake_in / t_cycle;

      double salt_load_lake = mt_lake / t_cycle;
      double sal_lake_in = -1 * (mt_lake - vol_lake_out * p->sal_lake) / vol_lake_in;

      // Sea side
      double mt_sea = mt_sea_2 + mt_sea_3 + mt_sea_4;

      double vol_sea_out =
          (volume_leveling_high_tide + volume_exchange_2 + p->ship_volume_lake_to_sea);
      double disch_sea_out = vol_sea_out / t_cycle;

      double vol_sea_in = (volume_leveling_low_tide + p->ship_volume_sea_to_lake +
                           volume_exchange_2 + flushing_discharge * 2 * t_open);
      double disch_sea_in = vol_sea_in / t_cycle;

      double salt_load_sea = mt_sea / t_cycle;
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
            (0.5 * (volume_lock_at_lake + volume_lock_at_sea) * (p->sal_sea - p->sal_lake));

        // Dimensionless door open time
        sal_diff = p->sal_sea - p->sal_lake;
        double head_avg = 0.5 * (p->head_sea + p->head_lake);
        double velocity_exchange =
            0.5 * sqrt(g * 0.8 * sal_diff / density_average * (head_avg - p->lock_bottom));
        t_lock_exchange = 2 * p->lock_length / velocity_exchange;

        aux_results->dimensionless_door_open_time = t_lock_exchange / t_open;

        // Salinities after each phase
        aux_results->salinity_lock_1 = sal_lock_1;
        aux_results->salinity_lock_2 = sal_lock_2;
        aux_results->salinity_lock_3 = sal_lock_3;
        aux_results->salinity_lock_4 = sal_lock_4;

        // Mass transports in each phase
        aux_results->mass_transport_lake_1 = mt_lake_1;
        aux_results->mass_transport_lake_2 = mt_lake_2;
        aux_results->mass_transport_lake_4 = mt_lake_4;

        aux_results->mass_transport_sea_2 = mt_sea_2;
        aux_results->mass_transport_sea_3 = mt_sea_3;
        aux_results->mass_transport_sea_4 = mt_sea_4;

        // Volumes from/to lake and sea
        aux_results->volume_lake_in = vol_lake_in;
        aux_results->volume_lake_out = vol_lake_out;
        aux_results->volume_sea_in = vol_sea_in;
        aux_results->volume_sea_out = vol_sea_out;

        // Dependent parameters
        aux_results->volume_lock_at_lake = volume_lock_at_lake;
        aux_results->volume_lock_at_sea = volume_lock_at_sea;

        aux_results->t_cycle = t_cycle;
        aux_results->t_open = t_open;
        aux_results->t_open_lake = t_open_lake;
        aux_results->t_open_sea = t_open_sea;
      }

      break;
    }
  }
}