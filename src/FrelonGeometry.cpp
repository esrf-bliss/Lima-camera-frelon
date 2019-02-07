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
#include "lima/MiscUtils.h"
#include <sstream>

using namespace lima;
using namespace lima::Frelon;
using namespace std;

DeadTimeChangedCallback::DeadTimeChangedCallback()
	: m_geom(NULL)
{
	DEB_CONSTRUCTOR();
}

DeadTimeChangedCallback::~DeadTimeChangedCallback()
{
	DEB_DESTRUCTOR();

	if (m_geom)
		m_geom->unregisterDeadTimeChangedCallback(*this);
}


Geometry::Geometry(Camera& cam)
	: m_cam(cam), m_model(cam.getModel()),
	  m_mis_cb_act(false), m_dead_time(0), m_dead_time_cb(NULL)
{
	DEB_CONSTRUCTOR();
}

Geometry::~Geometry()
{
	DEB_DESTRUCTOR();

	if (m_dead_time_cb)
		unregisterDeadTimeChangedCallback(*m_dead_time_cb);
}

void Geometry::writeRegister(Reg reg, int val)
{
	m_cam.writeRegister(reg, val);
}

void Geometry::readRegister(Reg reg, int& val)
{
	m_cam.readRegister(reg, val);
}

void Geometry::readFloatRegister(Reg reg, double& val)
{
	m_cam.readFloatRegister(reg, val);
}

void Geometry::sync()
{
	DEB_MEMBER_FUNCT();

	m_chan_roi_offset = 0;
	getRoiBinOffset(m_roi_bin_offset);
	deadTimeChanged();
}

int Geometry::getModesAvail()
{
	DEB_MEMBER_FUNCT();

	int modes_avail;
	if (m_model.has(Model::ModesAvail))
		readRegister(CcdModesAvail, modes_avail);
	else if (m_model.getChipType() == Kodak)
		modes_avail = KodakModesAvail;
	else
		modes_avail = AtmelModesAvail;
		
	DEB_RETURN() << DEB_VAR1(DEB_HEX(modes_avail));
	return modes_avail;
}

string Geometry::getInputChanModeName(FrameTransferMode ftm, 
				    InputChan input_chan)
{
	DEB_STATIC_FUNCT();

	ostringstream os;
	os << FTMNameMap[ftm] << "-";
	string sep;
	for (int chan = 1; chan <= 4; chan++) {
		int chan_bit = 1 << (chan - 1);
		if ((input_chan & chan_bit) != 0) {
			os << sep << chan;
			sep = "&";
		}
	}
	string mode_name = os.str();
	DEB_RETURN() << DEB_VAR1(mode_name);
	return mode_name;
}

void Geometry::setChanMode(int chan_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(chan_mode);

	int mode_bit = 1 << (chan_mode - 1);
	bool valid_mode = ((getModesAvail() & mode_bit) != 0);
	if (!valid_mode) {
		FrameTransferMode ftm;
		InputChan input_chan;
		calcFTMInputChan(chan_mode, ftm, input_chan);
		string mode_name = getInputChanModeName(ftm, input_chan);

		THROW_HW_ERROR(InvalidValue) 
			<< "Channel mode " << mode_name 
			<< " [" << DEB_VAR1(chan_mode) << "] "
			<< "not supported in " << m_model.getName();
	}
	writeRegister(ChanMode, chan_mode);
	deadTimeChanged();
}

void Geometry::getChanMode(int& chan_mode)
{
	DEB_MEMBER_FUNCT();
	readRegister(ChanMode, chan_mode);
	DEB_RETURN() << DEB_VAR1(chan_mode);
}

void Geometry::calcBaseChanMode(FrameTransferMode ftm, int& base_chan_mode)
{
	DEB_MEMBER_FUNCT();
	base_chan_mode = FTMChanRangeMap[ftm].first;
	DEB_RETURN() << DEB_VAR1(base_chan_mode);
}

void Geometry::calcChanMode(FrameTransferMode ftm, InputChan input_chan,
			  int& chan_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(FTMNameMap[ftm], DEB_HEX(input_chan));

	calcBaseChanMode(ftm, chan_mode);
	const InputChanList& chan_list = FTMInputChanListMap[ftm];
	InputChanList::const_iterator it;
	it = find(chan_list.begin(), chan_list.end(), input_chan);
	if (it == chan_list.end())
		THROW_HW_ERROR(InvalidValue) << "Invalid " 
					     << DEB_VAR1(input_chan);
	chan_mode += it - chan_list.begin();

	DEB_RETURN() << DEB_VAR1(chan_mode);
}

