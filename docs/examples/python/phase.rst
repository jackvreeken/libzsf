Phase-wise calculation
~~~~~~~~~~~~~~~~~~~~~~

.. image:: ../../images/delfzijl2.jpg

.. :href: https://beeldbank.rws.nl/MediaObject/Details/479953
.. https://beeldbank.rws.nl, Rijkswaterstaat / Harry van Reeken

Overview
--------

.. note::

    This example focuses on performing a phase-wise calculation of salt-intrusion through a shipping lock.
    It assumes basic exposure to the Python interface.
    If you are a first-time user of the ZSF, see the :doc:`steady` example.

The purpose of this example is to understand the basic steps to perform a phase-wise calculation of the salt intrusion through a shipping lock.
The scenario is similar to that of the :doc:`steady` example, in that there is a single lock connecting a canal to the sea.
The differences with the steady state example are:

    - there is a head difference now, with the sea at 2 mNAP
    - we are only going to calculate one full locking cycle during the daytime operation

Initializing the lock
---------------------

First we need to initialize the lock with a certain salinity and head.
The dimensions of the lock are equal to that of the :doc:`steady` example.
The boundary conditions are also equal, except that the ``head_sea`` is 2 mNAP.
The operational parameters differ more, as the two steady-state operation parameters ``num_cycles``, ``leveling_time`` and ``door_time_to_open`` are removed.

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 1-24
  :lineno-match:

The next step is to initialize a :py:class:`pyzsf.ZSFUnsteady` instance, with an initial salinity of 15 kg/m³ and head of 0.0 mNAP.
We also directly pass all other parameters to the constructor.
These updated parameters are stored in the instance, and we do not need to specify them again when calling the method to level or open the doors (contrary to the Excel interface), unless we want to change the value of one of them of course.

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 28-29
  :lineno-match:

The state of the lock after initialization is logged to the console with the ``pprint.pprint`` statements:

.. code-block:: text

    State after initialization
    {'head_lock': 0.0,
     'salinity_lock': 15.0,
     'saltmass_lock': 136752.00000000003,
     'volume_ship_in_lock': 0.0}

.. note::

    The lock is always initialized empty, i.e., without a ship in it.

Leveling to the lake side
-------------------------

The next step is to level the lock to the lake side, which is Phase 1.
See :numref:`chapter_eq_per_lock_phase` in the theory for more explanation on the phases in a locking cycle.
The lock was already initialized to the head of the lake side, so we expect to see no transports in this phase.
We call :py:class:`pyzsf.ZSFUnsteady.zsf_step_phase_1`, which needs the leveling time as an argument.

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 33
  :lineno-match:

The results of this method call, and the resulting state of the lock are printed to the console:

.. code-block:: text

  Phase 1:
  ********
  Transports:
  {'discharge_from_lake': 0.0,
   'discharge_from_sea': 0.0,
   'discharge_to_lake': 0.0,
   'discharge_to_sea': 0.0,
   'mass_transport_lake': 0.0,
   'mass_transport_sea': 0.0,
   'salinity_to_lake': 15.0,
   'salinity_to_sea': 15.0,
   'volume_from_lake': 0.0,
   'volume_from_sea': 0.0,
   'volume_to_lake': 0.0,
   'volume_to_sea': 0.0}
  State:
  {'head_lock': 0.0,
   'salinity_lock': 15.000000000000002,
   'saltmass_lock': 136752.00000000003,
   'volume_ship_in_lock': 0.0}

Note that the console output shows zero transport of both water and salt.
Also note that state of the lock after this phase is equal (barring rounding errors) to the state after initialization.

Opening the door to the lake side
---------------------------------

With the lock leveled to the lake side, the doors can now be opened.

.. important::

    Make sure that the leveling methods (**step_phase_1** and **step_phase_3**) have matching heads for the boundary conditions as the subsequent door-open methods (**step_phase_2** and **step_phase_4** respectively). If this is not the case, an exception is raised stating this requirement.

To open the doors, we call :py:class:`pyzsf.ZSFUnsteady.zsf_step_phase_2`:

    - `volume ship down` (lake to sea) is the default of 1000.0 m³.
      We therefore do not need to pass this argument.
    - `door open time` on the lake side to 840 seconds.

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 40
  :lineno-match:

The transports are logged to the console.
Note that there are no transports to the sea in this phase, as we would expect without any flushing discharge.

Leveling to the sea side
------------------------

The next step is to level the lock to the sea side.
The instructions are equal to those of leveling to the lake side, except that we call :py:class:`pyzsf.ZSFUnsteady.zsf_step_phase_3`.

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 47
  :lineno-match:

Opening the door to the sea side
--------------------------------

The last step is to open the doors to the sea side, and let the ship sail out and a new ship sail in.
This time we will also change the displacement of the ship that enters:

    - override the `volume ship up` (sea to lake) to 800.0 m³
    - `door open time` on the sea side to 840 seconds

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lines: 54
  :lineno-match:

Check the state output that is logged to the console after this phase:

.. code-block:: text

    {'head_lock': 2.0,
     'salinity_lock': 22.612960757739405,
     'saltmass_lock': 281775.5814100392,
     'volume_ship_in_lock': 800.0}

Note that the volume of ship inside the lock chamber has changed from 1000 m³ to 800.0 m³.

Calculating more lockages
-------------------------

One can repeat the above process, again adding a method call to :py:class:`pyzsf.ZSFUnsteady.zsf_step_phase_1` for leveling to the lake side next.
If the water levels and salinities on the lake and/or sea side are changing, you can change these parameters accordingly for each method call.
Typically, the water level is set to the average water level during the door-open phase, with the preceding leveling phase also leveling to said water level.
It quickly becomes rather tedious and error-prone to calculate many lockages this way, and it is better to write a loop over some input data.
For an example of this, see :doc:`phase-multiple-lockages` example.

The whole script
----------------

All together, the whole example script is as follows:

.. literalinclude:: ../../../examples/python/phase.py
  :language: python
  :lineno-match:
