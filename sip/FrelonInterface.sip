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

namespace Frelon
{


//class Interface;


class AcqEndCallback : Espia::AcqEndCallback
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	AcqEndCallback(Frelon::Camera& cam);
	virtual ~AcqEndCallback();

 protected:
	virtual void acqFinished(const HwFrameInfoType& /*finfo*/);

 private:
	AcqEndCallback(Frelon::AcqEndCallback&);
};


class EventCallback : EventCallback
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	EventCallback(HwEventCtrlObj& ctrl_obj);
	virtual ~EventCallback();

 protected:
	virtual void processEvent(Event *event);

 private:
	EventCallback(const Frelon::EventCallback&);
};


class DetInfoCtrlObj : HwDetInfoCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	DetInfoCtrlObj(Frelon::Camera& cam);
	virtual ~DetInfoCtrlObj();

	virtual void getMaxImageSize(Size& max_image_size /Out/);
	virtual void getDetectorImageSize(Size& det_image_size /Out/);

	virtual void getDefImageType(ImageType& def_image_type /Out/);
	virtual void getCurrImageType(ImageType& curr_image_type /Out/);
	virtual void setCurrImageType(ImageType  curr_image_type);

	virtual void getPixelSize(double& x_size /Out/, double& y_size /Out/);
	virtual void getDetectorType(std::string& det_type /Out/);
	virtual void getDetectorModel(std::string& det_model /Out/);

	virtual void registerMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb);
	virtual void unregisterMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb);

 private:
	DetInfoCtrlObj(const Frelon::DetInfoCtrlObj&);
};


class BufferCtrlObj : HwBufferCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	BufferCtrlObj(BufferCtrlMgr& buffer_mgr);
	virtual ~BufferCtrlObj();

	virtual void setFrameDim(const FrameDim& frame_dim);
	virtual void getFrameDim(      FrameDim& frame_dim /Out/);

	virtual void setNbBuffers(int  nb_buffers);
	virtual void getNbBuffers(int& nb_buffers /Out/);

	virtual void setNbConcatFrames(int  nb_concat_frames);
	virtual void getNbConcatFrames(int& nb_concat_frames /Out/);

	virtual void getMaxNbBuffers(int& max_nb_buffers /Out/);

	virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb = 0);
	virtual void *getFramePtr(int acq_frame_nb);

	virtual void getStartTimestamp(Timestamp& start_ts /Out/);
	virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info /Out/);

	virtual void   registerFrameCallback(HwFrameCallback& frame_cb);
	virtual void unregisterFrameCallback(HwFrameCallback& frame_cb);

 private:
	BufferCtrlObj(const Frelon::BufferCtrlObj&);
};


class SyncCtrlObj : HwSyncCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	SyncCtrlObj(Espia::Acq& acq, Frelon::Camera& cam);
	virtual ~SyncCtrlObj();

	virtual bool checkTrigMode(TrigMode trig_mode);
	virtual void setTrigMode(TrigMode  trig_mode);
	virtual void getTrigMode(TrigMode& trig_mode /Out/);

	virtual void setExpTime(double  exp_time);
	virtual void getExpTime(double& exp_time /Out/);

	virtual void setLatTime(double  lat_time);
	virtual void getLatTime(double& lat_time /Out/);

	virtual void setNbHwFrames(int  nb_frames);
	virtual void getNbHwFrames(int& nb_frames /Out/);

	virtual void getValidRanges(HwSyncCtrlObj::ValidRangesType& valid_ranges /Out/);

 private:
	SyncCtrlObj(const Frelon::SyncCtrlObj&);
};


class BinChangedCallback 
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	BinChangedCallback();
	virtual ~BinChangedCallback();

 protected:
	virtual void hwBinChanged(const Bin& hw_bin) = 0;

 private:
	BinChangedCallback(const Frelon::BinChangedCallback&);
};


class BinCtrlObj : HwBinCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	BinCtrlObj(Espia::Acq& acq, Frelon::Camera& cam);
	virtual ~BinCtrlObj();

	virtual void setBin(const Bin& bin);
	virtual void getBin(Bin& bin /Out/);
	virtual void checkBin(Bin& bin /In,Out/);

	void registerBinChangedCallback  (Frelon::BinChangedCallback& chg_cb);
	void unregisterBinChangedCallback(Frelon::BinChangedCallback& chg_cb);

 private:
	BinCtrlObj(const Frelon::BinCtrlObj&);
};