void Geometry::calcFTMInputChan(int chan_mode, FrameTransferMode& ftm, 
			      InputChan& input_chan)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(chan_mode);

	bool ftm_found = false;
	FTMChanRangeMapType::const_iterator it, end = FTMChanRangeMap.end();
	for (it = FTMChanRangeMap.begin(); it != end; ++it) {
		ftm = it->first;
		const ChanRange& range = it->second;
		if ((chan_mode >= range.first) && (chan_mode < range.second)) {
			ftm_found = true;
			break;
		}
	}
	if (!ftm_found)
		THROW_HW_ERROR(Error) << "Invalid " << DEB_VAR1(chan_mode);

	int base_chan_mode;
	calcBaseChanMode(ftm, base_chan_mode);
	input_chan = FTMInputChanListMap[ftm][chan_mode - base_chan_mode];
	DEB_RETURN() << DEB_VAR2(FTMNameMap[ftm], DEB_HEX(input_chan));
}

bool Geometry::getDefInputChan(FrameTransferMode ftm, InputChan& input_chan)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(FTMNameMap[ftm]);

	int modes_avail = getModesAvail();

	bool valid_mode = false;
	InputChanList::const_iterator it, end = DefInputChanList.end();
	for (it = DefInputChanList.begin(); it != end; ++it) {
		input_chan = *it;
		int chan_mode;
		try {
			calcChanMode(ftm, input_chan, chan_mode);
		} catch (...) {
			continue;
		}
		int mode_bit = 1 << (chan_mode - 1);
		valid_mode = ((modes_avail & mode_bit) != 0);
		if (valid_mode)
			break;
	}

	if (!valid_mode) {
		DEB_WARNING() << "Could not find default input_chan for " 
			      << FTMNameMap[ftm] << ": " 
			      << DEB_VAR1(DEB_HEX(modes_avail));
		DEB_RETURN() << DEB_VAR1(valid_mode);
	} else {
		DEB_RETURN() << DEB_VAR2(valid_mode, DEB_HEX(input_chan)) 
			     << " [" << getInputChanModeName(ftm, input_chan) 
			     << "]";
	}

	return valid_mode;
}

void Geometry::setInputChan(InputChan input_chan)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(DEB_HEX(input_chan));

	FrameTransferMode ftm;
	getFrameTransferMode(ftm);
	int chan_mode;
	calcChanMode(ftm, input_chan, chan_mode);
	setChanMode(chan_mode);
}

void Geometry::getInputChan(InputChan& input_chan)
{
	DEB_MEMBER_FUNCT();

	FrameTransferMode ftm;
	int chan_mode;
	getChanMode(chan_mode);
	calcFTMInputChan(chan_mode, ftm, input_chan);

	DEB_RETURN() << DEB_VAR1(DEB_HEX(input_chan));
}

void Geometry::setFrameTransferMode(FrameTransferMode ftm)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(FTMNameMap[ftm]);
	
	FrameTransferMode prev_ftm;
	getFrameTransferMode(prev_ftm);
	if (ftm == prev_ftm) {
		DEB_TRACE() << "Nothing to do";
		return;
	}

	InputChan input_chan;
	getInputChan(input_chan);
	int chan_mode;
	try {
		calcChanMode(ftm, input_chan, chan_mode);
		setChanMode(chan_mode);
	} catch (...) {
		DEB_TRACE() << DEB_VAR1(DEB_HEX(input_chan)) 
			    << " not available in " << FTMNameMap[ftm];
		DEB_TRACE() << "  Trying default input channel";
		if (!getDefInputChan(ftm, input_chan))
			THROW_HW_ERROR(Error) << "No input channel found for "
					      << FTMNameMap[ftm];
		calcChanMode(ftm, input_chan, chan_mode);
		setChanMode(chan_mode);
	}

	if (!m_mis_cb_act) 
		return;

	FrameDim frame_dim;
	getFrameDim(frame_dim);
	DEB_TRACE() << "MaxImageSizeChanged: " << DEB_VAR1(frame_dim);
	maxImageSizeChanged(frame_dim.getSize(), frame_dim.getImageType());
}

