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
#include "FrelonInterface.h"
#include <algorithm>

using namespace lima;
using namespace lima::Espia;
using namespace lima::Frelon;
using namespace std;

/*******************************************************************
 * \brief AcqEndCallback constructor
 *******************************************************************/

Frelon::AcqEndCallback::AcqEndCallback(Camera& cam) 
	: m_cam(cam)
{
	DEB_CONSTRUCTOR();
}

Frelon::AcqEndCallback::~AcqEndCallback()
{
	DEB_DESTRUCTOR();
}

void Frelon::AcqEndCallback::acqFinished(const HwFrameInfoType& /*finfo*/)
{
	DEB_MEMBER_FUNCT();
	m_cam.stop();
}


/*******************************************************************
 * \brief EventCallback constructor
 *******************************************************************/

Frelon::EventCallback::EventCallback(HwEventCtrlObj& ctrl_obj) 
	: m_ctrl_obj(ctrl_obj)
{
	DEB_CONSTRUCTOR();
}

Frelon::EventCallback::~EventCallback()
{
	DEB_DESTRUCTOR();
}

void Frelon::EventCallback::processEvent(Event *event)
{
	DEB_MEMBER_FUNCT();
	m_ctrl_obj.reportEvent(event);
}


/*******************************************************************
 * \brief DetInfoCtrlObj constructor
 *******************************************************************/

DetInfoCtrlObj::DetInfoCtrlObj(Camera& cam)
	: m_cam(cam)
{
	DEB_CONSTRUCTOR();
}

DetInfoCtrlObj::~DetInfoCtrlObj()
{
	DEB_DESTRUCTOR();
}

void DetInfoCtrlObj::getMaxImageSize(Size& max_image_size)
{
	DEB_MEMBER_FUNCT();
	FrameDim max_frame_dim;
	m_cam.getFrameDim(max_frame_dim);
	max_image_size = max_frame_dim.getSize();
}

void DetInfoCtrlObj::getDetectorImageSize(Size& det_image_size)
{
	DEB_MEMBER_FUNCT();
	FrameDim max_frame_dim;
	m_cam.getMaxFrameDim(max_frame_dim);
	det_image_size = max_frame_dim.getSize();
}

void DetInfoCtrlObj::getDefImageType(ImageType& def_image_type)
{
	DEB_MEMBER_FUNCT();
	FrameDim max_frame_dim;
	m_cam.getMaxFrameDim(max_frame_dim);
	def_image_type = max_frame_dim.getImageType();
}

void DetInfoCtrlObj::getCurrImageType(ImageType& curr_image_type)
{
	DEB_MEMBER_FUNCT();
	FrameDim max_frame_dim;
	m_cam.getFrameDim(max_frame_dim);
	curr_image_type = max_frame_dim.getImageType();
}

void DetInfoCtrlObj::setCurrImageType(ImageType curr_image_type)
{
	DEB_MEMBER_FUNCT();
	ImageType unique_image_type;
	getCurrImageType(unique_image_type);
	if (curr_image_type != unique_image_type)
		THROW_HW_ERROR(InvalidValue) 
			<< "Only " << DEB_VAR1(unique_image_type) << "allowed";
}

void DetInfoCtrlObj::getPixelSize(double& x_size, double& y_size)
{
	DEB_MEMBER_FUNCT();
	Model& model = m_cam.getModel();
	x_size = y_size = model.getPixelSize();
	DEB_RETURN() << DEB_VAR2(x_size, y_size);
}

void DetInfoCtrlObj::getDetectorType(std::string& det_type)
{
	DEB_MEMBER_FUNCT();
	det_type = "Frelon";
	DEB_RETURN() << DEB_VAR1(det_type);
}

void DetInfoCtrlObj::getDetectorModel(std::string& det_model)
{
	DEB_MEMBER_FUNCT();
	Model& model = m_cam.getModel();
	det_model = model.getName();
	DEB_RETURN() << DEB_VAR1(det_model);
}

void DetInfoCtrlObj::registerMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_cam.registerMaxImageSizeCallback(cb);
}

void DetInfoCtrlObj::unregisterMaxImageSizeCallback(
					HwMaxImageSizeCallback& cb)
{
	DEB_MEMBER_FUNCT();
	m_cam.unregisterMaxImageSizeCallback(cb);
}


