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
#include "MiscUtils.h"
#include "Mythen3Net.h"
#include "Mythen3.h"
//#include "CtSaving.h"
//#include "CtAcquisition.h"
//#include "MerlinCamera.h"
//#include "MerlinInterface.h"

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
//	float param;
//	int value;
//	Camera::Switch mode;
//	Camera::Depth depth;
//	Camera::Counter counter;
//	Camera::GainSetting gain;
//	Camera::Trigger trigger;
//
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
//		// Get operations
//		m_camera->getAcquisitionPeriod(param);
//		cout << "AcquisitionPeriod: " << param << endl;
//		m_camera->setAcquisitionPeriod(23.6);
//		m_camera->getAcquisitionPeriod(param);
//		cout << "AcquisitionPeriod: " << param << endl;
//		m_camera->getAcquisitionTime(param);
//		cout << "AcquisitionTime: " << param << endl;
//		m_camera->getChargeSumming(mode);
//		cout << "ChargeSumming: " << mode << endl;
//		m_camera->getColourMode(mode);
//		cout << "ColourMode: " << mode << endl;
//		m_camera->getContinuousRW(mode);
//		cout << "ContinuousRW: " << mode << endl;
//		m_camera->getCounterDepth(depth);
//		cout << "CounterDepth: " << depth << endl;
//		m_camera->getEnableCounters(counter);
//		cout << "EnableCounters: " << counter << endl;
//		m_camera->getFramesPerTrigger(value);
//		cout << "FramesPerTrigger: " << value << endl;
//		m_camera->getGain(gain);
//		cout << "Gain: " << gain << endl;
//		m_camera->getNumFramesToAcquire(value);
//		cout << "NumFramesToAcquire: " << value << endl;
//		m_camera->getOperatingEnergy(param);
//		cout << "OperatingEnergy: " << param << endl;
//		m_camera->getTemperature(param);
//		cout << "Temperature: " << param << endl;
//		m_camera->getThreshold0( param);
//		cout << "Threshold0: " << param << endl;
//		m_camera->getThreshold1( param);
//		cout << "Threshold1: " << param << endl;
//		m_camera->getThreshold2( param);
//		cout << "Threshold2: " << param << endl;
//		m_camera->getThreshold3( param);
//		cout << "Threshold3: " << param << endl;
//		m_camera->getThreshold4( param);
//		cout << "Threshold4: " << param << endl;
//		m_camera->getThreshold5( param);
//		cout << "Threshold5: " << param << endl;
//		m_camera->getThreshold6( param);
//		cout << "Threshold6: " << param << endl;
//		m_camera->getThreshold7( param);
//		cout << "Threshold7: " << param << endl;
//		m_camera->getTHScan(value);
//		cout << "THScan: " << value << endl;
//		m_camera->getTHStart(param);
//		cout << "TTHStart: " << param << endl;
//		m_camera->getTHStep(param);
//		cout << "THStep: " << param << endl;
//		m_camera->getTHStop(param);
//		cout << "THStop: " << param << endl;
//		m_camera->getTriggerStartType(trigger);
//		cout << "TriggerStart: " << trigger << endl;
//		m_camera->getTriggerStopType(trigger);
//		cout << "TriggerStop: " << trigger << endl;
//
//		// Set operations
//		m_camera->setAcquisitionPeriod(16.0);
//		m_camera->setAcquisitionTime(12.0);
//		m_camera->setChargeSumming(Camera::ON);
//		m_camera->setChargeSumming(Camera::OFF);
//		m_camera->setColourMode(Camera::ON);
//		m_camera->setColourMode(Camera::OFF);
//		m_camera->setContinuousRW(Camera::ON);
//		m_camera->setContinuousRW(Camera::OFF);
//		m_camera->setCounterDepth(Camera::BPP1);
//		m_camera->setCounterDepth(Camera::BPP6);
//		m_camera->setCounterDepth(Camera::BPP12);
//		m_camera->setCounterDepth(Camera::BPP24);
//		m_camera->setEnableCounters(Camera::BOTH);
//		m_camera->setEnableCounters(Camera::COUNTER0);
//		m_camera->setEnableCounters(Camera::COUNTER1);
//		m_camera->setFramesPerTrigger(430);
//		m_camera->setGain(Camera::SHGM);
//		m_camera->setGain(Camera::HGM);
//		m_camera->setGain(Camera::LGM);
//		m_camera->setGain(Camera::SLGM);
//		m_camera->setNumFramesToAcquire(345);
//		m_camera->setOperatingEnergy(358.0);
//		m_camera->setThreshold0(67.0);
//		m_camera->setThreshold1(67.1);
//		m_camera->setThreshold2(67.2);
//		m_camera->setThreshold3(67.3);
//		m_camera->setThreshold4(67.4);
//		m_camera->setThreshold5(67.5);
//		m_camera->setThreshold6(67.6);
//		m_camera->setThreshold7(67.7);
//		m_camera->setTHScan(5);
//		m_camera->setTHStart(30.);
//		m_camera->setTHStep(1.);
//		m_camera->setTHStop(40.);
//		m_camera->setTriggerStartType(Camera::INTERNAL);
//		m_camera->setTriggerStartType(Camera::RISING_EDGE);
//		m_camera->setTriggerStartType(Camera::FALLING_EDGE);
//		m_camera->setTriggerStopType(Camera::INTERNAL);
//		m_camera->setTriggerStopType(Camera::RISING_EDGE);
//		m_camera->setTriggerStopType(Camera::FALLING_EDGE);
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
//		saving->setDirectory("/home/grm84/users/merlin/data");
//		saving->setFormat(CtSaving::EDF);
//	 	saving->setPrefix("merlin_");
//		saving->setSuffix(".edf");
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