void Geometry::getFrameTransferMode(FrameTransferMode& ftm)
{
	DEB_MEMBER_FUNCT();

	int chan_mode;
	getChanMode(chan_mode);

	FTMChanRangeMapType::const_iterator it, end = FTMChanRangeMap.end();
	for (it = FTMChanRangeMap.begin(); it != end; ++it) {
		ftm = it->first;
		const ChanRange& range = it->second;
		if ((chan_mode >= range.first) && (chan_mode < range.second)) {
			DEB_RETURN() << DEB_VAR1(FTMNameMap[ftm]);
			return;
		}
	}

	THROW_HW_ERROR(Error) << "Invalid " << DEB_VAR1(chan_mode);
}

void Geometry::getMaxFrameDim(FrameDim& max_frame_dim)
{
	DEB_MEMBER_FUNCT();

	ChipType chip_type = m_model.getChipType();
	max_frame_dim = ChipMaxFrameDimMap[chip_type];

	GeomType geom_type = m_model.getGeomType();
	if (geom_type == SPB2_F16)
		max_frame_dim /= Point(2, 2);
	else if (geom_type == SPB8_F16_Single)
		max_frame_dim /= Point(1, 2);

	DEB_RETURN() << DEB_VAR1(max_frame_dim);
}

void Geometry::getFrameDim(FrameDim& frame_dim)
{
	DEB_MEMBER_FUNCT();

	getMaxFrameDim(frame_dim);
	FrameTransferMode ftm;
	getFrameTransferMode(ftm);
	if ((ftm == FTM) && !m_model.has(Model::HamaChip))
		frame_dim /= Point(1, 2);

	DEB_RETURN() << DEB_VAR1(frame_dim);
}

void Geometry::setFlipMode(int flip_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(flip_mode);
	writeRegister(FlipMode, flip_mode);
}

void Geometry::getFlipMode(int& flip_mode)
{
	DEB_MEMBER_FUNCT();
	readRegister(FlipMode, flip_mode);
	DEB_RETURN() << DEB_VAR1(flip_mode);
}

void Geometry::checkFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(flip);
	if (isFrelon16()) {
		DEB_TRACE() << "No flip is supported";
		flip = Flip(false);
	} else {
		DEB_TRACE() << "All standard flip modes are supported";
	}
	DEB_RETURN() << DEB_VAR1(flip);
}

void Geometry::setFlip(const Flip& flip)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(flip);
	int flip_mode = (flip.x << 1) | (flip.y << 0);
	setFlipMode(flip_mode);
}

void Geometry::getFlip(Flip& flip)
{
	DEB_MEMBER_FUNCT();

	int flip_mode;
	getFlipMode(flip_mode);
	flip.x = (flip_mode >> 1) & 1;
	flip.y = (flip_mode >> 0) & 1;

	DEB_RETURN() << DEB_VAR1(flip);
}

void Geometry::checkBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(bin);

	bool frelon16 = (isFrelon16());
	int max_bin_x = frelon16 ? 1 : int(MaxBinX);
	int max_bin_y = frelon16 ? 1 : int(MaxBinY);
	int bin_x = min(bin.getX(), max_bin_x);
	int bin_y = min(bin.getY(), max_bin_y);
	bin = Bin(bin_x, bin_y);

	DEB_RETURN() << DEB_VAR1(bin);
}

void Geometry::setBin(const Bin& bin)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(bin);
	
	if ((bin.getX() > MaxBinX) || (bin.getY() > MaxBinY))
		THROW_HW_ERROR(InvalidValue) << "Invalid " << DEB_VAR1(bin)
					     << ". Max. HW binning is " 
					     << Bin(MaxBinX, MaxBinY);

	Bin curr_bin;
	getBin(curr_bin);
	DEB_TRACE() << DEB_VAR1(curr_bin);
	if (bin == curr_bin) 
		return;

	DEB_TRACE() << "Reseting Roi";
	Roi roi;
	setRoi(roi);

	writeRegister(BinHorz, bin.getX());
	writeRegister(BinVert, bin.getY());

	resetRoiBinOffset();

	deadTimeChanged();
}

