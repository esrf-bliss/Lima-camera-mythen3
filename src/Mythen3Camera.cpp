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
#include "lima/Exceptions.h"
#include "Mythen3Camera.h"

using namespace lima;
using namespace lima::Mythen3;
using namespace std;

const string PredefinedSettings[] = {"Cu", "Mo", "Cr", "Ag"};

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


Camera::Camera(std::string hostname, int tcpPort, int npixels, bool simulate) :
	m_hostname(hostname), m_tcpPort(tcpPort), m_npixels(npixels), m_nrasters(1), m_simulated(simulate),
	m_acq_frame_nb(-1), m_nb_frames(1), m_image_type(Bpp32), m_bufferCtrlObj() {
	DEB_CONSTRUCTOR();

	DebParams::setModuleFlags(DebParams::AllFlags);
	DebParams::setTypeFlags(DebParams::AllFlags);
	DebParams::setFormatFlags(DebParams::AllFlags);
	m_acq_thread = new AcqThread(*this);
	m_acq_thread->start();
	if (!m_simulated) {
		init();
	}
}

Camera::~Camera() {
	DEB_DESTRUCTOR();
	m_mythen->disconnectFromServer();
	delete m_mythen;
	delete m_acq_thread;
}

void Camera::init() {
	DEB_MEMBER_FUNCT();
	m_mythen = new Mythen3Net();
	DEB_TRACE() << "Mythen3 connecting to " << DEB_VAR2(m_hostname, m_tcpPort);
	m_mythen->connectToServer(m_hostname, m_tcpPort);
}

void Camera::reset() {
	DEB_MEMBER_FUNCT();
	stopAcq();
	resetMythen();
	m_image_type = Bpp24;
}

void Camera::prepareAcq() {
	DEB_MEMBER_FUNCT();
}

void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	m_acq_frame_nb = 0; // Number of frames of data acquired;
//	m_read_frame_nb = 0; // Number of frames read into Lima buffers
	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
//	start();
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = false;
	m_quit = false;
	m_cond.broadcast();
}

void Camera::stopAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = true;
	while (m_thread_running)
		m_cond.wait();
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
//	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();

	while (!m_cam.m_quit) {
		while (m_cam.m_wait_flag && !m_cam.m_quit) {
			DEB_TRACE() << "Wait";
			m_cam.m_thread_running = false;
			m_cam.m_cond.broadcast();
			m_cam.m_cond.wait();
		}
		DEB_TRACE() << "AcqThread Running";
		m_cam.start();
		m_cam.m_thread_running = true;
		if (m_cam.m_quit)
			return;

		m_cam.m_cond.broadcast();
		aLock.unlock();

		bool continueFlag = true;
		while (continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames)) {

			struct timespec delay, remain;
			delay.tv_sec = (int)floor(m_cam.m_exp_time);
			delay.tv_nsec = (int)(1E9*(m_cam.m_exp_time-floor(m_cam.m_exp_time)));
			DEB_TRACE() << "acq thread will sleep for " << m_cam.m_exp_time << " second";
			while (nanosleep(&delay, &remain) == -1 && errno == EINTR)
			{
				// stop called ?
				AutoMutex aLock(m_cam.m_cond.mutex());
				continueFlag = !m_cam.m_wait_flag;
				if (m_cam.m_wait_flag) {
					DEB_TRACE() << "acq thread stopped  by user";
					m_cam.stop();
					break;
				}
				delay = remain;
			}
////
///// This is not finished need to add read/save code.
////
			if (m_cam.m_acq_frame_nb < m_cam.m_nb_frames-1) {
				DEB_TRACE() << "acq thread continuing";
			} else {
				DEB_TRACE() << "acq thread histogram stop";
				m_cam.stop();
			}
			aLock.lock();
			++m_cam.m_acq_frame_nb;
			DEB_TRACE() << "acq thread signal read thread: " << m_cam.m_acq_frame_nb << " frames collected";
			m_cam.m_cond.broadcast();
			aLock.unlock();
			DEB_TRACE() << "acquired " << m_cam.m_acq_frame_nb << " frames, required " << m_cam.m_nb_frames << " frames";
		}
		aLock.lock();
		m_cam.m_wait_flag = true;
	}
}
extern int pthread_attr_setscope (pthread_attr_t *__attr, int __scope);

