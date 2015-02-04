/*
 * reply.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: grm84
 */
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <math.h>
#include "lima/MiscUtils.h"
#include "Mythen3Net.h"
#include "Mythen3.h"

using namespace std;
using namespace lima;
using namespace lima::Mythen3;

//DEB_GLOBAL(DebModTest);
lima::Mythen3::Mythen3Net *m_mythen;





//-----------------------------------------------------------------------------------------
// g++ -std=c++0x -I ../../../common/include -o myth Mythenv3Test.cpp

int main () {
	
	m_mythen = new lima::Mythen3::Mythen3Net();
	cout << "mythenNet constructed" << endl;
	m_mythen->connectToServer("160.103.146.190", 1031);
	cout << "mythenNet connected" << endl;
	string s;
	getVersion(s);
	cout << "version is " << s << endl;
	getAssemblyDate(s);
	cout << "assemblydate is " << s << endl;
	int i;
	getNbMaxModules(i);
	cout << "maxmodules is " << i << endl;
	int nmod;
	getNbModules(nmod);
	cout << "nb modules " << nmod << endl;
	int m;
	getModule(m);
	cout << "module is " << m << endl;
	int id;
	getCommandId(id);
	cout << "command Id is " << id << endl;
	long long time;
	getTime(time);
	cout << "time " << time << endl;
	int flipEnabled;
	getFlipChannels(flipEnabled);
	cout << "flip " << flipEnabled << endl;
	float energy;
	getEnergyMax(energy);
	cout << "energy " << energy << endl;
	setModule(0);
	getModule(m);
	cout << "module is " << m << endl;
	reset();
	
	int len = nmod*1280;
	int badChannels[len];
	for (int j=0;j<len; j++)
	  badChannels[j] = 0;
	getBadChannels(badChannels, len);
	for (int j=0;j<len; j++) {
	  if (badChannels[j] != 0)
	    cout << " bad channel detected at location " << j << " value " << badChannels[j] << endl;
	}
	start();
	sleep(1);
	stop();
	int data[len];
	readout(data, len);
	for (int j=0;j<len; j++) {
	  if (data[j] != 0)
	    cout << " data at location " << j << " value " << data[j] << endl;
	}

//	DEB_GLOBAL_FUNCT();
//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);
//
//	Camera *m_camera;
//	Interface *m_interface;
//	CtControl* m_control;
//
//	string hostname = "tcfidell11";
//	int port = 6342;
//	int dataPort = 6341;
//	int nx = 256;
//	int ny = 256;
//	bool simulate = true;
//
///
//	try {
//
//		m_camera = new Camera(hostname, port, dataPort, nx, ny, simulate);
//		m_interface = new Interface(*m_camera);
//		m_control = new CtControl(m_interface);
//
//		string type;
//		m_camera->getDetectorType(type);
//		cout << "Detector type : " << type << endl;
//		string model;
//		m_camera->getDetectorModel(model);
//		cout << "Detector model: " << model << endl;
//
//
//		// Commands
//		m_camera->startAcquisition();
//		m_camera->stopAcquisition();
//		m_camera->abort();
//		m_camera->thscan();
//		m_camera->resetHw();
//
//		// setup fileformat and data saving info
//		CtSaving* saving = m_control->saving();
//		saving->setDirectory("/home/grm84/users/mythen3/data");
//		saving->setFormat(CtSaving::HDF5);
//	 	saving->setPrefix("mythen3_");
//		saving->setSuffix(".hdf");
//		saving->setSavingMode(CtSaving::AutoFrame);
//		saving->setOverwritePolicy(CtSaving::Append);
//
//		// do acquisition
//		int nframes = 3;
//		m_control->acquisition()->setAcqExpoTime(5.0);
//		m_control->acquisition()->setAcqNbFrames(nframes);
//		m_control->prepareAcq();
//		m_control->startAcq();
//		while(1) {
//			usleep(100);
//			if (!m_camera->isAcqRunning())
//				break;
//		}
//
//	} catch (Exception &e) {
//
//	}
}
