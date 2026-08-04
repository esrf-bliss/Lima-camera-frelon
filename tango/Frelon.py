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
from lima import core
from lima import frelon as FrelonHw
from lima.server import AttrHelper

EspiaDevNbInvalid = 256

class Frelon(PyTango.LatestDeviceImpl):

    core.DEB_CLASS(core.DebModule.DebModApplication, 'LimaFrelon')


#------------------------------------------------------------------
#    Device constructor
#------------------------------------------------------------------
    def __init__(self,*args) :
        PyTango.LatestDeviceImpl.__init__(self,*args)

        self.__ImageMode = {'FRAME TRANSFER': FrelonHw.FrameTransferMode.FTM,
                            'FULL FRAME': FrelonHw.FrameTransferMode.FFM}

        self.__RoiMode = {'NONE' : FrelonHw.RoiMode.None_,
                          'SLOW' : FrelonHw.RoiMode.Slow,
                          'FAST' : FrelonHw.RoiMode.Fast,
                          'KINETIC' : FrelonHw.RoiMode.Kinetic}

        self.__InputChannel = {'1'       : FrelonHw.InputChan.Chan1,
                               '2'       : FrelonHw.InputChan.Chan2,
                               '3'       : FrelonHw.InputChan.Chan3,
                               '4'       : FrelonHw.InputChan.Chan4,
                               '1-2'     : FrelonHw.InputChan.Chan12,
                               '3-4'     : FrelonHw.InputChan.Chan34,
                               '1-3'     : FrelonHw.InputChan.Chan13,
                               '2-4'     : FrelonHw.InputChan.Chan24,
                               '1-2-3-4' : FrelonHw.InputChan.Chan1234}

        self.__E2VCorrection = {'ON' : True,
                                'OFF' : False}

        self.__Spb2Config = {'PRECISION' : FrelonHw.SPB2Config.SPB2Precision,
                             'SPEED' : FrelonHw.SPB2Config.SPB2Speed}

        self.__Attribute2FunctionBase = {'image_mode' : 'FrameTransferMode',
                                         'input_channel' : 'InputChan',
                                         'e2v_correction' : 'E2VCorrectionActive',
                                         'spb2_config' : 'SPB2Config',
                                         'seq_status' : 'Status'}

        self.init_device()

#------------------------------------------------------------------
#    Device destructor
#------------------------------------------------------------------
    def delete_device(self):
        pass

#------------------------------------------------------------------
#    Device initialization
#------------------------------------------------------------------
    @core.DEB_MEMBER_FUNCT
    def init_device(self):
        self.set_state(PyTango.DevState.ON)
        self.get_device_properties(self.get_device_class())
        self.ResetLinkWaitTime = 5	

    @core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

    def __getattr__(self,name) :
        obj = _FrelonAcq if name.endswith("e2v_correction") \
                         else _FrelonAcq.getFrelonCamera()
        return AttrHelper.get_attr_4u(self, name, obj)

    @core.DEB_MEMBER_FUNCT
    def execSerialCommand(self, command_string) :
        return _FrelonAcq.execFrelonSerialCmd(command_string)

    @core.DEB_MEMBER_FUNCT
    def resetLink(self) :
        edev = _FrelonAcq.getEspiaAcqDev()
        edev.resetLink()
        time.sleep(self.ResetLinkWaitTime)

    @core.DEB_MEMBER_FUNCT
    def latchSeqTimValues(self) :
        cam = _FrelonAcq.getFrelonCamera()
        tm = cam.latchSeqTimValues()
        return [tm.readout_time, tm.transfer_time, tm.electronic_shutter_time,
                tm.exposure_time, tm.frame_period]

    @core.DEB_MEMBER_FUNCT
    def measureSeqTimValues(self, timeout) :
        cam = _FrelonAcq.getFrelonCamera()
        if timeout == 0:
            timeout = 3
        tm = cam.measureSeqTimValues(timeout)
        return [tm.readout_time, tm.transfer_time, tm.electronic_shutter_time,
                tm.exposure_time, tm.frame_period]

    ## @brief read the espia board id
    #
    def read_espia_dev_nb(self,attr) :
        espia_dev_nb = 0
        if self.espia_dev_nb:
            espia_dev_nb = self.espia_dev_nb
        attr.set_value(espia_dev_nb)

    def read_espia_dev_nb2(self,attr) :
        espia_dev_nb2 = -1
        if self.espia_dev_nb2 != EspiaDevNbInvalid:
            espia_dev_nb2 = self.espia_dev_nb2
        attr.set_value(espia_dev_nb2)

    def read_espia_lib_debug_level(self,attr) :
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        attr.set_value(lib_deb_lvl)

    def write_espia_lib_debug_level(self,attr) :
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        lib_deb_lvl = attr.get_write_value()
        edev.setDebugLevels(lib_deb_lvl, drv_deb_lvl)

    def read_espia_drv_debug_level(self,attr) :
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        attr.set_value(drv_deb_lvl)

    def write_espia_drv_debug_level(self,attr) :
        edev = _FrelonAcq.getEspiaAcqDev()
        lib_deb_lvl, drv_deb_lvl = edev.getDebugLevels()
        drv_deb_lvl = attr.get_write_value()
        edev.setDebugLevels(lib_deb_lvl, drv_deb_lvl)

    def read_roi_bin_offset(self,attr) :
        cam = _FrelonAcq.getFrelonCamera()
        roi_bin_offset = cam.getRoiBinOffset()
        attr.set_value(roi_bin_offset.y)

    def write_roi_bin_offset(self,attr) :
        roi_bin_offset = core.Point(0, attr.get_write_value())
        cam = _FrelonAcq.getFrelonCamera()
        cam.setRoiBinOffset(roi_bin_offset)

    def read_camera_serial(self,attr):
        model = _FrelonAcq.getCameraModel()
        serial = model.getSerialNb()
        attr.set_value("%d" % serial)

    def read_reset_trace_log(self,attr):
        cam = _FrelonAcq.getFrelonCamera()
        ser_line = cam.getSerialLine()
        reset_trace_log = ser_line.getResetTraceLog()
        attr.set_value(reset_trace_log)

    def read_readout_time(self,attr):
        cam = _FrelonAcq.getFrelonCamera()
        if cam.needSeqTimMeasure():
            raise core.Exception('Camera needs SeqTim measurement')
        attr.set_value(cam.getReadoutTime())

    def read_transfer_time(self,attr):
        cam = _FrelonAcq.getFrelonCamera()
        if cam.needSeqTimMeasure():
            raise core.Exception('Camera needs SeqTim measurement')
        attr.set_value(cam.getTransferTime())

    def read_need_seq_tim_measure(self,attr):
        cam = _FrelonAcq.getFrelonCamera()
        attr.set_value(cam.needSeqTimMeasure())


