############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2026
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
#=============================================================================
#
# file :        Frelon.py
#
# description : Python source for the Frelon and its commands.
#                The class is derived from Device. It represents the
#                CORBA servant object which will be accessed from the
#                network. All commands which can be executed on the
#                Frelon are implemented in this file.
#
# project :     TANGO Device Server
#
# copyleft :    European Synchrotron Radiation Facility
#               BP 220, Grenoble 38043
#               FRANCE
#
#=============================================================================
#         (c) - Bliss - ESRF
#=============================================================================
#
import time

import PyTango
import tango
from tango import AttrWriteType, DevState
from tango.server import Device, attribute, command, device_property

from lima import core
from lima import frelon as FrelonHw
from lima.server import AttrHelper

EspiaDevNbInvalid = 256


class Frelon(Device):

    core.DEB_CLASS(core.DebModule.DebModApplication, 'LimaFrelon')

    _ImageMode = {
        'FRAME TRANSFER': FrelonHw.FrameTransferMode.FTM,
        'FULL FRAME': FrelonHw.FrameTransferMode.FFM,
    }
    _RoiMode = {
        'NONE': FrelonHw.RoiMode.None_,
        'SLOW': FrelonHw.RoiMode.Slow,
        'FAST': FrelonHw.RoiMode.Fast,
        'KINETIC': FrelonHw.RoiMode.Kinetic,
    }
    _InputChannel = {
        '1': FrelonHw.InputChan.Chan1,
        '2': FrelonHw.InputChan.Chan2,
        '3': FrelonHw.InputChan.Chan3,
        '4': FrelonHw.InputChan.Chan4,
        '1-2': FrelonHw.InputChan.Chan12,
        '3-4': FrelonHw.InputChan.Chan34,
        '1-3': FrelonHw.InputChan.Chan13,
        '2-4': FrelonHw.InputChan.Chan24,
        '1-2-3-4': FrelonHw.InputChan.Chan1234,
    }
    _E2VCorrection = {'ON': True, 'OFF': False}
    _Spb2Config = {
        'PRECISION': FrelonHw.SPB2Config.SPB2Precision,
        'SPEED': FrelonHw.SPB2Config.SPB2Speed,
    }

    # ------------------------------------------------------------------
    #    Device properties (initial hardware configuration only - the
    #    live attributes of the same name below read/write the camera
    #    directly, they don't share storage with these)
    # ------------------------------------------------------------------
    espia_dev_nb = device_property(dtype=int, default_value=0)
    espia_dev_nb2 = device_property(dtype=int, default_value=EspiaDevNbInvalid)
    espia_lib_debug_level = device_property(dtype=int, default_value=-1)
    espia_drv_debug_level = device_property(dtype=int, default_value=-1)

    # ------------------------------------------------------------------
    #    Device initialization
    # ------------------------------------------------------------------
    @core.DEB_MEMBER_FUNCT
    def init_device(self):
        Device.init_device(self)

        # Leagcy code used by AttrHelper.get_attr_string_value_list
        self.__ImageMode = self._ImageMode
        self.__RoiMode = self._RoiMode
        self.__InputChannel = self._InputChannel
        self.__E2VCorrection = self._E2VCorrection
        self.__Spb2Config = self._Spb2Config

        self.ResetLinkWaitTime = 5

        self.set_state(DevState.ON)

    # ------------------------------------------------------------------
    #    Commands
    # ------------------------------------------------------------------
    @command(dtype_in=str, dtype_out=(str,))
    @core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

    @command(dtype_in=str, doc_in="command", dtype_out=str, doc_out="return command")
    @core.DEB_MEMBER_FUNCT
    def execSerialCommand(self, command_string):
        return _FrelonAcq.execFrelonSerialCmd(command_string)

    @command
    @core.DEB_MEMBER_FUNCT
    def resetLink(self):
        edev = _FrelonAcq.getEspiaAcqDev()
        edev.resetLink()
        time.sleep(self.ResetLinkWaitTime)

    @command(
        dtype_out=(float,),
        doc_out="<readout_time, transfer_time, electronic_shutter_time, "
        "exposure_time, frame_period>",
    )
    @core.DEB_MEMBER_FUNCT
    def latchSeqTimValues(self):
        cam = _FrelonAcq.getFrelonCamera()
        tm = cam.latchSeqTimValues()
        return [tm.readout_time, tm.transfer_time, tm.electronic_shutter_time,
                tm.exposure_time, tm.frame_period]

    @command(
        dtype_in=float,
        doc_in="timeout",
        dtype_out=(float,),
        doc_out="<readout_time, transfer_time, electronic_shutter_time, "
        "exposure_time, frame_period>",
    )
    @core.DEB_MEMBER_FUNCT
    def measureSeqTimValues(self, timeout):
        cam = _FrelonAcq.getFrelonCamera()
        if timeout == 0:
            timeout = 3
        tm = cam.measureSeqTimValues(timeout)
        return [tm.readout_time, tm.transfer_time, tm.electronic_shutter_time,
                tm.exposure_time, tm.frame_period]

    # ------------------------------------------------------------------
    #    Attributes with a name colliding with a device_property
    # ------------------------------------------------------------------
    def _espia_dev_nb_fget(self):
        return self.espia_dev_nb or 0

    espia_dev_nb_attr = attribute(
        name="espia_dev_nb", dtype=int, access=AttrWriteType.READ, fget=_espia_dev_nb_fget
    )

    def _espia_dev_nb2_fget(self):
        if self.espia_dev_nb2 != EspiaDevNbInvalid:
            return self.espia_dev_nb2
        return -1

    espia_dev_nb2_attr = attribute(
        name="espia_dev_nb2", dtype=int, access=AttrWriteType.READ, fget=_espia_dev_nb2_fget
    )

    def _espia_lib_debug_level_fget(self):
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        return lib_deb_lvl

    def _espia_lib_debug_level_fset(self, value):
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        edev.setDebugLevels(value, drv_deb_lvl)

    espia_lib_debug_level_attr = attribute(
        name="espia_lib_debug_level",
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        fget=_espia_lib_debug_level_fget,
        fset=_espia_lib_debug_level_fset,
    )

    def _espia_drv_debug_level_fget(self):
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        return drv_deb_lvl

    def _espia_drv_debug_level_fset(self, value):
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        edev.setDebugLevels(lib_deb_lvl, value)

    espia_drv_debug_level_attr = attribute(
        name="espia_drv_debug_level",
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        fget=_espia_drv_debug_level_fget,
        fset=_espia_drv_debug_level_fset,
    )

    # ------------------------------------------------------------------
    #    Attributes with custom read/write logic
    # ------------------------------------------------------------------
    def _roi_bin_offset_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        return cam.getRoiBinOffset().y

    def _roi_bin_offset_fset(self, value):
        cam = _FrelonAcq.getFrelonCamera()
        cam.setRoiBinOffset(core.Point(0, value))

    roi_bin_offset = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        fget=_roi_bin_offset_fget,
        fset=_roi_bin_offset_fset,
    )

    def _camera_serial_fget(self):
        model = _FrelonAcq.getCameraModel()
        return "%d" % model.getSerialNb()

    camera_serial = attribute(dtype=str, access=AttrWriteType.READ, fget=_camera_serial_fget)

    def _reset_trace_log_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        ser_line = cam.getSerialLine()
        return ser_line.getResetTraceLog()

    reset_trace_log = attribute(
        dtype=str, max_dim_x=65535, access=AttrWriteType.READ, fget=_reset_trace_log_fget
    )

    def _readout_time_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        if cam.needSeqTimMeasure():
            raise core.Exception('Camera needs SeqTim measurement')
        return cam.getReadoutTime()

    readout_time = attribute(dtype=float, access=AttrWriteType.READ, fget=_readout_time_fget)

    def _transfer_time_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        if cam.needSeqTimMeasure():
            raise core.Exception('Camera needs SeqTim measurement')
        return cam.getTransferTime()

    transfer_time = attribute(dtype=float, access=AttrWriteType.READ, fget=_transfer_time_fget)

    def _need_seq_tim_measure_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        return cam.needSeqTimMeasure()

    need_seq_tim_measure = attribute(
        dtype=bool, access=AttrWriteType.READ, fget=_need_seq_tim_measure_fget
    )

    def _seq_status_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        return cam.getStatus().value

    seq_status = attribute(dtype=int, access=AttrWriteType.READ, fget=_seq_status_fget)

    # naming-convention title-casing can't produce the all-caps "SPB2" in
    # the real method name (SPB2Config); use explicit fget/fset.
    def _spb2_config_fget(self):
        cam = _FrelonAcq.getFrelonCamera()
        return AttrHelper.getDictKey(self._Spb2Config, cam.getSPB2Config())

    def _spb2_config_fset(self, data):
        value = AttrHelper.getDictValue(self._Spb2Config, data.upper())
        if value is None:
            tango.Except.throw_exception(
                "WrongData", "Wrong value spb2_config: %s" % data.upper(), "LimaCCDs Class"
            )
        _FrelonAcq.getFrelonCamera().setSPB2Config(value)

    spb2_config = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        fget=_spb2_config_fget,
        fset=_spb2_config_fset,
    )

    # ------------------------------------------------------------------
    #    Attributes fully dispatched by naming convention
    # ------------------------------------------------------------------
    # naming-convention would compute getImageMode; the underlying hw method
    # is really named getFrameTransferMode (see the original
    # Attribute2FunctionBase override).
    _image_mode_fget, _image_mode_fset = AttrHelper.make_fget_fset(
        "frame_transfer_mode", lambda self: _FrelonAcq.getFrelonCamera(), enum=_ImageMode
    )
    image_mode = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        fget=_image_mode_fget,
        fset=_image_mode_fset,
    )

    # naming-convention would compute getInputChannel; the real method is
    # named getInputChan (see the original Attribute2FunctionBase override).
    _input_channel_fget, _input_channel_fset = AttrHelper.make_fget_fset(
        "input_chan", lambda self: _FrelonAcq.getFrelonCamera(), enum=_InputChannel
    )
    input_channel = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        fget=_input_channel_fget,
        fset=_input_channel_fset,
    )

    _roi_mode_fget, _roi_mode_fset = AttrHelper.make_fget_fset(
        "roi_mode", lambda self: _FrelonAcq.getFrelonCamera(), enum=_RoiMode
    )
    roi_mode = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        fget=_roi_mode_fget,
        fset=_roi_mode_fset,
    )

    # naming-convention would compute getE2vCorrection; the real method is
    # named getE2VCorrectionActive (see the original Attribute2FunctionBase
    # override) - dispatched on _FrelonAcq itself, not the camera object.
    _e2v_correction_fget, _e2v_correction_fset = AttrHelper.make_fget_fset(
        "e2v_correction_active", lambda self: _FrelonAcq, enum=_E2VCorrection
    )
    e2v_correction = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        fget=_e2v_correction_fget,
        fset=_e2v_correction_fset,
    )

    _auto_seq_tim_measure_fget, _auto_seq_tim_measure_fset = AttrHelper.make_fget_fset(
        "auto_seq_tim_measure", lambda self: _FrelonAcq.getFrelonCamera()
    )
    auto_seq_tim_measure = attribute(
        dtype=bool,
        access=AttrWriteType.READ_WRITE,
        fget=_auto_seq_tim_measure_fget,
        fset=_auto_seq_tim_measure_fset,
    )

    _image_count_fget, _ = AttrHelper.make_fget_fset(
        "image_count", lambda self: _FrelonAcq.getFrelonCamera()
    )
    image_count = attribute(dtype=int, access=AttrWriteType.READ, fget=_image_count_fget)

    _missing_ext_start_pulses_fget, _ = AttrHelper.make_fget_fset(
        "missing_ext_start_pulses", lambda self: _FrelonAcq.getFrelonCamera()
    )
    missing_ext_start_pulses = attribute(
        dtype=int, access=AttrWriteType.READ, fget=_missing_ext_start_pulses_fget
    )


