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

#include <errno.h>
#include <cmath>
#include <pthread.h>
#include <map>
#include <limits.h>
#include "lima/Exceptions.h"
#include "lima/Debug.h"
#include "lima/MiscUtils.h"
#include "Mythen3Camera.h"

using namespace lima;
using namespace lima::Mythen3;
using namespace std;

//---------------------------
//- utility thread
//---------------------------
class Camera::AcqThread: public Thread {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "AcqThread");
public:
	AcqThread(Camera &aCam);
	virtual ~AcqThread();

protected:
	virtual void threadFunction();

private:
	Camera& m_cam;
};

Camera::Camera(std::string hostname, int tcpPort, bool simulate) :
		m_hostname(hostname), m_tcpPort(tcpPort), m_simulated(simulate), m_acq_frame_nb(-1),
		m_nb_frames(1), m_image_type(Bpp32), m_bufferCtrlObj() {
	DEB_CONSTRUCTOR();

	DebParams::setModuleFlags(DebParams::AllFlags);
	DebParams::setTypeFlags(DebParams::AllFlags);
	DebParams::setFormatFlags(DebParams::AllFlags);
	m_use_raw_readout = false;
	m_acq_thread = new AcqThread(*this);
	m_acq_thread->start();
	if (!m_simulated) {
		init();
	}
}

Camera::~Camera() {
	DEB_DESTRUCTOR();
	if (!m_simulated) {
		delete m_mythen;
	}
	delete m_acq_thread;
}

void Camera::init() {
	DEB_MEMBER_FUNCT();
	m_mythen = new Mythen3Net();
	DEB_TRACE() << "Mythen3 connecting to " << DEB_VAR2(m_hostname, m_tcpPort);
	m_mythen->connectToServer(m_hostname, m_tcpPort);
	resetMythen();
}

void Camera::reset() {
	DEB_MEMBER_FUNCT();
	stopAcq();
	resetMythen();
	m_image_type = Bpp24;
}

void Camera::prepareAcq() {
	DEB_MEMBER_FUNCT();
	getNbits(m_nbits);
}

void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	m_acq_frame_nb = 0; // Number of frames of data acquired;
	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = false;
	m_quit = false;
	m_cond.broadcast();
}

void Camera::stopAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = true;
}

void Camera::getStatus(Camera::Status& status) {
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	getHwStatus(status);
}

int Camera::getNbHwAcquiredFrames() {
	DEB_MEMBER_FUNCT();
	return m_acq_frame_nb;
}

void Camera::AcqThread::threadFunction() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cam.m_cond.mutex());
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();
	bool useRaw;

	while (!m_cam.m_quit) {
		while (m_cam.m_wait_flag && !m_cam.m_quit) {
			DEB_TRACE() << "Wait";
			m_cam.m_thread_running = false;
			m_cam.m_cond.broadcast();
			m_cam.m_cond.wait();
		}
		if (m_cam.m_quit)
			return;

		DEB_TRACE() << "AcqThread Running" << DEB_VAR2(m_cam.m_wait_flag,m_cam.m_quit);
		m_cam.start();
		m_cam.m_thread_running = true;

		m_cam.m_cond.broadcast();
		Nbits nbits = m_cam.m_nbits;
		useRaw = (nbits == Camera::BPP24) ? false : m_cam.m_use_raw_readout;
		int width = m_cam.m_image_width;
		int size = width / (CHAR_BIT * sizeof(int) / nbits);
		DEB_TRACE() << DEB_VAR4(nbits, useRaw, width, size);
		aLock.unlock();

		bool continueFlag = true;
		while (continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames)) {

			void* bptr = buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);
			if (useRaw) {
				m_cam.readoutRaw((uint32_t*) bptr, size);
				m_cam.decodeRaw(nbits, (uint32_t*) bptr, width);
			} else {
				m_cam.readout((uint32_t*) bptr, width);
			}
			HwFrameInfoType frame_info;
			frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;
			continueFlag = buffer_mgr.newFrameReady(frame_info);
			DEB_TRACE() << "acqThread::threadFunction() newframe ready ";
			++m_cam.m_acq_frame_nb;
			DEB_TRACE() << "acquired " << m_cam.m_acq_frame_nb
					<< " frames, required " << m_cam.m_nb_frames << " frames";
			if (m_cam.m_wait_flag) {
				m_cam.stop();
				DEB_TRACE() << "acqThread::threadFunction() stop acquisition requested";
				break;
			}

		}
		aLock.lock();
		m_cam.m_wait_flag = true;
	}
}
extern int pthread_attr_setscope(pthread_attr_t *__attr, int __scope);

Camera::AcqThread::AcqThread(Camera& cam) : m_cam(cam) {
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_wait_flag = true;
	m_cam.m_quit = false;
	aLock.unlock();
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	aLock.unlock();
}

void Camera::getImageType(ImageType& type) {
	DEB_MEMBER_FUNCT();
	type = m_image_type;
}

void Camera::setImageType(ImageType type) {
	DEB_MEMBER_FUNCT();
	THROW_HW_ERROR(Error) << "Image type " << type << " not supported";

}

void Camera::getDetectorType(std::string& type) {
	DEB_MEMBER_FUNCT();
	type = "Mythen3";
}

void Camera::getDetectorModel(std::string& model) {
	DEB_MEMBER_FUNCT();
	getVersion(model);
}

void Camera::getDetectorImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	m_image_width = PixelsPerModule * nbModules;
	size = Size(m_image_width, 1);
}

void Camera::getPixelSize(double& sizex, double& sizey) {
	DEB_MEMBER_FUNCT();
	sizex = XPixelSize;
	sizey = YPixelSize;
}

HwBufferCtrlObj* Camera::getBufferCtrlObj() {
	return &m_bufferCtrlObj;
}

void Camera::setTrigMode(TrigMode mode) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setTrigMode() " << DEB_VAR1(mode);
	m_trigger_mode = mode;
	double exp_time;
	getExpTime(exp_time);
	if (m_trigger_mode == ExtGate) {
		setGateMode(Camera::ON);
		if (exp_time != 0) {
			setExpTime(0);
		}
	} else if (m_trigger_mode == ExtTrigMult) {
		setContinuousTrigger(Camera::ON);
		setTriggered(Camera::ON);
		if (exp_time != 0) {
			setExpTime(0);
		}
	} else if (m_trigger_mode == ExtTrigSingle) {
		setContinuousTrigger(Camera::OFF);
		setTriggered(Camera::ON);
	} else {
		setContinuousTrigger(Camera::OFF);
		setTriggered(Camera::OFF);
		setGateMode(Camera::OFF);
	}
}

void Camera::getTrigMode(TrigMode& mode) {
	DEB_MEMBER_FUNCT();
	mode = m_trigger_mode;
	DEB_RETURN() << DEB_VAR1(mode);
}

/**
 * Converts 100ns increments from Mythen to seconds
 */
void Camera::getExpTime(double& exp_time) {
	DEB_MEMBER_FUNCT();
	long long time;
	getTime(time);
	exp_time = time / 10000000.0;
}