void Geometry::getBin(Bin& bin)
{
	DEB_MEMBER_FUNCT();
	int bin_x, bin_y;
	readRegister(BinHorz, bin_x);
	readRegister(BinVert, bin_y);
	bin = Bin(bin_x, bin_y);
	DEB_RETURN() << DEB_VAR1(bin);
}

void Geometry::setRoiMode(RoiMode roi_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(roi_mode);

	bool roi_hw   = (roi_mode == Slow) || (roi_mode == Fast);
	bool roi_fast = (roi_mode == Fast);
	bool roi_kin  = (roi_mode == Kinetic);
	DEB_TRACE() << DEB_VAR3(roi_hw, roi_fast, roi_kin);

	writeRegister(RoiEnable,  roi_hw);
	writeRegister(RoiFast,    roi_fast);
	writeRegister(RoiKinetic, roi_kin);

	if (roi_mode == None)
		resetRoiBinOffset();

	deadTimeChanged();
}

void Geometry::getRoiMode(RoiMode& roi_mode)
{
	DEB_MEMBER_FUNCT();

	int roi_hw, roi_fast, roi_kin;
	readRegister(RoiEnable,  roi_hw);
	readRegister(RoiFast,    roi_fast);
	readRegister(RoiKinetic, roi_kin);
	DEB_TRACE() << DEB_VAR3(roi_hw, roi_fast, roi_kin);

	if (roi_kin)
		roi_mode = Kinetic;
	else if (roi_fast && roi_hw)
		roi_mode = Fast;
	else if (roi_hw)
		roi_mode = Slow;
	else
		roi_mode = None;

	DEB_RETURN() << DEB_VAR1(roi_mode);
}

Flip Geometry::getMirror()
{
	DEB_MEMBER_FUNCT();

	InputChan curr;
	getInputChan(curr);
	Flip mirror;
	mirror.x = isChanActive(curr, Chan12) || isChanActive(curr, Chan34);
	mirror.y = isChanActive(curr, Chan13) || isChanActive(curr, Chan24);
	if (isFrelon16())
		mirror = Flip(false);
	DEB_RETURN() << DEB_VAR1(mirror);
	return mirror;
}

Point Geometry::getNbChan()
{
	DEB_MEMBER_FUNCT();
	Point nb_chan = Point(getMirror()) + 1;
	DEB_RETURN() << DEB_VAR1(nb_chan);
	return nb_chan;
}

Size Geometry::getCcdSize()
{
	DEB_MEMBER_FUNCT();
	FrameDim frame_dim;
	getFrameDim(frame_dim);
	Size ccd_size = frame_dim.getSize();
	DEB_RETURN() << DEB_VAR1(ccd_size);
	return ccd_size;
}

Size Geometry::getChanSize()
{
	DEB_MEMBER_FUNCT();
	Size chan_size = getCcdSize() / getNbChan();
	DEB_RETURN() << DEB_VAR1(chan_size);
	return chan_size;
}

Flip Geometry::getRoiInsideMirror()
{
	DEB_MEMBER_FUNCT();
	Flip roi_inside_mirror(m_chan_roi_offset.x > 0, 
			       m_chan_roi_offset.y > 0);
	DEB_RETURN() << DEB_VAR1(roi_inside_mirror);
	return roi_inside_mirror;
}

void Geometry::xformChanCoords(const Point& point, Point& xform_point, 
			     Corner& ref_corner)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(point);

	Flip chan_flip;
	getFlip(chan_flip);
	Flip mirror = getMirror();
	Size chan_size = getChanSize();

	InputChan curr;
	getInputChan(curr);
	bool right  = !isChanActive(curr, Chan1) && !isChanActive(curr, Chan3);
	bool bottom = !isChanActive(curr, Chan1) && !isChanActive(curr, Chan2);
	Flip readout_flip(right, bottom);
	DEB_TRACE() << DEB_VAR2(chan_flip, readout_flip);

	Flip effect_flip = chan_flip & readout_flip;
	DEB_TRACE() << "After flip: " << DEB_VAR1(effect_flip);

	if (mirror.x)
		effect_flip.x = (point.x >= chan_size.getWidth());
	if (mirror.y)
		effect_flip.y = (point.y >= chan_size.getHeight());
	DEB_TRACE() << "After mirror: " << DEB_VAR1(effect_flip);

	ref_corner = effect_flip.getRefCorner();

	Size ccd_size = getCcdSize();
	xform_point = ccd_size.getCornerCoords(point, ref_corner);
	DEB_RETURN() << DEB_VAR2(xform_point, ref_corner);
}

