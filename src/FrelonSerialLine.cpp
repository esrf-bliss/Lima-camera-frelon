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
#include "FrelonSerialLine.h"
#include "RegExUtils.h"
#include "MiscUtils.h"
#include <sstream>

using namespace lima;
using namespace lima::Frelon;
using namespace std;

const double SerialLine::TimeoutSingle    = 0.5;
const double SerialLine::TimeoutNormal    = 2.0;
const double SerialLine::TimeoutMultiLine = 3.0;
const double SerialLine::TimeoutReset     = 5.0;


SerialLine::SerialLine(Espia::SerialLine& espia_ser_line)
	: m_espia_ser_line(espia_ser_line)
{
	DEB_CONSTRUCTOR();
	ostringstream os;
	os << "Serial#" << espia_ser_line.getDev().getDevNb();
	DEB_SET_OBJ_NAME(os.str());

	m_espia_ser_line.setLineTerm("\r\n");
	m_espia_ser_line.setTimeout(TimeoutNormal);

	m_last_warn = 0;

	m_curr_op = None;
	m_curr_cache = false;
	m_cache_act = true;

	flush();
}

SerialLine::~SerialLine()
{
	DEB_DESTRUCTOR();
}

Espia::SerialLine& SerialLine::getEspiaSerialLine()
{
	return m_espia_ser_line;
}

void SerialLine::write(const string& buffer, bool no_wait)
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = lock(AutoMutex::Locked);

	while (m_curr_op != None) {
		DEB_TRACE() << "Waiting end of current " << m_curr_op;
		m_cond.wait();
	}

	writeCmd(buffer, no_wait);

	l.leaveLocked();
}

void SerialLine::writeCmd(const string& buffer, bool no_wait)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(no_wait);

	MsgPartStrMapType msg_parts;
	splitMsg(buffer, msg_parts);
	const string& cmd = msg_parts[MsgCmd];

	if (cmd == CmdStrMap[Reset]) {
		m_curr_op = DoReset;
		DEB_TRACE() << "DoReset: clearing reg cache";
		m_reg_cache.clear();
	} else {
		MultiLineCmdStrMapType::const_iterator it, end;
		end = MultiLineCmdStrMap.end();
		it = FindMapValue(MultiLineCmdStrMap, cmd);
		if (it != end)
			m_curr_op = MultiRead;
	}

	bool reg_found = false;
	if (m_curr_op == None) {
		RegStrMapType::const_iterator it, end = RegStrMap.end();
		it = FindMapValue(RegStrMap, cmd);
		reg_found = (it != end);
		if (reg_found)
			m_curr_reg = it->first;
	}

	m_curr_cache = false;
	if (reg_found && isRegCacheable(m_curr_reg)) {
		bool is_req = !msg_parts[MsgReq].empty();
		m_curr_op = is_req ? ReadReg : WriteReg;
		if (!is_req)
			m_curr_resp = msg_parts[MsgVal];
		int cache_int;
		double cache_float;
		if (isFloatReg(m_curr_reg))
			m_curr_cache = getRegCacheVal(m_curr_reg, cache_float);
		else
			m_curr_cache = getRegCacheVal(m_curr_reg, cache_int);
		if (m_curr_cache) {
			ostringstream os;
			if (isFloatReg(m_curr_reg))
				os << cache_float;
			else
				os << cache_int;
			const string& cache_str = os.str();
			if (is_req)
				m_curr_resp = cache_str;
			else
				m_curr_cache = (m_curr_resp == cache_str);
		}
		if (m_curr_cache) {
			DEB_TRACE() << "Skipping " << m_curr_op 
				    << ", already in cache";
			return;
		}
	}

	if (m_curr_op == None)
		m_curr_op = DoCmd;

	DEB_TRACE() << DEB_VAR1(m_curr_op);

	string sync = msg_parts[MsgSync].empty() ? ">" : "";
	string term = msg_parts[MsgTerm].empty() ? "\r\n" : "";
	string msg = sync + buffer + term;

	m_espia_ser_line.write(msg, no_wait);
}

void SerialLine::read(string& buffer, int max_len, double timeout)
{
	DEB_MEMBER_FUNCT();
	m_espia_ser_line.read(buffer, max_len, timeout);
}