/*******************************************************************
 * \brief BufferCtrlObj constructor
 *******************************************************************/

BufferCtrlObj::BufferCtrlObj(BufferCtrlMgr& buffer_mgr)
	: m_buffer_mgr(buffer_mgr)
{
	DEB_CONSTRUCTOR();
}

BufferCtrlObj::~BufferCtrlObj()
{
	DEB_DESTRUCTOR();
}

void BufferCtrlObj::setFrameDim(const FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.setFrameDim(frame_dim);
}

void BufferCtrlObj::getFrameDim(FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getFrameDim(frame_dim);
}

void BufferCtrlObj::setNbBuffers(int nb_buffers)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.setNbBuffers(nb_buffers);
}

void BufferCtrlObj::getNbBuffers(int& nb_buffers)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getNbBuffers(nb_buffers);
}

void BufferCtrlObj::setNbConcatFrames(int nb_concat_frames)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.setNbConcatFrames(nb_concat_frames);
}

void BufferCtrlObj::getNbConcatFrames(int& nb_concat_frames)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getNbConcatFrames(nb_concat_frames);
}

void BufferCtrlObj::getMaxNbBuffers(int& max_nb_buffers)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getMaxNbBuffers(max_nb_buffers);

}

void *BufferCtrlObj::getBufferPtr(int buffer_nb, int concat_frame_nb)
{
	DEB_MEMBER_FUNCT();
	return m_buffer_mgr.getBufferPtr(buffer_nb, concat_frame_nb);
}

void *BufferCtrlObj::getFramePtr(int acq_frame_nb)
{
	DEB_MEMBER_FUNCT();
	return m_buffer_mgr.getFramePtr(acq_frame_nb);
}

void BufferCtrlObj::getStartTimestamp(Timestamp& start_ts)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getStartTimestamp(start_ts);
}

void BufferCtrlObj::getFrameInfo(int acq_frame_nb, HwFrameInfoType& info)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.getFrameInfo(acq_frame_nb, info);
}

void BufferCtrlObj::registerFrameCallback(HwFrameCallback& frame_cb)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.registerFrameCallback(frame_cb);
}

void BufferCtrlObj::unregisterFrameCallback(HwFrameCallback& frame_cb)
{
	DEB_MEMBER_FUNCT();
	m_buffer_mgr.unregisterFrameCallback(frame_cb);
}


/*******************************************************************
 * \brief SyncCtrlObj constructor
 *******************************************************************/

SyncCtrlObj::DeadTimeChangedCallback::DeadTimeChangedCallback(SyncCtrlObj *sync)
	: m_sync(sync)
{
	DEB_CONSTRUCTOR();
}

void SyncCtrlObj::DeadTimeChangedCallback::deadTimeChanged(double dead_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(dead_time);

	if (!m_sync)
		return;

	ValidRangesType valid_ranges;
	m_sync->getValidRanges(valid_ranges);
	m_sync->validRangesChanged(valid_ranges);
}
	
SyncCtrlObj::SyncCtrlObj(Acq& acq, Camera& cam)
	: HwSyncCtrlObj(), m_acq(acq), m_cam(cam), m_dead_time_cb(this)
{
	DEB_CONSTRUCTOR();
	m_cam.registerDeadTimeChangedCallback(m_dead_time_cb);
}

SyncCtrlObj::~SyncCtrlObj()
{
	DEB_DESTRUCTOR();
	m_dead_time_cb.m_sync = NULL;
}

