# Portable brake thermal inertia estimate

The C++20 header \`Vehicles/PortableBrakeThermalInertia.h\` is a bounded
source-only estimator for rotor heating (kilojoules / heat capacity) and
passive/airflow exponential cooling, with explicit finite-input checks.
The fade estimate between 450 C and 950 C is illustrative, not measured
brake compound data or a physical engine force policy.

Its independent strict CTest covers heating, stationary temperature,
airflow, no-heating frame subdivision equivalence, maximum energy clamps,
fade and bad nonfinite or ambient values. It performs **no Unreal calls**,
changes no existing physical brakes, writes no SaveGame or platform file,
and does not establish a functioning real-world brake system.

Shipping integration requires verified per-wheel energy input, real chaos
brake force, vehicle temperature monitoring, performance, PBR visuals
and installed Windows UE5.8 gameplay validation. This code does not
waive the native merge gate.
