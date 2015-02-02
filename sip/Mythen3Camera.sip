//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

namespace Mythen3 {

class Camera {

%TypeHeaderCode
#include "Mythen3Camera.h"
%End

public:
	Camera(std::string hostname, int tcpPort, int npixels=1280, bool simulate=false);
	~Camera();

	enum Status {
		Running = 0x1,
		WaitForTrigger = 0x8,
		NoDataInBuffer = 0x10000
	};
	void init();
	void reset();
	void prepareAcq();
	void startAcq();
	void stopAcq();
	void getStatus(Status& status /Out/);
	int getNbHwAcquiredFrames();

// -- detector info object
	void getImageType(ImageType& type /Out/);
	void setImageType(ImageType type);

	void getDetectorType(std::string& type /Out/);
	void getDetectorModel(std::string& model /Out/);
	void getDetectorImageSize(Size& size /Out/);
	void getPixelSize(double& sizex /Out/, double& sizey /Out/);

// -- Buffer control object
	HwBufferCtrlObj* getBufferCtrlObj();

//-- Synch control object
	void setTrigMode(TrigMode mode);
	void getTrigMode(TrigMode& mode /Out/);

	void setExpTime(double exp_time);
	void getExpTime(double& exp_time /Out/);

	void setLatTime(double lat_time);
	void getLatTime(double& lat_time /Out/);

	void getExposureTimeRange(double& min_expo /Out/, double& max_expo /Out/) const;
	void getLatTimeRange(double& min_lat /Out/, double& max_lat /Out/) const;

	void setNbFrames(int nb_frames);
	void getNbFrames(int& nb_frames /Out/);

	bool isAcqRunning() const;

///////////////////////////////
// -- mythen3 specific functions
///////////////////////////////

	enum Polarity {RISING_EDGE, FALLING_EDGE};
	enum Switch {OFF, ON};
	enum Nbits {BPP4=4, BPP8=8, BPP16=16, BPP24 = 24};
	enum Settings {Cu, Mo, Cr, Ag};

	void getAssemblyDate(string& date /Out/);
	void getBadChannels(int *badChannels /Out/, int len);
	void getCommandId(int& commandid /Out/);
	void getSerialNumbers(int* serialNums /Out/, int len);
	void getModule(int& module /Out/);
	void getMaxNbModules(int& maxmod /Out/);
	void getNbModules(int& nmod /Out/);
	void getSensorMaterial(int& sensor /Out/);
	void getSensorThickness(int& thickness /Out/);
	void getSystemNum(int& systemNum /Out/);
	void getVersion(string& version /Out/);
	void getLatency(long long& time /Out/);
	void getFrames(int& frames /Out/);
	void getNbits(Nbits& nbits /Out/);
	void getHwStatus(Status& status /Out/);
	void getTime(long long& time /Out/);
	void getEnergy(float& energy /Out/);
	void getEnergyMax(float& energyMax /Out/);
	void getEnergyMin(float& energyMin /Out/);
	void getKThresh(float& kthresh /Out/);
	void getKThreshMax(float& kthreshmax /Out/);
	void getKThreshMin(float& kthreshmin /Out/);
	void getBadChannelInterpolation(Switch& enable /Out/);
	void getCutoff(int& cutoff /Out/);
	void getFlatField(int& flatfield /Out/);
	void getFlatFieldCorrection(Switch& enable /Out/);
	void getRateCorrection(Switch& enable /Out/);
	void getTau(int& tau /Out/);
	void getContinuousTrigger(Switch& enable /Out/);
	void getDelBef(int& delbef /Out/);
	void getGateMode(Switch& enable /Out/);
	void getGates(int& gates /Out/);
	void getTriggered(Switch& enable /Out/);

	void setModule(int num);
	void setNbModules(int nmodule);
	void setFrames(int frames);
	void setNbits(Nbits nbits);
	void setGates(int gates);
	void setLatency(long long time);
	void setDelayAfterTrigger(long long time);
	void setTime(long long time);
	void setAutoReadoutTime(long long time);
	void setEnergy(float energy);
	void setKThresh(float kthresh);
	void setTau(float tau);
	void setKthreshEnergy(float kthresh, float energy);
	void setPredefinedSettings(Settings settings);
	void setBadChannelInterpolation(Switch enable);
	void setFlatFieldCorrection(Switch enable);
	void setRateCorrection(Switch enable);
	void setContinuousTrigger(Switch enable);
	void setGateMode(Switch enable);
	void setTriggered(Switch enable);

	void getInputSignalPolarity(Polarity& polarity /Out/);
	void setInputSignalPolarity(Polarity polarity);

	void getOutputSignalPolarity(Polarity& polarity /Out/);
	void setOutputSignalPolarity(Polarity polarity);

	void readout(int* data /Out/, int len);
	void readoutraw(int* data /Out/, int len);
	void resetMythen();
	void start();
	void stop();
	void logStart();
	void logStop(int& logSize /Out/);
	void logRead(char* buff /Out/, int logSize);
};

}; // namespace Mythen3