void Camera::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	long long time = static_cast<long long>(exp_time * 10000000);
	setTime(time);
	m_exp_time = exp_time;
}

void Camera::setLatTime(double lat_time) {
	DEB_MEMBER_FUNCT();
	setDelayAfterFrame(static_cast<long long>(lat_time));
}

void Camera::getLatTime(double& lat_time) {
	DEB_MEMBER_FUNCT();
	long long time;
	getDelayAfterFrame(time);
	lat_time = static_cast<double>(time);
}

#ifndef UINT64_MAX
#define UINT64_MAX (18446744073709551615ULL)
#endif
void Camera::getExposureTimeRange(double& min_expo, double& max_expo) const {
	DEB_MEMBER_FUNCT();
	min_expo = 0.;
	// --- do not know how to get the max_expo, fix it as the max long long
	max_expo = (double) UINT64_MAX;
	DEB_RETURN() << DEB_VAR2(min_expo, max_expo);
}

void Camera::getLatTimeRange(double& min_lat, double& max_lat) const {
	DEB_MEMBER_FUNCT();
	min_lat = 0.;
	// --- do not know how to get the max_lat, fix it as the max long long
	max_lat = (double) UINT64_MAX;
	DEB_RETURN() << DEB_VAR2(min_lat, max_lat);
}

void Camera::setNbFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setNbFrames() " << DEB_VAR1(nb_frames);
	if (m_nb_frames < 0) {
		THROW_HW_ERROR(Error) << "Number of frames to acquire has not been set";
	}
	setFrames(nb_frames);
	m_nb_frames = nb_frames;
}

void Camera::getNbFrames(int& nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getNbFrames";
	getFrames(m_nb_frames);
	DEB_RETURN() << DEB_VAR1(m_nb_frames);
	nb_frames = m_nb_frames;
}

bool Camera::isAcqRunning() const {
	AutoMutex aLock(m_cond.mutex());
	return m_thread_running;
}

///////////////////////////////
// -- mythen specific functions
///////////////////////////////

/**
 * Return the assembly date of the system.
 * @param[out] date the assembly date as a string.
 */
void Camera::getAssemblyDate(string& date) {
	DEB_MEMBER_FUNCT();
	requestGet(ASSEMBLYDATE, date, 50);
}

/**
 * Return the status of each channel. A zero indicates the channel is working and 1 if its defective.
 * @param[out] badChannels Returns the status of each channel.
 */
void Camera::getBadChannels(Data& badChannelData) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	int size = nbModules * PixelsPerModule;
	badChannelData.type = Data::INT32;
	badChannelData.frameNumber = 0;
	badChannelData.dimensions.push_back(size);
	Buffer *buffer = new Buffer(size * sizeof(Data::INT32));
	int32_t *iptr = reinterpret_cast<int32_t*>(buffer->data);
	requestGet(BADCHANNELS, iptr, size);
	badChannelData.setBuffer(buffer);
	buffer->unref();
}

/**
 * Get the command identifier. This number increments by one whenever a client send a command to the server.
 * Can be used to make sure that no other clients have changed the state of the system.
 * @param[out] commandId the command identifier.
 */
void Camera::getCommandId(int& commandId) {
	DEB_MEMBER_FUNCT();
	requestGet(COMMANDID, commandId);
}

/**
 * Get the serial numbers of the modules, which have to be interpreted as hex.
 * @param[out] serialNums a vector returned with serial numbers for each module.
 */
void Camera::getSerialNumbers(std::vector<int>& serialNums) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	int values[nbModules];
	requestGet(MODNUM, values, nbModules);
	for (int i = 0; i < nbModules; i++) {
		serialNums.push_back(values[i]);
	}
}

/**
 * Get the maximum number of modules which can be connected to the Mythen system.
 * @param[out] maxmod maximum number of modules.
 */
void Camera::getMaxNbModules(int& maxmod) {
	DEB_MEMBER_FUNCT();
	requestGet(NMAXMODULES, maxmod);
}

/**
 * Get an identifier for the sensor material;
 * @param[out] sensor at present only returns 0 = silicon
 */
void Camera::getSensorMaterial(int& sensor) {
	DEB_MEMBER_FUNCT();
	requestGet(SENSORMATERIAL, sensor);
}

/**
 * Get the nominal sensor thickness.
 * @param[out] thickness the sensor thickness in um.
 */
void Camera::getSensorThickness(int& thickness) {
	DEB_MEMBER_FUNCT();
	requestGet(SENSORTHICKNESS, thickness);
}

/**
 * Get the serial number of the Mythen system.
 * @param[out] systemNum the Mythen serial number.
 */
void Camera::getSystemNum(int& systemNum) {
	DEB_MEMBER_FUNCT();
	requestGet(SYSTEMNUM, systemNum);
}

/**
 * Get the software version of the socket server, which is of the form 'M3.0.0".
 * @param[out] version the version as a null terminated string.
 */
void Camera::getVersion(string& version) {
	DEB_MEMBER_FUNCT();
	requestGet(VERSION, version, 7);
}

/**
 * Get the position of the selected module.
 * @param[out] module the module position or 65535 if all modules are selected.
 */
void Camera::getModule(int& module) {
	DEB_MEMBER_FUNCT();
	requestGet(MODULE, module);
}

/**
 * Selects the module at the given position as the target of module specific commands.
 * An argument of 65535 selects all active modules. After initialisation all modules are selected.
 * @param[in] module the module position. (0<= module < nbModule | 65535)
 */
void Camera::setModule(int module) {
	DEB_MEMBER_FUNCT();
	requestSet(MODULE, module);
}

/**
 * Get the number of active modules.
 * @param[out] nbModule the number of active modules.
 */
void Camera::getNbModules(int& nbModule) {
	DEB_MEMBER_FUNCT();
	requestGet(NMODULES, nbModule);
}

/**
 * Set the number of active modules. When the number of active modules is increased, all the modules
 * are set back to default settings. After initialisation the number of active modules is set to the
 * number of delivered modules.
 * @param[in] nbModule the number of active modules
 */
void Camera::setNbModules(int nbModule) {
	DEB_MEMBER_FUNCT();
	requestSet(NMODULES, nbModule);
}

/**
 * Get the delay between two subsequent frames.
 * @param[out] time the delay time between frames.
 */
void Camera::getDelayAfterFrame(long long& time) {
	DEB_MEMBER_FUNCT();
	requestGet(DELAFTER, time);
}
/**
 * Sets the delay between two subsequent frames.
 * @param[in] time Delay in units of 100ns (default = 1)
 */
void Camera::setDelayAfterFrame(long long time) {
	DEB_MEMBER_FUNCT();
	requestSet(DELAFTER, time);
}

/**
 * Get the number of frames.
 * @param[out] frames the number of frames
 */
void Camera::getFrames(int& frames) {
	DEB_MEMBER_FUNCT();
	requestGet(FRAMES, frames);
}

/**
 * Set the number of frames within an acquisition. Default is 1.
 * @param[in] frames the number of frame to acquire.
 */