bool SyncCtrlObj::checkTrigMode(TrigMode trig_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);

	bool valid_mode;
	switch (trig_mode) {
	case IntTrig:
	case IntTrigMult:
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

void SyncCtrlObj::setTrigMode(TrigMode trig_mode)
{
	DEB_MEMBER_FUNCT();

	if (!checkTrigMode(trig_mode))
		THROW_HW_ERROR(InvalidValue) << "Invalid " 
					     << DEB_VAR1(trig_mode);
	m_cam.setTrigMode(trig_mode);
}

void SyncCtrlObj::getTrigMode(TrigMode& trig_mode)
{
	DEB_MEMBER_FUNCT();
	m_cam.getTrigMode(trig_mode);
}

void SyncCtrlObj::setExpTime(double exp_time)
{
	DEB_MEMBER_FUNCT();
	m_cam.setExpTime(exp_time);
}

void SyncCtrlObj::getExpTime(double& exp_time)
{
	DEB_MEMBER_FUNCT();
	m_cam.getExpTime(exp_time);
}

void SyncCtrlObj::setLatTime(double lat_time)
{
	DEB_MEMBER_FUNCT();
	m_cam.setTotalLatTime(lat_time);
}

void SyncCtrlObj::getLatTime(double& lat_time)
{
	DEB_MEMBER_FUNCT();
	m_cam.getTotalLatTime(lat_time);
}

void SyncCtrlObj::setNbHwFrames(int nb_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(nb_frames);

	m_acq.setNbFrames(nb_frames);

	int cam_nb_frames = nb_frames;
	if (cam_nb_frames > MaxRegVal) {
		DEB_TRACE() << "Too many frames: setting camera endless acq";
		cam_nb_frames = 0;
	}

	m_cam.setNbFrames(cam_nb_frames);
}

void SyncCtrlObj::getNbHwFrames(int& nb_frames)
{
	DEB_MEMBER_FUNCT();
	m_acq.getNbFrames(nb_frames);
}

void SyncCtrlObj::getValidRanges(ValidRangesType& valid_ranges)
{
	DEB_MEMBER_FUNCT();

	const double MinTimeUnit = TimeUnitFactorMap[Microseconds];
	const double MaxTimeUnit = TimeUnitFactorMap[Milliseconds];
	const double LatTimeUnit = TimeUnitFactorMap[Milliseconds];

	valid_ranges.min_exp_time = 1 * MinTimeUnit;
	valid_ranges.max_exp_time = MaxRegVal * MaxTimeUnit;

	double dead_time;
	m_cam.getDeadTime(dead_time);
	valid_ranges.min_lat_time = dead_time;
	valid_ranges.max_lat_time = dead_time + MaxRegVal * LatTimeUnit;

	DEB_RETURN() << DEB_VAR2(valid_ranges.min_exp_time, 
				 valid_ranges.max_exp_time);
	DEB_RETURN() << DEB_VAR2(valid_ranges.min_lat_time, 
				 valid_ranges.max_lat_time);
}


/*******************************************************************
 * \brief BinCtrlObj constructor
 *******************************************************************/

BinChangedCallback::BinChangedCallback()
	: m_bin_ctrl_obj(NULL)
{
	DEB_CONSTRUCTOR();
}

BinChangedCallback::~BinChangedCallback()
{
	DEB_DESTRUCTOR();
	if (m_bin_ctrl_obj != NULL)
		m_bin_ctrl_obj->unregisterBinChangedCallback(*this);
}

BinCtrlObj::BinCtrlObj(Espia::Acq& acq, Camera& cam)
	: m_acq(acq), m_cam(cam), m_bin_chg_cb(NULL)
{
	DEB_CONSTRUCTOR();
}

BinCtrlObj::~BinCtrlObj()
{
	DEB_DESTRUCTOR();
	if (m_bin_chg_cb)
		m_bin_chg_cb->m_bin_ctrl_obj = NULL;
}

void BinCtrlObj::setBin(const Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_cam.setBin(bin);

	Espia::SGImgConfig img_config;
	Size prev_size;
	m_acq.getSGImgConfig(img_config, prev_size);
	FrameDim det_frame_dim;
	m_cam.getFrameDim(det_frame_dim);
	Size new_size = det_frame_dim.getSize() / bin;
	if (new_size != prev_size)
		m_acq.setSGImgConfig(img_config, new_size);

	if (m_bin_chg_cb) {
		DEB_TRACE() << "Firing change callback";
		Bin hw_bin;
		getBin(hw_bin);
		m_bin_chg_cb->hwBinChanged(hw_bin);
	}
}

void BinCtrlObj::getBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_cam.getBin(bin);
}

void BinCtrlObj::checkBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	m_cam.checkBin(bin);
}