void Geometry::calcImageRoi(const Roi& chan_roi, const Flip& roi_inside_mirror,
			  Roi& image_roi, Point& roi_bin_offset)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(chan_roi, roi_inside_mirror);

	Point image_tl, chan_tl = chan_roi.getTopLeft();
	Point image_br, chan_br = chan_roi.getBottomRight();
	Corner c_tl, c_br;
	xformChanCoords(chan_tl, image_tl, c_tl);
	xformChanCoords(chan_br, image_br, c_br);
	Roi unbinned_roi(image_tl, image_br);
	DEB_TRACE() << "Before mirror shift " << DEB_VAR1(unbinned_roi);

	Bin bin;
	getBin(bin);
	Size bin_size = Point(bin);
	Point mirr_shift = roi_inside_mirror * (bin_size - 1);
	DEB_TRACE() << DEB_VAR1(mirr_shift);

	image_tl = unbinned_roi.getTopLeft() + mirr_shift;
	unbinned_roi.setTopLeft(image_tl);
	DEB_TRACE() << "After mirror shift " << DEB_VAR1(unbinned_roi);

	image_roi = unbinned_roi.getBinned(bin);

	image_tl %= bin_size;
	c_tl = roi_inside_mirror.getRefCorner();
	DEB_TRACE() << DEB_VAR2(image_tl, c_tl);

	roi_bin_offset = bin_size.getCornerCoords(image_tl, c_tl) % bin_size;

	DEB_RETURN() << DEB_VAR2(image_roi, roi_bin_offset);
}

void Geometry::calcFinalRoi(const Roi& image_roi, const Point& chan_roi_offset,
			  Roi& final_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(image_roi, chan_roi_offset);

	Point tl = image_roi.getTopLeft() + chan_roi_offset;
	Point nb_chan = getNbChan();
	Size size = image_roi.getSize() * nb_chan;
	final_roi = Roi(tl, size);

	DEB_RETURN() << DEB_VAR1(final_roi);
}

void Geometry::calcChanRoi(const Roi& image_roi, Roi& chan_roi,
			 Flip& roi_inside_mirror)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(image_roi);

	Bin bin;
	getBin(bin);
	Roi unbinned_roi = image_roi.getUnbinned(bin);
	DEB_TRACE() << DEB_VAR1(unbinned_roi);

	Point image_tl = unbinned_roi.getTopLeft();
	image_tl += m_roi_bin_offset;
	unbinned_roi.setTopLeft(image_tl);
	DEB_TRACE() << "shifted: " << DEB_VAR2(m_roi_bin_offset, unbinned_roi);

	Point image_br = unbinned_roi.getBottomRight();
	Point chan_tl, chan_br;
	Corner c_tl, c_br;
	xformChanCoords(image_tl, chan_tl, c_tl);
	xformChanCoords(image_br, chan_br, c_br);

	chan_roi.setCorners(chan_tl, chan_br);
	DEB_TRACE() << "xformChanCoords: " << DEB_VAR3(chan_roi, c_tl, c_br);

	chan_tl = chan_roi.getTopLeft();
	chan_br = chan_roi.getBottomRight();

	bool two_xchan = (c_tl.getX() != c_br.getX());
	bool two_ychan = (c_tl.getY() != c_br.getY());
	DEB_TRACE() << DEB_VAR2(two_xchan, two_ychan);

	Size chan_size = getChanSize();
	if (two_xchan)
		chan_br.x = chan_size.getWidth() - 1;
	if (two_ychan)
		chan_br.y = chan_size.getHeight() - 1;

	chan_roi.setCorners(chan_tl, chan_br);

	roi_inside_mirror = getMirror();
	roi_inside_mirror.x &= (image_tl.x > chan_br.x);
	roi_inside_mirror.y &= (image_tl.y > chan_br.y);

	DEB_RETURN() << DEB_VAR2(chan_roi, roi_inside_mirror);
}