void Camera::setFrames(int frames) {
	DEB_MEMBER_FUNCT();
	requestSet(FRAMES, frames);
}

/**
 * Get the number of bits to be readout.
 * @param[out] nbits the number of bits
 */
void Camera::getNbits(Nbits& nbits) {
	DEB_MEMBER_FUNCT();
	int bits;
	requestGet(NBITS, bits);
	nbits = static_cast<Nbits>(bits);
}

/**
 * Set the number of bits to readout, thereby determining the dynamic range and the maximum frame rate.
 * After initialisation the number of bits is BPP24.
 * @param[in] nbits the number of bits which may be BPP4 = 4, BPP8 = 8, BPP16=16, BPP24 = 24
 */
void Camera::setNbits(Nbits nbits) {
	DEB_MEMBER_FUNCT();
	requestSet(NBITS, static_cast<int>(nbits));
}

/**
 * Get the exposure time of one frame of data in units of 100ns
 * @param[out] time the acquisition time in units of 100ns
 */
void Camera::getTime(long long& time) {
	DEB_MEMBER_FUNCT();
	requestGet(TIME, time);
}

/**
 * Sets the exposure time of one frame
 * @param[in] time the exposure time in units of 100ns. Default value is 1 second.
 */
void Camera::setTime(long long time) {
	DEB_MEMBER_FUNCT();
	requestSet(TIME, time);
}

/**
 * Gets the status of the Mythen
 * @param[out] status the {@see status}
 */
void Camera::getHwStatus(Status& status) {
	DEB_MEMBER_FUNCT();
	int hwStatus;
	requestGet(STATUS, hwStatus);
	status = static_cast<Status>(hwStatus);
}

/**
 * Get the X-ray energies  of the selected modules
 * @param[out] energy a vector of X-ray energies
 */
void Camera::getEnergy(std::vector<float>& energy) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	float values[nbModules];
	requestGet(ENERGY, values, nbModules);
	for (int i = 0; i < nbModules; i++) {
		energy.push_back(values[i]);
	}
}

/**
 * Set the X-ray energy for the selected modules while keeping the current
 * threshold energy. The energy is used to provide an optimal flatfield correction.
 * The command takes ca. 2 seconds per module. The energy range is governed by the
 * calibration of the system
 * @param][in] energy the energy in keV. The default value is 8.05keV
 */
void Camera::setEnergy(float energy) {
	DEB_MEMBER_FUNCT();
	requestSet(ENERGY, energy);
}

/**
 * Get the maximum supported X-ray energy.
 * @param[out] energy  the maximum energy in keV
 */
void Camera::getEnergyMax(float& energy) {
	DEB_MEMBER_FUNCT();
	requestGet(ENERGYMAX, energy);
}

/**
 * Get the minimum supported X-ray energy.
 * @param[out] energy  the minimum energy in keV
 */
void Camera::getEnergyMin(float& energy) {
	DEB_MEMBER_FUNCT();
	requestGet(ENERGYMIN, energy);
}

/**
 * Get the threshold's of the selected modules
 * @param[out] energy a vector  of threshold values
 */
void Camera::getKThresh(std::vector<float>& kthresh) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	float values[nbModules];
	requestGet(KTHRESH, values, nbModules);
	for (int i = 0; i < nbModules; i++) {
		kthresh.push_back(values[i]);
	}
}

/**
 * Set the energy threshold for the selected modules. The supported ranges depend
 * on the calibration of the system. This command takes about 2seconds per module.
 * @param[in] kthresh the threshold energy in keV. The default value is 6.4keV
 */
void Camera::setKThresh(float kthresh) {
	DEB_MEMBER_FUNCT();
	requestSet(KTHRESH, kthresh);
}

/**
 * Get the maximum supported threshold energy.
 * @param[out] kthresh the maximum threshold energy in keV
 */
void Camera::getKThreshMax(float& kthresh) {
	DEB_MEMBER_FUNCT();
	requestGet(KTHRESHMAX, kthresh);
}

/**
 * Get the minimum supported threshold energy.
 * @param[out] kthresh the minimum threshold energy in keV
 */
void Camera::getKThreshMin(float& kthresh) {
	DEB_MEMBER_FUNCT();
	requestGet(KTHRESHMIN, kthresh);
}

/**
 * Sets the energy threshold and the X-ray energy for the selected modules.
 * The supported ranges depend on the calibration of the system. This
 * command takes 2 seconds per module.
 * @param[in] kthresh the threshold energy in keV. The default value is 6.4keV
 * @param[in] energy the energy in keV. The default value is 8.05keV
 */
void Camera::setKThreshEnergy(float kthresh, float energy) {
	DEB_MEMBER_FUNCT();
	requestSet(KTHRESHENERGY, kthresh, energy);
}

/**
 * Loads predefined settings for the selected modules and the specified X-ray radiation.
 * The energy threshold is set to a suitable value. The availability of Cr and Ag settings
 * depends on the calibration of the Mythen. This action takes 2 seconds pre modules.
 * After initialisation the Cu settings are loaded.
 * @param[in] settings {@see Settings}
 */
void Camera::setPredefinedSettings(Settings settings) {
	DEB_MEMBER_FUNCT();
	stringstream ss;
	ss << settings;
	requestSet(SETTINGS, ss.str());
}

/**
 * Returns whether the bad channel interpolation is enabled.
 * @param[out] enable {@see Switch}
 */
void Camera::getBadChannelInterpolation(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int badchannelinterpolation;
	requestGet(BADCHANNELINTERPOLATION, badchannelinterpolation);
	enable = static_cast<Switch>(badchannelinterpolation);
}

/**
 * Turns the interpolation routine for bad channels on or off. When enabled, the number
 * of counts for a bad channel is the average of its two neighbours. When disabled the
 * number of counts for a bad channel is set to -2.
 * @param[in] enable {@see Switch}
 */
void Camera::setBadChannelInterpolation(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(BADCHANNELINTERPOLATION, enable);
}

/**
 * Returns whether the flat field correction is enabled or not.
 * @param[out] enable {@see Switch}
 */
void Camera::getFlatFieldCorrection(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int flatfieldcorrection;
	requestGet(FLATFIELDCORRECTION, flatfieldcorrection);
	enable = static_cast<Switch>(flatfieldcorrection);
}

/**
 * Enables or disables flat field correction. After initialisation the flat
 * field correction is enabled.
 * @param[in] enable {@see Switch}
 */
void Camera::setFlatFieldCorrection(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(FLATFIELDCORRECTION, enable);
}

/**
 * Return the maximal count value before flat field correction.
 * @param[out] cutoff the maximum count
 */
void Camera::getCutoff(int& cutoff) {
	DEB_MEMBER_FUNCT();
	requestGet(CUTOFF, cutoff);
}

/**
 * Returns the currently loaded flat field calibration of all modules
 * @param[out] flatfieldData The calibration data of each channel.
 */
