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
#include "FrelonCamera.h"

using namespace lima;
using namespace lima::Frelon;
using namespace std;

const double Camera::ResetLinkWaitTime = 5;
const double Camera::UpdateCcdStatusTime = 0.1;
const double Camera::MaxIdleWaitTime = 2.5;
const double Camera::MaxBusyRetryTime = 0.2;	// 16 Mpixel image Aurora Xfer

Camera::Camera(Espia::SerialLine& espia_ser_line)
	: m_ser_line(espia_ser_line), m_timing_ctrl(m_model, m_ser_line),
	  m_geom(NULL)
{
	DEB_CONSTRUCTOR();

	sync();
}

Camera::~Camera()
{
	DEB_DESTRUCTOR();

	stop();
}

void Camera::sync()
{
	DEB_MEMBER_FUNCT();

	Espia::Dev& dev = getEspiaDev();
	int chan_up_led;
	dev.getChanUpLed(chan_up_led);
	if (!chan_up_led) {
		DEB_WARNING() << "Aurora link down. Forcing a link reset!";
		dev.resetLink();
		DEB_TRACE() << "Sleeping additional "
			    << DEB_VAR1(Frelon::Camera::ResetLinkWaitTime);
		Sleep(ResetLinkWaitTime);
	}

	DEB_TRACE() << "Synchronizing with the camera";

	m_model.reset();
	m_started = false;

	try {
		syncRegs();
	} catch (Exception e) {
		string err_msg = e.getErrMsg();
		DEB_TRACE() << "Error in sync: " << DEB_VAR1(err_msg);
		string timeout_msg = Espia::StrError(SCDXIPCI_ERR_TIMEOUT);
		bool timeout = (err_msg.find(timeout_msg) != string::npos);
		if (!timeout)
			throw;

		THROW_HW_ERROR(Error) << "Serial connection timeout: "
					 "is camera ON and connected?";
	}

	string ver;
	m_model.getFirmware().getVersionStr(ver);
	DEB_ALWAYS() << "Found Frelon " << m_model.getName() 
		     << " #" << m_model.getSerialNb() << ", FW:" << ver;
}

void Camera::syncRegs()
{
	DEB_MEMBER_FUNCT();

	m_ser_line.clearCache();

	string ver;
	getVersionStr(ver);
	m_model.setVersionStr(ver);

	int complex_ser_nb;
	getComplexSerialNb(complex_ser_nb);
	m_model.setComplexSerialNb(complex_ser_nb);
	if (m_model.has(Model::CamChar)) {
		int cam_char;
		readRegister(CamChar, cam_char);
		m_model.setCamChar(cam_char);
	}

	if (m_geom == NULL) {
		GeomType geom_type = m_model.getGeomType();
		switch (geom_type) {
		case SPB12_4_Quad:
		case Hamamatsu:
		case SPB2_F16:
		case SPB8_F16_Half:
			m_geom = new Geometry(*this);
			break;
		case SPB8_F16_Dual:
			THROW_HW_ERROR(NotSupported) << DEB_VAR1(geom_type);
		}
	}

	if (m_model.has(Model::GoodHTD))
		syncRegsGoodHTD();
	else 
		syncRegsBadHTD();

	double exp_time;
	getExpTime(exp_time);
	m_trig_mode = (exp_time == 0) ? ExtGate : IntTrig;

	// A sequencer cmd will send the CCD status byte to the Espia
	m_nb_frames = 1;
	writeRegister(NbFrames, m_nb_frames);
	DEB_TRACE() << "Sleeping " << UpdateCcdStatusTime << " s to allow "
		    << "CCD status byte get updated";
	Sleep(UpdateCcdStatusTime);

	m_geom->sync();
}