void Geometry::calcChanRoiOffset(const Roi& req_roi, const Roi& image_roi,
			       Point& chan_roi_offset)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(req_roi, image_roi);

	Point virt_tl = image_roi.getTopLeft();

	Size image_size, ccd_size = getCcdSize();
	Bin bin;
	getBin(bin);
	ccd_size /= bin;

	image_size = image_roi.getSize();
	Point image_br = image_roi.getBottomRight();

	Point mirror_tl = ccd_size - (image_br + 1) - image_size;
	Point req_tl = req_roi.getTopLeft();
	if (req_tl.x > image_br.x)
		virt_tl.x = mirror_tl.x;
	if (req_tl.y > image_br.y)
		virt_tl.y = mirror_tl.y;

	chan_roi_offset = virt_tl - image_roi.getTopLeft();
	DEB_RETURN() << DEB_VAR1(chan_roi_offset);
}

void Geometry::checkRoiMode(const Roi& roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(roi);

	RoiMode roi_mode;
	getRoiMode(roi_mode);
	if (!roi.isActive())
		roi_mode = None;
	else if (roi_mode == None)
		roi_mode = Slow;
	setRoiMode(roi_mode);
}

void Geometry::checkRoi(const Roi& set_roi, Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(set_roi);

	if (set_roi.isActive()) {
		Roi chan_roi;
		Point chan_roi_offset;
		processSetRoi(set_roi, hw_roi, chan_roi, chan_roi_offset);
	} else 
		hw_roi = set_roi;

	DEB_RETURN() << DEB_VAR1(hw_roi);
}

void Geometry::processSetRoi(const Roi& set_roi, Roi& hw_roi, 
			   Roi& chan_roi, Point& chan_roi_offset)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(set_roi);

	Roi aligned_roi = set_roi;
	aligned_roi.alignCornersTo(Point(32, 1), Ceil);
	Flip roi_inside_mirror;
	calcChanRoi(aligned_roi, chan_roi, roi_inside_mirror);
	Roi image_roi;
	Point roi_bin_offset;
	calcImageRoi(chan_roi, roi_inside_mirror, image_roi, roi_bin_offset);
	calcChanRoiOffset(set_roi, image_roi, chan_roi_offset);
	calcFinalRoi(image_roi, chan_roi_offset, hw_roi);

	DEB_RETURN() << DEB_VAR3(hw_roi, chan_roi, chan_roi_offset);
}

void Geometry::setRoi(const Roi& set_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(set_roi);

	checkRoiMode(set_roi);
	if (!set_roi.isActive()) {
		DEB_TRACE() << "Roi deactivated";
		return;
	}

	Roi hw_roi, chan_roi;
	Point chan_roi_offset;
	processSetRoi(set_roi, hw_roi, chan_roi, chan_roi_offset);

	writeChanRoi(chan_roi);
	m_chan_roi_offset = chan_roi_offset;
}

void Geometry::getRoi(Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();

	hw_roi.reset();

	RoiMode roi_mode;
	getRoiMode(roi_mode);
	if (roi_mode == None)
		return;

	Roi chan_roi, image_roi;
	readChanRoi(chan_roi);

	Flip roi_inside_mirror = getRoiInsideMirror();
	Point roi_bin_offset;
	calcImageRoi(chan_roi, roi_inside_mirror, image_roi, roi_bin_offset);

	calcFinalRoi(image_roi, m_chan_roi_offset, hw_roi);
	DEB_RETURN() << DEB_VAR1(hw_roi);
}

void Geometry::writeChanRoi(const Roi& chan_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(chan_roi);
	
	Point tl  = chan_roi.getTopLeft();
	Size size = chan_roi.getSize();
	
	writeRegister(RoiPixelBegin, tl.x);
	writeRegister(RoiPixelWidth, size.getWidth());
	writeRegister(RoiLineBegin,  tl.y);
	writeRegister(RoiLineWidth,  size.getHeight());

	deadTimeChanged();
}

void Geometry::readChanRoi(Roi& chan_roi)
{
	DEB_MEMBER_FUNCT();

	int rpb, rpw, rlb, rlw;
	readRegister(RoiPixelBegin, rpb);
	readRegister(RoiPixelWidth, rpw);
	readRegister(RoiLineBegin,  rlb);
	readRegister(RoiLineWidth,  rlw);

	chan_roi = Roi(Point(rpb, rlb), Size(rpw, rlw));
	DEB_RETURN() << DEB_VAR1(chan_roi);
}


