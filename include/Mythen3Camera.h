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

#ifndef MYTHEN3CAMERA_H
#define MYTHEN3CAMERA_H

#include <map>
#include <vector>
#include "lima/Debug.h"
#include "lima/Constants.h"
#include "lima/HwMaxImageSizeCallback.h"
#include "lima/HwBufferMgr.h"
#include "lima/ThreadUtils.h"
#include "processlib/Data.h"
#include "Mythen3Net.h"

namespace lima {
namespace Mythen3 {

const int XPixelSize = 50; // um
const int YPixelSize = 8000; // um
const int PixelsPerModule = 1280;

class BufferCtrlObj;

/*******************************************************************
 * \class Camera
 * \brief object controlling the Mythen3 camera
 *******************************************************************/
class Camera {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Mythen3");

public:
	Camera(std::string hostname, int tcpPort, bool simulate=false);
	~Camera();

	enum Status {
		Running = 0x1,           ///<
		WaitForTrigger = 0x8,    ///<
		NoDataInBuffer = 0x10000 ///<
	};
	void init();
	void reset();
	void prepareAcq();
	void startAcq();
	void stopAcq();
	void getStatus(Status& status);
	int getNbHwAcquiredFrames();

// -- detector info object
	void getImageType(ImageType& type);
	void setImageType(ImageType type);

	void getDetectorType(std::string& type);
	void getDetectorModel(std::string& model);
	void getDetectorImageSize(Size& size);
	void getPixelSize(double& sizex, double& sizey);

// -- Buffer control object
	HwBufferCtrlObj* getBufferCtrlObj();

//-- Synch control object
	void setTrigMode(TrigMode mode);
	void getTrigMode(TrigMode& mode);

	void setExpTime(double exp_time);
	void getExpTime(double& exp_time);

	void setLatTime(double lat_time);
	void getLatTime(double& lat_time);

	void getExposureTimeRange(double& min_expo, double& max_expo) const;
	void getLatTimeRange(double& min_lat, double& max_lat) const;

	void setNbFrames(int nb_frames);
	void getNbFrames(int& nb_frames);

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

	void getAssemblyDate(string& date);
	void getBadChannels(Data& badChannels);
	void getCommandId(int& commandId);
	void getSerialNumbers(std::vector<int>& serialNums);
	void getMaxNbModules(int& maxmod);
	void getSensorMaterial(int& sensor);
	void getSensorThickness(int& thickness);
	void getSystemNum(int& systemNum);
	void getVersion(string& version);
	void getModule(int& module);
	void setModule(int module);
	void getNbModules(int& nbModule);
	void setNbModules(int nbModule);
	void getDelayAfterFrame(long long& time);
	void setDelayAfterFrame(long long time);
	void getFrames(int& frames);
	void setFrames(int frames);
	void getNbits(Nbits& nbits);
	void setNbits(Nbits nbits);
	void getTime(long long& time);
	void setTime(long long time);
	void getHwStatus(Status& status);
	void getEnergy(std::vector<float>& energy);
	void setEnergy(float energy);
	void getEnergyMax(float& energy);
	void getEnergyMin(float& energy);
	void getKThresh(std::vector<float>& kthresh);
	void setKThresh(float kthresh);
	void getKThreshMax(float& kthresh);
	void getKThreshMin(float& kthresh);
	void setKThreshEnergy(float kthresh, float energy);
	void setPredefinedSettings(Settings settings);
	void getBadChannelInterpolation(Switch& enable);
	void setBadChannelInterpolation(Switch enable);
	void getFlatFieldCorrection(Switch& enable);
	void setFlatFieldCorrection(Switch enable);
	void getCutoff(int& cutoff);
	void getFlatField(Data& flatfield);
	void getRateCorrection(Switch& enable);
	void setRateCorrection(Switch enable);
	void getTau(std::vector<float>& tau);
	void setTau(float tau);
	void getGates(int& gates);
	void setGates(int gates);
	void getDelayBeforeFrame(long long& time);
	void setDelayBeforeFrame(long long time);
	void getContinuousTrigger(Switch& enable);
	void setContinuousTrigger(Switch enable);
	void getGateMode(Switch& enable);
	void setGateMode(Switch enable);
	void getTriggered(Switch& enable);
	void setTriggered(Switch enable);
	void getInputSignalPolarity(Polarity& polarity);
	void setInputSignalPolarity(Polarity polarity);
	void getOutputSignalPolarity(Polarity& polarity);
	void setOutputSignalPolarity(Polarity polarity);
	void setUseRawReadout(Switch enable);
	void getUseRawReadout(Switch& enable);
	void getTestPattern(Data& data);
	void resetMythen();
	void start();
	void stop();
	void logStart();
	void logStop();
	void logRead();
	void readFrame(Data& mythenData, int frame_nb);
	void readData(Data& mythenData);


private:

