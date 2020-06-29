.. _chapter_cycle_avg_flow_sals:

Cycle-averaged flows and salinities
===================================

Based on the volumes per locking cycle we now can, for each of the locking heads, determine the total transported volumes with their corresponding salinities.
From these volumes the cycle-averaged flows can be determined.

Fresh side
----------

The combined equation for the fresh side gives the total transport of the entire locking cycle.
This equation is as follows:

.. math::
    :label: cycleavg_fresh_side_total_mass

    M_F = M_{F,1} + M_{F,2} + M_{F,4}

Aside from that we have information about the amount of water that is withdrawn from the fresh side

.. math::
    :label: cycleavg_fresh_side_vol_withdrawn

    V_F^- = V_{Level,LT} + V_{Ship,Up} + V_{U,F} + Q_{flush} \cdot 2 \cdot T_{open}

and the volume that is discharged to the fresh side

.. math::
    :label: cycleavg_fresh_side_vol_discharged

    V_F^+ = V_{Level,HT} + V_{U,F} + V_{Ship,Down}


By dividing both volumes by the time spent on a total locking cycle, we can determine the cycle-averaged flows.
Each of these flows has a corresponding discharge, and can be connected to cells in a far-field model as a discharge or withdrawal:

- Withdrawal from the fresh side, with the prevailing salinity :math:`S_F`:

.. math::
    :label: cycleavg_fresh_side_flow_withdrawn

    Q_F^- = \frac{ V_F^- }{ T_{cycle} }

- Discharge to the fresh side with a to-be-determined average salinity:

.. math::
    :label: cycleavg_fresh_side_flow_discharged

    Q_F^+ = \frac{ V_F^+ } { T_{cycle} }; S = S_F^+

The average salinity for the water discharged to the fresh side is determined from the mass and volume transports:

.. math::
    :label: cycleavg_fresh_side_sal_discharged

    S_F^+ = -\frac{ \left( M_F - V_F^- S_F \right) }{ V_F^+ }

In case of stand-alone application, but also to compare with other locks or salt intrusion measure configurations, it can be useful to express the salt transport as a mass flux:

.. math::
    :label: cycleavg_fresh_side_mass_flux

    {\dot{M}}_F = \frac{ M_F }{ T_{cycle} }

Salt side
---------

The combined equation for the salt side is:

.. math::
    :label: cycleavg_salt_side_total_mass

    M_S = M_{S,2} + M_{S,3} + M_{S,4}

Again, we can write down the volumes going to and from the salt side. For the withdrawal that is:

.. math::
    :label: cycleavg_salt_side_vol_withdrawn

    V_S^- = V_{Level,HT} + V_{Ship,Down} + V_{U,S,Flush}

and the volume that is discharged to the salt side

.. math::
    :label: cycleavg_salt_side_vol_discharged

    V_S^+ = V_{Level,LT} + V_{U,S,Flush} + V_{Ship,Up} + Q_{flush} \cdot 2 \cdot T_{open}

By dividing both volumes by the time spent on a total locking cycle, we can determine the cycle-averaged flows.
Each of these flows has a corresponding discharge, and can be connected to cells in a far-field model as a discharge or withdrawal:

- Withdrawal from the salt side, with the prevailing salinity :math:`S_S`:

.. math::
    :label: cycleavg_salt_side_flow_withdrawn

    Q_S^- = \frac{ V_S^- }{ T_{cycle} }

- Discharge to the salt side with a to-be-determined average salinity:

.. math::
    :label: cycleavg_salt_side_flow_discharged

    Q_S^+ = \frac{ V_S^+ } { T_{cycle} }; S = S_S^+

The average salinity for the water discharged to the salt side is determined from the mass and volume transports:

.. math::
    :label: cycleavg_salt_side_sal_discharged

    S_S^+ = \frac{ \left( M_S - V_S^- S_S \right) }{ V_S^+ }

In case of stand-alone application, but also to compare with other locks or salt intrusion measure configurations, it can be useful to express the salt transport as a mass flux:

.. math::
    :label: cycleavg_salt_side_mass_flux

    {\dot{M}}_S = \frac{ M_S }{ T_{cycle} }

For an equilibrium state, with the lock operating with constant operation for long periods of time, it obviously holds that

.. math::
    :label: cycleavg_salt_side_mass_flux_equilibrium

    {\dot{M}}_S = {\dot{M}}_F
