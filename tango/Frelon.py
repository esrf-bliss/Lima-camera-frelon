############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2011
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
import time, string
import PyTango
from Lima import Core
from Lima import Frelon as FrelonHw
from Lima.Server import AttrHelper

class Frelon(PyTango.Device_4Impl):

    Core.DEB_CLASS(Core.DebModApplication, 'LimaFrelon')


#------------------------------------------------------------------
#    Device constructor
#------------------------------------------------------------------
    def __init__(self,*args) :
        PyTango.Device_4Impl.__init__(self,*args)

        self.__ImageMode = {'FRAME TRANSFER': FrelonHw.FTM,
                            'FULL FRAME': FrelonHw.FFM}

        self.__RoiMode = {'NONE' : FrelonHw.None,
                          'SLOW' : FrelonHw.Slow,
                          'FAST' : FrelonHw.Fast,
                          'KINETIC' : FrelonHw.Kinetic}

        self.__InputChannel = {'1'       : FrelonHw.Chan1,
                               '2'       : FrelonHw.Chan2,
                               '3'       : FrelonHw.Chan3,
                               '4'       : FrelonHw.Chan4,
                               '1-2'     : FrelonHw.Chan12,
                               '3-4'     : FrelonHw.Chan34,
                               '1-3'     : FrelonHw.Chan13,
                               '2-4'     : FrelonHw.Chan24,
                               '1-2-3-4' : FrelonHw.Chan1234}

        self.__E2VCorrection = {'ON' : True,
                                'OFF' : False}

        self.__Spb2Config = {'PRECISION' : FrelonHw.SPB2Precision,
                             'SPEED' : FrelonHw.SPB2Speed}

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
    @Core.DEB_MEMBER_FUNCT
    def init_device(self):
        self.set_state(PyTango.DevState.ON)
        self.get_device_properties(self.get_device_class())
        self.ResetLinkWaitTime = 5	

    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

    def __getattr__(self,name) :
        obj = _FrelonAcq if name == 'E2VCorrectionActive' \
                         else _FrelonAcq.getFrelonCamera()
        return AttrHelper.get_attr_4u(self, name, obj)

    @Core.DEB_MEMBER_FUNCT
    def execSerialCommand(self, command_string) :
        return _FrelonAcq.execFrelonSerialCmd(command_string)

    @Core.DEB_MEMBER_FUNCT
    def resetLink(self) :
        edev = _FrelonAcq.getEspiaDev()
        edev.resetLink()
        time.sleep(self.ResetLinkWaitTime)

    ## @brief read the espia board id
    #
    def read_espia_dev_nb(self,attr) :
        espia_dev_nb = 0
        if self.espia_dev_nb:
            espia_dev_nb = self.espia_dev_nb
        attr.set_value(espia_dev_nb)

    def read_roi_bin_offset(self,attr) :
        cam = _FrelonAcq.getFrelonCamera()
        roi_bin_offset = cam.getRoiBinOffset()
        attr.set_value(roi_bin_offset.y)

    def write_roi_bin_offset(self,attr) :
        roi_bin_offset = Core.Point(0, attr.get_write_value())
        cam = _FrelonAcq.getFrelonCamera()
        cam.setRoiBinOffset(roi_bin_offset)

    def read_camera_serial(self,attr):
        model = _FrelonAcq.getCameraModel()
        serial = model.getSerialNb()
        attr.set_value("%d" % serial)

