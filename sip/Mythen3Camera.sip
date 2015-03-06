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
#include <string>
%End

public:
	Camera(std::string hostname, int tcpPort, bool simulate);
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

	enum Polarity {
		RISING_EDGE,   ///< active high
		FALLING_EDGE,  ///< active low
	};

	enum Switch {
		OFF,   ///< disable
		ON,    ///< enable
	};
	enum Nbits {
		BPP4 = 4,    ///< 4 bits
		BPP8 = 8,    ///< 8 bits
		BPP16 = 16,  ///< 16 bits
		BPP24 = 24,  ///< 24 bits (default)
	};
	enum Settings {
		Cu,  ///< kthreshEnergy(4.8,5.41) (default)
		Mo,  ///< kthreshEnergy(6.4,8.05)
		Cr,  ///< kthreshEnergy(8.74,17.48)
		Ag,  ///< kthreshEnergy(11.08,22.16)
	};

	void getAssemblyDate(std::string& date /Out/);
	void getBadChannels(Data& badChannels /Out/);
	void getCommandId(int& commandId /Out/);
	void getSerialNumbers(std::vector<int>& serialNums /Out/);
	void getMaxNbModules(int& maxmod /Out/);
	void getSensorMaterial(int& sensor /Out/);
	void getSensorThickness(int& thickness /Out/);
	void getSystemNum(int& systemNum /Out/);
	void getVersion(std::string& version /Out/);
	void getModule(int& module /Out/);
	void setModule(int module);
	void getNbModules(int& nbModule /Out/);
	void setNbModules(int nbModule);
	void getDelayAfterFrame(long long& time /Out/);
	void setDelayAfterFrame(long long time);
	void getFrames(int& frames /Out/);
	void setFrames(int frames);
	void getNbits(Nbits& nbits /Out/);
	void setNbits(Nbits nbits);
	void getTime(long long& time /Out/);
	void setTime(long long time);
	void getHwStatus(Status& status /Out/);
	void getEnergy(std::vector<float>& energy /Out/);
	void setEnergy(float energy);
	void getEnergyMax(float& energy /Out/);
	void getEnergyMin(float& energy /Out/);
	void getKThresh(std::vector<float>& kthresh /Out/);
	void setKThresh(float kthresh);
	void getKThreshMax(float& kthresh /Out/);
	void getKThreshMin(float& kthresh /Out/);
	void setKThreshEnergy(float kthresh, float energy);
	void setPredefinedSettings(Settings settings);
	void getBadChannelInterpolation(Switch& enable /Out/);
	void setBadChannelInterpolation(Switch enable);
	void getFlatFieldCorrection(Switch& enable /Out/);
	void setFlatFieldCorrection(Switch enable);
	void getCutoff(int& cutoff /Out/);
	void getFlatField(Data& flatfield /Out/);
	void getRateCorrection(Switch& enable /Out/);
	void setRateCorrection(Switch enable);
	void getTau(std::vector<float>& tau /Out/);
	void setTau(float tau);
	void getGates(int& gates /Out/);
	void setGates(int gates);
	void getDelayBeforeFrame(long long& time /Out/);
	void setDelayBeforeFrame(long long time);
	void getContinuousTrigger(Switch& enable /Out/);
	void setContinuousTrigger(Switch enable);
	void getGateMode(Switch& enable /Out/);
	void setGateMode(Switch enable);
	void getTriggered(Switch& enable /Out/);
	void setTriggered(Switch enable);
	void getInputSignalPolarity(Polarity& polarity /Out/);
	void setInputSignalPolarity(Polarity polarity);
	void getOutputSignalPolarity(Polarity& polarity /Out/);
	void setOutputSignalPolarity(Polarity polarity);
	void setUseRawReadout(Switch enable);
	void getUseRawReadout(Switch& enable /Out/);
	void getTestPattern(Data& data /Out/);
	void resetMythen();
	void start();
	void stop();
	void logStart();
	void logStop();
	void logRead();
	void readFrame(Data& mythenData /Out/, int frame_nb);
	void readData(Data& mythenData /Out/);
};

}; // namespace Mythen3
