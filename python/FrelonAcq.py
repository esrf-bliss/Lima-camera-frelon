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
import gc, weakref
from Lima.Core import *
from Lima import Espia
from limafrelon import *
from processlib import Tasks

class FrelonAcq:

    DEB_CLASS(DebModApplication, 'FrelonAcq')

    MonitoredSerialCmds = [Frelon.LatencyTime,
                           Frelon.ShutEnable, Frelon.ShutCloseTime]
    MonitoredSerialDict = {}
    for x in MonitoredSerialCmds:
        MonitoredSerialDict[Frelon.Global.RegStrMap()[x]] = x
    del x

    class BinChangedCallback(Frelon.BinChangedCallback):

        DEB_CLASS(DebModApplication, "FrelonAcq.BinChangedCallback")

        @DEB_MEMBER_FUNCT
        def __init__(self, e2v_corr_update):
            self.m_e2v_corr_update = weakref.ref(e2v_corr_update)
            Frelon.BinChangedCallback.__init__(self)

        @DEB_MEMBER_FUNCT
        def hwBinChanged(self, hw_bin):
            e2v_corr_update = self.m_e2v_corr_update()
            if e2v_corr_update:
                e2v_corr_update.hwBinChanged(hw_bin);


    class RoiChangedCallback(Frelon.RoiChangedCallback):

        DEB_CLASS(DebModApplication, 'FrelonAcq.RoiChangedCallback')

        @DEB_MEMBER_FUNCT
        def __init__(self, e2v_corr_update):
            self.m_e2v_corr_update = weakref.ref(e2v_corr_update)
            Frelon.RoiChangedCallback.__init__(self)

        @DEB_MEMBER_FUNCT
        def hwRoiChanged(self, hw_roi):
            e2v_corr_update = self.m_e2v_corr_update()
            if e2v_corr_update:
                e2v_corr_update.hwRoiChanged(hw_roi);


    class E2VCorrectionUpdate:

        DEB_CLASS(DebModApplication, 'FrelonAcq.E2VCorrectionUpdate')

        @DEB_MEMBER_FUNCT
        def __init__(self, e2v_corr, hw_inter):
            self.m_reg_act = False
            self.m_e2v_corr = e2v_corr

            self.m_det_info_ctrl_obj = hw_inter.getHwCtrlObj(HwCap.DetInfo)
            self.m_bin_ctrl_obj      = hw_inter.getHwCtrlObj(HwCap.Bin)
            self.m_roi_ctrl_obj      = hw_inter.getHwCtrlObj(HwCap.Roi)

            self.m_hw_bin = Bin()
            self.m_bin_cb = FrelonAcq.BinChangedCallback(self)
            self.m_roi_cb = FrelonAcq.RoiChangedCallback(self)

        @DEB_MEMBER_FUNCT
        def __del__(self):
            self.setRegistrationActive(False)

        @DEB_MEMBER_FUNCT
        def setRegistrationActive(self, reg_act):
            if reg_act and not self.m_reg_act:
                self.m_bin_ctrl_obj.registerBinChangedCallback(self.m_bin_cb)
                self.m_roi_ctrl_obj.registerRoiChangedCallback(self.m_roi_cb)
                self.m_reg_act = True
            elif not reg_act and self.m_reg_act:
                self.m_bin_ctrl_obj.unregisterBinChangedCallback(self.m_bin_cb)
                self.m_roi_ctrl_obj.unregisterRoiChangedCallback(self.m_roi_cb)
                self.m_reg_act = False

        @DEB_MEMBER_FUNCT
        def hwBinChanged(self, hw_bin):
            deb.Param('hw_bin=%s' % hw_bin)
            self.m_hw_bin = hw_bin
            self.m_e2v_corr.setHwBin(hw_bin);

        @DEB_MEMBER_FUNCT
        def hwRoiChanged(self, hw_roi):
            deb.Param('hw_roi=%s' % hw_roi)
            if hw_roi.isEmpty():
                deb.Trace('Empty Roi, getting MaxImageSize')
                max_size = self.m_det_info_ctrl_obj.getMaxImageSize()
                unbinned_roi = Roi(Point(0), max_size)
                hw_roi = unbinned_roi.getBinned(self.m_hw_bin)
                deb.Trace('hw_roi=%s' % hw_roi)
            self.m_e2v_corr.setHwRoi(hw_roi);


    @DEB_MEMBER_FUNCT
    def __init__(self, espia_dev_nb, *args, **kws):
        espia_dev_nb2 = kws.get('espia_dev_nb2', None)
        if len(args) > 0:
            espia_dev_nb2 = args[0]

        self.m_cam_inited    = False

        self.m_e2v_corr      = None
        self.m_e2v_corr_update = None
        self.m_e2v_corr_act  = True

        self.m_bpm_mgr       = Tasks.BpmManager()
        self.m_bpm_task      = Tasks.BpmTask(self.m_bpm_mgr)

        self.m_ser_edev      = Espia.Dev(espia_dev_nb)
        self.m_eserline      = Espia.SerialLine(self.m_ser_edev)
        self.m_cam           = Frelon.Camera(self.m_eserline)

        self.m_acq_edev  = self.m_ser_edev
        model = self.m_cam.getModel()
        f16 = (model.getChipType() == Frelon.Andanta_CcdFT2k)
        if f16:
            if espia_dev_nb2 is not None:
                self.m_acq_edev  = Espia.Meta([espia_dev_nb, espia_dev_nb2])
            else:
                model.setF16ForceSingle(True);

        self.m_acq           = Espia.Acq(self.m_acq_edev)
        self.m_buffer_cb_mgr = Espia.BufferMgr(self.m_acq)
        self.m_buffer_mgr    = BufferCtrlMgr(self.m_buffer_cb_mgr)
        self.m_hw_inter      = Frelon.Interface(self.m_acq, self.m_buffer_mgr,
                                                self.m_cam)

        self.m_ct            = CtControl(self.m_hw_inter)
        self.m_ct_image      = self.m_ct.image()

        self.m_cam_inited    = True

        self.checkE2VCorrection()

    @DEB_MEMBER_FUNCT
    def __del__(self):
        if self.m_cam_inited:
            del self.m_ct_image
            del self.m_ct;		gc.collect()

            del self.m_hw_inter;	gc.collect()
            del self.m_buffer_mgr;	gc.collect()
            del self.m_cam;		gc.collect()

        del self.m_eserline;		gc.collect()
        del self.m_buffer_cb_mgr;	gc.collect()
        del self.m_acq;			gc.collect()
        del self.m_acq_edev;		gc.collect()
        del self.m_ser_edev;		gc.collect()

        if self.m_e2v_corr:
            del self.m_e2v_corr_update
            del self.m_e2v_corr;	gc.collect()

        del self.m_bpm_task;		gc.collect()
        del self.m_bpm_mgr;		gc.collect()

    def getEspiaSerDev(self):
        return self.m_ser_edev

    def getEspiaAcqDev(self):
        return self.m_acq_edev

    def getEspiaAcq(self):
        return self.m_acq

    def getEspiaBufferMgr(self):
        return self.m_buffer_cb_mgr

    def getEspiaSerialLine(self):
        return self.m_eserline

    def getFrelonCamera(self):
        return self.m_cam

    def getBufferMgr(self):
        return self.m_buffer_mgr

    def getFrelonInterface(self):
        return self.m_hw_inter

    def getGlobalControl(self):
        return self.m_ct

    @DEB_MEMBER_FUNCT
    def checkE2VCorrection(self):
        deb.Trace('Checking E2V correction')
        chip_type = self.m_cam.getModel().getChipType()
        is_e2v = (chip_type == Frelon.E2V_2k)
        corr_act = (is_e2v and self.m_e2v_corr_act)
        deb.Param('is_e2v=%s, self.m_e2v_corr_act=%s' % (is_e2v,
                                                         self.m_e2v_corr_act))
        if bool(corr_act) == bool(self.m_e2v_corr):
            return

        ct_status = self.m_ct.getStatus()
        if ct_status.AcquisitionStatus == AcqRunning:
            raise Exception('Acquisition is running')

        if corr_act:
            deb.Trace('Enabling E2V correction')
            self.m_e2v_corr = Frelon.E2VCorrection()
            self.m_e2v_corr_update = self.E2VCorrectionUpdate(self.m_e2v_corr,
                                                              self.m_hw_inter)
            self.m_e2v_corr_update.setRegistrationActive(True)
            self.m_ct.setReconstructionTask(self.m_e2v_corr)
        else:
            deb.Trace('Disabling E2V correction')
            self.m_ct.setReconstructionTask(None)
            self.m_e2v_corr_update.setRegistrationActive(False)
            self.m_e2v_corr_update = None
            self.m_e2v_corr = None

    @DEB_MEMBER_FUNCT
    def resetDefaults(self):
        deb.Trace('Reseting to default settings')
        self.m_hw_inter.resetDefaults()
        deb.Trace('Forcing Control Layer to synchronise')
        self.m_ct.reset()

    @DEB_MEMBER_FUNCT
    def getCameraModel(self):
        model = self.m_cam.getModel()
        deb.Return('Getting camera model')
        return model

    @DEB_MEMBER_FUNCT
    def getCameraModelStr(self):
        model = self.m_cam.getModel()
        model_str = 'Frelon %s #%d, FW:%s' % \
                    (model.getName(), model.getSerialNb(),
                     model.getFirmware().getVersionStr())
        deb.Return('Getting camera model str: %s' % model_str)
        return model_str

    @DEB_MEMBER_FUNCT
    def getFrameDim(self, max_dim=False):
        if max_dim:
            max_size = self.m_ct_image.getMaxImageSize()
            fdim = FrameDim(max_size, self.m_ct_image.getImageType())
        else:
            fdim = self.m_ct_image.getImageDim()
        deb.Return('Frame dim: %s' % fdim)
        return fdim

    @DEB_MEMBER_FUNCT
    def getMaxRoi(self):
        max_roi_size = self.m_ct_image.getMaxImageSize()
        max_roi_size /= Point(self.m_ct_image.getBin())
        max_roi = Roi(Point(0, 0), max_roi_size)
        deb.Return('Max roi: %s' % max_roi)
        return max_roi

    @DEB_MEMBER_FUNCT
    def setRoi(self, roi):
        deb.Param('Setting roi: %s' % roi)
        if roi == self.getMaxRoi():
            roi = Roi()
        self.m_ct_image.setRoi(roi)

    @DEB_MEMBER_FUNCT
    def getRoi(self):
        roi = self.m_ct_image.getRoi()
        if roi.isEmpty():
            roi = self.getMaxRoi()
        deb.Return('Getting roi: %s' % roi)
        return roi

    @DEB_MEMBER_FUNCT
    def setRoiLineBegin(self, roi_line_beg):
        deb.Param('Setting Roi line begin: %s' % roi_line_beg)
        bin = self.m_ct_image.getBin()
        roi = self.getRoi()
        roi = roi.getUnbinned(bin)

        tl = Point(roi.getTopLeft().x, roi_line_beg)
        tl_aligned = Point(tl)
        tl_aligned.alignTo(Point(bin), Floor)
        roi.setTopLeft(tl_aligned)
        roi = roi.getBinned(bin)
        self.m_ct_image.setRoi(roi)

        roi_bin_offset  = tl
        roi_bin_offset -= tl_aligned

        self.m_cam.setRoiBinOffset(roi_bin_offset)

    @DEB_MEMBER_FUNCT
    def getRoiLineBegin(self):
        bin = self.m_ct_image.getBin()
        roi = self.getRoi()
        roi = roi.getUnbinned(bin)
        tl  = roi.getTopLeft()
        tl += self.m_cam.getRoiBinOffset()
        roi_line_beg = tl.y
        deb.Return('Getting Roi line begin: %s' % roi_line_beg)
        return roi_line_beg

    @DEB_MEMBER_FUNCT
    def execFrelonSerialCmd(self, cmd):
        deb.Param('Executing Frelon serial cmd: %s' % cmd)
        ser_line = self.m_cam.getSerialLine()
        ser_line.write(cmd)
        resp = ser_line.readLine()
        self.checkMonitoredSerialCmd(cmd)
        deb.Return('Received response:')
        for line in resp.split('\r\n'):
            deb.Return(line)
        return resp

    @DEB_MEMBER_FUNCT
    def checkMonitoredSerialCmd(self, cmd):
        ser_line = self.m_cam.getSerialLine()
        msg_part = ser_line.splitMsg(cmd)
        cstr = msg_part[ser_line.MsgCmd]
        if cstr not in self.MonitoredSerialDict or msg_part[ser_line.MsgReq]:
            return
        reg = self.MonitoredSerialDict[cstr]
        try:
            val = int(msg_part[ser_line.MsgVal])
            vstr = '=%d' % val
        except:
            val, vstr = None, ''
        deb.Trace("Detected monitored serial command: %s%s" % (cstr, vstr))
        if reg == Frelon.LatencyTime:
            hw_sync = self.m_hw_inter.getHwCtrlObj(HwCap.Sync)
            lat_time = hw_sync.getLatTime()
            deb.Trace("Updating LatTime: %.3f ms" % (lat_time * 1e3))
            ct_acq = self.m_ct.acquisition()
            ct_acq.setLatencyTime(lat_time)
        elif reg == Frelon.ShutCloseTime:
            hw_shut = self.m_hw_inter.getHwCtrlObj(HwCap.Shutter)
            shut_time = hw_shut.getCloseTime()
            deb.Trace("Updating ShutCloseTime: %.3f ms" % (shut_time * 1e3))
            ct_shut = self.m_ct.shutter()
            ct_shut.setCloseTime(shut_time)
        elif reg == Frelon.ShutEnable:
            hw_shut = self.m_hw_inter.getHwCtrlObj(HwCap.Shutter)
            shut_mode = hw_shut.getMode()
            shut_enable = (((shut_mode == ShutterAutoFrame) and 'AutoFrame')
                                                            or  'Off')
            deb.Trace("Updating ShutEnable: %s" % shut_enable)
            ct_shut = self.m_ct.shutter()
            ct_shut.setMode(shut_mode)

    @DEB_MEMBER_FUNCT
    def setE2VCorrectionActive(self, e2v_corr_act):
        deb.Param('Setting e2v_corr_act to %s' % e2v_corr_act)
        self.m_e2v_corr_act = e2v_corr_act
        self.checkE2VCorrection()

    @DEB_MEMBER_FUNCT
    def getE2VCorrectionActive(self):
        e2v_corr_act = self.m_e2v_corr_act
        deb.Param('Getting e2v_corr_act: %s' % e2v_corr_act)
        return e2v_corr_act