void SerialLine::readStr(string& buffer, int max_len, 
			 const string& term, double timeout)
{
	DEB_MEMBER_FUNCT();
	m_espia_ser_line.readStr(buffer, max_len, term, timeout);
}

void SerialLine::readLine(string& buffer, int max_len, double timeout)
{
	DEB_MEMBER_FUNCT();
	AutoMutex l = lock(AutoMutex::PrevLocked);

	readResp(buffer, max_len, timeout);
}

void SerialLine::readResp(string& buffer, int max_len, double timeout)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR3(max_len, timeout, m_curr_op);

	if (m_curr_op == None)
		THROW_HW_FATAL(Error) << "readLine without previous write!";

	ReadRespCleanup read_resp_clenup(*this);

	if ((m_curr_op == MultiRead) && (timeout == TimeoutDefault))
		readMultiLine(buffer, max_len);
	else
		readSingleLine(buffer, max_len, timeout);

}

void SerialLine::readRespCleanup()
{
	DEB_MEMBER_FUNCT();
	m_curr_op = None;
	m_cond.signal();
}

void SerialLine::readSingleLine(string& buffer, int max_len, double timeout)
{
	DEB_MEMBER_FUNCT();

	bool is_req = (m_curr_op == ReadReg);
	if (m_curr_cache) {
		DEB_TRACE() << "Using cache value";
		ostringstream os;
		os << "!OK";
		if (is_req)
			os << ":" << m_curr_resp;
		os << "\r\n";
		buffer = os.str();
		m_curr_fmt_resp = m_curr_resp;
		DEB_TRACE() << DEB_VAR1(m_curr_fmt_resp);
		return;
	}

	if ((m_curr_op == DoReset) && (timeout == TimeoutDefault))
		timeout = TimeoutReset;
	m_espia_ser_line.readLine(buffer, max_len, timeout);

	decodeFmtResp(buffer, m_curr_fmt_resp);

	if (m_curr_op == WriteReg) {
		double sleep_time = getRegSleepTime(m_curr_reg);
		if (sleep_time > 0) {
			DEB_TRACE() << "Sleeping " << sleep_time << " s after "
				    << "changing " << RegStrMap[m_curr_reg];
			Sleep(sleep_time);
		}
	}

	bool reg_op = ((m_curr_op == WriteReg) || (m_curr_op == ReadReg));
	if (!reg_op || !isRegCacheable(m_curr_reg))
		return;

	const string& cache_str = is_req ? m_curr_fmt_resp : m_curr_resp;
	int cache_val;
	istringstream is(cache_str);
	is >> cache_val;
	m_reg_cache[m_curr_reg] = cache_val;
	DEB_TRACE() << "New " << DEB_VAR1(cache_val);
}

void SerialLine::readMultiLine(string& buffer, int max_len)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(max_len);

	Timestamp timeout = Timestamp::now() + Timestamp(TimeoutMultiLine);

	buffer.clear();

	while (Timestamp::now() < timeout) {
		string ans;
		int len = max_len - buffer.size();
		try {
			DEB_TRACE() << "Atempting to read: " << DEB_VAR1(len);
			m_espia_ser_line.readLine(ans, len, TimeoutSingle);
			buffer += ans;
		} catch (Exception e) {
			if (!buffer.empty())
				break;
		}
	}

	if (buffer.empty())
		THROW_HW_ERROR(Error) << "Timeout reading Frelon multi-line";
}

bool SerialLine::isRegCacheable(Reg reg)
{
	DEB_MEMBER_FUNCT();

	if (!m_cache_act) {
		DEB_TRACE() << "Reg cache is disabled";
		return false;
	}

	const RegListType& list = CacheableRegList;
	bool cacheable = (find(list.begin(), list.end(), reg) != list.end());
	DEB_RETURN() << DEB_VAR1(cacheable);
	return cacheable;
}


bool SerialLine::isFloatReg(Reg reg)
{
	const RegListType& list = FloatRegList;
	return (find(list.begin(), list.end(), reg) != list.end());
}

