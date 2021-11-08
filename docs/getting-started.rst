Getting Started
+++++++++++++++

Typical end-users should use either the Excel wrapper, or the Python `pyzsf` package, instructions for which are in the respective sections below.
When embedding the ZSF in other software, the easiest way is to download `libzsf` shared or static libraries for Linux or Windows, see :ref:`getstart_clib`.
It is of course also possible to build the Python and C libraries from source, see :ref:`getstart_fromsource`.

Excel workbook
==============

One of the easiest ways of using the ZSF on Windows is to download the Excel workbook from the `releases <https://gitlab.com/deltares/libzsf/-/releases>`_ page on GitLab.
This will download a zip file containing a macro-enabled Excel file, and two DLLs (one for 32-bit systems, and one for 64-bit systems).
These DLLs contain computational routines that are called using Excel macros.

When opening the workbook, you need to make sure to enable the macros if so prompted, otherwise it will not be possible to calculate anything.

To test whether the workbook runs as intended, select cells `AH4:BI4` that contain the output of the first row of input, and delete their contents.

.. thumbnail:: images/getting_started_excel.png
    :width: 50%

Next, with the cells still selected, press the `Run ZSF` button.
If correct, the output that was there should reappear.
For further instructions, it is best to go through the examples.

Troubleshooting
---------------

If nothing happens, there is a chance the macro did not run, because the button is unresponsive.
To test whether this is the case, and check that macros are indeed able to run, go to `File -> Options -> Customize Ribbon` and enable the Developer mode.

.. thumbnail:: images/getting_started_excel_enable_developer.png
    :width: 50%

With that done, select the Developer tab on the ribbon, and run the macro manually (still with the cells on row 4 selected).

.. thumbnail:: images/getting_started_excel_run_macro_manually.png
    :width: 50%

Python package
==============

The Python package of the ZSF is called `pyzsf`.
Although not required, it is recommended to install it in a virtual environment.
See the `official Python tutorial <https://docs.python.org/3/tutorial/venv.html>`_ for more information on how to set up and activate a virtual environment.

`pyzsf`, including its dependencies, can be installed using the `pip <https://pip.pypa.io/>`_ package manager::

    # Install pyzsf using the pip package manager
    pip install pyzsf

.. note::

    Pip version 20.0 or higher is required to install `pyzsf`, or it will fail to find a matching distribution for your platform.
    If you have an older version, please run ``python -m pip install -U pip`` before installing pyzsf.

To test whether it works, import pyzsf and call its ``zsf_calc_steady()`` function.

.. thumbnail:: images/getting_started_pyzsf.png
    :width: 50%

.. _getstart_clib:

C library
=========

Static and dynamic libraries for both Windows and Linux are available on the `releases <https://gitlab.com/deltares/libzsf/-/releases>`_ page on GitLab.
The Linux libraries are built using the `manylinux2010 <https://www.python.org/dev/peps/pep-0571/>`_ docker image, and should be therefore be compatible with most versions of Linux.

Fortran interface
=================

A wrapper is provided to easily call the static and dynamic libraries from Fortran.
See the `releases <https://gitlab.com/deltares/libzsf/-/releases>` page on GitLab, or download the ``zsf.f90`` interface file directly from the `git tree <https://gitlab.com/deltares/libzsf/-/tree/master/wrappers/fortran>`_.

.. _getstart_fromsource:

From Source
===========

The latest libzsf source can be downloaded using `Git <https://git-scm.com/>`_::

    # Get libzsf source
    git clone https://gitlab.com/deltares/libzsf.git

Note that `cmake` is needed to build libzsf, and a working Python installation is required to build the pyzsf wrapper.
For more detailed build instructions, it is probably easiest to look at the ``build:windows`` and ``build:linux`` sections in the `.gitlab.yml` file in the root of the source tree.
These instructions are always up to date, and give a concise and clear overview of the steps required to build from source.
