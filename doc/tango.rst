Mythen3 Tango device
======================

This is the reference documentation of the Mythen3 Tango device.

you can also find some useful information about the camera models/prerequisite/installation/configuration/compilation in the :ref:`Xspress3 camera plugin <camera-mythen3>` section.


Properties
----------

================= =============== =============== =========================================================================
Property name	  Mandatory       Default value   Description
================= =============== =============== =========================================================================
HostName          Yes                             The Mythen detector socket server IP address 
TcpPort           No              1031            The tcp communication port. 
Simulate          No              0               Command simulation mode.
================= =============== =============== =========================================================================

Attributes
----------
======================= ======= ================ ======================================================================
Attribute name		    RW	    Type			 Description
======================= ======= ================ ======================================================================
acqRunning              ro      DevBoolean       Is acquisition active
assemblyDate            ro      DevString        Assembly date of the Mythen system
badChannelInterpolation rw      DevString        Enable/Disable Bad Channel Interpolation Mode (**ON/OFF**)
badChannels             ro      DevLong[1280*Nb] Display state of each channel for each active module [Nb = nbModules]
commandID               ro      DevLong          Command identifier (increases by 1)
continuousTrigger       rw      DevString        Enable/Disable continuous trigger mode (**ON/OFF**)
cutoff                  ro      DevLong          Count value before flatfield correction
delayBeforeFrame        rw      DevLong64        Time delay between trigger & start (100ns increments)
energy                  rw      DevFloat[Nb]     X-ray Energy (4.09 < e keV < 40) [Nb = nbModules]
energyMax               ro      DevFloat         Maximum X-ray Energy keV
energyMin               ro      DevFloat         Minimum X-ray Energy keV
flatField               ro      DevLong[1280*Nb] Flat field correction values
flatFieldCorrection     rw      DevString        Enable/Disable Flat Field Correction Mode (**ON/OFF**)
gateMode                rw      DevString        Enable/Disable gate mode (**ON/OFF**)
gates                   rw      DevLong          Number of gates per frame
hwStatus                ro      DevString        The hardware status
inputSignalPolarity     rw      DevString        Input Signal Polarity (**RISING_EDGE/FALLING_EDGE**)
kthresh                 ro      DevFloat[Nb]     Threshold Energy (4.0 < e keV < 20) [Nb = nbModules]
kthreshEnergy           w       DevFloat[2]      Threshold & Energy keV
kthreshMax              ro      DevFloat         Maximum Threshold Energy keV
kthreshMin              ro      DevFloat         Minimum Threshold Energy keV
maxNbModules            ro      DevLong          Maximum nos. of Mythen modules
module                  rw      DevLong          Number of selected module (-1 = all)
nbits                   rw      DevString        Number of bits to readout (**BPP24/BPP16/BPP8/BPP4**)
nbModules               rw      DevLong          Number of modules in the system
outputSignalPolarity    rw      DevString        Output Signal Polarity (**RISING_EDGE/FALLING_EDGE**)
predefinedSettings      w       DevString        Load predefined energy/kthresh settings (**Cu/Ag/Mo/Cr**)
rateCorrection          rw      DevString        Enable/Disable rate correction mode (**ON/OFF**)
sensorMaterial          ro      DevLong          The sensor material (0=silicon)
sensorThickness         ro      DevLong          The sensor thickness um
serialNumbers           ro      DevLong[Nb]      Serial nos. of Mythen modules [Nb = nbModules]
systemNum               ro      DevLong          The serial number of the Mythen
tau                     rw      DevFloat[Nb]     Dead time constants for rate correction [Nb = nbModules]
testPattern             ro      DevLong[1280*Nb] Read back a test pattern
triggered               rw      DevString        Enable/Disable triggered mode (**ON/OFF**)
useRawReadout           rw      DevString        Raw readout packed Mode (**ON/OFF**)
version                 ro      DevString        The software version of the socket server
======================= ======= ================ ======================================================================

Commands
--------

=======================	================ ======================= ===========================================
Command name		    Arg. in		     Arg. out		         Description
=======================	================ ======================= ===========================================
Init			DevVoid 	 DevVoid                 Do not use
State			DevVoid		 DevLong                 Return the device state
Status			DevVoid		 DevString               Return the device state as a string
LogStart	        DevVoid 	 DevVoid                 Start logging server activity (use sparingly)
LogStop 		DevVoid 	 DevVoid                 Stop logging server activity
LogRead		        DevVoid 	 DevVoid                 Print logging file to terminal
ReadFrame               DevLong          DevVarULongArray        [in] frame number [out] a frame of mythen data
ReadData		DevVoid 	 DevVarULongArray        [out] all frames of mythen data
ResetMythen             DevVoid          DevVoid                 Reset
=======================	================ ======================= ===========================================
