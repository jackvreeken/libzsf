Phase-wise calculation
~~~~~~~~~~~~~~~~~~~~~~

.. image:: ../../images/delfzijl2.jpg

.. :href: https://beeldbank.rws.nl/MediaObject/Details/479953
.. https://beeldbank.rws.nl, Rijkswaterstaat / Harry van Reeken

Overview
--------

.. note::

    This example focuses on performing a phase-wise calculation of salt-intrusion through a shipping lock.
    It assumes basic exposure to the Excel interface.
    If you are a first-time user of the ZSF, see the :doc:`steady` example.

The purpose of this example is to understand the basic steps to perform a phase-wise calculation of the salt intrusion through a shipping lock.
The scenario is similar to that of the :doc:`steady` example, in that there is a single lock connecting a canal to the sea.
The differences with the steady state example are:

    - there is a head difference now, with the sea at 2 mNAP
    - we are only going to calculate one full locking cycle during the daytime operation

Different phases and routines
-----------------------------

There are a few phases the lock can go through, each having associated salt transports.
See :numref:`chapter_eq_per_lock_phase` in the theory for more explanation on what each phase entails.
The term `routines` refers to the actual functions that are called, and their naming mostly corresponds to that of the phases.
The only major difference is that there is an initialization routine **0** to set the starting state, but no such phase as there are no transports yet.

.. seealso::

    For an overview of phases and how the transports are determined, see :ref:`chapter_eq_per_lock_phase`.

Initializing the lock
---------------------

First we need to initialize the lock with a certain salinity and head.
The dimensions of the lock are equal to that of the :doc:`steady` example.
You can therefore copy the row of input parameters to the first calculation row in the `Phase` worksheet.
There are a few parameters that are no longer needed, because we will set them explicitly ourselves.
These input parameters that needed for steady state calculation, but not needed for phase-wise calculation, can be spotted by the lack of shading in the second row.
Instead, we will have to enter values in the columns shaded blue.
We set the head of the sea to `2.0 mNAP`, and set the initial head and salinity inside the lock chamber to `0.0 mNAP` and `15 kg/m³` respectively.

.. thumbnail:: ../../images/examples_excel_phase_diffcolumns.png
    :width: 50%

Next, we make sure that the `routine` column is set to **0**.
Then, just like with the :doc:`steady` example, we can select one or more cells on this row and press `Run ZSF`.
If all goes well, the output columns will then show the following results

.. thumbnail:: ../../images/examples_excel_phase_initialize_output.png
    :width: 30%

.. note::

    The lock is always initialized empty, i.e., without a ship in it.

Leveling to the lake side
-------------------------

The next step is to level the lock to the lake side.
The lock was already initialized to the head of the lake side, so we expect to see no transports in this phase.
Copy the input of the first row to the second row., and set

    - the leveling time to 300 seconds
    - the routine to **1**

Now press the `Run ZSF` button.
Note that if you had any remaining values in the `Initialize State` columns, these values will be cleared as they are not needed.
Check that the output columns have zero transport of both water and salt.

Opening the door to the lake side
---------------------------------

With the lock leveled to the lake side, the doors can now be opened.

.. important::

    Make sure that the leveling routines (**1** and **3**) have matching heads for the boundary conditions as the subsequent door-open routines (**2** and **4** respectively). If this is not the case, an exception is raised stating this requirement.

Once again, copy the lock dimensions and other inputs to a new row below the two already existing ones.
Remove the leveling time, and set the following parameters:

    - `volume ship down` (lake to sea) to 1000.0 m³
    - `door open time` on the lake side to 840 seconds
    - the routine to **2**


.. thumbnail:: ../../images/examples_excel_phase_routine_2_inputs.png
    :width: 50%

Press the `Run ZSF` button, and if all is correct the outputs should be similar to the following image:

.. thumbnail:: ../../images/examples_excel_phase_routine_2_outputs.png
    :width: 50%

The last columns show the state of the lock, and now indicate that there is a ship in the lock chamber.

Leveling to the sea side
------------------------

The next step is to level the lock to the sea side.
The instructions are equal to those of leveling to the lake side, except that you should set the routine to **3** instead of **1**.
Press the `Run ZSF` button, and inspect the output.

Opening the door to the sea side
--------------------------------

The last step is to open the doors to the sea side, and let the ship sail out and a new ship sail in.
Copy the last row to a new one, and set:

    - `volume ship up` (sea to lake) to 800.0 m³
    - `door open time` on the sea side to 840 seconds
    - the routine to **4**

The inputs should look as follows:

.. thumbnail:: ../../images/examples_excel_phase_routine_4_inputs.png
    :width: 50%

After pressing `Run ZSF`, the output should like like:

.. thumbnail:: ../../images/examples_excel_phase_routine_4_outputs.png
    :width: 50%

Note that the volume of ship inside the lock chamber has changed from 1000 m³ to 800.0 m³.

Calculating more lockages
-------------------------

One can repeat the above process, again adding a row for leveling to the lake side next.
If the water levels and salinities on the lake and/or sea side are changing, you can change these parameters accordingly.
Typically, the water level is set to the average water level during the door-open phase, with the preceding leveling phase also leveling to said water level.
It quickly becomes rather tedious and error-prone to calculate many lockages this way using Excel, especially if a lot of preprocessing is involved to get the parameters per locking phase and the source data is not set in stone.
Depending on your experience with Excel and Python, it might then be easier to use the Python wrapper to do these types of calculations, see the Python :doc:`../python/phase` example.
