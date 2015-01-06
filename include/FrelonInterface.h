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
#ifndef FRELONINTERFACE_H
#define FRELONINTERFACE_H

#include "lima/HwInterface.h"
#include "EspiaBufferMgr.h"
#include "FrelonCamera.h"

namespace lima
{

namespace Frelon
{

class Interface;

/*******************************************************************
 * \class AcqEndCallback
 * \brief Class executing camera commands at the end of acq.
 *******************************************************************/

class AcqEndCallback : public Espia::AcqEndCallback
{
	DEB_CLASS_NAMESPC(DebModCamera, "AcqEndCallback", "Frelon");

 public:
	AcqEndCallback(Camera& cam);
	virtual ~AcqEndCallback();

 protected:
	virtual void acqFinished(const HwFrameInfoType& /*finfo*/);

 private:
	Camera& m_cam;
};


/*******************************************************************
 * \class EventCallback
 * \brief Bridge class transfering events from Acq -> HwEventCtrlObj
 *******************************************************************/

class EventCallback : public lima::EventCallback
{
	DEB_CLASS_NAMESPC(DebModCamera, "EventCallback", "Frelon");

 public:
	EventCallback(HwEventCtrlObj& ctrl_obj);
	virtual ~EventCallback();

 protected:
	virtual void processEvent(Event *event);

 private:
	HwEventCtrlObj& m_ctrl_obj;
};


/*******************************************************************
 * \class DetInfoCtrlObj
 * \brief Control object providing Frelon detector info interface
 *******************************************************************/

class DetInfoCtrlObj : public HwDetInfoCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "DetInfoCtrlObj", "Frelon");

 public:
	DetInfoCtrlObj(Camera& cam);
	virtual ~DetInfoCtrlObj();

	virtual void getMaxImageSize(Size& max_image_size);
	virtual void getDetectorImageSize(Size& det_image_size);

	virtual void getDefImageType(ImageType& def_image_type);
	virtual void getCurrImageType(ImageType& curr_image_type);
	virtual void setCurrImageType(ImageType  curr_image_type);

	virtual void getPixelSize(double& x_size, double& y_size);
	virtual void getDetectorType(std::string& det_type);
	virtual void getDetectorModel(std::string& det_model);

	virtual void registerMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb);
	virtual void unregisterMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb);

 private:
	Camera& m_cam;
};


/*******************************************************************
 * \class BufferCtrlObj
 * \brief Control object providing Frelon buffering interface
 *******************************************************************/

class BufferCtrlObj : public HwBufferCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "BufferCtrlObj", "Frelon");

 public:
	BufferCtrlObj(BufferCtrlMgr& buffer_mgr);
	virtual ~BufferCtrlObj();

	virtual void setFrameDim(const FrameDim& frame_dim);
	virtual void getFrameDim(      FrameDim& frame_dim);

	virtual void setNbBuffers(int  nb_buffers);
	virtual void getNbBuffers(int& nb_buffers);

	virtual void setNbConcatFrames(int  nb_concat_frames);
	virtual void getNbConcatFrames(int& nb_concat_frames);

	virtual void getMaxNbBuffers(int& max_nb_buffers);

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb = 0);
	virtual void *getFramePtr(int acq_frame_nb);

	virtual void getStartTimestamp(Timestamp& start_ts);
	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info);

	virtual void   registerFrameCallback(HwFrameCallback& frame_cb);
	virtual void unregisterFrameCallback(HwFrameCallback& frame_cb);

 private:
	BufferCtrlMgr& m_buffer_mgr;
};


/*******************************************************************
 * \class SyncCtrlObj
 * \brief Control object providing Frelon synchronization interface
 *******************************************************************/

class SyncCtrlObj : public HwSyncCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "SyncCtrlObj", "Frelon");

 public:
	SyncCtrlObj(Espia::Acq& acq, Camera& cam);
	virtual ~SyncCtrlObj();

	virtual bool checkTrigMode(TrigMode trig_mode);
	virtual void setTrigMode(TrigMode  trig_mode);
	virtual void getTrigMode(TrigMode& trig_mode);

	virtual void setExpTime(double  exp_time);
	virtual void getExpTime(double& exp_time);

	virtual void setLatTime(double  lat_time);
	virtual void getLatTime(double& lat_time);

	virtual void setNbHwFrames(int  nb_frames);
	virtual void getNbHwFrames(int& nb_frames);

	virtual void getValidRanges(ValidRangesType& valid_ranges);

 private:
	Espia::Acq& m_acq;
	Camera& m_cam;
};


/*******************************************************************
 * \class BinCtrlObj
 * \brief Control object providing Frelon binning interface
 *******************************************************************/

class BinCtrlObj;

class BinChangedCallback 
{
	DEB_CLASS_NAMESPC(DebModCamera, "BinChangedCallback", "Frelon");

 public:
	BinChangedCallback();
	virtual ~BinChangedCallback();

 protected:
	virtual void hwBinChanged(const Bin& hw_bin) = 0;
	
 private:
	friend class BinCtrlObj;
	BinCtrlObj *m_bin_ctrl_obj;
};