void Camera::getFlatField(Data& flatFieldData) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	int size = nbModules * PixelsPerModule;
	flatFieldData.type = Data::INT32;
	flatFieldData.frameNumber = 0;
	flatFieldData.dimensions.push_back(size);
	Buffer *buffer = new Buffer(size * sizeof(Data::INT32));
	int32_t *iptr = reinterpret_cast<int32_t*>(buffer->data);
	requestGet(FLATFIELD, iptr, size);
	flatFieldData.setBuffer(buffer);
	buffer->unref();
}

/**
 * Returns whether the rate correction is enabled or not
 * @param[out] enable {@see Switch}
 */
void Camera::getRateCorrection(Switch& enable) {
	DEB_MEMBER_FUNCT();
	requestGet(RATECORRECTION, enable);
}

/**
 * Enables or disables the rate correction. After initialisation the flat
 * field correction is disabled.
 * @param[in] enable {@see Switch}
 */
void Camera::setRateCorrection(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(RATECORRECTION, enable);
}

/**
 * Returns the dead time constant for all active modules in nanoseconds.
 * @param[out] tau a vector if dead time constants returned for each active module.
 */
void Camera::getTau(std::vector<float>& tau) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	float values[nbModules];
	requestGet(TAU, values, nbModules);
	for (int i = 0; i < nbModules; i++) {
		tau.push_back(values[i]);
	}
}

/**
 * Set the dead time constant (used by the rate correction) for the selected
 * modules. If the value is -1.0 the system will use a predefined value appropriate
 * for the loaded settings.
 * @param[in] tau the value tau > 0 or tau = -1.0 ns
 */
void Camera::setTau(float tau) {
	DEB_MEMBER_FUNCT();
	requestSet(TAU, tau);
}

/**
 * Returns the number of gates
 * @param[out] gates the number of gates
 */
void Camera::getGates(int& gates) {
	DEB_MEMBER_FUNCT();
	requestGet(GATES, gates);
}

/*
 * Set the number of gate signals within one frame.
 * @param[in] gates the number of gates
 */
void Camera::setGates(int gates) {
	DEB_MEMBER_FUNCT();
	requestSet(GATES, gates);
}

/**
 * Returns the delay between a trigger signal and the start of the measurement.
 * @param[out] time the delay in ns
 */
void Camera::getDelayBeforeFrame(long long& time) {
	DEB_MEMBER_FUNCT();
	requestGet(DELBEF, time);
}

/**
 * Set the delay between a trigger signal and the
 * start of the measurement ("delay before frame")
 * @param[in] time the delay in units of 100ns (default = 0ns)
 */
void Camera::setDelayBeforeFrame(long long time) {
	DEB_MEMBER_FUNCT();
	requestSet(DELBEF, time);
}

/**
 * Returns whether the continuous trigger mode is enabled or not.
 * @param[out] enable {@see Switch}
 */
void Camera::getContinuousTrigger(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int trig;
	requestGet(CONTTRIG, trig);
	enable = static_cast<Switch>(trig);
}

/**
 * Enables or disables the continuous trigger mode. In this mode,
 * each frame requires a trigger signal.
 * @param[in] enable {@see Switch}
 */
void Camera::setContinuousTrigger(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(CONTTRIGEN, enable);
}

/**
 * Returns whether the gated mode is enabled or not.
 * @param[out] enable {@see Switch}
 */
void Camera::getGateMode(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int gate;
	requestGet(GATE, gate);
	enable = static_cast<Switch>(gate);
}

/**
 * Enables or disables the gated measurement mode.
 * @param[in] enable {@see Switch}
 */
void Camera::setGateMode(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(GATEEN, enable);
}

/**
 * Returns whether the trigger mode is enabled or not.
 * @param[out] enable {@see Switch}
 */
void Camera::getTriggered(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int trig;
	requestGet(TRIG, trig);
	enable = static_cast<Switch>(trig);
}

/**
 * Enables or disables the trigger mode. The acquisition starts after
 * the trigger signal. Subsequent frames are started automatically.
 * @param[in] enable {@see Switch}
 */
void Camera::setTriggered(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(TRIGEN, enable);
}

/**
 * Get the polarity of the enable input signal
 * @param[out] polarity {@see Polarity} (Rising_edge = active high)
 */
void Camera::getInputSignalPolarity(Polarity& polarity) {
	DEB_MEMBER_FUNCT();
	int in;
	requestGet(INPOL, in);
	polarity = static_cast<Polarity>(in);
}

/**
 * Sets the polarity of the enable out signal
 * @param[in] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::setInputSignalPolarity(Polarity polarity) {
	DEB_MEMBER_FUNCT();
	requestSet(INPOL, static_cast<int>(polarity));
}

/**
 * Get the polarity of the enable out signal
 * @param[out] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::getOutputSignalPolarity(Polarity& polarity) {
	DEB_MEMBER_FUNCT();
	int out;
	requestGet(OUTPOL, out);
	polarity = static_cast<Polarity>(out);
}

/**
 * Sets the polarity of the enable out signal
 * @param[in] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::setOutputSignalPolarity(Polarity polarity) {
	DEB_MEMBER_FUNCT();
	requestSet(OUTPOL, static_cast<int>(polarity));
}

/**
 * Switch the readout mode to raw. This is a packed format
 * @param[in] enable enables or disable raw readout {@see Switch}
 */
void Camera::setUseRawReadout(Switch enable) {
	DEB_MEMBER_FUNCT();
	m_use_raw_readout = static_cast<bool>(enable);
}

/**
 * Get the readout mode
 * @param[out] enable {@see Switch}
 */
void Camera::getUseRawReadout(Switch& enable) {
	DEB_MEMBER_FUNCT();
	enable = static_cast<Switch>(m_use_raw_readout);
}

/**
 * Return a test dataset with the number of counts for each channel equal
 * to the channel number. Can be used to verify the readout mechanism.
 * @param[out] data the test pattern data
 */
void Camera::getTestPattern(Data& testData) {
	DEB_MEMBER_FUNCT();
	int nbModules;
	getNbModules(nbModules);
	int size = nbModules * PixelsPerModule;
	testData.type = Data::UINT32;
	testData.frameNumber = 0;
	testData.dimensions.push_back(size);
	Buffer *buffer = new Buffer(size * sizeof(Data::UINT32));
	requestCmd(TESTPATTERN, reinterpret_cast<uint32_t*>(buffer->data), size);
	testData.setBuffer(buffer);
	buffer->unref();
}

/**
 * Reset the detector back to default settings. It takes 2 seconds per module.
 */
void Camera::resetMythen() {
	DEB_MEMBER_FUNCT();
	requestCmd(RESET);
}

/**
 * Start an acquisition with the programmed number of frames or gates
 */
void Camera::start() {
	DEB_MEMBER_FUNCT();
	requestCmd(START);
}

/**
 * Stops the current acquisition. The data of the ongoing frame is discarded.
 */
void Camera::stop() {
	DEB_MEMBER_FUNCT();
	requestCmd(STOP);
}

/**
 * Enables logging to a file on the Mythen. Do not enable logging unless its
 * really necessary (for debugging) as it slows down the server and there is
 * only limited space for the log file.
 */
void Camera::logStart() {
	DEB_MEMBER_FUNCT();
	m_logSize = 0;
	requestCmd(LOGSTART);
}

