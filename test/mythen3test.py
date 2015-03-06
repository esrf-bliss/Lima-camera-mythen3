import PyTango
import numpy
import time

dev=PyTango.DeviceProxy('d26s/mythen3/dcs1')
print "Assembly date         :", dev.read_attribute("assemblyDate").value
print "CommandID             :", dev.read_attribute("commandId").value
print "Max Nos Modules       :", dev.read_attribute("MaxNbModules").value
print "Sensor Material       :", dev.read_attribute("sensorMaterial").value
print "Sensor Thickness      :", dev.read_attribute("sensorThickness").value
print "Version               :", dev.read_attribute("version").value
print "System Number         :", dev.read_attribute("systemNum").value
print "Number of Modules     :", dev.read_attribute("nbModules").value



nframes = 4
exp_time = 0.5
 
lima=PyTango.DeviceProxy('d26s/limaccd/dcs1')
# do not change the order of the saving attributes!
lima.write_attribute("saving_directory","/buffer/dubble281/mythen")
lima.write_attribute("saving_format","HDF5")
lima.write_attribute("saving_overwrite_policy","Overwrite")
lima.write_attribute("saving_suffix", ".hdf")
lima.write_attribute("saving_prefix","mythen_")
lima.write_attribute("saving_mode","AUTO_FRAME")
lima.write_attribute("saving_frames_per_file", nframes)

# do acquisition
lima.write_attribute("acq_nb_frames",nframes)
lima.write_attribute("acq_expo_time",exp_time)
lima.write_attribute("acq_trigger_mode", "INTERNAL_TRIGGER")
lima.command_inout("prepareAcq")
lima.command_inout("startAcq")

while dev.read_attribute("acqRunning").value :
    time.sleep(0.5)

time.sleep(2)
