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


#define ConfigSeqTimVal(c, b, m, r, t)		\
	{{c, b, m, 0, 0, 0, 0, 0, 0, 0}, {r / 1e6, t / 1e6, 0, 0, 0}}

typedef TimingCtrl::ConfigTimingMeasureMap::value_type ConfigSeqTimPair;
static ConfigSeqTimPair InitialTimingMeasureCacheCList[] = {
	ConfigSeqTimVal(0, 1,  9, 99072, 0),
	ConfigSeqTimVal(0, 2,  9, 50880, 0),
	ConfigSeqTimVal(1, 1,  9, 50880, 0),
	ConfigSeqTimVal(1, 2,  9, 26784, 0),
	ConfigSeqTimVal(0, 1, 10, 49536, 1394),
	ConfigSeqTimVal(0, 2, 10, 25440, 1394),
	ConfigSeqTimVal(1, 1, 10, 25440, 1369),
	ConfigSeqTimVal(1, 2, 10, 13392, 1369),
};

TimingCtrl::TimingCtrl(Camera& cam)
	: m_cam(cam), m_model(cam.getModel()),
	  m_timing_measure_cache(C_LIST_ITERS(InitialTimingMeasureCacheCList))
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
		if (needSeqTimMeasure()) {
			DEB_ERROR() << "Camera needs SeqTim measurement";
			readout_time = -1;
		} else {
			Config config = getConfig();
			const SeqTimValues& st = m_timing_measure_cache[config];
			readout_time = st.readout_time;
		}
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
		if (needSeqTimMeasure()) {
			DEB_ERROR() << "Camera needs SeqTim measurement";
			xfer_time = -1;
		} else {
			Config config = getConfig();
			const SeqTimValues& st = m_timing_measure_cache[config];
			xfer_time = st.transfer_time;
		}
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

TimingCtrl::Config TimingCtrl::getConfig()
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::SeqTim))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "sequencer timing feature";

	Config config;
	readRegister(ConfigHD, config.config_hd);
	readRegister(BinVert, config.bin_vert);
	readRegister(ChanMode, config.chan_mode);
	readRegister(NbLinesXfer, config.nb_lines_xfer);
	readRegister(RoiEnable, config.roi_enable);
	readRegister(RoiFast, config.roi_fast);
	readRegister(RoiKinetic, config.roi_kinetic);
	if (config.roi_enable || config.roi_kinetic) {
		readRegister(RoiLineBegin, config.roi_line_begin);
		readRegister(RoiLineWidth, config.roi_line_width);
	} else {
		config.roi_line_begin = 0;
		config.roi_line_width = 0;
	}
	readRegister(ShutElecSelect, config.shut_elec_select);
	DEB_RETURN() << DEB_VAR1(config);
	return config;
}

bool TimingCtrl::needSeqTimMeasure()
{
	DEB_MEMBER_FUNCT();
	bool need_measure;
	if (m_model.has(Model::SeqTim)) {
		Config config = getConfig();
		DEB_TRACE() << DEB_VAR1(config);
		typedef ConfigTimingMeasureMap::const_iterator It;
		It it, end = m_timing_measure_cache.end();
		need_measure = (m_timing_measure_cache.find(config) == end);
	} else {
		need_measure = false;
	}
	DEB_RETURN() << DEB_VAR1(need_measure);
	return need_measure;
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
	DEB_PARAM() << DEB_VAR1(timeout);

	if (timeout == -1)
		timeout = 5.0;

	Config config = getConfig();

	ExtSync ext_sync_ena;
	m_cam.getExtSyncEnable(ext_sync_ena);
	if (ext_sync_ena != ExtSyncNone)
		THROW_HW_ERROR(Error) << "Camera ExtSyncEnable is not None";
		
	Camera::TempRegVal n = m_cam.getTempRegVal(NbFrames, 3);
	Camera::TempRegVal i = m_cam.getTempRegVal(ExpTime, 1);
	Camera::TempRegVal t = m_cam.getTempRegVal(LatencyTime, 0);
	Camera::TempRegVal u = m_cam.getTempRegVal(ShutEnable, 0);

	Camera::AcqSeq acq = m_cam.startAcqSeq();
	if (!acq.wait(timeout, true, true))
		THROW_HW_ERROR(Error) << "Camera not ready after "
				      << timeout << " sec";
	latchSeqTimValues(st);

	DEB_TRACE() << DEB_VAR2(config, st);
	m_timing_measure_cache[config] = st;
	DEB_RETURN() << DEB_VAR1(st);
}

std::ostream& lima::Frelon::operator <<(std::ostream& os,
					const TimingCtrl::Config& config)
{
	return os << '<'
		  << config.config_hd << ", "
		  << config.bin_vert << ", "
		  << config.chan_mode << ", "
		  << config.nb_lines_xfer << ", "
		  << config.roi_enable << ", "
		  << config.roi_fast << ", "
		  << config.roi_kinetic << ", "
		  << config.roi_line_begin << ", "
		  << config.roi_line_width << ", "
		  << config.shut_elec_select << '>';
}

bool lima::Frelon::operator <(const TimingCtrl::Config& a,
			      const TimingCtrl::Config& b)
{
#define check_val(a, b)				\
	do {					\
		if ((a) < (b))			\
			return true;		\
		else if ((a) > (b))		\
			return false;		\
	} while (0)

#define check(x) check_val(a.x, b.x)

	check(config_hd);
	check(bin_vert);
	check(chan_mode);
	check(nb_lines_xfer);
	check(roi_enable);
	check(roi_fast);
	check(roi_kinetic);
	check(roi_line_begin);
	check(roi_line_width);
	check(shut_elec_select);

#undef check
#undef check_val

	return false;
}