/**
 * Stops the logging of the Mythen socket server
 */
void Camera::logStop() {
	DEB_MEMBER_FUNCT();
	requestCmd(LOGSTOP, (uint32_t*) &m_logSize, 1);
	DEB_TRACE() << "Log file size = " << m_logSize << " bytes";
}

/**
 * Print the contents of the log file on the terminal
 */
void Camera::logRead() {
	DEB_MEMBER_FUNCT();
	char* buff = new char[m_logSize]();
	requestCmd(LOGREAD, buff, m_logSize);
	std::cout << buff << std::endl;
	delete[] buff;
}

/**
 * Returns the data of the specified frame in the buffer.
 * @param[out] mythenData the frame data
 * @param[in] frame_nb the number of the frame
 */
void Camera::readFrame(Data& mythenData, int frame_nb) {
	DEB_MEMBER_FUNCT();
	HwFrameInfo frame_info;
	if (frame_nb >= m_acq_frame_nb) {
		THROW_HW_ERROR(Error) << "Frame not available yet";
	} else {
		StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
		buffer_mgr.getFrameInfo(frame_nb, frame_info);
		Size size = frame_info.frame_dim.getSize();
		int width = size.getWidth();

		mythenData.type = Data::UINT32;
		mythenData.dimensions.push_back(width);
		mythenData.frameNumber = frame_nb;

		Buffer *buffer = new Buffer(width * sizeof(Data::UINT32));
		memcpy(buffer->data, frame_info.frame_ptr,
				width * sizeof(Data::UINT32));
		mythenData.setBuffer(buffer);
		buffer->unref();
	}
}

/**
 * Returns all frames of data acquired.
 * @param[out] mythenData the frame data
 */
void Camera::readData(Data& mythenData) {
	DEB_MEMBER_FUNCT();
	HwFrameInfo frame_info;

	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	Size size = frame_info.frame_dim.getSize();
	int width = size.getWidth();
	Buffer *buffer = new Buffer(m_acq_frame_nb * width * sizeof(Data::UINT32));

	mythenData.type = Data::UINT32;
	mythenData.dimensions.push_back(width);
	mythenData.dimensions.push_back(m_acq_frame_nb);
	mythenData.frameNumber = 1;

	uint32_t* bptr = (uint32_t*) buffer->data;
	for (int i = 0; i < m_acq_frame_nb; i++, bptr += width) {
		buffer_mgr.getFrameInfo(i, frame_info);
		memcpy(bptr, frame_info.frame_ptr, width * sizeof(Data::UINT32));
	}
	mythenData.setBuffer(buffer);
	buffer->unref();
}

///////////////////////
// private methods
///////////////////////

/*
 * Returns the data of the oldest frame in the buffer (i.e. the frames are
 * returned in the same order as they were acquired. The Mythen can internally
 * buffer the last four acquired frames.  If the buffer is empty and there is
 * an ongoing measurement, the command returns the data after the measurement
 * has finished. If the readout fails all count are set to -1.
 * @param[out] data an array containing the frame data
 * @param[in] len the size of the array = [nbModules*npixels].
 */
void Camera::readout(uint32_t* data, int len) {
	DEB_MEMBER_FUNCT();
	requestCmd(READOUT, data, len);
}

/*
 * Returns the data of the oldest frame in the buffer in a raw compressed format.
 * The Mythen does not apply any correction to the data , thereby allowing for
 * maximal frame rates.
 * @param[out] data an array containing the raw data
 * @param[in] len the size of the array = [nbModules*nBits].
 */
void Camera::readoutRaw(uint32_t* data, int len) {
	DEB_MEMBER_FUNCT();
	requestCmd(READOUTRAW, data, len);
}

/*
 * Decode the raw data buffer in-place.
 * @param[in] nbits number of bits read out
 * @param[in] width the image width = [nbModules*PixelsPerModule]
 * @param[out] buff packed data array in decoded array out
 */
void Camera::decodeRaw(Nbits nbits, uint32_t* buff, int width) {
	int chansPerPoint = CHAR_BIT * sizeof(int) / nbits;
	uint32_t mask = 0xffffffff >> ((nbits * (chansPerPoint - 1)));
	int size = width / chansPerPoint;
	uint32_t *bptr = buff + width - 1;
	uint32_t *iptr = buff + size - 1;
	for (int j = size - 1; j >= 0; j--, iptr--) {
		for (int i = chansPerPoint - 1; i >= 0; i--) {
			uint32_t shift = nbits * i;
			uint32_t shiftmask = mask << shift;
			*bptr-- = ((*iptr & shiftmask) >> shift) & mask;
		}
	}
}

template<typename T>
void Camera::checkReply(T rc) {
	DEB_MEMBER_FUNCT();
	int irc = *((unsigned int *) &rc);
	if (irc < 0) {
		THROW_HW_ERROR(Error) << serverStatusMap[irc];
	}
}

string Camera::findCmd(Action action, ServerCmd cmd) {
	DEB_MEMBER_FUNCT();
	stringstream ss;
	map<ServerCmd, std::string>::iterator it;
	if ((it = serverCmdMap.find(cmd)) != serverCmdMap.end()) {
		string cmdStr = it->second;
		if (action == Camera::GET) {
			ss << "-get " << cmdStr;
		} else {
			ss << "-" << cmdStr;
		}
	} else {
		THROW_HW_ERROR(Error) << "Not a 'server cmd'  command";
	}
	return ss.str();
}

void Camera::requestCmd(ServerCmd cmd) {
	DEB_MEMBER_FUNCT();
	sendCmd(Camera::CMD, cmd);
}

template<typename T>
void Camera::requestCmd(ServerCmd cmd, T* value, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t* buff;
	buff = reinterpret_cast<uint8_t*>(value);
	sendCmd(Camera::CMD, cmd, buff, len * sizeof(T));
}

template<typename T>
void Camera::requestSet(ServerCmd cmd, T value) {
	DEB_MEMBER_FUNCT();
	sendCmd(Camera::SET, cmd, value);
}
template<typename T>
void Camera::requestSet(ServerCmd cmd, T value1, T value2) {
	DEB_MEMBER_FUNCT();
	sendCmd(Camera::SET, cmd, value1, value2);
}

template<typename T>
void Camera::requestGet(ServerCmd cmd, T& value) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	recvBuf = reinterpret_cast<uint8_t*>(&value);
	sendCmd(Camera::GET, cmd, recvBuf, sizeof(T));
}

void Camera::requestGet(ServerCmd cmd, string& reply, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t buff[64];
	sendCmd(Camera::GET, cmd, buff, len);
	stringstream os;
	os << buff;
	reply = os.str();
}

template<typename T>
void Camera::requestGet(ServerCmd cmd, T* value, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t* buff;
	buff = reinterpret_cast<uint8_t*>(value);
	sendCmd(Camera::GET, cmd, buff, len * max(sizeof(uint32_t), sizeof(T)));
}