void Camera::syncRegsGoodHTD()
{
	DEB_MEMBER_FUNCT();

	Status status;
	bool use_ser_line, read_spb2;
	use_ser_line = read_spb2 = true;
	getStatus(status, use_ser_line, read_spb2);
	if (status != Wait)
		DEB_WARNING() << "Camera not IDLE: " 
			      << DEB_VAR1(DEB_HEX(status));
	int retry = 0;
	do {
		setExtSyncEnable(ExtSyncNone);
		sendCmd(Stop);
		bool ok = waitIdleStatus(status, use_ser_line, read_spb2);
		if (ok) {
			if (retry > 0)
				DEB_TRACE() << "Succeeded after " << retry 
					    << " retries";
			break;
		} else if (retry == 0) {
			DEB_WARNING() << "Tyring a hard reset ...";
			sendCmd(Reset);
		}
	} while (++retry < 2);
					  
	if (status != Wait)
		THROW_HW_ERROR(Error) << "Wrong camera BUSY status: "
				      << DEB_VAR1(DEB_HEX(status));
}


void Camera::syncRegsBadHTD()
{
	DEB_MEMBER_FUNCT();

	Status status;
	getStatus(status);
	if ((status != 0) && (status != Wait)) {
		DEB_WARNING() << "Camera not IDLE: " 
			      << DEB_VAR1(DEB_HEX(status));
		double exp_time;
		getExpTime(exp_time);
		if (exp_time > 0) {
			DEB_TRACE() << "Sending software STOP ...";
			sendCmd(Stop);
		}

		if (waitIdleStatus(status))
			DEB_TRACE() << "Succeeded";
		else
			DEB_WARNING() << "Camera still not IDLE: " 
				      << DEB_VAR1(DEB_HEX(status));
	}
					  
	Espia::Dev& dev = getEspiaDev();
	DEB_TRACE() << "Forcing Aurora link reset on old firmware!";
	dev.resetLink();
	DEB_TRACE() << "Sleeping additional "
		    << DEB_VAR1(Frelon::Camera::ResetLinkWaitTime);
	Sleep(ResetLinkWaitTime);

	if (m_model.has(Model::HTDCmd))
		setExtSyncEnable(ExtSyncBoth);
}


SerialLine& Camera::getSerialLine()
{
	return m_ser_line;
}

Espia::Dev& Camera::getEspiaDev()
{
	Espia::SerialLine& ser_line = m_ser_line.getEspiaSerialLine();
	Espia::Dev& dev = ser_line.getDev();
	return dev;
}

void Camera::sendCmd(Cmd cmd)
{
	DEB_MEMBER_FUNCT();
	const string& cmd_str = CmdStrMap[cmd];
	DEB_PARAM() << DEB_VAR2(cmd, cmd_str);
	if (cmd_str.empty())
		THROW_HW_ERROR(InvalidValue) << "Invalid " 
					     << DEB_VAR1(cmd);

	string resp;
	m_ser_line.sendFmtCmd(cmd_str, resp);
}

void Camera::writeRegister(Reg reg, int val)
{
	DEB_MEMBER_FUNCT();

	Timestamp t0 = Timestamp::now();
	int retry;
	bool end, prev_busy = false;
	for (retry = 0, end = false; !end; retry++) {
		bool fail, busy;
		try {
			m_ser_line.writeRegister(reg, val);
			if (retry > 0)
				DEB_WARNING() << "Succeeded after " << retry 
					      << " retrie(s)";
			return;
		} catch (Exception e) {
			string err_msg = e.getErrMsg();
			DEB_TRACE() << "Error in write: " << DEB_VAR1(err_msg);

			static const string fail_msg = "Frelon Error: FAI";
			fail = (err_msg.find(fail_msg) != string::npos);
			static const string busy_msg = "Frelon Error: BSY";
			busy = (err_msg.find(busy_msg) != string::npos);

			if (!fail && !busy)
				throw;
		}

		if ((prev_busy != busy) && (retry > 0)) {
			DEB_WARNING() << "Write error type changed after " 
				      << retry << " retrie(s)! Restarting ...";
			retry = 0;
			t0 = Timestamp::now();
		}
		prev_busy = busy;

		if (busy)
			end = ((Timestamp::now() - t0) >= MaxBusyRetryTime);
		else
			end = (retry > 0);
		if (!end)
			DEB_TRACE() << "Retrying ...";
	}

	if (prev_busy)
		THROW_HW_ERROR(Error) << "Frelon Camera still busy after "
				      << MaxBusyRetryTime << " sec";
	else
		THROW_HW_ERROR(Error) << "Frelon Camera still failing after "
				      << retry << " retrie(s)";
}

