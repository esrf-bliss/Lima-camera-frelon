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
	ReadoutTime,	TransferTime,   CcdModesAvail,	StatusSeqA,
	StatusAMTA,	StatusAMTB,	StatusAMTC,	StatusAMTD,
	StatusAMTE,
	LookUpTable,	ImagesPerEOF,	WeightValDFl,	WeightValSig,
	SeqClockFreq,	CamChar,
};

typedef std::map<Reg, std::string> RegStrMapType;
extern RegStrMapType RegStrMap;

typedef std::vector<Reg> RegListType;
extern RegListType CacheableRegList;
extern RegListType FloatRegList;
extern RegListType SignedRegList;

typedef std::map<Reg, double> RegDoubleMapType;
extern RegDoubleMapType RegSleepMap;
extern RegDoubleMapType RegTimeoutMap;

extern const int MaxRegVal;

enum Cmd {
	Reset,		Start,		Stop,		Save,		Reload,
	SendEOF,
};

typedef std::map<Cmd, std::string> CmdStrMapType;
extern CmdStrMapType CmdStrMap;


enum MultiLineCmd {
	Help,		Config,		Dac,		MonitorVolt,
	Aoi,		PLL,		Timing,		StatusCam,
	SampWeight,	ConfigSeq,	ConfigSPB,	ConfigDLine,
	ConfigDAC,	ConfigADC,	ConfigPLL,	ConfigSWeight,
	ConfigVCXO,	ConfigAlarm,	ConfigAoi,	UserInfo,
	MonitorADC,
};

typedef std::map<MultiLineCmd, std::string> MultiLineCmdStrMapType;
extern MultiLineCmdStrMapType MultiLineCmdStrMap;


enum FrameTransferMode {
	FFM = 0, FTM = 1,
};

typedef std::map<FrameTransferMode, std::string> FTMStrMapType;
extern FTMStrMapType FTMNameMap;

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
	SerNb        = 0x00ff,
	SPB1Kodak    = 0x2000,
	SPB1Adc16    = 0x4000,
	SPBTypeMask  = 0x0300,
	ChipTypeMask = 0x7800,
	TaperFlag    = 0x8000,
};

enum RoiMode {
	None, Slow, Fast, Kinetic,
};

enum TimeUnitFactor {
	Milliseconds, Microseconds,
};

typedef std::map<TimeUnitFactor, double> TimeUnitFactorMapType;
extern TimeUnitFactorMapType TimeUnitFactorMap;


enum SPBType {
	SPBType1, SPBType2, SPBType8,
};

enum ChipType {
	Atmel,
	Kodak,
	E2V_2k,
	E2V_2kNotMPP,
	E2V_4k,
	E2V_4kNotMPP,
	Hama,
	Andanta_CcdFT2k = 8,
};

enum SPBConType {
	SPBConNone, SPBConX, SPBConY, SPBConXY,
};

enum GeomType {
	SPB12_4_Quad,
	Hamamatsu,
	SPB8_F16_Single,
	SPB8_F16_Dual,
};

typedef std::map<ChipType, FrameDim> ChipMaxFrameDimMapType;
extern ChipMaxFrameDimMapType ChipMaxFrameDimMap;

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

enum ExtSync {
	ExtSyncNone  = 0,
	ExtSyncStart = 1,
	ExtSyncStop  = 2,
	ExtSyncBoth  = 3,
};

enum Status {
	InInit     = 0x200,
	EspiaXfer  = 0x100,
        Wait       = 0x080,
        Transfer   = 0x040,
        Exposure   = 0x020,
        Shutter    = 0x010,
        Readout    = 0x008,
        Latency    = 0x004,
        ExtStart   = 0x002,
        ExtStop    = 0x001,
	StatusMask = 0x3ff,
};

enum StatusSPB2_SAA {
	SPB2_SAA_GBitFifoPixEmpty = 0x8000,
	SPB2_SAA_FifoOutEmpty     = 0x4000,
	SPB2_SAA_FifoInEmpty      = 0x2000,
	SPB2_SAA_FifoLUTEmpty     = 0x1000,
	SPB2_SAA_FifoChan4Empty   = 0x0800,
	SPB2_SAA_FifoChan3Empty   = 0x0400,
	SPB2_SAA_FifoChan2Empty   = 0x0200,
	SPB2_SAA_FifoChan1Empty   = 0x0100,
	SPB2_SAA_FifoEmptyMask    = 0xff00,
	SPB2_SAA_TstEnvFrmOut     = 0x0040,
	SPB2_SAA_TstEnvFrmIn      = 0x0020,
	SPB2_SAA_TstEnvMask       = 0x0060,
	SPB2_SAA_TstFlashUsrReady = 0x0010,
	SPB2_SAA_EndInitRam       = 0x0008,
	SPB2_SAA_EndFlashRead     = 0x0004,
	SPB2_SAA_AuroraChanUp     = 0x0002,
	SPB2_SAA_DcmLocked        = 0x0001,
	SPB2_SAA_TstInitMask      = 0x001f,
	SPB2_SAA_TstInitGood      = 0x001f,
};

enum StatusSPB8_SAA {
	SPB8_SAA_PLL8Locked   = 0x8000,
	SPB8_SAA_PLL7Locked   = 0x4000,
	SPB8_SAA_PLL6Locked   = 0x2000,
	SPB8_SAA_PLL5Locked   = 0x1000,
	SPB8_SAA_PLL4Locked   = 0x0800,
	SPB8_SAA_PLL3Locked   = 0x0400,
	SPB8_SAA_PLL2Locked   = 0x0200,
	SPB8_SAA_PLL1Locked   = 0x0100,
	SPB8_SAA_DCMLocked    = 0x0020,
	SPB8_SAA_AuroraChanUp = 0x0010,
	SPB8_SAA_DDR1CalDone  = 0x0002,
	SPB8_SAA_DDR0CalDone  = 0x0001,
	SPB8_SAA_TstInitMask  = 0xff33,
	SPB8_SAA_TstInitGood  = 0xff33,
};

enum StatusSPB8_SAE {
	SPB8_SAE_TstEnvFrmOut   = 0x8000,
	SPB8_SAE_TstEnvFrmIn    = 0x4000,
	SPB8_SAE_TstEnvMask     = 0xc000,
	SPB8_SAE_FifoTXAEmpty   = 0x0800,
	SPB8_SAE_FifoOutEmpty   = 0x0400,
	SPB8_SAE_FifoInEmpty    = 0x0200,
	SPB8_SAE_FifoMuxEmpty   = 0x0100,
	SPB8_SAE_FifoChan8Empty = 0x0080,
	SPB8_SAE_FifoChan7Empty = 0x0040,
	SPB8_SAE_FifoChan6Empty = 0x0020,
	SPB8_SAE_FifoChan5Empty = 0x0010,
	SPB8_SAE_FifoChan4Empty = 0x0008,
	SPB8_SAE_FifoChan3Empty = 0x0004,
	SPB8_SAE_FifoChan2Empty = 0x0002,
	SPB8_SAE_FifoChan1Empty = 0x0001,
};

enum ShutMode {
	Off, AutoFrame,
};

enum SPB2Config {
	SPB2Precision, SPB2Speed,
};
typedef std::map<SPB2Config, std::string> SPB2ConfigStrMapType;
extern SPB2ConfigStrMapType SPB2ConfigNameMap;

} // namespace Frelon

} // namespace lima

#endif // FRELON_H