Camera::AcqThread::AcqThread(Camera& cam) :
	m_cam(cam) {
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
	Nbits nbits;
	getNbits(nbits);
	switch(nbits) {
	case BPP4:
		type = Bpp4;
		break;
	case BPP8:
		type = Bpp8;
		break;
	case BPP16:
		type = Bpp16;
		break;
	case BPP24:
	default:
		type = Bpp24;
		break;
	}
	type = m_image_type;
}

void Camera::setImageType(ImageType type) {
	DEB_MEMBER_FUNCT();
	Nbits nbits;
	switch(type) {
	case Bpp4:
		nbits = BPP4;
		break;
	case Bpp8:
		nbits = BPP8;
		break;
	case Bpp16:
		nbits = BPP16;
		break;
    case Bpp24:
		nbits = BPP24;
		break;
	default:
		THROW_HW_ERROR(Error) << "Image type " << type << " not supported";
		break;
	}
	setNbits(nbits);
	m_image_type = type;
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
	size = Size(m_npixels, m_nrasters);
}

void Camera::getPixelSize(double& sizex, double& sizey) {
	DEB_MEMBER_FUNCT();
	sizex = xPixelSize;
	sizey = yPixelSize;
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

void Camera::getExpTime(double& exp_time) {
	DEB_MEMBER_FUNCT();
	long long time;
	getTime(time);
	exp_time = static_cast<double>(time);
}

void Camera::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	setTime(static_cast<long long>(exp_time));
	m_exp_time = exp_time;
}

void Camera::setLatTime(double lat_time) {
	DEB_MEMBER_FUNCT();
	setLatency(static_cast<long long>(lat_time));
}

