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
#ifndef FRELON_H
#define FRELON_H

#include "EspiaSerialLine.h"
#include <string>
#include <map>
#include <vector>

namespace lima
{

namespace Frelon
{

enum Reg {
	NbFrames,	ExpTime,	ShutCloseTime,	LatencyTime,
	RoiLineBegin,	RoiLineWidth,	RoiPixelBegin,	RoiPixelWidth,
	ChanMode,	TimeUnit,	RoiEnable,	RoiFast, 
	AntiBloom,	BinVert,	BinHorz,	ConfigHD,
	RoiKinetic,	ShutEnable,	HardTrigDisable,
	PixelFreq,	LineFreq,	FlipMode,	IntCalib,
	DisplayImage,	AdcFloatDiode,	AdcSignal,	
	DarkPixelCalib,	DarkPixelMode,	ChanControl,	Mire,
	AoiLineBegin,	AoiLineWidth,	AoiPixelBegin,	AoiPixelWidth,
	AoiImageHeight,	AoiImageWidth,	ChanOnImage,	ChanOnCcd,
	Version,	CompSerNb,	Warn,		LastWarn,
	LineClockPer,	PixelClockPer,	FirstPHIVLen,	PHIHSetupLen,
	SingleVertXfer,	SingleHorzXfer,	AllVertXfer,	AllHorzXfer,
	ReadoutTime,	TransferTime,   CcdModesAvail,
};

typedef std::map<Reg, std::string> RegStrMapType;
extern RegStrMapType RegStrMap;

typedef std::vector<Reg> RegListType;
extern RegListType CacheableRegList;

typedef std::map<Reg, double> RegDoubleMapType;
extern RegDoubleMapType RegSleepMap;

extern const int MaxRegVal;

enum Cmd {
	Reset,		Start,		Stop,		Save,
};

typedef std::map<Cmd, std::string> CmdStrMapType;
extern CmdStrMapType CmdStrMap;


enum MultiLineCmd {
	Help,		Config,		Dac,		Volt,
	Aoi,		PLL,		Timing,
};

typedef std::map<MultiLineCmd, std::string> MultiLineCmdStrMapType;
extern MultiLineCmdStrMapType MultiLineCmdStrMap;


enum FrameTransferMode {
	FFM = 0, FTM = 1,
};

enum InputChan {
	Chan1    = (1 << 0),
	Chan2    = (1 << 1),
	Chan3    = (1 << 2),
	Chan4    = (1 << 3),
	Chan13   = Chan1  | Chan3,
	Chan24   = Chan2  | Chan4,
	Chan12   = Chan1  | Chan2,
	Chan34   = Chan3  | Chan4,
	Chan1234 = Chan12 | Chan34,
};

typedef std::pair<int, int> ChanRange;
typedef std::map<FrameTransferMode, ChanRange> FTMChanRangeMapType;
extern FTMChanRangeMapType FTMChanRangeMap;

typedef std::vector<InputChan> InputChanList;

typedef std::map<FrameTransferMode, InputChanList> FTMInputChanListMapType;
extern FTMInputChanListMapType FTMInputChanListMap;

extern InputChanList DefInputChanList;


enum SerNbParam {
	SerNb      = 0x00ff,
	SPB1Kodak  = 0x2000,
	SPB1Adc16  = 0x4000,
	SPB2Sign   = 0x0100,
	SPB2Type   = 0x7000,
	Taper      = 0x8000,
};

enum RoiMode {
	None, Slow, Fast, Kinetic,
};

enum TimeUnitFactor {
	Milliseconds, Microseconds,
};

typedef std::map<TimeUnitFactor, double> TimeUnitFactorMapType;
extern TimeUnitFactorMapType TimeUnitFactorMap;


extern const FrameDim MaxFrameDim;

enum ChipType {
	Atmel,
	Kodak,
	E2V,
};

typedef std::map<ChipType, double> ChipPixelSizeMapType;
extern ChipPixelSizeMapType ChipPixelSizeMap;

enum {
	AtmelModesAvail = 0x0fff,
	KodakModesAvail = 0x0100,
};

enum {
	MaxBinX = 8,
	MaxBinY = 1024,
};

enum Status {
        Wait       = 0x80,
        Transfer   = 0x40,
        Exposure   = 0x20,
        Shutter    = 0x10,
        Readout    = 0x08,
        Latency    = 0x04,
        ExtStart   = 0x02,
        ExtStop    = 0x01,
	StatusMask = 0xff,
};

enum ShutMode {
	Off, AutoFrame,
};


} // namespace Frelon

} // namespace lima

#endif // FRELON_H
