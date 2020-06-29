Steady-state calculation
~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: ../../images/delfzijl1.jpg

.. :href: https://beeldbank.rws.nl/MediaObject/Details/479988
.. https://beeldbank.rws.nl, Rijkswaterstaat / Harry van Reeken

Overview
--------

The purpose of this example is to understand the basic steps to calculate the salt transports through a shipping lock in steady state operation.
The scenario is the following: A single lock connects a canal to the sea, and the lock is busy during the day but quiet during the night.
We want to know how much salt comes in on average, and figure out ways to reduce the salt intrusion by means of mitigating measures like bubble screens.

Properties of the lock and its operation
----------------------------------------

The physical dimensions of our single shipping lock are:

    - length: 148 m
    - width: 14 m
    - bottom: -4.4 mNAP (Dutch Ordnance Datum)

For the boundary conditions we will assume that the sea level is the same as the canal level, with both being equal to 0.0 mNAP.
The salinity on the salt side is significantly higher than on the canal side.

    - salinities: 5 kg/m³ on the canal side close to the lock, and 25 kg/m³ on the sea side
    - head: 0.0 mNAP on both sides
    - temperature: 15.0 °C on both sides

The last step is to derive basic parameters from the locking information.

    - During daytime, the lock operates at a pace of 1.25 cycles per hour (= 30 cycles per 24 hours).
      During the night, the lock operates at a pace of 10 cycles per 24 hours.
    - It takes 5 minutes to open or close the doors on either side, and also 5 minutes to level.
    - There are the same number of ships going from the sea to the canal, and vice-versa.
    - The ships going to and from the canal also have an equal displacement of 1000 m³.

In the first calculation, the lock does not have any sills or bubble screens, nor is there any flushing.
These are of course measures that we will take a look at later in this example to reduce the amount of salt intrusion.

.. seealso::

    For an overview of these parameters, and more in-depth discussion on them, see :ref:`sec_c_api_input` and :ref:`sec_procdef`.

Salt load without measures
--------------------------

The next step is to enter all these physical and operational characteristics in the Excel workbook.
As we are assuming constant operation and boundary conditions, we can use the `Steady` tab.
We need a total of two rows, one for daytime operation and one for nighttime.
Entering the values above into the Excel sheet should result in a something like shown in the image below.
Note that some field are not used (yet); they will be discussed below in detail.

.. thumbnail:: ../../images/examples_excel_steady_input.png
    :width: 50%

The next step is to select (at least) one cell on each of the rows you want to calculate, and press the `Run ZSF` button in the top-left.
The output columns in the sheet should then take on values similar to those shown in the image below.

.. thumbnail:: ../../images/examples_excel_steady_output.png
    :width: 50%

From these results we can see that there is a salt load of almost 37 kg/s during daytime, and 19 kg/s during nighttime.

Comparing salt intrusion measures
---------------------------------

The maximum allowable salt load has been determined to be 12 kg/s, so the salt load during daytime and nighttime are currently not acceptable.
The ZSF can help compare various salt intrusion measures that can be taken to reduce the salt load to acceptable levels.
For this particular lock sills are not a feasible option, but bubble screens and flushing discharges could be.

A typical maximally efficient bubble screen can reduce the pace of the lock exchange to about 25%.
If we fill in 0.25 at both `η Lake` and `η Sea`, the salt load during daytime is reduced to about 10 kg/s.
However, the salt load at nighttime is reduced much less.
With a value of about 13.5 kg/s it is now even higher than that during the daytime, even though there are fewer ships passing through the lock.

.. thumbnail:: ../../images/examples_excel_steady_etacolumns.png
    :width: 50%

Bubble screens are only effective if the doors are closed well before the (reduced) lock is reduced.
With only 2 to 3 locking cycles during the night, the doors are open for more than an hour at a time, see the output columns `Time Door Lake Open` and `Time Door Sea Open`.

.. thumbnail:: ../../images/examples_excel_steady_timedoorcolumns.png
    :width: 50%

If we can tell the lock operator to close the doors right after ships have finished sailing out, we can reduce the salt intrusion significantly.
To reduce these door-open durations, we can use `Calibration Factor`.
If we know that the doors are open about 20 minutes at a time during the night, we can fill in a value of approximately 0.3 here to reduce the current duration of about an hour with.
Recalculating with this will give a salt load of about 4 kg/s during the night, which is acceptable.