void Geometry::setRoiBinOffset(const Point& roi_bin_offset)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(roi_bin_offset, m_roi_bin_offset);

	if (roi_bin_offset == m_roi_bin_offset)
		return;

	RoiMode roi_mode;
	getRoiMode(roi_mode);
	if (roi_mode == None) {
		if (!roi_bin_offset.isNull())
			THROW_HW_ERROR(InvalidValue) << "HW Roi not active";
		return;
	} 
	Bin bin;
	getBin(bin);
	Size bin_size = Point(bin);
	Roi valid_offset_range(Point(0), bin_size);
	if (!valid_offset_range.containsPoint(roi_bin_offset)) {
		THROW_HW_ERROR(InvalidValue) << "Invalid unaligned " 
					     << DEB_VAR1(roi_bin_offset);
	} else if (roi_bin_offset.x != 0) {
		THROW_HW_ERROR(InvalidValue) 
			<< "Invalid " << DEB_VAR1(roi_bin_offset) << ". "
			<< "Must be horizontally aligned to " << DEB_VAR1(bin);
	}

	Roi chan_roi;
	readChanRoi(chan_roi);

	Point image_tl, image_br;
	Corner c_tl, c_br;
	xformChanCoords(chan_roi.getTopLeft(),     image_tl, c_tl);
	xformChanCoords(chan_roi.getBottomRight(), image_br, c_br);
	Roi image_roi(image_tl, image_br);
	DEB_TRACE() << DEB_VAR1(image_roi);

	Flip roi_inside_mirror = getRoiInsideMirror();
	Point mirr_shift = roi_inside_mirror * (bin_size - 1);
	DEB_TRACE() << DEB_VAR1(mirr_shift);

	image_tl = image_roi.getTopLeft() + mirr_shift;
	image_tl -= image_tl % bin_size;
	DEB_TRACE() << "After alignment " << DEB_VAR1(image_tl);

	c_tl = roi_inside_mirror.getRefCorner();
	image_tl += roi_bin_offset * c_tl.getDir();
	DEB_TRACE() << "After roi_bin_offset " << DEB_VAR1(image_tl);

	Roi max_chan_roi(Point(0), getChanSize());
	bool ok = max_chan_roi.containsPoint(image_tl);
	if (ok) {
		DEB_TRACE() << "Image top-left is OK";
		image_roi.setTopLeft(image_tl);
		ok = max_chan_roi.containsRoi(image_roi);
	}
	if (!ok)
		THROW_HW_ERROR(InvalidValue) << "Cannot apply requested "
					     << DEB_VAR1(roi_bin_offset);

	Point chan_tl, chan_br;
	xformChanCoords(image_roi.getTopLeft(),     chan_tl, c_tl);
	xformChanCoords(image_roi.getBottomRight(), chan_br, c_br);
	chan_roi = Roi(chan_tl, chan_br);

	writeChanRoi(chan_roi);

	m_roi_bin_offset = roi_bin_offset;
}

void Geometry::getRoiBinOffset(Point& roi_bin_offset)
{
	DEB_MEMBER_FUNCT();

	RoiMode roi_mode;
	getRoiMode(roi_mode);
	if (roi_mode == None) {
		roi_bin_offset = 0;
		return;
	}

	Roi chan_roi, image_roi;
	readChanRoi(chan_roi);

	Flip roi_inside_mirror = getRoiInsideMirror();
	calcImageRoi(chan_roi, roi_inside_mirror, image_roi, roi_bin_offset);

	DEB_RETURN() << DEB_VAR1(roi_bin_offset);
}

void Geometry::resetRoiBinOffset()
{
	DEB_MEMBER_FUNCT();

	m_roi_bin_offset = 0;
	DEB_TRACE() << "Forcing " << DEB_VAR1(m_roi_bin_offset);

	int bin_y;
	readRegister(BinVert, bin_y);
	if (bin_y == 1)
		return;

	int roi_line_beg, bin_y_offset;
	readRegister(RoiLineBegin, roi_line_beg);
	bin_y_offset = roi_line_beg % bin_y;
	if (bin_y_offset != 0) {
		roi_line_beg -= bin_y_offset;
		DEB_TRACE() << "Forcing alignment " << DEB_VAR2(bin_y, 
								roi_line_beg);
		writeRegister(RoiLineBegin, roi_line_beg);

		deadTimeChanged();
	}
}

