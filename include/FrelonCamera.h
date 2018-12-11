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
#ifndef FRELONCAMERA_H
#define FRELONCAMERA_H

#include "FrelonSerialLine.h"
#include "FrelonModel.h"
#include "FrelonGeometry.h"
#include "FrelonTimingCtrl.h"
#include "lima/HwMaxImageSizeCallback.h"

namespace lima
{

namespace Frelon
{

class Camera
{
	DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Frelon");

 public:
	Camera(Espia::SerialLine& espia_ser_line);
	~Camera();

	SerialLine& getSerialLine();

	void writeRegister(Reg reg, int  val);
	void readRegister (Reg reg, int& val);
	void readFloatRegister(Reg reg, double& val);

	void hardReset();
	void getVersionStr(std::string& ver);
	void getComplexSerialNb(int& complex_ser_nb);
	Model& getModel();

	TimingCtrl& getTimingCtrl();

	bool getDefInputChan(FrameTransferMode ftm, InputChan& input_chan);
	void setInputChan(InputChan  input_chan);
	void getInputChan(InputChan& input_chan);

	void setFrameTransferMode(FrameTransferMode  ftm);
	void getFrameTransferMode(FrameTransferMode& ftm);

	std::string getInputChanModeName(FrameTransferMode ftm, 
					 InputChan input_chan);

	void getMaxFrameDim(FrameDim& max_frame_dim);
	void getFrameDim(FrameDim& frame_dim);

	bool isChanActive(InputChan curr, InputChan chan);

	void checkFlip(Flip& flip);
	void setFlip(const Flip& flip);
	void getFlip(Flip& flip);

	void checkBin(Bin& bin);
	void setBin(const Bin& bin);
	void getBin(Bin& bin);

	void setRoiMode(RoiMode  roi_mode);
	void getRoiMode(RoiMode& roi_mode);

	void checkRoi(const Roi& set_roi, Roi& hw_roi);
	void setRoi(const Roi& set_roi);
	void getRoi(Roi& hw_roi);

	void setRoiBinOffset(const Point& roi_bin_offset);
	void getRoiBinOffset(Point& roi_bin_offset);

	void setTrigMode(TrigMode  trig_mode);
	void getTrigMode(TrigMode& trig_mode);
	
	void setExpTime(double  exp_time);
	void getExpTime(double& exp_time);

	void setShutMode(ShutMode  shut_mode);
	void getShutMode(ShutMode& shut_mode);

	void setShutCloseTime(double  shut_time);
	void getShutCloseTime(double& shut_time);

	void setUserLatTime(double  lat_time);
	void getUserLatTime(double& lat_time);
	void getReadoutTime(double& readout_time);
	void getTransferTime(double& xfer_time);
	void getDeadTime(double& dead_time);
	void setTotalLatTime(double  lat_time);
	void getTotalLatTime(double& lat_time);

	void setNbFrames(int  nb_frames);
	void getNbFrames(int& nb_frames);

	void setSPB2Config(SPB2Config  spb2_config);
	void getSPB2Config(SPB2Config& spb2_config);

        std::string getSPB2ConfigName(SPB2Config spb2_config);

	void setExtSyncEnable(ExtSync  ext_sync_ena);
	void getExtSyncEnable(ExtSync& ext_sync_ena);

	void getStatus(Status& status, bool use_ser_line=false,
		       bool read_spb2=false);
	bool waitStatus(Status& status, Status mask, double timeout,
			bool use_ser_line=false, bool read_spb2=false);

	void getImageCount(unsigned int& img_count, bool only_lsw=false);

	void start();
	void stop();
	bool isRunning();

	void registerDeadTimeChangedCallback(DeadTimeChangedCallback& cb);
	void unregisterDeadTimeChangedCallback(DeadTimeChangedCallback& cb);

	void registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb);
	void unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb);

 private:
	static const double ResetLinkWaitTime;
	static const double UpdateCcdStatusTime;
	static const double MaxIdleWaitTime;
	static const double MaxBusyRetryTime;

	Espia::Dev& getEspiaDev();

	void sync();
	void syncRegs();
	void syncRegsGoodHTD();
	void syncRegsBadHTD();

	void sendCmd(Cmd cmd);

	void setTimeUnitFactor(TimeUnitFactor  time_unit_factor);
	void getTimeUnitFactor(TimeUnitFactor& time_unit_factor);
	int  calcTimeUnits(double time_sec, TimeUnitFactor time_unit_factor);

	double getMaxIdleWaitTime();
	bool waitIdleStatus(Status& status, bool use_ser_line=false,
			    bool read_spb2=false);

	AutoMutex lock();

	SerialLine m_ser_line;
	Model m_model;
	TimingCtrl m_timing_ctrl;
	AutoPtr<Geometry> m_geom;
	TrigMode m_trig_mode;
	int m_nb_frames;
	Mutex m_lock;
	bool m_started;
};

inline AutoMutex Camera::lock()
{
	return AutoMutex(m_lock);
}

inline bool Camera::waitIdleStatus(Status& status, bool use_ser_line, 
				   bool read_spb2)
{
	status = Wait;
	return waitStatus(status, StatusMask, getMaxIdleWaitTime(),
			  use_ser_line, read_spb2);
}


} // namespace Frelon

} // namespace lima


#endif // FRELONCAMERA_H