void Camera::getLatTime(double& lat_time) {
	DEB_MEMBER_FUNCT();
	long long time;
	getLatency(time);
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
 * @param[out] date assembly date as a string.
 */
void Camera::getAssemblyDate(string& date) {
	DEB_MEMBER_FUNCT();
	requestGet(ASSEMBLYDATE, date, 50);
}

/**
 * Return the status of each channel. A zero means the channel is working and 1 if its defective.
 * @param[out] badChannel array returned with status of each channel.
 * @param[in] len the size of the array = [nbModules*npiixels].
 */
void Camera::getBadChannels(int *badChannels, int len) {
	DEB_MEMBER_FUNCT();
	requestGet(BADCHANNELS, badChannels, len);
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
 * @param[out] serialNums array returned with serial numbers for each module.
 * @param[in] len the size of the array = [nbModules].
 */
void Camera::getSerialNumbers(int* serialNums, int len) {
	DEB_MEMBER_FUNCT();
	requestGet(MODNUM, serialNums, len);
}

/**
 * Get the miximum number of modules which can be connected to the Mythen system.
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
 *  are set back to default settings. After initialisation the number of active modules is set to the
 *  number of delivered modules.
 *  @param[in] nbModule the number of active modules
 */
void Camera::setNbModules(int nbModule) {
	DEB_MEMBER_FUNCT();
	requestSet(NMODULES, nbModule);
}

/**
 * Get the delay between two subsequent frames.
 * @param[out] time the delay time between frames.
 */
void Camera::getLatency(long long& time) {
	DEB_MEMBER_FUNCT();
	requestGet(DELAFTER, time);
}
/**
 * Sets the delay between two subsequent frames.
 * @param[in] time Delay in units of 100ns (default = 1)
 */
void Camera::setLatency(long long time) {
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
 * @param[out] energy array of X-ray energies
 * @param[in] len length of array of dimension nbModules
 */
void Camera::getEnergy(float* energy, int len) {
	DEB_MEMBER_FUNCT();
	requestGet(ENERGY, energy);
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
 * @param[out] energy array of threshold's
 * @param[in] len length of array of dimension nbModules
 */
void Camera::getKThresh(float* kthresh, int len) {
	DEB_MEMBER_FUNCT();
	requestGet(KTHRESH, kthresh);
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
 * @param[out] energy the maximum threshold energy in keV
 */
void Camera::getKThreshMax(float& kthresh) {
	DEB_MEMBER_FUNCT();
	requestGet(KTHRESHMAX, kthresh);
}

/**
 * Get the minimum supported threshold energy.
 * @param[out] energy the minimum threshold energy in keV
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
void Camera::setKthreshEnergy(float kthresh, float energy) {
	DEB_MEMBER_FUNCT();
	requestSet(KTHRESHENERGY, kthresh, energy);
}

/**
 * Loads predefined settings for the selected modules and the specified X-ray radiation.
 * The energy threshold is set to a suitable value. The availability of Cr and Ag settings
 * depends on the calibration of the Mythen. This action takes 2 seconds pre modules.
 * After initialisation the Cu settings are loaded.
 * @param[in] settings {@see Settings]
 */
void Camera::setPredefinedSettings(Settings settings) {
	DEB_MEMBER_FUNCT();
	requestSet(SETTINGS, PredefinedSettings[settings]);
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
 *
 */
void Camera::getFlatFieldCorrection(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int flatfieldcorrection;
	requestGet(FLATFIELDCORRECTION, flatfieldcorrection);
	enable = static_cast<Switch>(flatfieldcorrection);
}

void Camera::setFlatFieldCorrection(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(FLATFIELDCORRECTION, enable);
}

void Camera::getCutoff(int& cutoff) {
	DEB_MEMBER_FUNCT();
	requestGet(CUTOFF, cutoff);
}

void Camera::getFlatField(int& flatfield) {
	DEB_MEMBER_FUNCT();
	requestGet(FLATFIELD, flatfield);
}

void Camera::getRateCorrection(Switch& enable) {
	DEB_MEMBER_FUNCT();
	requestGet(RATECORRECTION, enable);
}

void Camera::setRateCorrection(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(RATECORRECTION, enable);
}

void Camera::getTau(int& tau) {
	DEB_MEMBER_FUNCT();
	requestGet(TAU, tau);
}

void Camera::setTau(float tau) {
	DEB_MEMBER_FUNCT();
	requestSet(TAU, tau);
}

void Camera::getGates(int& gates) {
	DEB_MEMBER_FUNCT();
	requestGet(GATES, gates);
}

void Camera::setGates(int gates) {
	DEB_MEMBER_FUNCT();
	requestSet(GATES, gates);
}

void Camera::setDelayAfterTrigger(long long time) {
	DEB_MEMBER_FUNCT();
	requestSet(DELBEF, time);
}

void Camera::getContinuousTrigger(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int trig;
	requestGet(CONTTRIG, trig);
	enable = static_cast<Switch>(trig);
}

void Camera::setContinuousTrigger(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(CONTTRIGEN, enable);
}

void Camera::getGateMode(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int gate;
	requestGet(GATE, gate);
	enable = static_cast<Switch>(gate);
}

void Camera::setGateMode(Switch enable) {
	DEB_MEMBER_FUNCT();
	requestSet(GATEEN, enable);
}

void Camera::getTriggered(Switch& enable) {
	DEB_MEMBER_FUNCT();
	int trig;
	requestGet(TRIG, trig);
	enable = static_cast<Switch>(trig);
}

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
	static_cast<Polarity>(in);
}

/**
 * Sets the polarity of the enable out signal
 * @param[in] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::setInputSignalPolarity(Polarity polarity) {
	DEB_MEMBER_FUNCT();
	requestSet(INPOL, polarity);
}

/**
 * Get the polarity of the enable out signal
 * @param[out] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::getOutputSignalPolarity(Polarity& polarity) {
	DEB_MEMBER_FUNCT();
	int out;
	requestGet(OUTPOL,out);
	static_cast<Polarity>(out);
}

/**
 * Sets the polarity of the enable out signal
 * @param[in] polarity @see Polarity (Rising_edge = active high)
 */
void Camera::setOutputSignalPolarity(Polarity polarity) {
	DEB_MEMBER_FUNCT();
	requestSet(OUTPOL, polarity);
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
	requestCmd(LOGSTART);
}

/**
 * Stops the logging of the Mythen socket server
 * @param[out] logSize returns the size of the log file.
 */
void Camera::logStop(int& logSize) {
	DEB_MEMBER_FUNCT();
	requestCmd(LOGSTOP, &logSize, 1);
}

/**
 * Get the contents of the log file
 * @param[out] buff the array containing the contents of the log file
 * @param[in] logSize the size of the log file returned from the {@see logStop} command
 */
void Camera::logRead(char* buff, int logSize) {
	DEB_MEMBER_FUNCT();
	int* ptr = reinterpret_cast<int*>(buff);
	requestCmd(LOGREAD, ptr, logSize);
}

/**
 * Returns the data of the oldest frame in the buffer (i.e. the frames are
 * returned in the same order as they were acquired. The Mythen can internally
 * buffer the last four acquired frames.  If the buffer is empty and there is
 * an ongoing measurement, the command returns the data after the measurement
 * has finished. If the readout fails all count are set to -1.
 */
void Camera::readout(int* data, int len) {
	DEB_MEMBER_FUNCT();
	requestCmd(READOUT, data, len);
}

/** Returns the data of the oldest frame in the buffer in a raw compressed format.
 * The Mythen does not apply any correction to the data , thereby allowing for
 * maximal frame rates.
 */
void Camera::readoutraw(int* data, int len) {
	DEB_MEMBER_FUNCT();
	requestCmd(READOUTRAW, data, len);
}

template<typename T>
void Camera::checkReply(T rc) {
	DEB_MEMBER_FUNCT();
	int irc = *((unsigned int *) &rc);
	if (irc < 0) {
		THROW_HW_ERROR(Error) << serverStatusMap[irc];
	}
}

string Camera::findCmd(ServerCmd cmd) {
	DEB_MEMBER_FUNCT();
	stringstream ss;
	map<ServerCmd, std::string>::iterator it;
	if ((it = serverCmdMap.find(cmd)) != serverCmdMap.end()) {
		string cmdStr = it->second;
		ss << "-" << cmdStr;
	} else {
		THROW_HW_ERROR(Error) << "Not a 'server cmd'  command";
	}
	return ss.str();
}

void Camera::requestCmd(ServerCmd cmd) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(&rc);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, sizeof(int));
	checkReply(rc);
}


void Camera::requestCmd(ServerCmd cmd, int* value, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(value);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, len * sizeof(4));
	rc = *((uint32_t *) recvBuf);
	checkReply(rc);
}

template<typename T>
void Camera::requestSet(ServerCmd cmd, T value) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(&rc);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, sizeof(int));
	checkReply(rc);
}