class FrelonClass(PyTango.DeviceClass):

    class_property_list = {}

    device_property_list = {
        'espia_dev_nb':
        [PyTango.DevShort,
         "Espia board device number",[]],
        'espia_dev_nb2':
        [PyTango.DevShort,
         "Second Espia board device number",[EspiaDevNbInvalid]],
        'espia_lib_debug_level':
        [PyTango.DevLong,
         "Espia library debug level",[-1]],
        'espia_drv_debug_level':
        [PyTango.DevLong,
         "Espia driver debug level",[-1]],
        }

    cmd_list = {
        'getAttrStringValueList':
        [[PyTango.DevString, "Attribute name"],
         [PyTango.DevVarStringArray, "Authorized String value list"]],
        'execSerialCommand':
        [[PyTango.DevString,"command"],
         [PyTango.DevString,"return command"]],
        'resetLink':
        [[PyTango.DevVoid,""],
         [PyTango.DevVoid,""]],
        'latchSeqTimValues':
        [[PyTango.DevVoid,""],
         [PyTango.DevVarDoubleArray,"<readout_time, transfer_time, "
          "electronic_shutter_time, exposure_time, frame_period>"]],
        'measureSeqTimValues':
        [[PyTango.DevDouble,"timeout"],
         [PyTango.DevVarDoubleArray,"<readout_time, transfer_time, "
          "electronic_shutter_time, exposure_time, frame_period>"]],
        }

    attr_list = {
        'espia_dev_nb':
        [[PyTango.DevShort,
          PyTango.SCALAR,
          PyTango.READ]],
        'espia_dev_nb2':
        [[PyTango.DevShort,
          PyTango.SCALAR,
          PyTango.READ]],
        'espia_lib_debug_level':
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'espia_drv_debug_level':
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'image_mode' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'input_channel' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'roi_mode' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'roi_bin_offset' :
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'e2v_correction' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'spb2_config' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'seq_status' :
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ]],
        'readout_time' :
        [[PyTango.DevFloat,
          PyTango.SCALAR,
          PyTango.READ]],
        'transfer_time' :
        [[PyTango.DevFloat,
          PyTango.SCALAR,
          PyTango.READ]],
        'need_seq_tim_measure' :
        [[PyTango.DevBoolean,
          PyTango.SCALAR,
          PyTango.READ]],
        'auto_seq_tim_measure' :
        [[PyTango.DevBoolean,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],
        'image_count' :
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ]],
        'missing_ext_start_pulses' :
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ]],
        'camera_serial' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ]],
        'reset_trace_log' :
        [[PyTango.DevString,
          PyTango.SPECTRUM,
          PyTango.READ, 65535]],
        }

    def __init__(self,name) :
        PyTango.DeviceClass.__init__(self,name)
        self.set_type(name)


#----------------------------------------------------------------------------
# TACO extensions
#----------------------------------------------------------------------------

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
    return FrelonClass,Frelon

def get_taco_specific_cmd_list_n_proxy_cont():
    return FrelonTacoCmdList,FrelonTacoProxyCont
