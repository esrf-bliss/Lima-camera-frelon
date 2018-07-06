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
    def __init__(self, espia_dev_nb):
        self.m_cam_inited    = False

        self.m_e2v_corr      = None
        self.m_e2v_corr_update = None
        self.m_e2v_corr_act  = True

        self.m_bpm_mgr       = Tasks.BpmManager()
        self.m_bpm_task      = Tasks.BpmTask(self.m_bpm_mgr)

        self.m_edev          = Espia.Dev(espia_dev_nb)
        self.m_acq           = Espia.Acq(self.m_edev)
        self.m_buffer_cb_mgr = Espia.BufferMgr(self.m_acq)
        self.m_eserline      = Espia.SerialLine(self.m_edev)
        self.m_cam           = Frelon.Camera(self.m_eserline)
        self.m_buffer_mgr    = BufferCtrlMgr(self.m_buffer_cb_mgr)
        self.m_hw_inter      = Frelon.Interface(self.m_acq, self.m_buffer_mgr,
                                                self.m_cam)
        self.m_hw_sync       = self.m_hw_inter.getHwCtrlObj(HwCap.Sync)
        self.m_hw_shut       = self.m_hw_inter.getHwCtrlObj(HwCap.Shutter)

        self.m_ct            = CtControl(self.m_hw_inter)
        self.m_ct_acq        = self.m_ct.acquisition()
        self.m_ct_saving     = self.m_ct.saving()
        self.m_ct_image      = self.m_ct.image()
        self.m_ct_buffer     = self.m_ct.buffer()
        self.m_ct_display    = self.m_ct.display()
        self.m_ct_shut       = self.m_ct.shutter()

        self.m_cam_inited    = True

        self.checkE2VCorrection()

    @DEB_MEMBER_FUNCT
    def __del__(self):
        if self.m_cam_inited:
            del self.m_ct_acq, self.m_ct_saving, self.m_ct_image, \
                self.m_ct_buffer, self.m_ct_display, self.m_ct_shut
            del self.m_ct;		gc.collect()

            del self.m_hw_sync, self.m_hw_shut
            del self.m_hw_inter;	gc.collect()
            del self.m_buffer_mgr;	gc.collect()
            del self.m_cam;		gc.collect()

        del self.m_eserline;		gc.collect()
        del self.m_buffer_cb_mgr;	gc.collect()
        del self.m_acq;			gc.collect()
        del self.m_edev;		gc.collect()

        if self.m_e2v_corr:
            del self.m_e2v_corr_update
            del self.m_e2v_corr;	gc.collect()

        del self.m_bpm_task;		gc.collect()
        del self.m_bpm_mgr;		gc.collect()

    def getEspiaDev(self):
        return self.m_edev

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
    def reset(self):
        deb.Trace('Reseting the device!')
        self.m_ct.reset()

    @DEB_MEMBER_FUNCT
    def resetDefaults(self):
        deb.Trace('Reseting to default settings')
        self.m_hw_inter.resetDefaults()
        deb.Trace('Forcing Control Layer to synchronise')
        self.reset()

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
    def getStatus(self):
        ct_status = self.m_ct.getStatus()
        deb.Return('Getting global status: %s' % ct_status)
        return ct_status

    @DEB_MEMBER_FUNCT
    def resetStatus(self):
        deb.Trace('Reseting global status')
        self.m_ct.resetStatus(True)

    @DEB_MEMBER_FUNCT
    def getCcdStatus(self):
        ccd_status = self.m_cam.getStatus()
        deb.Return('Getting CCD status: 0x%02X' % ccd_status)
        return ccd_status

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
    def setAcqMode(self, acq_mode):
        deb.Param('Setting acq. mode: %s' % acq_mode)
        self.m_ct_acq.setAcqMode(acq_mode)

    @DEB_MEMBER_FUNCT
    def getAcqMode(self):
        acq_mode = self.m_ct_acq.getAcqMode()
        deb.Return('Getting acq. mode: %s' % acq_mode)
        return acq_mode

    @DEB_MEMBER_FUNCT
    def setFlip(self, flip):
        deb.Param('Setting flip mode: %s' % flip)
        self.m_ct_image.setFlip(flip)

    @DEB_MEMBER_FUNCT
    def getFlip(self):
        flip = self.m_ct_image.getFlip()
        deb.Return('Getting flip mode: %s' % flip)
        return flip

    @DEB_MEMBER_FUNCT
    def setBin(self, bin):
        deb.Param('Setting binning: %s' % bin)
        self.m_ct_image.setBin(bin)

    @DEB_MEMBER_FUNCT
    def getBin(self):
        bin = self.m_ct_image.getBin()
        deb.Return('Getting binning: %s' % bin)
        return bin

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
    def setRoiMode(self, roi_mode):
        deb.Param('Setting roi mode: %s' % roi_mode)
        self.m_cam.setRoiMode(roi_mode)

    @DEB_MEMBER_FUNCT
    def getRoiMode(self):
        roi_mode = self.m_cam.getRoiMode()
        deb.Return('Getting roi mode: %s' % roi_mode)
        return roi_mode

    @DEB_MEMBER_FUNCT
    def setRoiBinOffset(self, roi_bin_offset):
        deb.Param('Setting Roi-Bin offset: %s' % roi_bin_offset)
        self.m_cam.setRoiBinOffset(roi_bin_offset)

    @DEB_MEMBER_FUNCT
    def getRoiBinOffset(self):
        roi_bin_offset = self.m_cam.getRoiBinOffset()
        deb.Return('Getting Roi-Bin offset: %s' % roi_bin_offset)
        return roi_bin_offset

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

        self.setRoiBinOffset(roi_bin_offset)

    @DEB_MEMBER_FUNCT
    def getRoiLineBegin(self):
        bin = self.m_ct_image.getBin()
        roi = self.getRoi()
        roi = roi.getUnbinned(bin)
        tl  = roi.getTopLeft()
        tl += self.getRoiBinOffset()
        roi_line_beg = tl.y
        deb.Return('Getting Roi line begin: %s' % roi_line_beg)
        return roi_line_beg

    @DEB_MEMBER_FUNCT
    def setFrameTransferMode(self, ftm):
        deb.Param('Setting frame transfer mode: ftm=%s' % ftm)
        self.m_cam.setFrameTransferMode(ftm)

    @DEB_MEMBER_FUNCT
    def getFrameTransferMode(self):
        ftm = self.m_cam.getFrameTransferMode()
        deb.Return('Getting frame transfer mode: ftm=%s' % ftm)
        return ftm

    @DEB_MEMBER_FUNCT
    def setInputChan(self, input_chan):
        input_chan = Frelon.InputChan(input_chan)
        ftm = self.m_cam.getFrameTransferMode()
        mode_name = self.m_cam.getInputChanModeName(ftm, input_chan)
        deb.Param('Setting input channel: %s [%s]' % (input_chan, mode_name))
        self.m_cam.setInputChan(input_chan)

    @DEB_MEMBER_FUNCT
    def getInputChan(self):
        input_chan = self.m_cam.getInputChan()
        ftm = self.m_cam.getFrameTransferMode()
        mode_name = self.m_cam.getInputChanModeName(ftm, input_chan)
        deb.Return('Getting input channel: %s [%s]' % (input_chan, mode_name))
        return input_chan

    @DEB_MEMBER_FUNCT
    def setTriggerMode(self, trig_mode):
        deb.Param('Setting trigger mode: %s' % trig_mode)
        prev_trig_mode = self.m_ct_acq.getTriggerMode()
        set_exp_time = ((trig_mode == IntTrig) and (prev_trig_mode == ExtGate))
        self.m_ct_acq.setTriggerMode(trig_mode)
        if set_exp_time:
            self.m_ct_acq.setAcqExpoTime(1.0)

    @DEB_MEMBER_FUNCT
    def getTriggerMode(self):
        trig_mode = self.m_ct_acq.getTriggerMode()
        deb.Return('Getting trigger mode: %s' % trig_mode)
        return trig_mode

    @DEB_MEMBER_FUNCT
    def setNbFrames(self, nb_frames):
        deb.Param('Setting nb. frames: %s' % nb_frames)
        self.m_ct_acq.setAcqNbFrames(nb_frames)

    @DEB_MEMBER_FUNCT
    def getNbFrames(self):
        nb_frames = self.m_ct_acq.getAcqNbFrames()
        deb.Return('Getting nb. frames: %s' % nb_frames)
        return nb_frames

    @DEB_MEMBER_FUNCT
    def setStripeConcat(self, stripe_concat):
        deb.Param('Setting stripe concat.: %s' % stripe_concat)
        if stripe_concat:
            acq_mode = Concatenation
        else:
            acq_mode = Single
        self.setAcqMode(acq_mode)

    @DEB_MEMBER_FUNCT
    def getStripeConcat(self):
        acq_mode = self.getAcqMode()
        stripe_concat = (acq_mode == Concatenation)
        deb.Return('Getting stripe concat.: %s' % stripe_concat)
        return stripe_concat

    @DEB_MEMBER_FUNCT
    def setNbConcatFrames(self, concat_frames):
        deb.Param('Setting nb. concat. frames: %s' % concat_frames)
        self.m_ct_acq.setConcatNbFrames(concat_frames)

    @DEB_MEMBER_FUNCT
    def getNbConcatFrames(self):
        concat_frames = self.m_ct_acq.getConcatNbFrames()
        deb.Return('Getting nb. concat. frames: %s' % concat_frames)
        return concat_frames

    @DEB_MEMBER_FUNCT
    def setExpTime(self, exp_time):
        deb.Param('Setting exp. time: %s' % exp_time)
        self.m_ct_acq.setAcqExpoTime(exp_time)

    @DEB_MEMBER_FUNCT
    def getExpTime(self):
        exp_time = self.m_ct_acq.getAcqExpoTime()
        deb.Return('Getting exp. time: %s' % exp_time)
        return exp_time

    @DEB_MEMBER_FUNCT
    def setSPB2Config(self, spb2_config):
        deb.Param('Setting SPB2 config: %s' % spb2_config)
        self.m_cam.setSPB2Config(spb2_config)

    @DEB_MEMBER_FUNCT
    def getSPB2Config(self):
        spb2_config = self.m_cam.getSPB2Config()
        deb.Param('Getting SPB2 config: %s' % spb2_config)
        return spb2_config

    @DEB_MEMBER_FUNCT
    def setFileStreamAct(self, act, stream_idx):
        deb.Param('Setting file stream %d act: %s' % (stream_idx, act))
        self.m_ct_saving.setStreamActive(stream_idx, act)

    @DEB_MEMBER_FUNCT
    def getFileStreamAct(self, stream_idx):
        act = self.m_ct_saving.getStreamActive(stream_idx)
        deb.Return('Getting file stream %d act: %s' % (stream_idx, act))
        return act

    @DEB_MEMBER_FUNCT
    def setFileStreamPar(self, file_par, stream_idx=0):
        deb.Param('Setting file stream %d par: %s' % (stream_idx, file_par))
        self.m_ct_saving.setParameters(file_par, stream_idx)

    @DEB_MEMBER_FUNCT
    def getFileStreamPar(self, stream_idx=0):
        file_par = self.m_ct_saving.getParameters(stream_idx)
        deb.Return('Getting file stream %d par: %s' % (stream_idx, file_par))
        return file_par

    @DEB_MEMBER_FUNCT
    def setCommonFileHeader(self, header_map):
        deb.Param('Setting common file header: %d keys' % len(header_map))
        self.m_ct_saving.setCommonHeader(header_map)

    @DEB_MEMBER_FUNCT
    def writeFile(self, frame_nb, nb_frames=1):
        deb.Param('Writing %d frame(s) starting from %s to file' %
                  (nb_frames, frame_nb))
        self.m_ct_saving.writeFrame(frame_nb, nb_frames)

    @DEB_MEMBER_FUNCT
    def setAutosave(self, autosave_act):
        deb.Param('Setting autosave active: %s' % autosave_act)
        if autosave_act:
            saving_mode = CtSaving.AutoFrame
        else:
            saving_mode = CtSaving.Manual
        self.m_ct_saving.setSavingMode(saving_mode)

    @DEB_MEMBER_FUNCT
    def getAutosave(self):
        saving_mode = self.m_ct_saving.getSavingMode()
        autosave_act = (saving_mode == CtSaving.AutoFrame)
        deb.Return('Getting autosave active: %s' % autosave_act)
        return autosave_act

    @DEB_MEMBER_FUNCT
    def setLiveDisplay(self, livedisplay_act):
        deb.Param('Setting live display active: %s' % livedisplay_act)
        if livedisplay_act:
            try:
                self.m_ct_display.setNames('_ccd_ds_', 'frelon_live')
            except:
                pass
        self.m_ct_display.setActive(livedisplay_act)

    @DEB_MEMBER_FUNCT
    def getLiveDisplay(self):
        livedisplay_act = self.m_ct_display.isActive()
        deb.Return('Getting live display active: %s' % livedisplay_act)
        return livedisplay_act

    @DEB_MEMBER_FUNCT
    def startLive(self):
        deb.Trace('Starting live mode')
        self.setNbFrames(0)
        self.startAcq()

    @DEB_MEMBER_FUNCT
    def stopLive(self):
        deb.Trace('Stoping live mode')
        self.stopAcq()

    @DEB_MEMBER_FUNCT
    def startAcq(self):
        deb.Trace('Starting the device')
        self.m_ct.prepareAcq()
        self.m_ct.startAcq()

    @DEB_MEMBER_FUNCT
    def stopAcq(self):
        deb.Trace('Stopping the device')
        self.m_ct.stopAcq()

    @DEB_MEMBER_FUNCT
    def readFrames(self, frame_nb, read_block_len=1):
        img_data = self.m_ct.ReadImage(frame_nb, read_block_len)
        data = img_data.buffer
        s = data.tostring()
        deb.Return('Getting frame(s) #%s: %s bytes' % (frame_nb, len(s)))
        return s

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
            lat_time = self.m_hw_sync.getLatTime()
            deb.Trace("Updating LatTime: %.3f ms" % (lat_time * 1e3))
            self.m_ct_acq.setLatencyTime(lat_time)
        elif reg == Frelon.ShutCloseTime:
            shut_time = self.m_hw_shut.getCloseTime()
            deb.Trace("Updating ShutCloseTime: %.3f ms" % (shut_time * 1e3))
            self.m_ct_shut.setCloseTime(shut_time)
        elif reg == Frelon.ShutEnable:
            shut_mode = self.m_hw_shut.getMode()
            shut_enable = (((shut_mode == ShutterAutoFrame) and 'AutoFrame')
                                                            or  'Off')
            deb.Trace("Updating ShutEnable: %s" % shut_enable)
            self.m_ct_shut.setMode(shut_mode)

    @DEB_MEMBER_FUNCT
    def readBeamParams(self):
        frame_nb = -1
        img_data = self.m_ct.ReadImage(frame_nb)
        beam_params = self.calcBeamParams(img_data)
        deb.Return('Beam params: %s' % beam_params)
        return beam_params

    @DEB_MEMBER_FUNCT
    def calcBeamParams(self, img_data):
        self.m_bpm_task.process(img_data)
        timeout = 1
        bpm_pars = self.m_bpm_mgr.getResult(timeout)
        if bpm_pars.errorCode != self.m_bpm_mgr.OK:
            raise Exception('Error calculating beam params: %d' %
                              bpm_pars.errorCode)

        nr_spots = 1
        auto_cal = -1
        exp_time = self.getExpTime()
        if exp_time > 0:
            norm_intensity = bpm_pars.beam_intensity / exp_time
        else:
            norm_intensity = 0

        beam_params = [nr_spots,
                       bpm_pars.beam_intensity,
                       bpm_pars.beam_center_x,
                       bpm_pars.beam_center_y,
                       bpm_pars.beam_fwhm_x,
                       bpm_pars.beam_fwhm_y,
                       bpm_pars.AOI_max_x - bpm_pars.AOI_min_x,
                       bpm_pars.AOI_max_y - bpm_pars.AOI_min_y,
                       bpm_pars.max_pixel_value,
                       bpm_pars.max_pixel_x,
                       bpm_pars.max_pixel_y,
                       bpm_pars.AOI_min_x,
                       bpm_pars.AOI_min_y,
                       bpm_pars.AOI_max_x,
                       bpm_pars.AOI_max_y,
                       bpm_pars.beam_center_x - bpm_pars.beam_fwhm_x / 2,
                       bpm_pars.beam_center_y - bpm_pars.beam_fwhm_y / 2,
                       bpm_pars.beam_center_x + bpm_pars.beam_fwhm_x / 2,
                       bpm_pars.beam_center_y + bpm_pars.beam_fwhm_y / 2,
                       norm_intensity,
                       auto_cal]
        deb.Return('Getting beam params: %s' % beam_params)
        return beam_params

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
