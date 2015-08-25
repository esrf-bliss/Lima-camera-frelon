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
#include "Frelon.h"
#include "MiscUtils.h"

using namespace lima;
using namespace lima::Frelon;
using namespace std;

typedef pair<Reg, string> RegPair;
static const RegPair RegStrCList[] = {
	RegPair(NbFrames,	"N"),
	RegPair(ExpTime,	"I"),
	RegPair(ShutCloseTime,	"F"),
	RegPair(LatencyTime,	"T"),
	RegPair(RoiLineBegin,	"RLB"),
	RegPair(RoiLineWidth,	"RLW"),
	RegPair(RoiPixelBegin,	"RPB"),
	RegPair(RoiPixelWidth,	"RPW"),
	RegPair(ChanMode,	"M"),
	RegPair(TimeUnit,	"Z"),
	RegPair(RoiEnable,	"R"),
	RegPair(RoiFast,	"RF"),
	RegPair(AntiBloom,	"BL"),
	RegPair(BinVert,	"BV"),
	RegPair(BinHorz,	"BH"),
	RegPair(ConfigHD,	"CNF"),
	RegPair(RoiKinetic,	"SPE"),
	RegPair(ShutEnable,	"U"),
	RegPair(HardTrigDisable,"HTD"),

	RegPair(PixelFreq,	"P"),
	RegPair(LineFreq,	"L"),

	RegPair(FlipMode,	"FLI"),
	RegPair(IntCalib,	"IE"),
	RegPair(DisplayImage,	"X"),
	RegPair(AdcFloatDiode,	"ADS"),
	RegPair(AdcSignal,	"ASS"),
	RegPair(DarkPixelCalib,	"DPE"),
	RegPair(DarkPixelMode,	"DPM"),
	RegPair(ChanControl,	"CCS"),
	RegPair(Mire,		"MIR"),

	RegPair(AoiLineBegin,	"ALB"),
	RegPair(AoiLineWidth,	"ALW"),
	RegPair(AoiPixelBegin,	"APB"),
	RegPair(AoiPixelWidth,	"APW"),
	RegPair(AoiImageHeight,	"IMH"),
	RegPair(AoiImageWidth,	"IMW"),
	RegPair(ChanOnImage,	"COI"),
	RegPair(ChanOnCcd,	"COC"),

	RegPair(Version,	"VER"),
	RegPair(CompSerNb,	"SN"),
	RegPair(Warn,		"W"),
	RegPair(LastWarn,	"LW"),

	RegPair(LineClockPer,	"TLC"),
	RegPair(PixelClockPer,	"TPC"),
	RegPair(FirstPHIVLen,	"TFV"),
	RegPair(PHIHSetupLen,	"THS"),
	RegPair(SingleVertXfer,	"TOV"),
	RegPair(SingleHorzXfer,	"TOH"),
	RegPair(AllVertXfer,	"TAV"),
	RegPair(AllHorzXfer,	"TAH"),
	RegPair(ReadoutTime,	"TRD"),
	RegPair(TransferTime,	"TTR"),
	RegPair(CcdModesAvail,	"CMA"),

	RegPair(StatusSeqA,	"SSA"),
	RegPair(StatusAMTA,	"SAA"),
	RegPair(StatusAMTB,	"SAB"),
	RegPair(StatusAMTC,	"SAC"),
	RegPair(StatusAMTD,	"SAD"),

	RegPair(LookUpTable,	"LUT"),
	RegPair(ImagesPerEOF,	"NEF"),
};
RegStrMapType lima::Frelon::RegStrMap(C_LIST_ITERS(RegStrCList));

static Reg CacheableRegCList[] = {
	NbFrames,	ExpTime,	ShutCloseTime,	LatencyTime,
	RoiLineBegin,	RoiLineWidth,	RoiPixelBegin,	RoiPixelWidth,
	ChanMode,	TimeUnit,	RoiEnable,	RoiFast,
	RoiKinetic,	BinVert,	BinHorz,	ConfigHD,
	ShutEnable,	HardTrigDisable, FlipMode,	CompSerNb,
	CcdModesAvail,	ImagesPerEOF,
};
RegListType 
lima::Frelon::CacheableRegList(C_LIST_ITERS(CacheableRegCList));

static Reg FloatRegCList[] = {
	ReadoutTime,
};
RegListType lima::Frelon::FloatRegList(C_LIST_ITERS(FloatRegCList));

typedef pair<Reg, double> RegSleepPair;
static const RegSleepPair RegSleepCList[] = {
	RegSleepPair(ConfigHD,    2.0),
	RegSleepPair(BinHorz,     2.0),
	RegSleepPair(LookUpTable, 2.0),
};
RegDoubleMapType lima::Frelon::RegSleepMap(C_LIST_ITERS(RegSleepCList));

const int lima::Frelon::MaxRegVal = (1 << 16) - 1;

typedef pair<Cmd, string> CmdPair;
static const CmdPair CmdStrCList[] = {
	CmdPair(Reset,		"RST"),
	CmdPair(Start,		"S"),
	CmdPair(Stop,		"O"),
	CmdPair(Save,		"SAV"),
	CmdPair(Reload,		"RLD"),
	CmdPair(SendEOF,	"SEF"),
};
CmdStrMapType lima::Frelon::CmdStrMap(C_LIST_ITERS(CmdStrCList));