	Mythen3Net* m_mythen;
	string m_hostname;
	int m_tcpPort;
	bool m_simulated;
	bool m_thread_running;
	bool m_wait_flag;
	bool m_quit;
	int m_image_width;
	int m_acq_frame_nb; // nos of frames acquired
	int m_nb_frames; // nos of frame to acquire
	double m_exp_time;
	TrigMode m_trigger_mode;
	ImageType m_image_type;
	mutable Cond m_cond;
	bool m_use_raw_readout;
	Nbits m_nbits;
	int m_logSize;

	class AcqThread;

	AcqThread *m_acq_thread;

	// Buffer control object
	SoftBufferCtrlObj m_bufferCtrlObj;

	enum Action {GET, SET, CMD};
#if __GNUC_MINOR__ < 4
public:
#endif
	// V3.0.0 defines deprecated commands which are not included in this implementation
	enum ServerCmd {ASSEMBLYDATE, BADCHANNELS, COMMANDID, MODNUM, MODULE, NMAXMODULES, NMODULES, SENSORMATERIAL,
		SENSORTHICKNESS, SYSTEMNUM, VERSION, RESET, DELAFTER, FRAMES, NBITS, STATUS, TIME, READOUT, READOUTRAW,
		START, STOP, ENERGY, ENERGYMAX, ENERGYMIN, KTHRESH, KTHRESHMAX, KTHRESHMIN, KTHRESHENERGY, SETTINGS,
		BADCHANNELINTERPOLATION, FLATFIELDCORRECTION, CUTOFF, FLATFIELD, RATECORRECTION, TAU, CONTTRIGEN, DELBEF,
		GATEEN, GATES, CONTTRIG, GATE, INPOL, OUTPOL, TRIG, TRIGEN, LOGSTART, LOGSTOP, LOGREAD, TESTPATTERN};
#if __GNUC_MINOR__ < 4
private:
#endif

	string findCmd(Action action, ServerCmd cmd);
	template<typename T> void checkReply(T rc);
	template<typename T> void requestSet(ServerCmd cmd, T value);
	template<typename T> void requestSet(ServerCmd cmd, T value1, T value2);
	template<typename T> void requestGet(ServerCmd cmd, T& value);
	template<typename T> void requestGet(ServerCmd cmd, T* value, int len);
	template<typename T> void requestCmd(ServerCmd cmd, T* value, int len);
	void requestGet(ServerCmd cmd, string& reply, int len);
	void requestCmd(ServerCmd cmd);
	void sendCmd(Action action, ServerCmd cmd);
	void sendCmd(Action action, ServerCmd cmd, uint8_t* recvBuf, int len);
	template<typename T> void sendCmd(Action action, ServerCmd cmd, T value);
	template<typename T> void sendCmd(Action action, ServerCmd cmd, T value1, T value2);
	void simulate(Action action, ServerCmd cmd, uint8_t* recvBuf, int len);
	void readout(uint32_t* data, int len);
	void readoutRaw(uint32_t* data, int len);
	void decodeRaw(Nbits nbits, uint32_t* rawData, int image_width);

	static std::map<int, std::string> serverStatusMap;
	static std::map<ServerCmd, std::string> serverCmdMap;
};

std::ostream& operator <<(std::ostream& os, Camera::Polarity const &polarity);
std::ostream& operator <<(std::ostream& os, Camera::Settings const &settings);

} // namespace Mythen3
} // namespace Lima

#endif //MYTHEN3CAMERA_H
