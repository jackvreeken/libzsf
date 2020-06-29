Numerical approach cycle-averaged values
========================================

The description in Chapter :ref:`chapter_eq_per_lock_phase` of how the transports and how the salinity in the lock chamber changes from phase to phase, also gives a recipe for calculating the cycle-averaged values of volumes and salinities.
Namely, we can iteratively calculate our way through the locking cycle until the calculated value no longer change significantly.
From these steady state values the cycle-averaged flows (discharges and withdrawals) can be determined, together with the salinities (see :numref:`sec_numapproach_iterative`).

An alternative approach is setting up a system of equations (see :numref:`sec_numapproach_syseq`).
From Chapter :ref:`chapter_eq_per_lock_phase` and :ref:`chapter_cycle_avg_flow_sals` the essential equations can be selected, that together form a system of equations with unknowns.
Solving this system of equations than resolves the unknowns that are necessary to calculate the cycle-averaged flows and salinities.

.. _sec_numapproach_iterative:

Iteratively calculating the locking cycle
-----------------------------------------

In Chapter :ref:`chapter_cycle_avg_flow_sals` the expression for the flows (discharges and withdrawals) have been set up, but a few unknowns remain.
These values arise in the locking cycle, as described in Chapter :ref:`chapter_eq_per_lock_phase`.

This cycle can then be considered iterative process: for fixed boundary conditions and after enough cycles, eventually these unknowns will converge to their respective values.
To enter this iterative process, e.g. starting at LT 1 / HT 1, we only need an initial guess for the salinity of the lock chamber.
Obviously this guess has to be higher than the salinity of the fresh side, and lower than that of the salt side.
A good starting point would then be the average of the salinities of the boundary conditions.

In case of calculating through time for varying boundary conditions (e.g. a tide on the sea side, or a time-varying operation of the lock), the converged lock chamber salinity of the previous time step can be chosen as the initial guess.
For slowly changing boundary conditions, the previous chamber salinity is a reasonable estimate.
The closer the guess to the eventual solution, the fewer iterations are needed to converge.

The numerical approach for determining cycle-averaged values then consists of two steps:

1. Iteratively determining the unknown values
2. Calculate the discharges :math:`Q_F^+` with :math:`S_F^+` and :math:`Q_S^+` with :math:`S_S^+`, and the withdrawals :math:`Q_F^-` and :math:`Q_S^-`.

.. _sec_numapproach_syseq:

System of equations
-------------------

An alternative to the iterative approach as described above is setting up a system of equations in which the number of equations is equal to the number of unknowns.
From the previous chapters the relevant equations can be selected, each with a number of unknowns.
(The unknowns in this case are all values that cannot be directly calculated from the boundary conditions or input).
Not all equations are linear, so it is necessary to repeatedly solve a linearized system of equations until convergence.
In practice, this way of solving has very little advantage over calculating iteratively as described in :ref:`sec_numapproach_iterative`.
We therefore do not elaborate further on this approach.

Overview of output
------------------

By the formulation the steady state values of the following quantities are calculated, all as function of time:

:math:`M_F`, :math:`{\dot{M}}_F`, :math:`Q_F^-`, :math:`Q_F^+`, :math:`S_F^+`, :math:`M_S`, :math:`{\dot{M}}_S`, :math:`Q_S^-`, :math:`Q_S^+`, :math:`S_S^+`

Getting to these parameters is what the formulation was designed for: the mass transports (per cycle) and the fluxes and flows (cycle-averaged) that enter and exit the lock on both sides.
Aside from that there are a few other parameters that can help interpret the output.
These can be geometric parameters, like the volume of the lock chamber, or operational parameters like door-open times.
It is also possible to export the cycle-averaged transports per locking phase.

Aside from that, a few other useful parameters can be determined.
These parameters can help compare the ZSF to more naive methods of determined the salt load, or with other theoretical or experimental relations.

Dimensionless salt transport
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the dimensionless salt transport over the lock per cycle, we use the parameter :math:`Z_{fraction}`.
This parameter is defined as a factor on the lock chamber volume times the difference in salinity between the boundary conditions:

.. math::
    :label: numapproach_z_fraction_mass

    M = Z_{fraction} V_L \cdot \Delta S

As such, :math:`Z_{fraction}` indicates what fraction of the lock chamber, in regular locking operation, exchanges and contributes to the salt transport.

In the process of calculating transports we get transports for both lock heads.
Aside from that, the volume of the lock chamber, due to a difference in water level on both sides, is not always equal.
Therefore, we have to use average values for these quantities.
From this, :math:`Z_{fraction}` can be written as:

.. math::
    :label: numapproach_z_fraction_explicit

    Z_{fraction} = \frac{ \overline{M} }{ \overline{V_L} \cdot \left(S_S - S_F \right) } = \frac{ 0.5 \cdot \left( M_F + M_S \right) }{0.5 \cdot \left( V_{L,F} + V_{L,S} \right) \cdot \left( S_S - S_F \right) }

Dimensionless door-open time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The dimensionless door-open time is :math:`T_{LE} / T_{open}`. In the calculations the value of :math:`T_{LE}` is determined per lock head, with the prevailing salinity difference.
With that, the value of :math:`T_{LE}` is not a direct function of the input, but is dependent on the calculations.
To get to a :math:`T_{LE}` that is only determined by input (boundary conditions and geometry), we define a variant: :math:`T_{LE,FS}`.
This quantity is based solely on the salinity difference over the lock.
This parameter is not used in the calculations, but is sometimes used in determining auxiliary outputs, e.g. of the dimensionless door-open time :math:`T_{LE} / T_{open}`:

.. math::
    :label: numapproach_dimless_dooropentime_ci

    c_{i,FS} = \frac{1}{2} \sqrt{ g^\prime \overline{H} } = \frac{1}{2} \sqrt{ g \frac{\Delta \rho}{\overline{\rho}} \overline{H} } \approx \frac{1}{2} \sqrt{ \frac{0.8 \left( S_S - S_F \right) }{ \overline{\rho}_{FS} } \left( \frac{H_F + H_S}{2} \right)}

.. math::
    :label: numapproach_dimless_dooropentime

    T_{LE,FS} = \frac{ 2L }{ c_{i,FS} }