void BinCtrlObj::registerBinChangedCallback(BinChangedCallback& bin_chg_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&bin_chg_cb, m_bin_chg_cb);

	if (m_bin_chg_cb != NULL)
		THROW_HW_ERROR(InvalidValue) << "a cb is already registered";

	m_bin_chg_cb = &bin_chg_cb;
	bin_chg_cb.m_bin_ctrl_obj = this;

	DEB_TRACE() << "Firing first callback for update";
	Bin hw_bin;
	getBin(hw_bin);
	m_bin_chg_cb->hwBinChanged(hw_bin);
}

void BinCtrlObj::unregisterBinChangedCallback(BinChangedCallback& bin_chg_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&bin_chg_cb, m_bin_chg_cb);

	if (&bin_chg_cb != m_bin_chg_cb)
		THROW_HW_ERROR(InvalidValue) << "cb is not registered";

	m_bin_chg_cb = NULL;
	bin_chg_cb.m_bin_ctrl_obj = NULL;
}


/*******************************************************************
 * \brief RoiCtrlObj constructor
 *******************************************************************/

RoiChangedCallback::RoiChangedCallback()
	: m_roi_ctrl_obj(NULL)
{
	DEB_CONSTRUCTOR();
}

RoiChangedCallback::~RoiChangedCallback()
{
	DEB_DESTRUCTOR();

	if (m_roi_ctrl_obj != NULL)
		m_roi_ctrl_obj->unregisterRoiChangedCallback(*this);
}

RoiCtrlObj::RoiCtrlObj(Espia::Acq& acq, Camera& cam)
	: m_acq(acq), m_cam(cam), m_roi_chg_cb(NULL)
{
	DEB_CONSTRUCTOR();
}

RoiCtrlObj::~RoiCtrlObj()
{
	DEB_DESTRUCTOR();

	if (m_roi_chg_cb)
		m_roi_chg_cb->m_roi_ctrl_obj = NULL;
}

void RoiCtrlObj::checkRoi(const Roi& set_roi, Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	m_cam.checkRoi(set_roi, hw_roi);

	Size det_frame_size;
	Roi espia_roi;
	checkEspiaRoi(set_roi, hw_roi, det_frame_size, espia_roi);
}

void RoiCtrlObj::setRoi(const Roi& set_roi)
{
	DEB_MEMBER_FUNCT();
	m_cam.setRoi(set_roi);

	Roi hw_roi, espia_roi;
	m_cam.getRoi(hw_roi);
	Size det_frame_size;
	checkEspiaRoi(set_roi, hw_roi, det_frame_size, espia_roi);
	m_acq.setSGRoi(det_frame_size, espia_roi);

	if (m_roi_chg_cb) {
		DEB_TRACE() << "Firing change callback";
		m_roi_chg_cb->hwRoiChanged(hw_roi);
	}
}

void RoiCtrlObj::getRoi(Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	m_cam.getRoi(hw_roi);

	Size det_frame_size;
	Roi espia_roi;
	m_acq.getSGRoi(det_frame_size, espia_roi);
	if (!espia_roi.isEmpty()) {
		if (det_frame_size != hw_roi.getSize())
			THROW_HW_ERROR(Error) << "Camera/Espia roi mismatch: "
					      << DEB_VAR2(det_frame_size, 
							  hw_roi);
		Roi final_hw_roi = hw_roi.subRoiRel2Abs(espia_roi);
		hw_roi = final_hw_roi;
	}

	DEB_RETURN() << DEB_VAR1(hw_roi);
}
 
void RoiCtrlObj::checkEspiaRoi(const Roi& set_roi, Roi& hw_roi, 
			       Size& det_frame_size, Roi& espia_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(set_roi, hw_roi);

	det_frame_size = hw_roi.getSize();

	bool sg_roi = (!set_roi.isEmpty() && (hw_roi != set_roi));
	if (sg_roi) {
		espia_roi = hw_roi.subRoiAbs2Rel(set_roi);
		int width = set_roi.getSize().getWidth();
		bool horz_match = ((espia_roi.getTopLeft().x == 0) && 
				   (espia_roi.getSize().getWidth() == width));
		sg_roi = (horz_match && ((width % 4) == 0));
	}

	if (sg_roi) {
		DEB_TRACE() << "Horz. aligned roi qualifies for Espia SG!";
		hw_roi = set_roi;
	} else
		espia_roi.reset();

	DEB_RETURN() << DEB_VAR3(hw_roi, det_frame_size, espia_roi);
}