void Camera::sendCmd(Action action, ServerCmd cmd) {
	DEB_MEMBER_FUNCT();
	int rc;
	uint8_t *buff = reinterpret_cast<uint8_t*>(&rc);
	if (m_simulated) {
		simulate(action, cmd, buff, sizeof(int));
	} else {
		stringstream ss;
		ss << findCmd(action, cmd);
		buff = reinterpret_cast<uint8_t*>(&rc);
		m_mythen->sendCmd(ss.str(), buff, sizeof(int));
		checkReply(rc);
	}
}

template<typename T>
void Camera::sendCmd(Action action, ServerCmd cmd, T value) {
	DEB_MEMBER_FUNCT();
	int rc;
	uint8_t *buff;
	if (m_simulated) {
		simulate(action, cmd, reinterpret_cast<uint8_t*>(&value), sizeof(T));
	} else {
		stringstream ss;
		ss << findCmd(action, cmd) << " " << value;
		buff = reinterpret_cast<uint8_t*>(&rc);
		m_mythen->sendCmd(ss.str(), buff, sizeof(int));
		checkReply(rc);
	}
}

template<typename T>
void Camera::sendCmd(Action action, ServerCmd cmd, T value1, T value2) {
	DEB_MEMBER_FUNCT();
	int rc;
	uint8_t *buff;
	if (m_simulated) {
		int size = sizeof(T);
		uint8_t values[2*size];
		memcpy(values, &value1, size);
		memcpy((values+size), &value2, size);
		simulate(action, cmd, values, size*2);
	} else {
		stringstream ss;
		ss << findCmd(action, cmd) << " " << value1 << " " << value2;
		buff = reinterpret_cast<uint8_t*>(&rc);
		m_mythen->sendCmd(ss.str(), buff, sizeof(int));
		checkReply(rc);
	}
}

void Camera::sendCmd(Action action, ServerCmd cmd, uint8_t* buff, int len) {
	DEB_MEMBER_FUNCT();
	int rc;
	if (m_simulated) {
		simulate(action, cmd, buff, len);
	} else {
		stringstream ss;
		ss << findCmd(action, cmd);
		if (action == Camera::SET) {
			ss << " " << buff;
		}
		m_mythen->sendCmd(ss.str(), buff, len);
		rc = *((uint32_t *) buff);
		checkReply(rc);
	}
}

#if __GNUC_MINOR__ < 4
typedef pair<int, string> StatusPair;
static const StatusPair serverStatusList[] = {
    StatusPair(  0, "Success"),
    StatusPair( -1, "Unknown command"),
    StatusPair( -2, "Invalid argument"),
    StatusPair( -3, "Unknown settings"),
    StatusPair( -4, "Out of memory"),
    StatusPair( -5, "Module calibration files not found"),
    StatusPair( -6, "Readout failed"),
    StatusPair(-10, "Flatfield file not found"),
    StatusPair(-11, "Bad channel file not found"),
    StatusPair(-12, "Energy calibration file not found"),
    StatusPair(-13, "Noise file not found"),
    StatusPair(-14, "Trimbit file not found"),
    StatusPair(-15, "Invalid format of the flatfield file"),
    StatusPair(-16, "Invalid format of the bad channel file"),
    StatusPair(-17, "Invalid format of the energy calibration file"),
    StatusPair(-18, "Invalid format of the noise file"),
    StatusPair(-19, "Invalid format of the trimbit file"),
    StatusPair(-20, "Version file not found"),
    StatusPair(-21, "Invalid format of the version file"),
    StatusPair(-22, "Gain calibration file not found"),
    StatusPair(-23, "Invalid format of the gain calibration file"),
    StatusPair(-24, "Dead time file not found"),
    StatusPair(-25, "Invalid format of the dead time file"),
    StatusPair(-30, "Could not create log file"),
    StatusPair(-31, "Could not close the log file"),
    StatusPair(-32, "Could not read the log file")
};
std::map<int, std::string> Camera::serverStatusMap(C_LIST_ITERS(serverStatusList));

typedef pair<const Camera::ServerCmd, string> ServerCmdPair;
static const ServerCmdPair serverCmdList[] = {
	ServerCmdPair(Camera::ASSEMBLYDATE,"assemblydate"),
	ServerCmdPair(Camera::BADCHANNELS, "badchannels"),
	ServerCmdPair(Camera::COMMANDID, "commandid"),
	ServerCmdPair(Camera::MODNUM, "modnum"),
	ServerCmdPair(Camera::MODULE, "module"),
	ServerCmdPair(Camera::NMAXMODULES, "nmaxmodules"),
	ServerCmdPair(Camera::NMODULES, "nmodules"),
	ServerCmdPair(Camera::SENSORMATERIAL, "sensormaterial"),
	ServerCmdPair(Camera::SENSORTHICKNESS, "sensorthickness"),
	ServerCmdPair(Camera::SYSTEMNUM, "systemnum"),
	ServerCmdPair(Camera::VERSION, "version"),
	ServerCmdPair(Camera::RESET, "reset"),
	ServerCmdPair(Camera::DELAFTER, "delafter"),
	ServerCmdPair(Camera::FRAMES, "frames"),
	ServerCmdPair(Camera::NBITS, "nbits"),
	ServerCmdPair(Camera::STATUS, "status"),
	ServerCmdPair(Camera::READOUT, "readout"),
	ServerCmdPair(Camera::READOUTRAW, "readoutraw"),
	ServerCmdPair(Camera::START, "start"),
	ServerCmdPair(Camera::STOP, "stop"),
	ServerCmdPair(Camera::TIME, "time"),
	ServerCmdPair(Camera::ENERGY, "energy"),
	ServerCmdPair(Camera::ENERGYMAX, "energymax"),
	ServerCmdPair(Camera::ENERGYMIN, "energymin"),
	ServerCmdPair(Camera::KTHRESH, "kthresh"),
	ServerCmdPair(Camera::KTHRESHMAX, "kthreshmax"),
	ServerCmdPair(Camera::KTHRESHMIN, "kthreshmin"),
	ServerCmdPair(Camera::KTHRESHENERGY, "kthreshenergy"),
	ServerCmdPair(Camera::SETTINGS, "settings"),
	ServerCmdPair(Camera::BADCHANNELINTERPOLATION, "badchannelinterpolation"),
	ServerCmdPair(Camera::FLATFIELDCORRECTION, "flatfieldcorrection"),
	ServerCmdPair(Camera::CUTOFF, "cutoff"),
	ServerCmdPair(Camera::FLATFIELD, "flatfield"),
	ServerCmdPair(Camera::RATECORRECTION, "ratecorrection"),
	ServerCmdPair(Camera::TAU, "tau"),
	ServerCmdPair(Camera::CONTTRIGEN, "conttrigen"),
	ServerCmdPair(Camera::DELBEF, "delbef"),
	ServerCmdPair(Camera::GATEEN, "gateen"),
	ServerCmdPair(Camera::GATES, "gates"),
	ServerCmdPair(Camera::CONTTRIG, "conttrig"),
	ServerCmdPair(Camera::GATE, "gate"),
	ServerCmdPair(Camera::INPOL, "inpol"),
	ServerCmdPair(Camera::OUTPOL, "outpol"),
	ServerCmdPair(Camera::TRIG, "trig"),
	ServerCmdPair(Camera::TRIGEN, "trigen"),
	ServerCmdPair(Camera::LOGSTART, "log start"),
	ServerCmdPair(Camera::LOGSTOP, "log stop"),
	ServerCmdPair(Camera::LOGREAD, "log read"),
	ServerCmdPair(Camera::TESTPATTERN, "testpattern"),
};
std::map<Camera::ServerCmd, std::string> Camera::serverCmdMap(C_LIST_ITERS(serverCmdList));