#----------------------------------------------------------------------------
# TACO extensions
#----------------------------------------------------------------------------
# Untouched by the tango.server migration - a completely separate legacy
# TACO protocol export, not part of the PyTango Device pattern at all (see
# LimaCCDs.py's TacoSpecificName / get_taco_specific_cmd_list_n_proxy_cont).
# Still references the plain module-level _FrelonAcq global below, so that
# stays a bare global rather than a Frelon class attribute.

FrelonTacoCmdList = {

    'DevCcdGetType' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevLong, "CCD type code"]],
    'DevCcdGetLstErrMsg' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevString, "Last error message"]],
    'DevCcdSetChannel' :
    [[PyTango.DevLong, "Input channel mode"],
     [PyTango.DevVoid, ""]],
    'DevCcdGetChannel' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevLong, "Input channel mode"]],
    'DevCcdSetHwPar' :
    [[PyTango.DevString, "Flip KinLineBeg KinStripes 0 RoiMode"],
     [PyTango.DevVoid, ""]],
    'DevCcdGetHwPar' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevString, "Flip KinLineBeg KinStripes 0 RoiMode"]],
    'DevCcdSetKinetics' :
    [[PyTango.DevLong, "ImageMode: 0=FFM, 3=FTM"],
     [PyTango.DevVoid, ""]],
    'DevCcdGetKinetics' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevLong, "ImageMode: 0=FFM, 3=FTM"]],
    'DevCcdSetKinWinSize' :
    [[PyTango.DevLong, "Kinetics window size"],
     [PyTango.DevVoid, ""]],
    'DevCcdGetKinWinSize' :
    [[PyTango.DevVoid, ""],
     [PyTango.DevLong, "Kinetics window size"]],
    'DevCcdCommand' :
    [[PyTango.DevString, "Frelon serial command"],
     [PyTango.DevString, "Frelon serial response"]],
    'DevCcdGetChanges':
    [[PyTango.DevVoid, ""],
     [PyTango.DevLong, "Change count"]],
}

