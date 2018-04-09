.. _camera-mythen3:

Dectris Mythen3
---------------

.. image:: mythen.jpg

Intoduction
```````````

Server for the control of a Mythen detector.

Module configuration
````````````````````

Follow the generic instructions in :ref:`build_installation`. If using CMake directly, add the following flag:

.. code-block:: sh

 -DLIMACAMERA_MYTHEN=true

For the Tango server installation, refers to :ref:`tango_installation`.

Testing
````````````

Here is a simple python test program:

.. code-block:: python

  import time
  from Lima import Mythen3
  from Lima import Core
  import time

  camera = Mythen3.Camera("160.103.146.190", 1031, False)
  interface = Mythen3.Interface(camera)
  control = Core.CtControl(interface)

  # check its OK
  print camera.getDetectorType()
  print camera.getDetectorModel()
  print camera.getVersion()

  nframes=10
  acqtime=2.0
  # setting new file parameters and autosaving mode
  saving=control.saving()
  saving.setDirectory("/buffer/dubble281/mythen")
  saving.setFramesPerFile(nframes)
  saving.setFormat(Core.CtSaving.HDF5)
  saving.setPrefix("mythen3_")
  saving.setSuffix(".hdf")
  saving.setSavingMode(Core.CtSaving.AutoFrame)
  saving.setOverwritePolicy(Core.CtSaving.Overwrite)

  # do acquisition
  acq = control.acquisition()
  acq.setAcqExpoTime(acqtime)
  acq.setAcqNbFrames(nframes)

  control.prepareAcq()
  control.startAcq()
  time.sleep(25)