void RoiCtrlObj::registerRoiChangedCallback(RoiChangedCallback& roi_chg_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&roi_chg_cb, m_roi_chg_cb);

	if (m_roi_chg_cb != NULL)
		THROW_HW_ERROR(InvalidValue) << "a cb is already registered";

	m_roi_chg_cb = &roi_chg_cb;
	roi_chg_cb.m_roi_ctrl_obj = this;

	DEB_TRACE() << "Firing first callback for update";
	Roi hw_roi;
	getRoi(hw_roi);
	m_roi_chg_cb->hwRoiChanged(hw_roi);
}

void RoiCtrlObj::unregisterRoiChangedCallback(RoiChangedCallback& roi_chg_cb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(&roi_chg_cb, m_roi_chg_cb);

	if (&roi_chg_cb != m_roi_chg_cb)
		THROW_HW_ERROR(InvalidValue) << "cb is not registered";

	m_roi_chg_cb = NULL;
	roi_chg_cb.m_roi_ctrl_obj = NULL;
}


/*******************************************************************
 * \brief FlipCtrlObj constructor
 *******************************************************************/

FlipCtrlObj::FlipCtrlObj(Espia::Acq& acq, Camera& cam)
	: m_acq(acq), m_cam(cam)
{
	DEB_CONSTRUCTOR();
}

FlipCtrlObj::~FlipCtrlObj()
{
	DEB_DESTRUCTOR();
}

void FlipCtrlObj::setFlip(const Flip& flip)
{
	DEB_MEMBER_FUNCT();

	m_cam.setFlip(flip);
	if (!m_cam.getModel().isFrelon16Dual())
		return;

	Espia::SGImgConfig img_config = (flip.y ? Espia::SGImgConcatVertInv2 :
						  Espia::SGImgConcatVert2);
	FrameDim det_frame_dim;
	m_cam.getFrameDim(det_frame_dim);
	Bin bin;
	m_cam.getBin(bin);
	det_frame_dim /= bin;
	m_acq.setSGImgConfig(img_config, det_frame_dim.getSize());
}

void FlipCtrlObj::getFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();
	m_cam.getFlip(flip);
}

void FlipCtrlObj::checkFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();
	m_cam.checkFlip(flip);
}


/*******************************************************************
 * \brief ShutterCtrlObj constructor
 *******************************************************************/

ShutterCtrlObj::ShutterCtrlObj(Camera& cam)
	: m_cam(cam)
{
	DEB_CONSTRUCTOR();
}

ShutterCtrlObj::~ShutterCtrlObj()
{
	DEB_DESTRUCTOR();
}

bool ShutterCtrlObj::checkMode(ShutterMode shut_mode) const
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(shut_mode);

	bool valid_mode;
	switch (shut_mode) {
	case ShutterManual:
	case ShutterAutoFrame:
		valid_mode = true;
		break;
	default:
		valid_mode = false;
	}

	DEB_RETURN() << DEB_VAR1(valid_mode);
	return valid_mode;
}

void ShutterCtrlObj::getModeList(ShutterModeList& mode_list) const
{
	DEB_MEMBER_FUNCT();
	mode_list.push_back(lima::ShutterManual);
	mode_list.push_back(lima::ShutterAutoFrame);	
}

void ShutterCtrlObj::setMode(ShutterMode shut_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(shut_mode);

	if (!checkMode(shut_mode))
		THROW_HW_ERROR(NotSupported) << "Invalid (not supported) " 
					     << DEB_VAR1(shut_mode);

	ShutMode cam_mode;
	cam_mode = (shut_mode == ShutterAutoFrame) ? Frelon::AutoFrame : 
	  					     Frelon::Off;
	m_cam.setShutMode(cam_mode);
}

void ShutterCtrlObj::getMode(ShutterMode& shut_mode) const
{
	DEB_MEMBER_FUNCT();

	ShutMode cam_mode;
	m_cam.getShutMode(cam_mode);
	shut_mode = (cam_mode == Frelon::AutoFrame) ? ShutterAutoFrame : 
	  					      ShutterManual;
	DEB_RETURN() << DEB_VAR1(shut_mode);
}