void Camera::readRegister(Reg reg, int& val)
{
	DEB_MEMBER_FUNCT();
	m_ser_line.readRegister(reg, val);
}

void Camera::readFloatRegister(Reg reg, double& val)
{
	DEB_MEMBER_FUNCT();
	m_ser_line.readFloatRegister(reg, val);
}

void Camera::hardReset()
{
	DEB_MEMBER_FUNCT();

	DEB_TRACE() << "Reseting the camera";
	sendCmd(Reset);

	sync();
}

void Camera::getVersionStr(string& ver)
{
	DEB_MEMBER_FUNCT();
	string cmd = RegStrMap[Version] + "?";
	m_ser_line.sendFmtCmd(cmd, ver);
	DEB_RETURN() << DEB_VAR1(ver);
}

void Camera::getComplexSerialNb(int& complex_ser_nb)
{
	DEB_MEMBER_FUNCT();
	readRegister(CompSerNb, complex_ser_nb);
	DEB_RETURN() << DEB_VAR1(DEB_HEX(complex_ser_nb));
}

Model& Camera::getModel()
{
	return m_model;
}

TimingCtrl& Camera::getTimingCtrl()
{
	return m_timing_ctrl;
}

string Camera::getInputChanModeName(FrameTransferMode ftm, 
				    InputChan input_chan)
{
	DEB_MEMBER_FUNCT();
	return m_geom->getInputChanModeName(ftm, input_chan);
}

bool Camera::getDefInputChan(FrameTransferMode ftm, InputChan& input_chan)
{
	DEB_MEMBER_FUNCT();
	return m_geom->getDefInputChan(ftm, input_chan);
}

void Camera::setInputChan(InputChan input_chan)
{
	DEB_MEMBER_FUNCT();
	m_geom->setInputChan(input_chan);
}

void Camera::getInputChan(InputChan& input_chan)
{
	DEB_MEMBER_FUNCT();
	m_geom->getInputChan(input_chan);
}

void Camera::setFrameTransferMode(FrameTransferMode ftm)
{
	DEB_MEMBER_FUNCT();
	m_geom->setFrameTransferMode(ftm);
}

void Camera::getFrameTransferMode(FrameTransferMode& ftm)
{
	DEB_MEMBER_FUNCT();
	m_geom->getFrameTransferMode(ftm);
}

void Camera::getMaxFrameDim(FrameDim& max_frame_dim)
{
	DEB_MEMBER_FUNCT();
	m_geom->getMaxFrameDim(max_frame_dim);
}

void Camera::getFrameDim(FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();
	m_geom->getFrameDim(frame_dim);
}

bool Camera::isChanActive(InputChan curr, InputChan chan)
{
	DEB_MEMBER_FUNCT();
	return m_geom->isChanActive(curr, chan);
}

void Camera::checkFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();
	m_geom->checkFlip(flip);
}

void Camera::setFlip(const Flip& flip)
{
	DEB_MEMBER_FUNCT();
	m_geom->setFlip(flip);
}

void Camera::getFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();
	m_geom->getFlip(flip);
}

void Camera::checkBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_geom->checkBin(bin);
}

void Camera::setBin(const Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_geom->setBin(bin);
}

void Camera::getBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_geom->getBin(bin);
}

void Camera::setRoiMode(RoiMode roi_mode)
{
	DEB_MEMBER_FUNCT();
	m_geom->setRoiMode(roi_mode);
}