namespace lima
{
namespace Frelon
{

template <>
void SerialLine::checkRegType<int>(Reg reg)
{
	DEB_MEMBER_FUNCT();
	if (isFloatReg(reg))
		THROW_HW_ERROR(InvalidValue) << "Invalid int-call for a "
						"float-register";
}

template <>
void SerialLine::checkRegType<double>(Reg reg)
{
}

template <class T>
bool SerialLine::getRegCacheVal(Reg reg, T& val)
{
	DEB_MEMBER_FUNCT();
	checkRegType<T>(reg);
	RegValMapType::const_iterator it = m_reg_cache.find(reg);
	bool in_cache = (it != m_reg_cache.end());
	val = in_cache ? it->second : 0;
	DEB_RETURN() << DEB_VAR2(in_cache, val);
	return in_cache;
}

template <class T>
bool SerialLine::getRegCacheValSafe(Reg reg, T& val)
{
	AutoMutex l = lock(AutoMutex::Locked);
	return (isRegCacheable(reg) && getRegCacheVal(reg, val));
}

template <class T>
void SerialLine::readCameraRegister(Reg reg, T& val)
{
	DEB_MEMBER_FUNCT();
	const string& reg_str = RegStrMap[reg];
	DEB_PARAM() << DEB_VAR2(reg, reg_str);

	bool in_cache = getRegCacheValSafe(reg, val);
	if (in_cache) {
		DEB_TRACE() << "Using cache value";
		DEB_RETURN() << DEB_VAR1(val);
		return;
	} 

	if (reg_str.empty())
		THROW_HW_ERROR(InvalidValue) << "Invalid " << DEB_VAR1(reg);

	string resp;
	sendFmtCmd(reg_str + "?", resp);
	istringstream is(resp);
	is >> val;
	DEB_RETURN() << DEB_VAR1(val);
}

template void SerialLine::readCameraRegister<int>(Reg reg, int& val);
template void SerialLine::readCameraRegister<double>(Reg reg, double& val);


} // namespace Frelon
} // namespace lima

double SerialLine::getRegSleepTime(Reg reg)
{
	DEB_MEMBER_FUNCT();
	RegDoubleMapType::const_iterator it = RegSleepMap.find(reg);
	bool in_map = (it != RegSleepMap.end());
	double sleep_time = in_map ? it->second : 0;
	DEB_RETURN() << DEB_VAR1(sleep_time);
	return sleep_time;
}

void SerialLine::flush()
{
	DEB_MEMBER_FUNCT();
	m_espia_ser_line.flush();
}

void SerialLine::getNbAvailBytes(int &avail)
{
	DEB_MEMBER_FUNCT();
	m_espia_ser_line.getNbAvailBytes(avail);
	DEB_RETURN() << DEB_VAR1(avail);
}

void SerialLine::setTimeout(double timeout)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(timeout);
	m_espia_ser_line.setTimeout(timeout);
}

void SerialLine::getTimeout(double& timeout) const
{
	DEB_MEMBER_FUNCT();
	m_espia_ser_line.getTimeout(timeout);
	DEB_RETURN() << DEB_VAR1(timeout);
}


void SerialLine::splitMsg(const string& msg, 
			  MsgPartStrMapType& msg_parts) const
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(msg);

	msg_parts.clear();

	const static RegEx re("^(?P<sync>>)?"
			      "(?P<cmd>[A-Za-z]+)"
			      "((?P<req>\\?)|"
			      "(?P<val>[0-9]+(\\.(?P<dec>[0-9]+))?))?"
			      "(?P<term>[\r\n]+)?$");

	RegEx::FullNameMatchType match;
	if (!re.matchName(msg, match))
		THROW_HW_ERROR(InvalidValue) << "Invalid Frelon message: "
					     << DEB_VAR1(msg);

	typedef pair<MsgPart, string> KeyPair;
	static const KeyPair key_list[] = {
		KeyPair(MsgSync, "sync"), KeyPair(MsgCmd, "cmd"), 
		KeyPair(MsgVal,  "val"),  KeyPair(MsgDec, "dec"),  
		KeyPair(MsgReq,  "req"),  KeyPair(MsgTerm, "term"),
	};
	const KeyPair *it, *end = C_LIST_END(key_list);
	for (it = key_list; it != end; ++it) {
		const MsgPart& key = it->first;
		const string&  grp = it->second;
		msg_parts[key] = match[grp];
	}

	DEB_RETURN() << DEB_VAR2(msg_parts[MsgSync], msg_parts[MsgCmd]);
	DEB_RETURN() << DEB_VAR3(msg_parts[MsgReq], msg_parts[MsgVal],
				 msg_parts[MsgDec]);
}

