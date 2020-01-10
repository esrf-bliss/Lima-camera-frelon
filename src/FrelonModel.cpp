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
#include "FrelonModel.h"
#include "lima/RegExUtils.h"

#include <stdlib.h>

using namespace lima;
using namespace lima::Frelon;
using namespace std;

const Firmware Firmware::v2_0c("2.0c");
const Firmware Firmware::v2_1b("2.1b");
const Firmware Firmware::v3_0i("3.0i");
const Firmware Firmware::v3_1c("3.1c");
const Firmware Firmware::v4_1("4.1");

Firmware::Firmware()
{
	DEB_CONSTRUCTOR();

	reset();
}


Firmware::Firmware(const string& ver)
{
	DEB_CONSTRUCTOR();
	DEB_PARAM() << DEB_VAR1(ver);

	reset();
	setVersionStr(ver);
}


Firmware::~Firmware()
{
	DEB_DESTRUCTOR();

	reset();
}


void Firmware::reset()
{
	DEB_MEMBER_FUNCT();

	m_major = m_minor = 0;
	m_rel.clear();
}

void Firmware::setVersionStr(const string& ver)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(ver);

	RegEx re("(?P<major>[0-9]+)\\.(?P<minor>[0-9]+)(?P<rel>[a-z]+)?");
	RegEx::FullNameMatchType match;

	if (!re.matchName(ver, match))
		THROW_HW_ERROR(InvalidValue) << "Invalid firmware "
					     << DEB_VAR1(ver);

	m_major = atoi(string(match["major"]).c_str());
	m_minor = atoi(string(match["minor"]).c_str());
	m_rel = match["rel"];

	if (!isValid()) {
		reset();
		THROW_HW_ERROR(InvalidValue) << "Invalid firmware "
					     << DEB_VAR1(ver);
	}
}

void Firmware::getVersionStr(string& ver) const
{
	DEB_MEMBER_FUNCT();

	ostringstream os;
	if (isValid())
		os << m_major << "." << m_minor << m_rel;
	else
		os << "Unknown";

	ver = os.str();
	DEB_RETURN() << DEB_VAR1(ver);
}

bool Firmware::isValid() const
{
	DEB_MEMBER_FUNCT();

	bool valid = (m_major > 0) || (m_minor > 0);
	DEB_RETURN() << DEB_VAR1(valid);
	return valid;
}

int Firmware::getMajor() const
{
	return m_major;
}

int Firmware::getMinor() const
{
	return m_minor;
}

string Firmware::getRelease() const
{
	return m_rel;
}

void Firmware::checkValid()
{
	DEB_MEMBER_FUNCT();

	if (!isValid())
		THROW_HW_ERROR(InvalidValue) 
			<< "Frelon Firmware not fully initialised yet";
}

Model::Model()
	: m_f16_force_single(false)
{
	DEB_CONSTRUCTOR();

	reset();
}

Model::~Model()
{
	DEB_DESTRUCTOR();
}

void Model::setVersionStr(const std::string& ver)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(ver);

	m_valid = false;
	m_firmware.setVersionStr(ver);
	update();
}

const Firmware& Model::getFirmware()
{
	return m_firmware;
}

void Model::setComplexSerialNb(int complex_ser_nb)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(DEB_HEX(complex_ser_nb));

	m_complex_ser_nb = complex_ser_nb;
	update();
}

void Model::getComplexSerialNb(int& complex_ser_nb)
{
	DEB_MEMBER_FUNCT();
	complex_ser_nb = m_complex_ser_nb;
	DEB_RETURN() << DEB_VAR1(DEB_HEX(complex_ser_nb));
}

void Model::setCamChar(int cam_char)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(DEB_HEX(cam_char));

	m_cam_char = cam_char;
	update();
}

void Model::getCamChar(int& cam_char)
{
	DEB_MEMBER_FUNCT();
	cam_char = m_cam_char;
	DEB_RETURN() << DEB_VAR1(DEB_HEX(cam_char));
}

void Model::setF16ForceSingle(bool f16_force_single)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(f16_force_single);

	m_f16_force_single = f16_force_single;
	update();
}