#else
std::map<int, std::string> Camera::serverStatusMap = {
		{  0, "Success"},
	    { -1, "Unknown command"},
	    { -2, "Invalid argument"},
	    { -3, "Unknown settings"},
	    { -4, "Out of memory"},
	    { -5, "Module calibration files not found"},
	    { -6, "Readout failed"},
	    {-10, "Flatfield file not found"},
	    {-11, "Bad channel file not found"},
	    {-12, "Energy calibration file not found"},
	    {-13, "Noise file not found"},
	    {-14, "Trimbit file not found"},
	    {-15, "Invalid format of the flatfield file"},
	    {-16, "Invalid format of the bad channel file"},
	    {-17, "Invalid format of the energy calibration file"},
	    {-18, "Invalid format of the noise file"},
	    {-19, "Invalid format of the trimbit file"},
	    {-20, "Version file not found"},
	    {-21, "Invalid format of the version file"},
	    {-22, "Gain calibration file not found"},
	    {-23, "Invalid format of the gain calibration file"},
	    {-24, "Dead time file not found"},
	    {-25, "Invalid format of the dead time file"},
	    {-30, "Could not create log file"},
	    {-31, "Could not close the log file"},
	    {-32, "Could not read the log file"}
};

std::map<Camera::ServerCmd, std::string> Camera::serverCmdMap = {
	{ASSEMBLYDATE,"assemblydate"},
	{BADCHANNELS, "badchannels"},
	{COMMANDID, "commandid"},
	{MODNUM, "modnum"},
	{MODULE, "module"},
	{NMAXMODULES, "nmaxmodules"},
	{NMODULES, "nmodules"},
	{SENSORMATERIAL, "sensormaterial"},
	{SENSORTHICKNESS, "sensorthickness"},
	{SYSTEMNUM, "systemnum"},
	{VERSION, "version"},
	{RESET, "reset"},
	{DELAFTER, "delafter"},
	{FRAMES, "frames"},
	{NBITS, "nbits"},
	{STATUS, "status"},
	{READOUT, "readout"},
	{READOUTRAW, "readoutraw"},
	{START, "start"},
	{STOP, "stop"},
	{TIME, "time"},
	{ENERGY, "energy"},
	{ENERGYMAX, "energymax"},
	{ENERGYMIN, "energymin"},
	{KTHRESH, "kthresh"},
	{KTHRESHMAX, "kthreshmax"},
	{KTHRESHMIN, "kthreshmin"},
	{KTHRESHENERGY, "kthreshenergy"},
	{SETTINGS, "settings"},
	{BADCHANNELINTERPOLATION, "badchannelinterpolation"},
	{FLATFIELDCORRECTION, "flatfieldcorrection"},
	{CUTOFF, "cutoff"},
	{FLATFIELD, "flatfield"},
	{RATECORRECTION, "ratecorrection"},
	{TAU, "tau"},
	{CONTTRIGEN, "conttrigen"},
	{DELBEF, "delbef"},
	{GATEEN, "gateen"},
	{GATES, "gates"},
	{CONTTRIG, "conttrig"},
	{GATE, "gate"},
	{INPOL, "inpol"},
	{OUTPOL, "outpol"},
	{TRIG, "trig"},
	{TRIGEN, "trigen"},
	{LOGSTART, "log start"},
	{LOGSTOP, "log stop"},
	{LOGREAD, "log read"},
	{TESTPATTERN, "testpattern"},
};
#endif

ostream& lima::Mythen3::operator <<(ostream& os, Camera::Polarity const &polarity) {
	const char* name = "Unknown";
	switch (polarity) {
	case Camera::RISING_EDGE:
		name = "Rising Edge";
		break;
	case Camera::FALLING_EDGE:
		name = "Falling Edge";
		break;
	}
	return os << name;
}

ostream& lima::Mythen3::operator <<(ostream& os, Camera::Settings const &settings) {
	const char* name = "Unknown";
	switch (settings) {
	case Camera::Cu:
		name = "Cu";
		break;
	case Camera::Mo:
		name = "Mo";
		break;
	case Camera::Cr:
		name = "Cr";
		break;
	case Camera::Ag:
		name = "Ag";
		break;
	}
	return os << name;
}

// ----------------------- Simulate code now --------------------
/*
 * Simulate commands sent to the Mythen socket server. This routine incorporate the checkReply()
 * functionality by throwing an exception on error.
 */
