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
#ifndef FRELONGEOMETRY_H
#define FRELONGEOMETRY_H

#include "FrelonModel.h"
#include "lima/HwMaxImageSizeCallback.h"

namespace lima
{

namespace Frelon
{

class Geometry;
class Camera;

class DeadTimeChangedCallback
{
	DEB_CLASS_NAMESPC(DebModCamera, "DeadTimeChangedCallback", "Frelon");

 public:
	DeadTimeChangedCallback();
	virtual ~DeadTimeChangedCallback();

 protected:
	virtual void deadTimeChanged(double dead_time) = 0;

 private:
	friend class Geometry;
	Geometry *m_geom;
};

class Geometry : public HwMaxImageSizeCallbackGen
{
	DEB_CLASS_NAMESPC(DebModCamera, "Geometry", "Frelon");

 public:
	Geometry(Camera& cam);
	virtual ~Geometry();

	void sync();

	bool getDefInputChan(FrameTransferMode ftm,
			     InputChan& input_chan);
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

	void setSPB2Config(SPB2Config  spb2_config);
	void getSPB2Config(SPB2Config& spb2_config);

	std::string getSPB2ConfigName(SPB2Config spb2_config);

	void deadTimeChanged();

	void registerDeadTimeChangedCallback(DeadTimeChangedCallback& cb);
	void unregisterDeadTimeChangedCallback(DeadTimeChangedCallback& cb);

 protected:
	virtual void setMaxImageSizeCallbackActive(bool cb_active);

	bool isFrelon16();

	void writeRegister(Reg reg, int  val);
	void readRegister (Reg reg, int& val);
	
	int getModesAvail();

	void setChanMode(int  chan_mode);
	void getChanMode(int& chan_mode);
	
	void calcBaseChanMode(FrameTransferMode ftm, int& base_chan_mode);
	void calcChanMode(FrameTransferMode ftm, InputChan input_chan,
			  int& chan_mode);
	void calcFTMInputChan(int chan_mode, FrameTransferMode& ftm, 
			      InputChan& input_chan);

	void setFlipMode(int  flip_mode);
	void getFlipMode(int& flip_mode);

	Flip  getMirror();
	Point getNbChan();
	Size  getCcdSize();
	Size  getChanSize();
        Flip  getRoiInsideMirror();

	void writeChanRoi(const Roi& chan_roi);
	void readChanRoi(Roi& chan_roi);

	void xformChanCoords(const Point& point, Point& chan_point, 
			     Corner& ref_corner);
	void calcImageRoi(const Roi& chan_roi, const Flip& roi_inside_mirror,
			  Roi& image_roi, Point& roi_bin_offset);
	void calcFinalRoi(const Roi& image_roi, const Point& roi_offset,
			  Roi& final_roi);
	void calcChanRoi(const Roi& image_roi, Roi& chan_roi,
			 Flip& roi_inside_mirror);
	void calcChanRoiOffset(const Roi& req_roi, const Roi& image_roi,
			       Point& roi_offset);
        void checkRoiMode(const Roi& roi);
	void processSetRoi(const Roi& req_roi, Roi& hw_roi, Roi& chan_roi, 
			   Point& roi_offset);
	void resetRoiBinOffset();

	Camera& m_cam;
	Model& m_model;
	Point m_chan_roi_offset;
	Point m_roi_bin_offset;
	bool m_mis_cb_act;
	double m_dead_time;
	DeadTimeChangedCallback *m_dead_time_cb;
};

inline bool Geometry::isChanActive(InputChan curr, InputChan chan)
{
	return (curr & chan) == chan;
};

inline bool Geometry::isFrelon16()
{
	return m_model.isFrelon16();
}


} // namespace Frelon

} // namespace lima


#endif // FRELONGEOMETRY_H