void Model::getF16ForceSingle(bool& f16_force_single)
{
	DEB_MEMBER_FUNCT();
	f16_force_single = m_f16_force_single;
	DEB_RETURN() << DEB_VAR1(f16_force_single);
}

void Model::reset()
{
	DEB_MEMBER_FUNCT();

	m_firmware.reset();
	m_complex_ser_nb = 0;
	m_cam_char = 0;

	update();
}

void Model::update()
{
	DEB_MEMBER_FUNCT();

	m_valid = (m_complex_ser_nb > 0) && m_firmware.isValid();
	DEB_TRACE() << DEB_VAR1(m_valid);
	if (!m_valid)
		return;

	m_spb_type = SPBType(getSerialNbParam(SPBTypeMask) >> 8);
	m_feature[SPB1] = (m_spb_type == SPBType1);
	m_feature[SPB2] = (m_spb_type == SPBType2);
	m_feature[SPB8] = (m_spb_type == SPBType8);

	m_feature[Taper] = bool(getSerialNbParam(TaperFlag));

	bool is_spb1 = has(SPB1);
	if (!is_spb1) {
		int raw = getSerialNbParam(ChipTypeMask) >> 11;
		m_chip_type = ChipType(((raw & 1) << 3) | (raw >> 1));
	} else {
		m_chip_type = bool(getSerialNbParam(SPB1Kodak)) ? Kodak : Atmel;
	}
	m_feature[HamaChip] = (m_chip_type == Hama);

	bool firm_v2_0c = (!is_spb1 && (m_firmware >= Firmware::v2_0c));
	m_feature[HTDCmd] = firm_v2_0c;

	bool firm_v2_1b = (!is_spb1 && (m_firmware >= Firmware::v2_1b));
	m_feature[ModesAvail] = firm_v2_1b;
	m_feature[TimeCalc] = firm_v2_1b;

	bool firm_v3_0i = (!is_spb1 && (m_firmware >= Firmware::v3_0i));
	m_feature[GoodHTD] = firm_v3_0i;

	bool firm_v3_1c = (!is_spb1 && (m_firmware >= Firmware::v3_1c));
	m_feature[ImagesPerEOF] = firm_v3_1c;

	m_spb_con_type = SPBConX;
	bool firm_v4_1 = (!is_spb1 && (m_firmware >= Firmware::v4_1));
	m_feature[CamChar] = firm_v4_1;
	if (has(CamChar) && m_cam_char) {
		int type_bits = (m_cam_char >> 8) & SPBConXY;
		SPBConType spb_con_type = SPBConType(type_bits);
		if ((spb_con_type == SPBConXY) && m_f16_force_single) {
			DEB_TRACE() << "Limiting to single SPB8";
			spb_con_type = SPBConX;
		}
		if (spb_con_type != SPBConNone)
			m_spb_con_type = spb_con_type;
	}

	m_feature[SeqTim] = firm_v4_1;
	if (has(SeqTim))
		m_feature[TimeCalc] = 0;

	DEB_TRACE() << DEB_VAR3(m_spb_type, m_spb_con_type, m_chip_type);
	bool has_Taper = has(Taper), has_HamaChip = has(HamaChip);
	bool has_ModesAvail = has(ModesAvail), has_TimeCalc = has(TimeCalc);
	bool has_HTDCmd = has(HTDCmd), has_GoodHTD = has(GoodHTD);
	bool has_ImagesPerEOF = has(ImagesPerEOF), has_CamChar = has(CamChar);
	bool has_SeqTim = has(SeqTim);
	DEB_TRACE() << DEB_VAR6(has_Taper, has_HamaChip, has_ModesAvail, 
				has_TimeCalc, has_HTDCmd, has_GoodHTD);
	DEB_TRACE() << DEB_VAR3(has_ImagesPerEOF, has_CamChar, has_SeqTim);
}

bool Model::isValid()
{
	DEB_MEMBER_FUNCT();
	DEB_RETURN() << DEB_VAR1(m_valid);
	return m_valid;
}