void Geometry::getReadoutTime(double& readout_time)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::TimeCalc))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "readout time calculation";

	if (isFrelon16()) {
		readout_time = 100e-3;
	} else {
		readFloatRegister(ReadoutTime, readout_time);
		readout_time *= 1e-6;
	}
	DEB_RETURN() << DEB_VAR1(readout_time);
}

void Geometry::getTransferTime(double& xfer_time)
{
	DEB_MEMBER_FUNCT();
	if (!m_model.has(Model::TimeCalc))
		THROW_HW_ERROR(NotSupported) << "Camera does not have "
					     << "shift time calculation";

	if (isFrelon16()) {
		xfer_time = 100e-3;
	} else {
		readFloatRegister(TransferTime, xfer_time);
		xfer_time *= 1e-6;
	}
	DEB_RETURN() << DEB_VAR1(xfer_time);
}

void Geometry::getDeadTime(double& dead_time)
{
	DEB_MEMBER_FUNCT();
	getTransferTime(dead_time);
	if (dead_time == 0)
		getReadoutTime(dead_time);
	DEB_RETURN() << DEB_VAR1(dead_time);
}

void Geometry::setSPB2Config(SPB2Config spb2_config)
{
	DEB_MEMBER_FUNCT();
	if (m_model.has(Model::SPB1))
		THROW_HW_ERROR(NotSupported) << "Camera is SPB1: SPB2 config " 
					     << "not supported by hardware!";
	DEB_PARAM() << DEB_VAR1(spb2_config) 
		    << " [" << getSPB2ConfigName(spb2_config) << "]";
	writeRegister(ConfigHD, int(spb2_config));
	deadTimeChanged();
}

void Geometry::getSPB2Config(SPB2Config& spb2_config)
{
	DEB_MEMBER_FUNCT();
	if (m_model.has(Model::SPB1))
		THROW_HW_ERROR(NotSupported) << "Camera is SPB1: SPB2 config " 
					     << "not supported by hardware!";
	int config_hd;
	readRegister(ConfigHD, config_hd);
	spb2_config = SPB2Config(config_hd);
	DEB_RETURN() << DEB_VAR1(spb2_config) 
		     << " [" << getSPB2ConfigName(spb2_config) << "]";
}

string Geometry::getSPB2ConfigName(SPB2Config spb2_config)
{
	DEB_STATIC_FUNCT();
	DEB_PARAM() << DEB_VAR1(spb2_config);

	SPB2ConfigStrMapType::const_iterator it, end = SPB2ConfigNameMap.end();
	it = SPB2ConfigNameMap.find(spb2_config);
	string name = (it != end) ? it->second : "Unknown";

	DEB_RETURN() << DEB_VAR1(name);
	return name;
}

void Geometry::setMaxImageSizeCallbackActive(bool cb_active)
{
	m_mis_cb_act = cb_active;
}

void Geometry::registerDeadTimeChangedCallback(DeadTimeChangedCallback& cb)
{
	DEB_MEMBER_FUNCT();

	if (m_dead_time_cb)
		THROW_HW_ERROR(InvalidValue) << "a cb is already registered";

	cb.m_geom = this;
	m_dead_time_cb = &cb;
}

void Geometry::unregisterDeadTimeChangedCallback(DeadTimeChangedCallback& cb)
{
	DEB_MEMBER_FUNCT();

	if (&cb != m_dead_time_cb)
		THROW_HW_ERROR(InvalidValue) << "the cb is not registered";

	m_dead_time_cb = NULL;
	cb.m_geom = NULL;
}

void Geometry::deadTimeChanged()
{
	DEB_MEMBER_FUNCT();

	double dead_time;
	getDeadTime(dead_time);
	DEB_TRACE() << DEB_VAR2(dead_time, m_dead_time);
	if (dead_time == m_dead_time) 
		return;

	m_dead_time = dead_time;
	if (m_dead_time_cb)
		m_dead_time_cb->deadTimeChanged(dead_time);
}