void SerialLine::decodeFmtResp(const string& ans, string& fmt_resp)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(ans);

	fmt_resp.clear();

	const static RegEx re("!(OK(:(?P<resp>[^\r]+))?|"
			        "W\a?:(?P<warn>[^\r]+)|"
			        "E\a?:(?P<err>[^\r]+))\r\n");

	RegEx::FullNameMatchType match;
	if (!re.matchName(ans, match))
		THROW_HW_ERROR(Error) << "Unexpected Frelon answer: "
				      << DEB_VAR1(ans);

	RegEx::SingleMatchType& err = match["err"];
	if (err.found()) 
		THROW_HW_ERROR(Error) << "Frelon Error: " << string(err);

	RegEx::SingleMatchType& warn = match["warn"];
	if (warn.found()) {
		string warn_str = warn;
		DEB_WARNING() << "Camera warning: " << warn_str;
		istringstream is(warn_str);
		is >> m_last_warn;
		return;
	}

	fmt_resp = match["resp"];
	if (!fmt_resp.empty())
		DEB_RETURN() << DEB_VAR1(fmt_resp);
}

void SerialLine::sendFmtCmd(const string& cmd, string& resp)
{
	DEB_MEMBER_FUNCT();

	AutoMutex l = lock(AutoMutex::Locked);

	m_curr_fmt_resp.clear();

	writeCmd(cmd);
	string ans;
	readResp(ans);

	resp = m_curr_fmt_resp;
}

int SerialLine::getLastWarning()
{
	DEB_MEMBER_FUNCT();
	int last_warn = m_last_warn;
	m_last_warn = 0;
	DEB_RETURN() << DEB_VAR1(last_warn);
	return last_warn;
}

void SerialLine::clearCache()
{
	DEB_MEMBER_FUNCT();
	AutoMutex l = lock(AutoMutex::Locked);
	DEB_TRACE() << "Clearing reg cache";
	m_reg_cache.clear();
}

void SerialLine::setCacheActive(bool cache_act)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR2(cache_act, m_cache_act);
	AutoMutex l = lock(AutoMutex::Locked);
	if (cache_act && !m_cache_act) {
		DEB_TRACE() << "Clearing reg cache";
		m_reg_cache.clear();
	}
	m_cache_act = cache_act;
}

void SerialLine::getCacheActive(bool& cache_act)
{
	DEB_MEMBER_FUNCT();
	AutoMutex l = lock(AutoMutex::Locked);
	cache_act = m_cache_act;
	DEB_RETURN() << DEB_VAR1(cache_act);
}

void SerialLine::writeRegister(Reg reg, int  val)
{
	DEB_MEMBER_FUNCT();
	const string& reg_str = RegStrMap[reg];
	DEB_PARAM() << DEB_VAR3(reg, reg_str, val);

	int cache_val;
	bool in_cache = getRegCacheValSafe(reg, cache_val);
	if (in_cache && (cache_val == val)) {
		DEB_TRACE() << "Value already in cache";
		return;
	} 

	if (reg_str.empty())
		THROW_HW_ERROR(InvalidValue) << "Invalid " << DEB_VAR1(reg);

	ostringstream cmd;
	cmd << reg_str << val;
	string resp;
	sendFmtCmd(cmd.str(), resp);
}

ostream& lima::Frelon::operator <<(ostream& os, SerialLine::RegOp op)
{
	const char *name = "Unknown";
	switch (op) {
	case SerialLine::None:      name = "None";      break;
	case SerialLine::DoCmd:     name = "DoCmd";     break;
	case SerialLine::ReadReg:   name = "ReadReg";   break;
	case SerialLine::WriteReg:  name = "WriteReg";  break;
	case SerialLine::DoReset:   name = "DoReset";   break;
	case SerialLine::MultiRead: name = "MultiRead"; break;
	}
	return os << name;
}