void Camera::getRoiMode(RoiMode& roi_mode)
{
	DEB_MEMBER_FUNCT();
	m_geom->getRoiMode(roi_mode);
}

void Camera::checkRoi(const Roi& set_roi, Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	m_geom->checkRoi(set_roi, hw_roi);
}

void Camera::setRoi(const Roi& set_roi)
{
	DEB_MEMBER_FUNCT();
	m_geom->setRoi(set_roi);
}

void Camera::getRoi(Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	m_geom->getRoi(hw_roi);
}

void Camera::setRoiBinOffset(const Point& roi_bin_offset)
{
	DEB_MEMBER_FUNCT();
	m_geom->setRoiBinOffset(roi_bin_offset);
}

void Camera::getRoiBinOffset(Point& roi_bin_offset)
{
	DEB_MEMBER_FUNCT();
	m_geom->getRoiBinOffset(roi_bin_offset);
}

void Camera::setTrigMode(TrigMode trig_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);
	m_trig_mode = trig_mode;
	double exp_time;
	getExpTime(exp_time);
	if ((trig_mode == ExtGate) && (exp_time != 0))
		setExpTime(0);
	else if ((trig_mode != ExtGate) && (exp_time == 0)) 
		setExpTime(1);
	setNbFrames(m_nb_frames);
}

void Camera::getTrigMode(TrigMode& trig_mode)
{
	DEB_MEMBER_FUNCT();
	trig_mode = m_trig_mode;
	DEB_RETURN() << DEB_VAR1(trig_mode);
}

void Camera::setTimeUnitFactor(TimeUnitFactor time_unit_factor)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(time_unit_factor);
	int time_unit = int(time_unit_factor);
	writeRegister(TimeUnit, time_unit);
	m_geom->deadTimeChanged();
}

void Camera::getTimeUnitFactor(TimeUnitFactor& time_unit_factor)
{
	DEB_MEMBER_FUNCT();
	int time_unit;
	readRegister(TimeUnit, time_unit);
	time_unit_factor = TimeUnitFactor(time_unit);
	DEB_RETURN() << DEB_VAR1(time_unit_factor);
}

int Camera::calcTimeUnits(double time_sec, TimeUnitFactor time_unit_factor)
{
	return int(time_sec / TimeUnitFactorMap[time_unit_factor] + 0.1);
}

void Camera::setExpTime(double exp_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(exp_time);

	TrigMode trig_mode;
	getTrigMode(trig_mode);
	if ((trig_mode == ExtGate) && (exp_time != 0)) {
		DEB_TRACE() << "Ignoring " << DEB_VAR1(exp_time) 
			    << " in ExtGate trigger mode";
		return;
	} else if (exp_time < 0)
		THROW_HW_ERROR(InvalidValue) << "Invalid negative "
					     << DEB_VAR1(exp_time);
	double MaxExp = MaxRegVal * TimeUnitFactorMap[Milliseconds]; 
	if (exp_time > MaxExp) 
		THROW_HW_ERROR(InvalidValue) << "Exp. time too high: " 
					     << DEB_VAR2(exp_time, MaxExp);
	double MinExp = 1 * TimeUnitFactorMap[Microseconds];
	if ((exp_time > 0) && (exp_time < MinExp)) {
		DEB_WARNING() << "Rounding non-null " << DEB_VAR1(exp_time)
			      << " to " << DEB_VAR1(MinExp);
		exp_time = MinExp;
	}

	int exp_us = calcTimeUnits(exp_time, Microseconds);
	int exp_ms = calcTimeUnits(exp_time, Milliseconds);

	int exp_val;
	TimeUnitFactor time_unit;
	if ((exp_us <= MaxRegVal) && (exp_us != exp_ms * 1000)) {
		exp_val = exp_us;
		time_unit = Microseconds;
	} else {
		exp_val = exp_ms;
		time_unit = Milliseconds;
	}

	setTimeUnitFactor(time_unit);
	writeRegister(ExpTime, exp_val);
}

