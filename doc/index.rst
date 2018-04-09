.. _camera-frelon:

Frelon camera
--------------

.. image:: FReLoN-2k.jpg

Introduction
````````````

The FReLoN camera is a 14 bit dynamic CCD camera, with a 2048*2048 pixel chip. This camera has been developped by the awesome people with the 'Analog and Transient Electronic' ESRF group.

Prerequisite
````````````

Installation & Module configuration
```````````````````````````````````

Follow the generic instructions in :ref:`build_installation`. If using CMake directly, add the following flag:

.. code-block:: sh

 -DLIMACAMERA_FRELON=true

For the Tango server installation, refers to :ref:`tango_installation`.

Initialisation and Capabilities
```````````````````````````````

Implementing a new plugin for new detector is driven by the LIMA framework but the developer has some freedoms to choose which standard and specific features will be made available. This section is supposed to give you the correct information regarding how the camera is exported within the LIMA framework.

Camera initialisation
.....................

The Frelon plugin provides a helper class :cpp:class:`FrelonAcq` which manages the initialisation sequence with
the camera and interface object. An Espia board channel number should be set as the initialisation
parameter (default is 0).

.. code-block:: c++

  frelon = Frelon.FrelonAcq(int(espia_dev_nb))
  return frelon.getGlobalControl()

Std capabilites
...............

This plugin has been implemented in respect of the mandatory capabilites but with limitations according
due to the detector specific features and with some programmer's  choices.  We do not explain here the standard Lima capabilites
but you can find in this section the useful information on the Dexela specfic features.

* HwDetInfo

 TODO

* HwSync

 TODO

Optional capabilites
....................

In addition to the standard capabilities, we make the choice to implement some optional capabilities in order to
have an improved simulation.

* HwShutter

 TODO

* HwRoi

 TODO

* HwBin

 TODO

Configuration
`````````````

The main configuration will consist in providing the correct ``DexelaConfig.cfg`` file to the detector API.
The file has to be provided by the manufacturer with a second file like ``sensor2923.fmt``. The ``.fmt`` file contains some calibration data.

How to use
````````````

The LimaCCDs tango server provides a complete interface to the dexela plugin so feel free to test.

For a quick test one can use python, this is a short example code:

.. code-block:: python

  from Lima import Frelon
  from lima import Core
  import time

  FrelonAcq  = Frelon.FrelonAcq(int(espia_dev_nb))
  control = FrelonAcq.getGlobalControl()

  acq = control.acquisition()

  # setting new file parameters and autosaving mode
  saving=control.saving()

  pars=saving.getParameters()
  pars.directory='/tmp/'
  pars.prefix='testfrelon_'
  pars.suffix='.edf'
  pars.fileFormat=Core.CtSaving.EDF
  pars.savingMode=Core.CtSaving.AutoFrame
  saving.setParameters(pars)


  # now ask for 2 sec. exposure and 10 frames
  acq.setAcqExpoTime(2)
  acq.setNbImages(10)

  acq.prepareAcq()
  acq.startAcq()

  # wait for last image (#9) ready
  lastimg = control.getStatus().ImageCounters.LastImageReady
  while lastimg !=9:
    time.sleep(1)
    lastimg = control.getStatus().ImageCounters.LastImageReady

  # read the first image
  im0 = control.ReadImage(0)