class BinCtrlObj : public HwBinCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "BinCtrlObj", "Frelon");

 public:
	BinCtrlObj(Camera& cam);
	virtual ~BinCtrlObj();

	virtual void setBin(const Bin& bin);
	virtual void getBin(Bin& bin);
	virtual void checkBin(Bin& bin);

	void registerBinChangedCallback  (BinChangedCallback& bin_chg_cb);
	void unregisterBinChangedCallback(BinChangedCallback& bin_chg_cb);

 private:
	Camera& m_cam;
	BinChangedCallback *m_bin_chg_cb;
};


/*******************************************************************
 * \class RoiCtrlObj
 * \brief Control object providing Frelon Roi interface
 *******************************************************************/

class RoiCtrlObj;

class RoiChangedCallback 
{
	DEB_CLASS_NAMESPC(DebModCamera, "RoiChangedCallback", "Frelon");

 public:
	RoiChangedCallback();
	virtual ~RoiChangedCallback();

 protected:
	virtual void hwRoiChanged(const Roi& hw_roi) = 0;

 private:
	friend class RoiCtrlObj;
	RoiCtrlObj *m_roi_ctrl_obj;
};


class RoiCtrlObj : public HwRoiCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "RoiCtrlObj", "Frelon");

 public:
	RoiCtrlObj(Espia::Acq& acq, Camera& cam);
	virtual ~RoiCtrlObj();

	virtual void setRoi(const Roi& set_roi);
	virtual void getRoi(Roi& hw_roi);
	virtual void checkRoi(const Roi& set_roi, Roi& hw_roi);

	void registerRoiChangedCallback  (RoiChangedCallback& roi_chg_cb);
	void unregisterRoiChangedCallback(RoiChangedCallback& roi_chg_cb);

 private:
	void checkEspiaRoi(const Roi& set_roi, Roi& hw_roi, 
			   Size& det_frame_size, Roi& espia_roi);

	Espia::Acq& m_acq;
	Camera& m_cam;
	RoiChangedCallback *m_roi_chg_cb;
};


/*******************************************************************
 * \class FlipCtrlObj
 * \brief Control object providing Frelon flip interface
 *******************************************************************/

class FlipCtrlObj : public HwFlipCtrlObj
{
	DEB_CLASS_NAMESPC(DebModCamera, "FlipCtrlObj", "Frelon");

 public:
	FlipCtrlObj(Camera& cam);
	virtual ~FlipCtrlObj();

	virtual void setFlip(const Flip& flip);
	virtual void getFlip(Flip& flip);
	virtual void checkFlip(Flip& flip);

 private:
	Camera& m_cam;
};


/*******************************************************************
 * \class ShutterCtrlObj
 * \brief Control object providing Frelon shutter interface
 *******************************************************************/

class ShutterCtrlObj : public HwShutterCtrlObj
{
	DEB_CLASS(DebModCamera, "ShutterCtrlObj");

public:
	ShutterCtrlObj(Camera& cam);
	virtual ~ShutterCtrlObj();

	virtual bool checkMode(ShutterMode shut_mode) const;
	virtual void getModeList(ShutterModeList&  mode_list) const;
	virtual void setMode(ShutterMode  shut_mode);
	virtual void getMode(ShutterMode& shut_mode) const;

	virtual void setState(bool  shut_open);
	virtual void getState(bool& shut_open) const;

	virtual void setOpenTime (double  shut_open_time);
	virtual void getOpenTime (double& shut_open_time) const;
	virtual void setCloseTime(double  shut_close_time);
	virtual void getCloseTime(double& shut_close_time) const;

 private:
	Camera& m_cam;
};

/*******************************************************************
 * \class EventCtrlObj
 * \brief Control object providing Frelon event interface
 *******************************************************************/

class EventCtrlObj : public HwEventCtrlObj
{
	DEB_CLASS(DebModCamera, "EventCtrlObj");

public:
	EventCtrlObj();
	virtual ~EventCtrlObj();
};

/*******************************************************************
 * \class Interface
 * \brief Frelon hardware interface
 *******************************************************************/

class Interface : public HwInterface
{
	DEB_CLASS_NAMESPC(DebModCamera, "Interface", "Frelon");

 public:
	Interface(Espia::Acq& acq, BufferCtrlMgr& buffer_mgr, Camera& cam);
	virtual ~Interface();

	virtual void getCapList(CapList&) const;

	virtual void reset(ResetLevel reset_level);
	virtual void prepareAcq();
	virtual void startAcq();
	virtual void stopAcq();
	virtual void getStatus(StatusType& status);
	virtual int getNbHwAcquiredFrames();

	void resetDefaults();

 private:
	Espia::Acq&    m_acq;
	BufferCtrlMgr& m_buffer_mgr;
	Camera&        m_cam;

	CapList m_cap_list;
	DetInfoCtrlObj m_det_info;
	BufferCtrlObj  m_buffer;
	SyncCtrlObj    m_sync;
	BinCtrlObj     m_bin;
	RoiCtrlObj     m_roi;
	FlipCtrlObj    m_flip;
	ShutterCtrlObj m_shutter;
	EventCtrlObj   m_event;

	Frelon::AcqEndCallback m_acq_end_cb;
	Frelon::EventCallback  m_event_cb;
};




} // namespace Frelon

} // namespace lima

#endif // FRELONINTERFACE_H