void Camera::getExpTime(double& exp_time)
{
	DEB_MEMBER_FUNCT();
	TimeUnitFactor time_unit_factor;
	getTimeUnitFactor(time_unit_factor);
	int exp_val;
	readRegister(ExpTime, exp_val);
	exp_time = exp_val * TimeUnitFactorMap[time_unit_factor];
	DEB_RETURN() << DEB_VAR1(exp_time);
}

void Camera::setShutMode(ShutMode shut_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(shut_mode);
	int cam_shut_mode = (shut_mode == Frelon::AutoFrame) ? 1 : 0;
	writeRegister(ShutEnable, cam_shut_mode);
}

void Camera::getShutMode(ShutMode& shut_mode)
{
	DEB_MEMBER_FUNCT();
	int cam_shut_mode;
	readRegister(ShutEnable, cam_shut_mode);
	shut_mode = (cam_shut_mode == 1) ? Frelon::AutoFrame : Frelon::Off;
	DEB_RETURN() << DEB_VAR1(shut_mode);
}

void Camera::setShutCloseTime(double shut_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(shut_time);
	TimeUnitFactor time_unit_factor;
	getTimeUnitFactor(time_unit_factor);
	int shut_val = calcTimeUnits(shut_time, time_unit_factor);
	if (shut_val > MaxRegVal)
		THROW_HW_ERROR(InvalidValue) << "Shutter close time too high: "
					     << DEB_VAR1(shut_time);
	writeRegister(ShutCloseTime, shut_val);
}

void Camera::getShutCloseTime(double& shut_time)
{
	DEB_MEMBER_FUNCT();
	TimeUnitFactor time_unit_factor;
	getTimeUnitFactor(time_unit_factor);
	int shut_val;
	readRegister(ShutCloseTime, shut_val);
	shut_time = shut_val * TimeUnitFactorMap[time_unit_factor];
	DEB_RETURN() << DEB_VAR1(shut_time);
}

void Camera::setUserLatTime(double lat_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(lat_time);
	TimeUnitFactor time_unit_factor;
	getTimeUnitFactor(time_unit_factor);
	int lat_val = calcTimeUnits(lat_time, time_unit_factor);
	writeRegister(LatencyTime, lat_val);
}

void Camera::getUserLatTime(double& lat_time)
{
	DEB_MEMBER_FUNCT();
	TimeUnitFactor time_unit_factor;
	getTimeUnitFactor(time_unit_factor);
	int lat_val;
	readRegister(LatencyTime, lat_val);
	lat_time = lat_val * TimeUnitFactorMap[time_unit_factor];
	DEB_RETURN() << DEB_VAR1(lat_time);
}

void Camera::getReadoutTime(double& readout_time)
{
	DEB_MEMBER_FUNCT();
	m_geom->getReadoutTime(readout_time);
}

void Camera::getTransferTime(double& xfer_time)
{
	DEB_MEMBER_FUNCT();
	m_geom->getTransferTime(xfer_time);
}

void Camera::getDeadTime(double& dead_time)
{
	DEB_MEMBER_FUNCT();
	m_geom->getDeadTime(dead_time);
}

void Camera::setTotalLatTime(double lat_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(lat_time);
	double dead_time;
	getDeadTime(dead_time);
	double user_lat_time = max(0.0, lat_time - dead_time);
	setUserLatTime(user_lat_time);
}

void Camera::getTotalLatTime(double& lat_time)
{
	DEB_MEMBER_FUNCT();
	double user_lat_time;
	getUserLatTime(user_lat_time);
	double dead_time;
	getDeadTime(dead_time);
	lat_time = dead_time + user_lat_time;
	DEB_RETURN() << DEB_VAR1(lat_time);
}