void ShutterCtrlObj::setState(bool open)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(open);

	ShutterMode mode;
	getMode(mode);
	if (mode != ShutterManual)
		THROW_HW_ERROR(NotSupported) << "Not in manual shutter mode";
	else if (open)
		THROW_HW_ERROR(NotSupported) << "Manual shutter open "
					        "not supported";
}

void ShutterCtrlObj::getState(bool& open) const
{
	DEB_MEMBER_FUNCT();

	ShutterMode mode;
	getMode(mode);
	if (mode != ShutterManual)
		THROW_HW_ERROR(NotSupported) << "Not in manual shutter mode";

	open = false;
	DEB_RETURN() << DEB_VAR1(open);
}

void ShutterCtrlObj::setOpenTime(double shut_open_time)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(shut_open_time);

	if (shut_open_time != 0)
		THROW_HW_ERROR(NotSupported) << "Shutter open time not "
						"supported";
}

void ShutterCtrlObj::getOpenTime(double& shut_open_time) const
{
	DEB_MEMBER_FUNCT();
	shut_open_time = 0;
	DEB_RETURN() << DEB_VAR1(shut_open_time);
}

void ShutterCtrlObj::setCloseTime(double shut_close_time)
{
	DEB_MEMBER_FUNCT();
	m_cam.setShutCloseTime(shut_close_time);
}

void ShutterCtrlObj::getCloseTime(double& shut_close_time) const
{
	DEB_MEMBER_FUNCT();
	m_cam.getShutCloseTime(shut_close_time);
}


/*******************************************************************
 * \brief ShutterCtrlObj constructor
 *******************************************************************/

EventCtrlObj::EventCtrlObj()
{
	DEB_CONSTRUCTOR();
}

EventCtrlObj::~EventCtrlObj()
{
	DEB_DESTRUCTOR();
}


/*******************************************************************
 * \brief Hw Interface constructor
 *******************************************************************/

Interface::Interface(Espia::Acq& acq, BufferCtrlMgr& buffer_mgr,
		     Camera& cam)
	: m_acq(acq), m_buffer_mgr(buffer_mgr), m_cam(cam),
	  m_det_info(cam), m_buffer(buffer_mgr), m_sync(acq, cam), 
	  m_bin(acq, cam), m_roi(acq, cam), m_flip(acq, cam), m_shutter(cam),
	  m_acq_end_cb(cam), m_event_cb(m_event)
{
	DEB_CONSTRUCTOR();

	bool f16_dual = m_cam.getModel().isFrelon16Dual();
	if (f16_dual != m_acq.getDev().isMeta())
		THROW_HW_ERROR(Error) << "Frelon16 2xSPB8 / Espia mismatch";
	Espia::SGImgConfig img_config = (f16_dual ? Espia::SGImgConcatVert2 :
						    Espia::SGImgNorm);
	FrameDim det_frame_dim;
	m_cam.getFrameDim(det_frame_dim);
	m_acq.setSGImgConfig(img_config, det_frame_dim.getSize());

	m_acq.registerAcqEndCallback(m_acq_end_cb);
	m_acq.registerEventCallback(m_event_cb);

	HwDetInfoCtrlObj *det_info = &m_det_info;
	m_cap_list.push_back(HwCap(det_info));

	HwBufferCtrlObj *buffer = &m_buffer;
	m_cap_list.push_back(HwCap(buffer));

	HwSyncCtrlObj *sync = &m_sync;
	m_cap_list.push_back(HwCap(sync));

	HwBinCtrlObj *bin = &m_bin;
	m_cap_list.push_back(HwCap(bin));

	HwRoiCtrlObj *roi = &m_roi;
	m_cap_list.push_back(HwCap(roi));

	HwFlipCtrlObj *flip = &m_flip;
	m_cap_list.push_back(HwCap(flip));

	HwShutterCtrlObj *shutter = &m_shutter;
	m_cap_list.push_back(HwCap(shutter));

	HwEventCtrlObj *event = &m_event;
	m_cap_list.push_back(HwCap(event));

	reset(SoftReset);
	resetDefaults();
}

Interface::~Interface()
{
	DEB_DESTRUCTOR();
}

