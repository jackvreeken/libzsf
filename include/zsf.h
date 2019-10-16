/*****************************************************************************
 * zsf.h: zsf public header
 *****************************************************************************/

// We want to be compatible with as many languages as possible. Most compilers
// do 8-byte alignment, but VBA wants structs packed to 4-byte boundaries.
// Other languages have different assumptions. We try to keep everything
// packed at 8-bytes ourselves, by only using 8-byte types.

#ifndef ZSF_ZSF_H
#define ZSF_ZSF_H

#if defined(_WIN32)
#  if defined ZSF_STATIC
#    define ZSF_EXPORT
#  elif defined ZSF_EXPORTS
#    define ZSF_EXPORT __declspec(dllexport)
#  else
#    define ZSF_EXPORT __declspec(dllimport)
#  endif
#elif defined(__CYGWIN__)
#  define ZSF_EXPORT
#else
#  if (defined __GNUC__ && __GNUC__ >= 4) || defined __INTEL_COMPILER
#    define ZSF_EXPORT __attribute__((visibility("default")))
#  else
#    define ZSF_EXPORT
#  endif
#endif

#if (defined ZSF_USE_STDCALL) && (defined _WIN32)
#  define ZSF_CALLCONV __stdcall
#else
#  define ZSF_CALLCONV
#endif

// A custom value to signify "not specified"
#define ZSF_NAN -999.0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zsf_param_t {
  double lock_length;
  double lock_width;
  double lock_bottom;
  double num_cycles;
  double door_time_to_open;
  double leveling_time;
  double calibration_coefficient;
  double symmetry_coefficient;
  double ship_volume_sea_to_lake;
  double ship_volume_lake_to_sea;
  double sal_lock;
  double head_sea;
  double sal_sea;
  double head_lake;
  double sal_lake;
  double temperature;
  double flushing_discharge_high_tide;
  double flushing_discharge_low_tide;
  double density_current_factor_sea;
  double density_current_factor_lake;
  double rtol;
  double atol;
} zsf_param_t;

typedef struct zsf_results_t {
  double mass_transport_lake;
  double salt_load_lake;
  double discharge_lake_out;
  double discharge_lake_in;
  double salinity_lake_in;

  double mass_transport_sea;
  double salt_load_sea;
  double discharge_sea_out;
  double discharge_sea_in;
  double salinity_sea_in;
} zsf_results_t;

typedef struct zsf_aux_results_t {
  double z_fraction;
  double dimensionless_door_open_time;
  double salinity_lock_1;
  double salinity_lock_2;
  double salinity_lock_3;
  double salinity_lock_4;
  double mass_transport_lake_1;
  double mass_transport_lake_2;
  double mass_transport_lake_4;
  double mass_transport_sea_2;
  double mass_transport_sea_3;
  double mass_transport_sea_4;
  double volume_lake_in;
  double volume_lake_out;
  double volume_sea_in;
  double volume_sea_out;
  double volume_lock_at_lake;
  double volume_lock_at_sea;
  double t_cycle;
  double t_open;
  double t_open_lake;
  double t_open_sea;
} zsf_aux_results_t;

/* Structs when stepping through phases explicitly, i.e. not looping until steady */
typedef struct zsf_phase_state_t {
  double sal_lock;
  double saltmass_lock;
  double head_lock;
  double volume_ship_in_lock;
} zsf_phase_state_t;

/* Per phase we calculate the mass transports and volume transports over the
   lock gates/openings. A positive values means "from lake to lock" or "from lock
   to sea". */
typedef struct zsf_phase_transports_t {
  double mass_transport_lake;
  double volume_lake_out;
  double volume_lake_in;
  double discharge_lake_out;
  double discharge_lake_in;
  double salinity_lake_in;

  double mass_transport_sea;
  double volume_sea_out;
  double volume_sea_in;
  double discharge_sea_out;
  double discharge_sea_in;
  double salinity_sea_in;
} zsf_phase_transports_t;

/* zsf_initialize_state:
 *      fill zsf_state_t with an initial condition for an empty (no ships) lock */
ZSF_EXPORT int ZSF_CALLCONV zsf_initialize_state(const zsf_param_t *p, zsf_phase_state_t *state,
                                                 double sal_lock, double head_lock);

/* zsf_step_phase_1:
 *      Perform step 1: levelling to lake side */
ZSF_EXPORT int ZSF_CALLCONV zsf_step_phase_1(const zsf_param_t *p, double t_level,
                                             zsf_phase_state_t *state,
                                             zsf_phase_transports_t *results);

/* zsf_step_phase_2:
 *      Perform step 1: door open to lake side (ships out, lock exchange + flushing, ships in) */
ZSF_EXPORT int ZSF_CALLCONV zsf_step_phase_2(const zsf_param_t *p, double t_open_lake,
                                             zsf_phase_state_t *state,
                                             zsf_phase_transports_t *results);

/* zsf_step_phase_3:
 *      Perform step 1: levelling to sea side */
ZSF_EXPORT int ZSF_CALLCONV zsf_step_phase_3(const zsf_param_t *p, double t_level,
                                             zsf_phase_state_t *state,
                                             zsf_phase_transports_t *results);

/* zsf_step_phase_4:
 *      Perform step 1: door open to sea side (ships out, lock exchange + flushing, ships in) */
ZSF_EXPORT int ZSF_CALLCONV zsf_step_phase_4(const zsf_param_t *p, double t_open_sea,
                                             zsf_phase_state_t *state,
                                             zsf_phase_transports_t *results);

/* zsf_param_default:
 *      fill zsf_param_t with default values */
ZSF_EXPORT void ZSF_CALLCONV zsf_param_default(zsf_param_t *p);

/* zsf_calc_steady:
 *      calculate the salt intrusion for a set of parameters, assuming steady operation*/
ZSF_EXPORT int ZSF_CALLCONV zsf_calc_steady(const zsf_param_t *p, zsf_results_t *results,
                                            zsf_aux_results_t *aux_results);
/* zsf_error_msg:
 *      Get error messeage corresponding to error code */
ZSF_EXPORT const char *ZSF_CALLCONV zsf_error_msg(int code);

#ifdef __cplusplus
}
#endif

#endif