void Camera::setNbFrames(int nb_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(nb_frames);

	TrigMode trig_mode;
	getTrigMode(trig_mode);
	bool one_frame = ((trig_mode == IntTrigMult) ||
			  (trig_mode == ExtTrigMult));
	int cam_nb_frames = one_frame ? 1 : nb_frames;
	writeRegister(NbFrames, cam_nb_frames);
	m_nb_frames = nb_frames;
}

void Camera::getNbFrames(int& nb_frames)
{
	DEB_MEMBER_FUNCT();
	nb_frames = m_nb_frames;
	DEB_RETURN() << DEB_VAR1(nb_frames);
}

void Camera::setSPB2Config(SPB2Config spb2_config)
{
	DEB_MEMBER_FUNCT();
	m_geom->setSPB2Config(spb2_config);
}

void Camera::getSPB2Config(SPB2Config& spb2_config)
{
	DEB_MEMBER_FUNCT();
	m_geom->getSPB2Config(spb2_config);
}

string Camera::getSPB2ConfigName(SPB2Config spb2_config)
{
	DEB_MEMBER_FUNCT();
	return m_geom->getSPB2ConfigName(spb2_config);
}

void Camera::setExtSyncEnable(ExtSync ext_sync_ena)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::HTDCmd))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "HTD cmd: upgrade firmware";
	DEB_PARAM() << DEB_VAR1(ext_sync_ena);
	int hard_trig_dis = int(~ext_sync_ena) & ExtSyncBoth;
	writeRegister(HardTrigDisable, hard_trig_dis);
}

void Camera::getExtSyncEnable(ExtSync& ext_sync_ena)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::HTDCmd))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "HTD cmd: upgrade firmware";
	int hard_trig_dis;
	readRegister(HardTrigDisable, hard_trig_dis);
	ext_sync_ena = ExtSync(~hard_trig_dis & ExtSyncBoth);
	DEB_RETURN() << DEB_VAR1(ext_sync_ena);
}

void Camera::getStatus(Status& status, bool use_ser_line, bool read_spb2)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(use_ser_line, read_spb2);

	bool isSPB8 = (m_model.getSPBType() == SPBType8);
	if (isSPB8 && read_spb2) {
		DEB_TRACE() << "SPB8: Ignoring " << DEB_VAR1(read_spb2);
		read_spb2 = false;
	}

	bool has_good_htd = m_model.has(Model::GoodHTD);
	if ((use_ser_line || read_spb2) && !has_good_htd)
		THROW_HW_ERROR(NotSupported) << "SPB2/ser. line status "
			"not supported: must upgrade to good HTD firmware";

	int spb2_status = 0;
	if (read_spb2)
		readRegister(StatusAMTA, spb2_status);

	int ccd_status;
	if (use_ser_line) {
		readRegister(StatusSeqA, ccd_status);
	} else {
		Espia::Dev& dev = getEspiaDev();
		dev.getCcdStatus(ccd_status);
	}

	if (read_spb2) {
		if ((spb2_status & SPB2_TstEnvMask) != 0)
			ccd_status |= EspiaXfer;
		if ((spb2_status & SPB2_TstInitMask) != SPB2_TstInitGood)
			ccd_status |= InInit;
	}

	status = Status(ccd_status);
	DEB_RETURN() << DEB_VAR1(DEB_HEX(status));
}

bool Camera::waitStatus(Status& status, Status mask, double timeout,
			bool use_ser_line, bool read_spb2)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR5(DEB_HEX(status), DEB_HEX(mask), timeout,
				use_ser_line, read_spb2);

	Timestamp end;
	if (timeout > 0)
		end = Timestamp::now() + Timestamp(timeout);

	bool good_status = false;
	Status curr_status = Status(0xffff);
	while (!good_status) {
		if (end.isSet() && (Timestamp::now() >= end)) {
			DEB_WARNING() << "Timeout waiting for " 
				      << DEB_VAR2(DEB_HEX(status), 
						  DEB_HEX(curr_status));
			break;
		}

		getStatus(curr_status, use_ser_line, read_spb2);
		good_status = ((curr_status & mask) == status);
	}

	status = curr_status;
	DEB_RETURN() << DEB_VAR2(DEB_HEX(status), good_status);
	return good_status;
}