template<typename T>
void Camera::requestSet(ServerCmd cmd, T value1, T value2) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(&rc);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, sizeof(int));
	checkReply(rc);
}

template<typename T>
void Camera::requestGet(ServerCmd cmd, T& value) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	recvBuf = reinterpret_cast<uint8_t*>(&value);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, sizeof(T));
	checkReply(value);
}

void Camera::requestGet(ServerCmd cmd, string& reply, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t buffer[64];
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(buffer);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, len);
	rc = *((uint32_t *) recvBuf);
	checkReply(rc);
	stringstream os;
	os << buffer;
	reply = os.str();
}

template<typename T>
void Camera::requestGet(ServerCmd cmd, T* value, int len) {
	DEB_MEMBER_FUNCT();
	uint8_t* recvBuf;
	int rc;
	recvBuf = reinterpret_cast<uint8_t*>(value);
	m_mythen->sendCmd(findCmd(cmd), recvBuf, len * sizeof(4));
	rc = *((uint32_t *) recvBuf);
	checkReply(rc);
}

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

//typedef pair<const ServerCmd, string> serverCmdPair;
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
	{KTHRESHMAX, "kthreshmx"},
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
	{DELBEF, "delbef"},
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

ostream& operator <<(ostream& os, Camera::Polarity &polarity) {
	const char* name = "Unknown";
	switch (polarity) {
	case Camera::RISING_EDGE: name = "Rising Edge";	break;
	case Camera::FALLING_EDGE: name = "Falling Edge"; break;
	}
	return os << name;
}

ostream& operator <<(ostream& os, Camera::Settings &settings) {
	const char* name = "Unknown";
	switch (settings) {
	case Camera::Cu: name = "Cu"; break;
	case Camera::Mo: name = "Mo"; break;
	case Camera::Cr: name = "Cr"; break;
	case Camera::Ag: name = "Ag"; break;
	}
	return os << name;
}