class RoiChangedCallback 
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	RoiChangedCallback();
	virtual ~RoiChangedCallback();

 protected:
	virtual void hwRoiChanged(const Roi& hw_roi) = 0;

 private:
	RoiChangedCallback(const Frelon::RoiChangedCallback&);
};


class RoiCtrlObj : HwRoiCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	RoiCtrlObj(Espia::Acq& acq, Frelon::Camera& cam);
	virtual ~RoiCtrlObj();

	virtual void checkRoi(const Roi& set_roi, Roi& hw_roi /Out/);
	virtual void setRoi(const Roi& set_roi);
	virtual void getRoi(Roi& hw_roi /Out/);

	void registerRoiChangedCallback  (Frelon::RoiChangedCallback& chg_cb);
	void unregisterRoiChangedCallback(Frelon::RoiChangedCallback& chg_cb);

 private:
	RoiCtrlObj(const Frelon::RoiCtrlObj&);

};


class FlipCtrlObj : HwFlipCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	FlipCtrlObj(Espia::Acq& acq, Frelon::Camera& cam);
	virtual ~FlipCtrlObj();

	virtual void setFlip(const Flip& flip);
	virtual void getFlip(Flip& flip /Out/);

	virtual void checkFlip(Flip& flip /In,Out/);

 private:
	FlipCtrlObj(const Frelon::FlipCtrlObj&);
};


class ShutterCtrlObj : HwShutterCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	ShutterCtrlObj(Frelon::Camera& cam);
	virtual ~ShutterCtrlObj();

	virtual bool checkMode(ShutterMode shut_mode) const;
	virtual void getModeList(ShutterModeList&  mode_list /Out/) const;
	virtual void setMode(ShutterMode  shut_mode);
	virtual void getMode(ShutterMode& shut_mode /Out/) const;

	virtual void setState(bool  shut_open);
	virtual void getState(bool& shut_open /Out/) const;

	virtual void setOpenTime (double  shut_open_time);
	virtual void getOpenTime (double& shut_open_time /Out/) const;
	virtual void setCloseTime(double  shut_close_time);
	virtual void getCloseTime(double& shut_close_time /Out/) const;

 private:
	ShutterCtrlObj(const Frelon::ShutterCtrlObj&);
};


class EventCtrlObj : HwEventCtrlObj
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>
%End

 public:
	EventCtrlObj();
	virtual ~EventCtrlObj();
	
 private:
	EventCtrlObj(const Frelon::EventCtrlObj&);
};


class Interface : HwInterface
{

%TypeHeaderCode
#include "FrelonInterface.h"
#include <algorithm>

#define LIMA_HWCAP_TO_SIP_FRELON_CTRLOBJ_CASE(cppHwCap, aCap)		\
	LIMA_HWCAP_TO_SIP_CTRLOBJ_CASE(cppHwCap, aCap, Frelon::aCap##CtrlObj)
%End

 public:
	Interface(Espia::Acq& acq /KeepReference/,
	          BufferCtrlMgr& buffer_mgr /KeepReference/, 
		  Frelon::Camera& cam /KeepReference/);
	virtual ~Interface();

//	virtual const HwInterface::CapList& getCapList() const;
	virtual void getCapList(std::vector<HwCap> &cap_list /Out/) const;

	virtual void reset(HwInterface::ResetLevel reset_level);
	virtual void prepareAcq();
	virtual void startAcq();
	virtual void stopAcq();
	virtual void getStatus(HwInterface::StatusType& status /Out/);
	virtual int getNbHwAcquiredFrames();

	void resetDefaults();

	SIP_PYOBJECT getHwCtrlObj(HwCap::Type cap_type);
%MethodCode
	HwInterface::CapList cap_list;
	sipCpp->getCapList(cap_list);

	typedef HwInterface::CapList::const_iterator It;
	for (It i = cap_list.begin(); i != cap_list.end(); ++i)
		if (i->getType() == a0) {
			LIMA_CONVERT_HWCAP_TO_SIP_CTRLOBJ(i, FRELON);
			break;
		}
%End

 private:
	Interface(const Frelon::Interface&);
};


}; // namespace Frelon