class FrelonClass(PyTango.DeviceClass):

    class_property_list = {}

    device_property_list = {
        'espia_dev_nb':
        [PyTango.DevShort,
         "Espia board device number",[]],
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
        }

    attr_list = {
        'espia_dev_nb':
        [[PyTango.DevShort,
          PyTango.SCALAR,
          PyTango.READ]],
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
        'camera_serial' :
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ]],
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

    Core.DEB_CLASS(Core.DebModApplication, 'FrelonTacoProxy')

    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetType(self):
        model = _FrelonAcq.getCameraModel()
        type_nb = ((model.getAdcBits() == 16) and 2016) or 2014
        deb.Return('Getting type: %s' % type_nb)
        return type_nb

    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetLstErrMsg(self):
        err_msg = ''
        deb.Return('Getting last err. msg: %s' % err_msg)
        return err_msg

    @Core.DEB_MEMBER_FUNCT
    def DevCcdSetChannel(self, input_chan):
        _FrelonAcq.setInputChan(input_chan)
    
    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetChannel(self):
        return _FrelonAcq.getInputChan()
    
    @Core.DEB_MEMBER_FUNCT
    def DevCcdSetHwPar(self, hw_par_str):
        hw_par = map(int, string.split(hw_par_str))
        deb.Param('Setting hw par: %s' % hw_par)
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        flip_mode, kin_line_beg, kin_stripes, d0, roi_mode_int = hw_par
        flip = Core.Flip(flip_mode >> 1, flip_mode & 1)
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        ct_image.setFlip(flip)
        roi_mode = FrelonHw.RoiMode(roi_mode_int)
        cam = _FrelonAcq.getFrelonCamera()
        cam.setRoiMode(roi_mode)
        if roi_mode == FrelonHw.Kinetic:
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

    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetHwPar(self):
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        flip = ct_image.getFlip()
        flip_mode = flip.x << 1 | flip.y
        cam = _FrelonAcq.getFrelonCamera()
        roi_mode = cam.getRoiMode()
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        hw_par = [flip_mode, kin_line_beg, kin_stripes, 0, roi_mode]
        deb.Return('Getting hw par: %s' % hw_par)
        hw_par_str = string.join(map(str, hw_par))
        return hw_par_str

    @Core.DEB_MEMBER_FUNCT
    def DevCcdSetKinetics(self, kinetics):
        deb.Param('Setting the profile: %s' % kinetics)
        if kinetics == 0:
            ftm = FrelonHw.FFM
        elif kinetics == 3:
            ftm = FrelonHw.FTM
        else:
            raise Core.Exception('Invalid profile value: %s' % kinetics)
        _FrelonAcq.setFrameTransferMode(ftm)
        
    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetKinetics(self):
        ftm = _FrelonAcq.getFrameTransferMode()
        if ftm == FrelonHw.FTM:
            kinetics = 3
        else:
            kinetics = 0
        deb.Return('Getting the profile: %s' % kinetics)
        return kinetics
    
    @Core.DEB_MEMBER_FUNCT
    def DevCcdSetKinWinSize(self, kin_win_size):
        deb.Param('Setting the kinetics window size: %s' % kin_win_size)
        prev_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        self.setKinPars(kin_win_size, kin_line_beg, kin_stripes)
    
    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetKinWinSize(self):
        kin_win_size, kin_line_beg, kin_stripes = self.getKinPars()
        deb.Return('Getting the kinetics window size: %s' % kin_win_size)
        return kin_win_size

    @Core.DEB_MEMBER_FUNCT
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
            raise Core.Exception(msg)

        roi = _FrelonAcq.getRoi()
        roi = roi.getUnbinned(bin)
        tl = Core.Point(roi.getTopLeft().x, kin_line_beg)
        size = Core.Size(roi.getSize().getWidth(), kin_win_size)
        roi = Core.Roi(tl, size)
        roi = roi.getBinned(bin)
        _FrelonAcq.setRoi(roi)

        _FrelonAcq.setRoiLineBegin(kin_line_beg)
        
    @Core.DEB_MEMBER_FUNCT
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

    @Core.DEB_MEMBER_FUNCT
    def getBin(self):
        control = _FrelonAcq.getGlobalControl()
        ct_image = control.image()
        bin = ct_image.getBin()
        deb.Return('Getting binning: %s' % bin)
        return bin

    @Core.DEB_MEMBER_FUNCT
    def DevCcdCommand(self, cmd):
        return _FrelonAcq.execFrelonSerialCmd(cmd)

    @Core.DEB_MEMBER_FUNCT
    def DevCcdGetChanges(self):
        changes = 0
        deb.Trace('Getting changes: %s' % changes)
        return changes

    @Core.DEB_MEMBER_FUNCT
    def getAutoResetCtStatus(self):
        return True

FrelonTacoProxyCont = [FrelonTacoProxy()]


#----------------------------------------------------------------------------
# Plugins
#----------------------------------------------------------------------------
_FrelonAcq = None

def get_control(espia_dev_nb = 0,**keys) :
    global _FrelonAcq
    if _FrelonAcq is None:
       _FrelonAcq = FrelonHw.FrelonAcq(int(espia_dev_nb))
    return _FrelonAcq.getGlobalControl() 

def get_tango_specific_class_n_device():
    return FrelonClass,Frelon

def get_taco_specific_cmd_list_n_proxy_cont():
    return FrelonTacoCmdList,FrelonTacoProxyCont