void Model::checkValid()
{
	DEB_MEMBER_FUNCT();

	if (!m_valid)
		THROW_HW_ERROR(InvalidValue) 
			<< "Frelon model not fully initialised yet";
}

int Model::getSerialNbParam(SerNbParam param)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(DEB_HEX(param));

	int val = m_complex_ser_nb & int(param);
	DEB_RETURN() << DEB_VAR1(val);
	return val;
}

int Model::getSerialNb()
{
	DEB_MEMBER_FUNCT();
	checkValid();

	int ser_nb = getSerialNbParam(SerNb);
	DEB_RETURN() << DEB_VAR1(ser_nb);
	return ser_nb;
}

SPBType Model::getSPBType()
{
	DEB_MEMBER_FUNCT();
	DEB_RETURN() << DEB_VAR1(m_spb_type);
	return m_spb_type;
}

bool Model::has(Feature feature)
{
	DEB_MEMBER_FUNCT();
	bool feature_act = m_feature[feature];
	DEB_RETURN() << DEB_VAR1(feature_act);
	return feature_act;
}
	
int Model::getAdcBits()
{
	DEB_MEMBER_FUNCT();

	int adc_bits;
	if (has(SPB1))
		adc_bits = bool(getSerialNbParam(SPB1Adc16)) ? 16 : 14;
	else
		adc_bits = 16;
	DEB_RETURN() << DEB_VAR1(adc_bits);
	return adc_bits;
}

ChipType Model::getChipType()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_chip_type);
	return m_chip_type;
}

SPBConType Model::getSPBConType()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_spb_con_type);
	return m_spb_con_type;
}

GeomType Model::getGeomType()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	GeomType geom_type = SPB12_4_Quad;
	ChipType chip_type = getChipType();
	if (has(SPB2)) {
		if (chip_type == Hama)
			geom_type = Hamamatsu;
		else if (chip_type == Andanta_CcdFT2k)
			THROW_HW_ERROR(NotSupported) << "Obsolete Frelon16 "
						     << "with SPB2";
	} else if (has(SPB8)) {
		if (chip_type != Andanta_CcdFT2k)
			THROW_HW_ERROR(NotSupported) << "SPB8 only supports "
						     << "Frelon16";
		SPBConType spb_con_type = getSPBConType();
		bool dual_spb = (spb_con_type == SPBConXY);
		geom_type = dual_spb ? SPB8_F16_Dual : SPB8_F16_Single;
	}

	DEB_RETURN() << DEB_VAR1(geom_type);
	return geom_type;
}

double Model::getPixelSize()
{
	DEB_MEMBER_FUNCT();

	ChipType chip_type = getChipType();
	double pixel_size = ChipPixelSizeMap[chip_type];
	DEB_RETURN() << DEB_VAR1(pixel_size);
	return pixel_size;
}

string Model::getName()
{
	DEB_MEMBER_FUNCT();
	checkValid();

	string chip_model;
	ChipType chip_type = getChipType();
	switch (chip_type) {
	case Atmel:           chip_model = "A7899";              break;
	case Kodak:           chip_model = "K4320";              break;
	case E2V_2k:          chip_model = "E230-42";            break;
	case E2V_2kNotMPP:    chip_model = "E231-42";            break;
	case E2V_4k:          chip_model = "E230-84";            break;
	case E2V_4kNotMPP:    chip_model = "E231-84";            break;
	case Hama:            chip_model = "Hama";               break;
	case Andanta_CcdFT2k: chip_model = "CcdFT2k-F16";        break;
	default:              chip_model = "Unknown";
	}

	string prefix = "";
	if (chip_type == Andanta_CcdFT2k) {
		prefix = "SPB";
		if (has(SPB2)) {
			prefix += "2";
		} else {
			prefix += "8";
			if (getSPBConType() == SPBConXY)
				prefix += "x2";
		}
	} else if (has(SPB2)) {
		prefix = "HD";
	}
	string name = prefix + (!prefix.empty() ? " " : "") + chip_model;

	if (has(SPB1) && (getAdcBits() == 16))
		name += " 16bit";

	if (has(Taper))
		name += "T";

	DEB_RETURN() << DEB_VAR1(name);
	return name;
}
