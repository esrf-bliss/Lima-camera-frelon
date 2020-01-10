//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
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
#include "FrelonTimingCtrl.h"
#include "FrelonCamera.h"

#include "lima/MiscUtils.h"

using namespace lima;
using namespace lima::Frelon;
using namespace std;

const double TimingCtrl::SeqTim::ClockPeriod = 100e-9;

typedef TimingCtrl::SeqTim::RegPair SeqTimRegPair;
static SeqTimRegPair SeqTimRegPairCList[] = {
	SeqTimRegPair(SeqTimRdOutH,		SeqTimRdOutL),
	SeqTimRegPair(SeqTimTransferH,		SeqTimTransferL),
	SeqTimRegPair(SeqTimEShutH,		SeqTimEShutL),
	SeqTimRegPair(SeqTimExposureH,		SeqTimExposureL),
	SeqTimRegPair(SeqTimFramePeriodH,	SeqTimFramePeriodL),
};
TimingCtrl::SeqTim::RegPairList
TimingCtrl::SeqTim::RegList(C_LIST_ITERS(SeqTimRegPairCList));

SeqTimValues TimingCtrl::SeqTim::calcValues(const ValPairList& l)
{
	if (l.size() != RegList.size())
		throw LIMA_EXC(Hardware, InvalidValue,
			       "SeqTim::calcValues: bad val list");

	SeqTimValues st;
	ValPairList::const_iterator it = l.begin();
	st.readout_time = calcSeqTim(*it++);
	st.transfer_time = calcSeqTim(*it++);
	st.electronic_shutter_time = calcSeqTim(*it++);
	st.exposure_time = calcSeqTim(*it++);
	st.frame_period = calcSeqTim(*it++);
	return st;
}

TimingCtrl::TimingCtrl(Camera& cam)
	: m_cam(cam), m_model(cam.getModel())
{
	DEB_CONSTRUCTOR();
}

TimingCtrl::~TimingCtrl()
{
	DEB_DESTRUCTOR();
}

void TimingCtrl::writeRegister(Reg reg, int val)
{
	m_cam.writeRegister(reg, val);
}

void TimingCtrl::readRegister(Reg reg, int& val)
{
	m_cam.readRegister(reg, val);
}

void TimingCtrl::readFloatRegister(Reg reg, double& val)
{
	m_cam.readFloatRegister(reg, val);
}

void TimingCtrl::getReadoutTime(double& readout_time)
{
	DEB_MEMBER_FUNCT();
	if (m_model.has(Model::SeqTim)) {
		readout_time = 100e-3;
	} else if (m_model.has(Model::TimeCalc)) {
		readFloatRegister(ReadoutTime, readout_time);
		readout_time *= 1e-6;
	} else {
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "readout time calculation";
	}
	DEB_RETURN() << DEB_VAR1(readout_time);
}

void TimingCtrl::getTransferTime(double& xfer_time)
{
	DEB_MEMBER_FUNCT();
	if (m_model.has(Model::SeqTim)) {
		xfer_time = 100e-3;
	} else if (m_model.has(Model::TimeCalc)) {
		readFloatRegister(TransferTime, xfer_time);
		xfer_time *= 1e-6;
	} else {
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "shift time calculation";
	}
	DEB_RETURN() << DEB_VAR1(xfer_time);
}

void TimingCtrl::getDeadTime(double& dead_time)
{
	DEB_MEMBER_FUNCT();
	getTransferTime(dead_time);
	if (dead_time == 0)
		getReadoutTime(dead_time);
	DEB_RETURN() << DEB_VAR1(dead_time);
}

void TimingCtrl::latchSeqTimValues(SeqTimValues& st)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::SeqTim))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "sequencer timing feature";

	SeqTim::ValPairList l;
	const SeqTim::RegPairList& r = SeqTim::RegList;
	SeqTim::RegPairList::const_iterator it, end = r.end();
	for (it = r.begin(); it != end; ++it) {
		SeqTim::ValPair v;
		readRegister(it->first, v.first);
		readRegister(it->second, v.second);
		l.push_back(v);
	}
	st = SeqTim::calcValues(l);
	DEB_RETURN() << DEB_VAR1(st);
}

void TimingCtrl::measureSeqTimValues(SeqTimValues& st, double timeout)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::SeqTim))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "sequencer timing feature";
	ExtSync ext_sync_ena;
	m_cam.getExtSyncEnable(ext_sync_ena);
	if (ext_sync_ena != ExtSyncNone)
		THROW_HW_ERROR(Error) << "Camera ExtSyncEnable is not None";
		
	Camera::TempRegVal n = m_cam.getTempRegVal(NbFrames, 3);
	Camera::TempRegVal i = m_cam.getTempRegVal(ExpTime, 1);
	Camera::TempRegVal u = m_cam.getTempRegVal(ShutEnable, 0);

	Camera::AcqSeq acq = m_cam.startAcqSeq();
	if (!acq.wait(timeout))
		THROW_HW_ERROR(Error) << "Camera not ready after "
				      << timeout << " sec";
	latchSeqTimValues(st);
}