void Camera::getImageCount(unsigned int& img_count, bool only_lsw)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(only_lsw);

	int reg_count;
	readRegister(StatusAMTC, reg_count);
	img_count = reg_count;
	if (!only_lsw) {
		readRegister(StatusAMTD, reg_count);
		img_count |= reg_count << 16;
	}

	DEB_RETURN() << DEB_VAR1(img_count);
}

void Camera::start()
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = lock();

	TrigMode trig_mode;
	getTrigMode(trig_mode);
	if (m_started && (trig_mode != IntTrigMult))
		THROW_HW_ERROR(InvalidValue) << "Camera already running!";

	if ((trig_mode == IntTrig) || (m_trig_mode == IntTrigMult)) {
		DEB_TRACE() << "Starting camera by software";
		sendCmd(Start);
	} else if (m_model.has(Model::GoodHTD)) {
		DEB_TRACE() << "Enabling Ext. Sync. signals";
		setExtSyncEnable(ExtSyncBoth);
	}

	m_started = true;
}

void Camera::stop()
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = lock();

	if (!m_started)
		return;

	TrigMode trig_mode;
	getTrigMode(trig_mode);

	bool has_good_htd = m_model.has(Model::GoodHTD);
	if (has_good_htd && (trig_mode != IntTrig)) {
		DEB_TRACE() << "Disabling Ext. Sync. signals";
		setExtSyncEnable(ExtSyncNone);
	}

	Status status;
	getStatus(status);
	if (status == Wait) {
		m_started = false;
		return;
	}

	bool send_stop;
	if (has_good_htd) {
		send_stop = true;
	} else if (trig_mode == ExtGate) {
		send_stop = false;
	} else {
		int cam_nb_frames;
		readRegister(NbFrames, cam_nb_frames);
		send_stop = (cam_nb_frames != 1);
	}

	if (send_stop) {
		DEB_TRACE() << "Aborting current acquisition";
		sendCmd(Stop);
	}

	DEB_TRACE() << "Waiting for camera to become idle";
	Timestamp t0 = Timestamp::now();
	bool idle = waitIdleStatus(status);
	double wait_time = Timestamp::now() - t0;
	DEB_TRACE() << "Waited " << DEB_VAR2(wait_time, idle);
	if (!idle)
		DEB_WARNING() << "Camera not idle after " 
			      << DEB_VAR1(wait_time);
	m_started = false;
}

bool Camera::isRunning()
{
	DEB_MEMBER_FUNCT();
	AutoMutex l = lock();
	DEB_RETURN() << DEB_VAR1(m_started);
	return m_started;
}

double Camera::getMaxIdleWaitTime()
{
	DEB_MEMBER_FUNCT();

	double max_wait_time = MaxIdleWaitTime;
	if (!m_model.has(Model::GoodHTD)) {
		double exp_time;
		getExpTime(exp_time);
		if (exp_time > 0) {
			max_wait_time += exp_time;
			DEB_TRACE() << "Adjusted " << DEB_VAR1(max_wait_time);
		}
	}

	DEB_RETURN() << DEB_VAR1(max_wait_time);
	return max_wait_time;
}

void Camera::registerDeadTimeChangedCallback(DeadTimeChangedCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_geom->registerDeadTimeChangedCallback(cb);
}

void Camera::unregisterDeadTimeChangedCallback(DeadTimeChangedCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_geom->unregisterDeadTimeChangedCallback(cb);
}


void Camera::registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_geom->registerMaxImageSizeCallback(cb);
}

void Camera::unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_geom->unregisterMaxImageSizeCallback(cb);
}

