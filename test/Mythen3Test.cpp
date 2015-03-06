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
#include "lima/HwInterface.h"
#include "lima/CtControl.h"
#include "lima/CtAccumulation.h"
#include "lima/CtAcquisition.h"
#include "lima/CtSaving.h"
#include "lima/CtShutter.h"
#include "lima/Constants.h"

#include "Mythen3Camera.h"
#include "Mythen3Interface.h"
#include "lima/Debug.h"

using namespace std;
using namespace lima;
using namespace lima::Mythen3;

DEB_GLOBAL(DebModTest);

int main () {
	DEB_GLOBAL_FUNCT();

	Camera *m_camera;
	Interface *m_interface;
	CtControl* m_control;

	string hostname = "160.103.146.190";
	int port = 1031;
	bool simulate = false;

	try {
		m_camera = new Camera(hostname, port, simulate);
		m_interface = new Interface(*m_camera);
		m_control = new CtControl(m_interface);

		string type;
		m_camera->getDetectorType(type);
		cout << "Detector type : " << type << endl;
		string model;
		m_camera->getDetectorModel(model);
		cout << "Detector model: " << model << endl;


		// setup fileformat and data saving info
		CtSaving* saving = m_control->saving();
		saving->setDirectory("/buffer/dubble281/mythen");
		saving->setFormat(CtSaving::HDF5);
	 	saving->setPrefix("mythen3_");
		saving->setSuffix(".hdf");
		saving->setSavingMode(CtSaving::AutoFrame);
		saving->setOverwritePolicy(CtSaving::Append);

		// do acquisition
		int nframes = 4;
		m_control->acquisition()->setAcqExpoTime(2.0);
		m_control->acquisition()->setAcqNbFrames(nframes);
		m_control->prepareAcq();
		m_control->startAcq();
		sleep(12);

	} catch (Exception &e) {
		cout << "Exception!!!!" << endl;
	}
}
