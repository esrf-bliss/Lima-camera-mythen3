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

#include <algorithm>
#include "Debug.h"
#include "Mythen3Interface.h"

using namespace lima;
using namespace lima::Mythen3;
using namespace std;

/*******************************************************************
 * \brief DetInfoCtrlObj constructor
 *******************************************************************/
DetInfoCtrlObj::DetInfoCtrlObj(Camera& cam) :
		m_cam(cam) {
	DEB_CONSTRUCTOR();
}

DetInfoCtrlObj::~DetInfoCtrlObj() {
	DEB_DESTRUCTOR();
}

void DetInfoCtrlObj::getMaxImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	// get the max image size
	m_cam.getDetectorImageSize(size);
}

void DetInfoCtrlObj::getDetectorImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	// get the max image size of the detector
	m_cam.getDetectorImageSize(size);
}

void DetInfoCtrlObj::getDefImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
// TODO change to Bpp24 when available
	image_type = Bpp32;
}

void DetInfoCtrlObj::getCurrImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.getImageType(image_type);
}

void DetInfoCtrlObj::setCurrImageType(ImageType image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.setImageType(image_type);
}

void DetInfoCtrlObj::getPixelSize(double& xsize, double& ysize) {
	DEB_MEMBER_FUNCT();
	m_cam.getPixelSize(xsize, ysize);
}

void DetInfoCtrlObj::getDetectorType(std::string& type) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorType(type);
}

void DetInfoCtrlObj::getDetectorModel(std::string& model) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorModel(model);
}

void DetInfoCtrlObj::registerMaxImageSizeCallback(
		HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam.registerMaxImageSizeCallback(cb);
}

void DetInfoCtrlObj::unregisterMaxImageSizeCallback(
		HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam.unregisterMaxImageSizeCallback(cb);
}

/*******************************************************************
 * \brief SyncCtrlObj constructor
 *******************************************************************/

SyncCtrlObj::SyncCtrlObj(Camera& cam) :
		HwSyncCtrlObj(), m_cam(cam) {
	DEB_CONSTRUCTOR();
}

SyncCtrlObj::~SyncCtrlObj() {
	DEB_DESTRUCTOR();
}

bool SyncCtrlObj::checkTrigMode(TrigMode trig_mode) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);

	bool valid_mode;
	switch (trig_mode) {
	case IntTrig:
	case ExtTrigSingle:
	case ExtTrigMult:
	case ExtGate:
		valid_mode = true;
		break;
	default:
		valid_mode = false;
	}
	DEB_RETURN() << DEB_VAR1(valid_mode);
	return valid_mode;
}

void SyncCtrlObj::setTrigMode(TrigMode trig_mode) {
	DEB_MEMBER_FUNCT();
	if (!checkTrigMode(trig_mode))
		THROW_HW_ERROR(InvalidValue) << "Invalid "
					     << DEB_VAR1(trig_mode);
	m_cam.setTrigMode(trig_mode);
}

void SyncCtrlObj::getTrigMode(TrigMode& trig_mode) {
	DEB_MEMBER_FUNCT();
	m_cam.getTrigMode(trig_mode);
}

void SyncCtrlObj::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	m_cam.setExpTime(exp_time);
}

void SyncCtrlObj::getExpTime(double& exp_time) {
	DEB_MEMBER_FUNCT();
	m_cam.getExpTime(exp_time);
}

void SyncCtrlObj::setLatTime(double lat_time) {
	DEB_MEMBER_FUNCT();
	m_cam.setLatTime(lat_time);
}

void SyncCtrlObj::getLatTime(double& lat_time) {
	DEB_MEMBER_FUNCT();
	m_cam.getLatTime(lat_time);
}

void SyncCtrlObj::setNbHwFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	m_cam.setNbFrames(nb_frames);
}

void SyncCtrlObj::getNbHwFrames(int& nb_frames) {
	DEB_MEMBER_FUNCT();
	m_cam.getNbFrames(nb_frames);
}

void SyncCtrlObj::getValidRanges(ValidRangesType& valid_ranges) {
	double min_time = 10e-9;
	double max_time = 1e6;
// TODO check with m_cam
	valid_ranges.min_exp_time = min_time;
	valid_ranges.max_exp_time = max_time;
	valid_ranges.min_lat_time = min_time;
	valid_ranges.max_lat_time = max_time;
}

/*******************************************************************
 * \brief Interface constructor
 *******************************************************************/

Interface::Interface(Camera& cam) :
		m_cam(cam), m_det_info(cam), m_sync(cam) {
	DEB_CONSTRUCTOR();

	HwDetInfoCtrlObj *det_info = &m_det_info;
	m_cap_list.push_back(HwCap(det_info));

	m_bufferCtrlObj = m_cam.getBufferCtrlObj();
	HwBufferCtrlObj *m_bufferCtrlObj = m_cam.getBufferCtrlObj();
	m_cap_list.push_back(m_bufferCtrlObj);

	HwSyncCtrlObj *sync = &m_sync;
	m_cap_list.push_back(HwCap(sync));
}

Interface::~Interface() {
	DEB_DESTRUCTOR();
}

void Interface::getCapList(HwInterface::CapList &cap_list) const {
	DEB_MEMBER_FUNCT();
	cap_list = m_cap_list;
}

void Interface::reset(ResetLevel reset_level) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(reset_level);

	stopAcq();
	m_cam.reset();

	Size image_size;
	m_det_info.getMaxImageSize(image_size);
	ImageType image_type;
	m_det_info.getDefImageType(image_type);
	FrameDim frame_dim(image_size, image_type);

	m_bufferCtrlObj->setFrameDim(frame_dim);
	m_bufferCtrlObj->setNbConcatFrames(1);
	m_bufferCtrlObj->setNbBuffers(1);
}

void Interface::prepareAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.prepareAcq();
}

void Interface::startAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.startAcq();
}

void Interface::stopAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.stopAcq();
}

void Interface::getStatus(StatusType& status) {
	DEB_MEMBER_FUNCT();
	status.acq = m_cam.isAcqRunning() ? AcqRunning : AcqReady;
	status.det_mask = DetExposure | DetReadout | DetLatency;

//	Camera::Status mythen_status;
//	m_cam.getStatus(mythen_status);
//	switch (mythen_status) {
//	case Camera::Ready:
//		status.acq = AcqReady;
//		status.det = DetIdle;
//		break;
//	case Camera::Exposure:
//		status.det = DetExposure;
//		status.acq = AcqRunning;
//		break;
//	case Camera::Readout:
//		status.det = DetReadout;
//		status.acq = AcqRunning;
//		break;
//	case Camera::Latency:
//		status.det = DetLatency;
//		status.acq = AcqRunning;
//		break;
//	}
}

int Interface::getNbHwAcquiredFrames() {
	DEB_MEMBER_FUNCT();
	return getNbHwAcquiredFrames();
}