void Camera::simulate(Action action, ServerCmd cmd, uint8_t* recvBuf, int len) {
	DEB_MEMBER_FUNCT();
	string assemblyDate = "27-feb-2015 21:08 GMT                             ";
	static int commandID = 1;
	static int modnum[] = { 31, 32, 33, 34, 35, 36 };
	static int module = -1;
	static int maxModules = 6;
	static int nmodules = 1;
	static int sensorMaterial = 0;
	static int sensorThickness = 320;
	static int sysNum = 116;
	static string version = "M3.0.1";
	static bool badChannelInterpolation = true;
	static bool flatfieldCorrection = true;
	static bool rateCorrection = false;
	static bool triggerMode = false;
	static bool gate = false;
	static bool continuousTrigger = false;
	static float energy[] = { 8.05, 8.05, 8.05, 8.05, 8.05, 8.05 };
	static float kthresh[] = { 6.4, 6.4, 6.4, 6.4, 6.4, 6.4 };
	static float tau[] = { 197.6159, 197.6159, 197.6159, 197.6159, 197.6159, 197.6159 };
	static float energyMax = 40.0;
	static float energyMin = 4.09;
	static float kthreshMax = 20.0;
	static float kthreshMin = 4.0;
	static int inpol = 0;
	static int outpol = 0;
	static int gates = 1;
	static int zero = 0;
	static long long delafter = 0;
	static long long delbef = 0;
	static long long time = 10000000;
	static int frames = 1;
	static int nbits = 24;
	static int status = 0x10000;
	static int cutoff = 1280;
	int length;

	commandID++;
	int* iptr = reinterpret_cast<int*>(recvBuf);
	int* bptr = reinterpret_cast<int*>(recvBuf);
	float* fptr = reinterpret_cast<float*>(recvBuf);
	long long* llptr = reinterpret_cast<long long*>(recvBuf);
	char *cptr = reinterpret_cast<char*>(recvBuf);
	switch (action) {
	case Camera::GET:
		DEB_TRACE() << "sendCmd(" << findCmd(action, cmd) << ")";
		switch (cmd) {
		case ASSEMBLYDATE:
			length = assemblyDate.length();
			memcpy(cptr, assemblyDate.c_str(), assemblyDate.length()-1);
			*(cptr+assemblyDate.length() - 1) = '\0';
			break;
		case BADCHANNELS:
			for (int i = 0; i < nmodules; i++)
				for (int j = 0; j < PixelsPerModule; j++)
					*iptr++ = zero;
			break;
		case COMMANDID:
			iptr[0] = commandID;
			break;
		case MODNUM:
			for (int i = 0; i < nmodules; i++)
				*iptr++ = modnum[i];
			break;
		case MODULE:
			iptr[0] = module;
			break;
		case NMAXMODULES:
			iptr[0] = maxModules;
			break;
		case NMODULES:
			iptr[0] = nmodules;
			break;
		case SENSORMATERIAL:
			iptr[0] = sensorMaterial;
			break;
		case SENSORTHICKNESS:
			iptr[0] = sensorThickness;
			break;
		case SYSTEMNUM:
			iptr[0] = sysNum;
			break;
		case VERSION:
			memcpy(cptr, version.c_str(), version.length());
			break;
		case DELAFTER:
			llptr[0] = delafter;
			break;
		case FRAMES:
			iptr[0] = frames;
			break;
		case NBITS:
			iptr[0] = nbits;
			break;
		case STATUS:
			iptr[0] = status;
			break;
		case TIME:
			llptr[0] = time;
			break;
		case ENERGY:
			if (module == -1) {
				for (int i = 0; i < nmodules; i++) {
					*fptr++ = energy[i];
				}
			} else {
				*fptr = energy[module];
			}
			break;
		case ENERGYMAX:
			fptr[0] = energyMax;
			break;
		case ENERGYMIN:
			fptr[0] = energyMin;
			break;
		case KTHRESH:
			if (module == -1) {
				for (int i = 0; i < nmodules; i++) {
					*fptr++ = kthresh[i];
				}
			} else {
				*fptr = kthresh[module];
			}
			break;
		case KTHRESHMAX:
			fptr[0] = kthreshMax;
			break;
		case KTHRESHMIN:
			fptr[0] = kthreshMin;
			break;
		case BADCHANNELINTERPOLATION:
			bptr[0] = badChannelInterpolation;
			break;
		case FLATFIELDCORRECTION:
			bptr[0] = flatfieldCorrection;
			break;
		case CUTOFF:
			iptr[0] = cutoff;
			break;
		case FLATFIELD:
			for (int i = 0; i < nmodules; i++)
				for (int j = 0; j < PixelsPerModule; j++)
					*iptr++ = j;
			break;
		case RATECORRECTION:
			cout << "rate correction requested" << endl;
			bptr[0] = rateCorrection;
			cout << "rate correction got " << bptr[0] << endl;
			break;
		case TAU:
			if (module == -1) {
				for (int i = 0; i < nmodules; i++) {
					*fptr++ = tau[i];
				}
			} else {
				*fptr = tau[module];
			}
			break;
		case DELBEF:
			llptr[0] = delbef;
			break;
		case GATES:
			iptr[0] = gates;
			break;
		case CONTTRIG:
			bptr[0] = continuousTrigger;
			break;
		case GATE:
			bptr[0] = gate;
			break;
		case INPOL:
			iptr[0] = inpol;
			break;
		case OUTPOL:
			iptr[0] = outpol;
			break;
		case TRIG:
			bptr[0] = triggerMode;
			break;
		default:
			THROW_HW_ERROR(Error) << "Mythen3Camera::simulate(): cmd unknown. Please report";
			break;
		}
		break;
	case Camera::SET:
		DEB_TRACE() << "sendCmd(" << findCmd(action, cmd) << " " << recvBuf
				<< ")";
		switch (cmd) {
		case MODULE:
			module = iptr[0];
			break;
		case NMODULES:
			nmodules = iptr[0];
			break;
		case DELAFTER:
			delafter = llptr[0];
			break;
		case FRAMES:
			frames = iptr[0];
			break;
		case NBITS:
			nbits = iptr[0];
			break;
		case TIME:
			time = llptr[0];
			break;
		case ENERGY:
			if (module == -1)
				for (int i = 0; i < nmodules; i++)
					energy[i] = fptr[0];
			else
				energy[module] = fptr[0];
			break;
		case KTHRESH:
			if (module == -1)
				for (int i = 0; i < nmodules; i++)
					kthresh[i] = fptr[0];
			else
				kthresh[module] = fptr[0];
			break;
		case KTHRESHENERGY:
			if (module == -1) {
				for (int i = 0; i < nmodules; i++) {
					kthresh[i] = fptr[0];
					energy[i] = fptr[1];
				}
			} else {
				kthresh[module] = fptr[0];
				energy[module] = fptr[1];
			}
			break;
		case SETTINGS:
			break;
		case BADCHANNELINTERPOLATION:
			badChannelInterpolation = bptr[0];
			break;
		case FLATFIELDCORRECTION:
			flatfieldCorrection = bptr[0];
			break;
		case RATECORRECTION:
			rateCorrection = bptr[0];
			break;
		case TAU:
			if (module == -1)
				for (int i = 0; i < nmodules; i++)
					tau[i] = fptr[0];
			else
				tau[module] = fptr[0];
			break;
		case CONTTRIGEN:
			continuousTrigger = bptr[0];
			break;
		case DELBEF:
			delbef = llptr[0];
			break;
		case GATEEN:
			gate = bptr[0];
			break;
		case GATES:
			gates = iptr[0];
			break;
		case INPOL:
			inpol = iptr[0];
			break;
		case OUTPOL:
			outpol = iptr[0];
			break;
		case TRIGEN:
			triggerMode = bptr[0];
			break;
		default:
			THROW_HW_ERROR(Error) << "Mythen3Camera::simulate(): cmd unknown. Please report";
			break;
		}
		break;
	case Camera::CMD:
		switch (cmd) {
		case TESTPATTERN:
			for (int i = 0; i < nmodules; i++)
				for (int j = 0; j < PixelsPerModule; j++)
					*iptr++ = j * 2;
			break;
		case RESET:
			break;
		case START:
			status = 0x1;
			break;
		case STOP:
			status = 0x10000;
			break;
		case READOUT:
			for (int i = 0; i < nmodules; i++)
				for (int j = 0; j < PixelsPerModule; j++)
					*iptr++ = j * 3;
			break;
		case READOUTRAW:
			for (int i = 0; i < nmodules; i++)
				for (int j = 0; j < PixelsPerModule; j++)
					*iptr++ = 0;
			break;
		case LOGSTART:
		case LOGSTOP:
		case LOGREAD:
			THROW_HW_ERROR(Error) << "Mythen3Camera::simulate(): Not implemented in simulate mode";
			break;
		default:
			THROW_HW_ERROR(Error) << "Mythen3Camera::simulate(): ServerCmd unknown. Please report";
			break;
		}
		break;
	default:
		THROW_HW_ERROR(Error) << "Mythen3Camera::simulate(): Action unknown. Please report";
		break;
	};
}