void Interface::getCapList(HwInterface::CapList &cap_list) const
{
	DEB_MEMBER_FUNCT();
	cap_list = m_cap_list;
}

void Interface::reset(ResetLevel reset_level)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(reset_level);

	stopAcq();

	if (reset_level == HardReset) {
		DEB_TRACE() << "Performing camera hard reset";
		m_cam.hardReset();

		resetDefaults();
	}
}

void Interface::resetDefaults()
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Setting default configuration";

	stopAcq();

	bool f16 = (m_cam.getModel().getChipType() == Andanta_CcdFT2k);
	FrameTransferMode ftm_seq[2] = {!f16 ? FFM : FTM, !f16 ? FTM : FFM};
	FrameTransferMode ftm;
	InputChan input_chan;
	if (!m_cam.getDefInputChan(ftm=ftm_seq[0], input_chan)	&&
	    !m_cam.getDefInputChan(ftm=ftm_seq[1], input_chan))
		THROW_HW_ERROR(Error) << "Cannot find default input channel!!";
	m_cam.setFrameTransferMode(ftm);
	m_cam.setInputChan(input_chan);

	m_flip.setFlip(Flip(false));

	m_sync.setNbFrames(1);
	m_sync.setExpTime(1.0);
	m_sync.setTrigMode(IntTrig);
	m_cam.setUserLatTime(0.0);

	m_shutter.setMode(ShutterAutoFrame);
	m_shutter.setCloseTime(0.0);

	m_cam.setRoiBinOffset(Point(0));
	m_bin.setBin(Bin(1));
	m_roi.setRoi(Roi());
	
	Size image_size;
	m_det_info.getMaxImageSize(image_size);
	ImageType image_type;
	m_det_info.getDefImageType(image_type);
	FrameDim frame_dim(image_size, image_type);
	m_buffer.setFrameDim(frame_dim);

	m_buffer.setNbConcatFrames(1);
	m_buffer.setNbBuffers(1);
}

void Interface::prepareAcq()
{
	DEB_MEMBER_FUNCT();
}

void Interface::startAcq()
{
	DEB_MEMBER_FUNCT();

	TrigMode trig_mode;
	m_cam.getTrigMode(trig_mode);
	bool was_running = (trig_mode == IntTrigMult) && m_cam.isRunning();
	if (!was_running) {
		m_buffer_mgr.setStartTimestamp(Timestamp::now());
		m_acq.start();
	}

	try {
		m_cam.start();
	} catch (...) {
		if (!was_running)
			m_acq.stop();
		throw;
	}
}

void Interface::stopAcq()
{
	DEB_MEMBER_FUNCT();
	m_cam.stop();
	m_acq.stop();
}

void Interface::getStatus(StatusType& status)
{
	DEB_MEMBER_FUNCT();

	status.acq = m_cam.isRunning() ? AcqRunning : AcqReady;

	static const DetStatus det_mask = 
		(DetWaitForTrigger | DetExposure    | DetShutterClose   | 
		 DetChargeShift    | DetReadout     | DetLatency);

	status.det_mask = det_mask;
	status.det = DetIdle;
	Frelon::Status cam;
	m_cam.getStatus(cam);
	if (cam & Frelon::Wait)
		status.det |= DetWaitForTrigger;
	if (cam & Frelon::Exposure)
		status.det |= DetExposure;
	if (cam & Frelon::Shutter)
		status.det |= DetShutterClose;
	if (cam & Frelon::Transfer)
		status.det |= DetChargeShift;
	if (cam & Frelon::Readout)
		status.det |= DetReadout;
	if (cam & Frelon::Latency)
		status.det |= DetLatency;

	TrigMode trig_mode;
	m_cam.getTrigMode(trig_mode);
	if ((trig_mode == IntTrigMult) && (status.det == DetWaitForTrigger))
		status.det = DetIdle;

	DEB_RETURN() << DEB_VAR1(status);
}

int Interface::getNbHwAcquiredFrames()
{
	DEB_MEMBER_FUNCT();
	Acq::Status acq_status;
	m_acq.getStatus(acq_status);
	int nb_hw_acq_frames = acq_status.last_frame_nb + 1;
	DEB_RETURN() << DEB_VAR1(nb_hw_acq_frames);
	return nb_hw_acq_frames;
}