typedef pair<MultiLineCmd, string> MLCmdPair;
static const MLCmdPair MLCmdStrCList[] = {
	MLCmdPair(Help,		"H"),
	MLCmdPair(Config,	"C"),
	MLCmdPair(Dac,		"D"),
	MLCmdPair(Volt,		"V"),
	MLCmdPair(Aoi,		"AOI"),
	MLCmdPair(PLL,		"PLL"),
	MLCmdPair(Timing,	"TIM"),
	MLCmdPair(StatusCam,	"STC"),
};
MultiLineCmdStrMapType 
lima::Frelon::MultiLineCmdStrMap(C_LIST_ITERS(MLCmdStrCList));


typedef pair<FrameTransferMode, string> FTMStrPair;
static const FTMStrPair FTMNameCList[] = {
	FTMStrPair(FFM, "FFM"),
	FTMStrPair(FTM, "FTM"),
};
FTMStrMapType lima::Frelon::FTMNameMap(C_LIST_ITERS(FTMNameCList));

typedef pair<FrameTransferMode, ChanRange> RangePair;
static const RangePair FTMChanRangeCList[] = {
	RangePair(FFM, ChanRange(1,  10)),
	RangePair(FTM, ChanRange(10, 17)),
};
FTMChanRangeMapType 
lima::Frelon::FTMChanRangeMap(C_LIST_ITERS(FTMChanRangeCList));


static const InputChan FFMInputChanCList[] = {
	Chan1, Chan2, Chan3, Chan4, Chan13, Chan24, Chan12, Chan34, Chan1234,
};
static const InputChan FTMInputChanCList[] = {
	Chan1234, Chan34, Chan12, Chan24, Chan13, Chan4, Chan1,
};
typedef pair<FrameTransferMode, InputChanList> InputChanPair;
static const InputChanPair FTMInputChanListCList[] = {
	InputChanPair(FFM, InputChanList(C_LIST_ITERS(FFMInputChanCList))),
	InputChanPair(FTM, InputChanList(C_LIST_ITERS(FTMInputChanCList))),
};
FTMInputChanListMapType 
lima::Frelon::FTMInputChanListMap(C_LIST_ITERS(FTMInputChanListCList));

static const InputChan DefInputChanCList[] = {
	Chan1234, Chan12, Chan34, Chan13, Chan24, Chan1, Chan2, Chan3, Chan4, 
};
InputChanList lima::Frelon::DefInputChanList(C_LIST_ITERS(DefInputChanCList));

typedef pair<TimeUnitFactor, double> FactorPair;
static const FactorPair TimeUnitFactorCList[] = {
	FactorPair(Milliseconds, 1e-3),
	FactorPair(Microseconds, 1e-6),
};
TimeUnitFactorMapType 
lima::Frelon::TimeUnitFactorMap(C_LIST_ITERS(TimeUnitFactorCList));

typedef pair<ChipType, FrameDim> ChipFrameDimPair;
static const ChipFrameDimPair ChipMaxFrameDimCList[] = {
	ChipFrameDimPair(Atmel,        FrameDim(2048, 2048, Bpp16)),
	ChipFrameDimPair(Kodak,        FrameDim(2048, 2048, Bpp16)),
	ChipFrameDimPair(E2V_2k,       FrameDim(2048, 2048, Bpp16)),
	ChipFrameDimPair(E2V_2kNotMPP, FrameDim(2048, 2048, Bpp16)),
	ChipFrameDimPair(E2V_4k,       FrameDim(4096, 4096, Bpp16)),
	ChipFrameDimPair(E2V_4kNotMPP, FrameDim(4096, 4096, Bpp16)),
	ChipFrameDimPair(Hama,         FrameDim(2048,    1, Bpp16)),
};
ChipMaxFrameDimMapType 
lima::Frelon::ChipMaxFrameDimMap(C_LIST_ITERS(ChipMaxFrameDimCList));

typedef pair<ChipType, double> ChipSizePair;
static const ChipSizePair ChipPixelSizeCList[] = {
	ChipSizePair(Atmel,        14e-6),
	ChipSizePair(Kodak,        24e-6),
	ChipSizePair(E2V_2k,       15e-6),
	ChipSizePair(E2V_2kNotMPP, 15e-6),
	ChipSizePair(E2V_4k,       15e-6),
	ChipSizePair(E2V_4kNotMPP, 15e-6),
	ChipSizePair(Hama,         14e-6),
};
ChipPixelSizeMapType 
lima::Frelon::ChipPixelSizeMap(C_LIST_ITERS(ChipPixelSizeCList));

typedef pair<SPB2Config, string> SPB2ConfigStrPair;
static const SPB2ConfigStrPair SPB2ConfigNameCList[] = {
	SPB2ConfigStrPair(SPB2Precision, "Precision"),
	SPB2ConfigStrPair(SPB2Speed    , "Speed"),
};
SPB2ConfigStrMapType
lima::Frelon::SPB2ConfigNameMap(C_LIST_ITERS(SPB2ConfigNameCList));
