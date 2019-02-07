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
#ifndef FRELONMODEL_H
#define FRELONMODEL_H

#include "Frelon.h"

namespace lima
{

namespace Frelon
{

class Firmware
{
	DEB_CLASS_NAMESPC(DebModCamera, "Firmware", "Frelon");

 public:
	Firmware();
	Firmware(const std::string& ver);
	~Firmware();

	void setVersionStr(const std::string& ver);
	void getVersionStr(std::string& ver) const;

	void reset();
	bool isValid() const;

	int getMajor() const; 
	int getMinor() const;
	std::string getRelease() const;

	static const Firmware v2_0c;
	static const Firmware v2_1b;
	static const Firmware v3_0i;
	static const Firmware v3_1c;
	static const Firmware v4_1;

 private:
	void checkValid();

	int m_major;
	int m_minor;
	std::string m_rel;
};

inline bool operator ==(const Firmware& f1, const Firmware& f2)
{
	return ((f1.getMajor() == f2.getMajor()) && 
		(f1.getMinor() == f2.getMinor()) &&
		(f1.getRelease() == f2.getRelease()));
}

inline bool operator !=(const Firmware& f1, const Firmware& f2)
{
	return !(f1 == f2);
}

inline bool operator <(const Firmware& f1, const Firmware& f2)
{
	if (f1.getMajor() < f2.getMajor())
		return true;
	if (f1.getMajor() > f2.getMajor())
		return false;
	if (f1.getMinor() < f2.getMinor())
		return true;
	if (f1.getMinor() > f2.getMinor())
		return false;
	return (f1.getRelease() < f2.getRelease());
}

inline bool operator >(const Firmware& f1, const Firmware& f2)
{
	return !((f1 == f2) || (f1 < f2));
}

inline bool operator <=(const Firmware& f1, const Firmware& f2)
{
	return ((f1 == f2) || (f1 < f2));
}

inline bool operator >=(const Firmware& f1, const Firmware& f2)
{
	return !(f1 < f2);
}


class Model
{
	DEB_CLASS_NAMESPC(DebModCamera, "Model", "Frelon");

 public:
	enum Feature {
		Taper, HamaChip,
		ModesAvail, TimeCalc, HTDCmd, GoodHTD, ImagesPerEOF, CamChar,
		SPB1, SPB2, SPB8,
	};

	Model();
	~Model();

	void setVersionStr(const std::string& ver);
	const Firmware& getFirmware();

	void setComplexSerialNb(int  complex_ser_nb);
	void getComplexSerialNb(int& complex_ser_nb);

	void setCamChar(int  cam_char);
	void getCamChar(int& cam_char);

	void setF16ForceSingle(bool  f16_force_single);
	void getF16ForceSingle(bool& f16_force_single);

	void reset();
	bool isValid();

	int  getSerialNb();
	SPBType getSPBType();
	int  getAdcBits();
	ChipType getChipType();
	SPBConType getSPBConType();
	GeomType getGeomType();
	bool has(Feature feature);
	double getPixelSize();

	std::string getName();

 private:
	typedef std::map<Feature, bool> FeatureMap;

	void update();
	void checkValid();
	int getSerialNbParam(SerNbParam param);

	Firmware m_firmware;
	int m_complex_ser_nb;
	int m_cam_char;
	bool m_f16_force_single;

	bool       m_valid;
	SPBType    m_spb_type;
	ChipType   m_chip_type;
	SPBConType m_spb_con_type;
	FeatureMap m_feature;
};


} // namespace Frelon

} // namespace lima


#endif // FRELONMODEL_H
