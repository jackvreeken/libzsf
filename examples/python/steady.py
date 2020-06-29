import pyzsf


lock_parameters = {
    "lock_length": 148.0,
    "lock_width": 14.0,
    "lock_bottom": -4.4,
}

boundary_conditions = {
    "head_lake": 0.0,
    "salinity_lake": 5.0,
    "temperature_lake": 15.0,
    "head_sea": 0.0,
    "salinity_sea": 25.0,
    "temperature_sea": 15.0,
}

operational_parameters = {
    "num_cycles": 30,
    "door_time_to_open": 300.0,
    "leveling_time": 300.0,
    "ship_volume_sea_to_lake": 1000.0,
    "ship_volume_lake_to_sea": 1000.0,
}

daytime_parameters = {**lock_parameters, **boundary_conditions, **operational_parameters}
nighttime_parameters = {**daytime_parameters, "num_cycles": 10}

# Calculate the transports without protection
print("No measures:")
results = pyzsf.zsf_calc_steady(**daytime_parameters)
print("Day = {:.1f} kg/s".format(-1 * results["salt_load_lake"]))

results = pyzsf.zsf_calc_steady(**nighttime_parameters)
print("Night = {:.1f} kg/s".format(-1 * results["salt_load_lake"]))

# Transports with a bubble screen
bubble_screen_parameters = {
    "density_current_factor_lake": 0.25,
    "density_current_factor_sea": 0.25,
}

print("\nBubble screen (25%):")
results = pyzsf.zsf_calc_steady(**daytime_parameters, **bubble_screen_parameters)
print("Day = {:.1f} kg/s".format(-1 * results["salt_load_lake"]))

results = pyzsf.zsf_calc_steady(**nighttime_parameters, **bubble_screen_parameters)
print("Night = {:.1f} kg/s".format(-1 * results["salt_load_lake"]))

# Auxiliary results, showing how long the doors are open
print("\nDoor open times at night:")
results = pyzsf.zsf_calc_steady(True, **nighttime_parameters, **bubble_screen_parameters)
for k, v in results.items():
    if k.startswith("t_open"):
        print(f"{k} = {v}")

# Transports at night with bubble screen and closing the doors sooner
print("\nBubble screen (25%), and close doors sooner:")
results = pyzsf.zsf_calc_steady(
    **nighttime_parameters, **bubble_screen_parameters, calibration_coefficient=0.3
)
print("Night = {:.1f} kg/s".format(-1 * results["salt_load_lake"]))