class FrelonTacoProxy:

    core.DEB_CLASS(core.DebModule.DebModApplication, 'FrelonTacoProxy')

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetType(self):
        model = _FrelonAcq.getCameraModel()
        type_nb = ((model.getAdcBits() == 16) and 2016) or 2014
        deb.Return('Getting type: %s' % type_nb)
        return type_nb

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetLstErrMsg(self):
        err_msg = ''
        deb.Return('Getting last err. msg: %s' % err_msg)
        return err_msg

    @core.DEB_MEMBER_FUNCT
    def DevCcdSetChannel(self, input_chan):
        _FrelonAcq.getFrelonCamera().setInputChan(FrelonHw.InputChan(input_chan))

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetChannel(self):
        return _FrelonAcq.getFrelonCamera().getInputChan().value

    @core.DEB_MEMBER_FUNCT
    def DevCcdSetHwPar(self, hw_par_str):
        hw_par = map(int, hw_par_str.split())
        deb.Param('Setting hw par: %s' % hw_par)
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        flip_mode, kin_line_beg, kin_stripes, d0, roi_mode_int = hw_par
        flip = core.Flip(flip_mode >> 1, flip_mode & 1)
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        ct_image.setFlip(flip)
        roi_mode = FrelonHw.RoiMode(roi_mode_int)
        cam = _FrelonAcq.getFrelonCamera()
        cam.setRoiMode(roi_mode)
        if roi_mode == FrelonHw.RoiMode.Kinetic:
            max_frame_dim = _FrelonAcq.getFrameDim(max_dim=True)
            frame_height = max_frame_dim.getSize().getHeight()
            if kin_line_beg + kin_win_size > frame_height:
                kin_win_size = frame_height - kin_line_beg
                bin_y = self.getBin().getY()
                kin_win_size = (kin_win_size / bin_y) * bin_y
                deb.Trace('Re-adjusting kin_win_size to %d to fit chip' %
                          kin_win_size)
            self.setKinPars(kin_win_size, kin_line_beg, kin_stripes)
        else:
            deb.Warning('Ingoring Kinetic parameters')

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetHwPar(self):
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        flip = ct_image.getFlip()
        flip_mode = flip.x << 1 | flip.y
        cam = _FrelonAcq.getFrelonCamera()
        roi_mode = cam.getRoiMode()
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        hw_par = [flip_mode, kin_line_beg, kin_stripes, 0, roi_mode.value]
        deb.Return('Getting hw par: %s' % hw_par)
        hw_par_str = ' '.join(map(str, hw_par))
        return hw_par_str

    @core.DEB_MEMBER_FUNCT
    def DevCcdSetKinetics(self, kinetics):
        deb.Param('Setting the profile: %s' % kinetics)
        if kinetics == 0:
            ftm = FrelonHw.FrameTransferMode.FFM
        elif kinetics == 3:
            ftm = FrelonHw.FrameTransferMode.FTM
        else:
            raise core.Exception('Invalid profile value: %s' % kinetics)
        _FrelonAcq.getFrelonCamera().setFrameTransferMode(ftm)

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetKinetics(self):
        ftm = _FrelonAcq.getFrelonCamera().getFrameTransferMode()
        if ftm == FrelonHw.FrameTransferMode.FTM:
            kinetics = 3
        else:
            kinetics = 0
        deb.Return('Getting the profile: %s' % kinetics)
        return kinetics

    @core.DEB_MEMBER_FUNCT
    def DevCcdSetKinWinSize(self, kin_win_size):
        deb.Param('Setting the kinetics window size: %s' % kin_win_size)
        prev_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        self.setKinPars(kin_win_size, kin_line_beg, kin_stripes)

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetKinWinSize(self):
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        deb.Return('Getting the kinetics window size: %s' % kin_win_size)
        return kin_win_size

    @core.DEB_MEMBER_FUNCT
    def setKinPars(self, kin_win_size, kin_line_beg, kin_stripes):
        deb.Param('Setting kin pars: ' +
                  'kin_win_size=%s, kin_line_beg=%s, kin_stripes=%s' % \
                  (kin_win_size, kin_line_beg, kin_stripes))
        if kin_stripes > 1:
            deb.Warning('Ignoring kin_stripes=%d' % kin_stripes)

        bin = self.getBin()
        if kin_win_size % bin.getY() != 0:
            msg = 'Invalid kinetics window size (%d): ' % kin_win_size + \
                  'must be multiple of vert. bin (%d)' % bin.getY()
            raise core.Exception(msg)

        roi = _FrelonAcq.getRoi()
        roi = roi.getUnbinned(bin)
        tl = core.Point(roi.getTopLeft().x, kin_line_beg)
        size = core.Size(roi.getSize().getWidth(), kin_win_size)
        roi = core.Roi(tl, size)
        roi = roi.getBinned(bin)
        _FrelonAcq.setRoi(roi)

        _FrelonAcq.setRoiLineBegin(kin_line_beg)

    @core.DEB_MEMBER_FUNCT
    def getKinPars(self):
        bin = self.getBin()
        roi = _FrelonAcq.getRoi()
        roi = roi.getUnbinned(bin)
        kin_win_size = roi.getSize().getHeight()
        kin_line_beg = _FrelonAcq.getRoiLineBegin()
        kin_stripes = 1
        deb.Return('Getting kin pars: ' +
                   'kin_win_size=%s, kin_line_beg=%s, kin_stripes=%s' % \
                   (kin_win_size, kin_line_beg, kin_stripes))
        return kin_win_size, kin_line_beg, kin_stripes

    @core.DEB_MEMBER_FUNCT
    def getBin(self):
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        bin = ct_image.getBin()
        deb.Return('Getting binning: %s' % bin)
        return bin

    @core.DEB_MEMBER_FUNCT
    def DevCcdCommand(self, cmd):
        return _FrelonAcq.execFrelonSerialCmd(cmd)

    @core.DEB_MEMBER_FUNCT
    def DevCcdGetChanges(self):
        changes = 0
        deb.Trace('Getting changes: %s' % changes)
        return changes

    @core.DEB_MEMBER_FUNCT
    def getAutoResetCtStatus(self):
        return True

FrelonTacoProxyCont = [FrelonTacoProxy()]


#----------------------------------------------------------------------------
# Plugins
#----------------------------------------------------------------------------
_FrelonAcq = None

def get_control(espia_dev_nb = 0, espia_dev_nb2 = EspiaDevNbInvalid,
                espia_lib_debug_level = -1, espia_drv_debug_level = -1,
                **keys) :
    global _FrelonAcq
    if _FrelonAcq is None:
       _FrelonAcq = FrelonHw.FrelonAcq(int(espia_dev_nb), int(espia_dev_nb2),
                                       int(espia_lib_debug_level),
                                       int(espia_drv_debug_level))
    return _FrelonAcq.getGlobalControl()

def get_tango_specific_class_n_device():
    return Frelon.TangoClassClass, Frelon

def get_taco_specific_cmd_list_n_proxy_cont():
    return FrelonTacoCmdList,FrelonTacoProxyCont
