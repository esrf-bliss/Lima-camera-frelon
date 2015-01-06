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

using namespace lima;
using namespace lima::Frelon;
using namespace std;

const Firmware Firmware::v2_0c("2.0c");
const Firmware Firmware::v2_1b("2.1b");
const Firmware Firmware::v3_0i("3.0i");
const Firmware Firmware::v3_1c("3.1c");

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

void Model::reset()
{
	DEB_MEMBER_FUNCT();

	m_firmware.reset();
	m_complex_ser_nb = 0;
	update();
}

void Model::update()
{
	DEB_MEMBER_FUNCT();

	m_valid = (m_complex_ser_nb > 0) && m_firmware.isValid();
	DEB_TRACE() << DEB_VAR1(m_valid);
	if (!m_valid)
		return;

	bool is_spb2 = isSPB2();
	if (is_spb2)
		m_chip_type = ChipType(getSerialNbParam(SPB2Type) >> 12);
	else
		m_chip_type = bool(getSerialNbParam(SPB1Kodak)) ? Kodak : Atmel;
	m_is_hama = (m_chip_type == Hama);

	bool firm_v2_0c = (is_spb2 && (m_firmware >= Firmware::v2_0c));
	m_htd_cmd = firm_v2_0c;

	bool firm_v2_1b = (is_spb2 && (m_firmware >= Firmware::v2_1b));
	m_modes_avail = m_time_calc = firm_v2_1b;

	bool firm_v3_0i = (is_spb2 && (m_firmware >= Firmware::v3_0i));
	m_good_htd = firm_v3_0i;

	bool firm_v3_1c = (is_spb2 && (m_firmware >= Firmware::v3_1c));
	m_images_per_eof = firm_v3_1c;

	DEB_TRACE() << DEB_VAR6(m_chip_type, m_is_hama, m_modes_avail, 
				m_time_calc, m_good_htd, m_images_per_eof);
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

bool Model::isSPB1()
{
	DEB_MEMBER_FUNCT();

	bool frelon_spb1 = !isSPB2();
	DEB_RETURN() << DEB_VAR1(frelon_spb1);
	return frelon_spb1;
}
	
bool Model::isSPB2()
{
	DEB_MEMBER_FUNCT();
	checkValid();

	bool frelon_spb2 = bool(getSerialNbParam(SPB2Sign));
	DEB_RETURN() << DEB_VAR1(frelon_spb2);
	return frelon_spb2;
}
	
int Model::getAdcBits()
{
	DEB_MEMBER_FUNCT();

	int adc_bits;
	if (isSPB1())
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

bool Model::isHama()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_is_hama);
	return m_is_hama;
}

bool Model::hasTaper()
{
	DEB_MEMBER_FUNCT();

	bool taper = bool(getSerialNbParam(Taper));
	DEB_RETURN() << DEB_VAR1(taper);
	return taper;
}

bool Model::hasModesAvail()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_modes_avail);
	return m_modes_avail;
}

bool Model::hasTimeCalc()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_time_calc);
	return m_time_calc;
}

bool Model::hasHTDCmd()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_htd_cmd);
	return m_htd_cmd;
}

bool Model::hasGoodHTD()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_good_htd);
	return m_good_htd;
}

bool Model::hasImagesPerEOF()
{
	DEB_MEMBER_FUNCT();

	checkValid();

	DEB_RETURN() << DEB_VAR1(m_images_per_eof);
	return m_images_per_eof;
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
	switch (getChipType()) {
	case Atmel:        chip_model = "A7899";   break;
	case Kodak:        chip_model = "K4320";   break;
	case E2V_2k:       chip_model = "E230-42"; break;
	case E2V_2kNotMPP: chip_model = "E231-42"; break;
	case E2V_4k:       chip_model = "E230-84"; break;
	case E2V_4kNotMPP: chip_model = "E231-84"; break;
	case Hama:         chip_model = "Hama";    break;
	default:           chip_model = "Unknown";
	}

	string hd = isSPB2() ? "HD " : "";
	string name = hd + chip_model;

	if (isSPB1() && (getAdcBits() == 16))
		name += " 16bit";

	if (hasTaper())
		name += "T";

	DEB_RETURN() << DEB_VAR1(name);
	return name;
}
