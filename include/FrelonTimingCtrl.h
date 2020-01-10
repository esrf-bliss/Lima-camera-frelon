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
#ifndef FRELONTIMINGCTRL_H
#define FRELONTIMINGCTRL_H

#include "FrelonModel.h"

namespace lima
{

namespace Frelon
{

class Camera;

class TimingCtrl
{
	DEB_CLASS_NAMESPC(DebModCamera, "TimingCtrl", "Frelon");

 public:
	struct SeqTim {
		typedef std::pair<Reg, Reg> RegPair;
		typedef std::vector<RegPair> RegPairList;
		typedef std::pair<int, int> ValPair;
		typedef std::vector<ValPair> ValPairList;
	
		static RegPairList RegList;
		static const double ClockPeriod;
	
		static SeqTimValues calcValues(const ValPairList& l);
		static double calcSeqTim(const ValPair& v)
		{
			unsigned long v_first = v.first;
			return ((v_first << 16) + v.second) * ClockPeriod;
		}
	};

	TimingCtrl(Camera& cam);
	virtual ~TimingCtrl();

	void getReadoutTime(double& readout_time);
	void getTransferTime(double& xfer_time);
	void getDeadTime(double& dead_time);

	void latchSeqTimValues(SeqTimValues& st);
	void measureSeqTimValues(SeqTimValues& st, double timeout);

 protected:
	void writeRegister(Reg reg, int  val);
	void readRegister (Reg reg, int& val);
	void readFloatRegister(Reg reg, double& val);

	Camera& m_cam;
	Model& m_model;
};


} // namespace Frelon

} // namespace lima


#endif // FRELONTIMINGCTRL_H
