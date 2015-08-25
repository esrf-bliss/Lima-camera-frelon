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
import sys
import getopt
from Lima import Core, Frelon
import time

Core.DEB_GLOBAL(Core.DebModTest)

class ImageStatusCallback(Core.CtControl.ImageStatusCallback):

    Core.DEB_CLASS(Core.DebModTest, "ImageStatusCallback")

    @Core.DEB_MEMBER_FUNCT
    def __init__(self, ct, acq_state):
        Core.CtControl.ImageStatusCallback.__init__(self)

        self.m_ct = ct
        self.m_acq_state = acq_state
        self.m_nb_frames = 0

    @Core.DEB_MEMBER_FUNCT
    def start(self):
        self.m_acq_state.set(Core.AcqState.Acquiring)
        self.m_start_time = time.time()
        self.m_stat_acc = [0.0] * 4
        ct_acq = self.m_ct.acquisition()
        self.m_nb_frames = ct_acq.getAcqNbFrames()
        
    @Core.DEB_MEMBER_FUNCT
    def imageStatusChanged(self, cb_img_status):
        t = time.time()
        global_status = self.m_ct.getStatus()
        acq_status = global_status.AcquisitionStatus
        img_status = global_status.ImageCounters
        
        last_acq_frame_nb = img_status.LastImageAcquired;
        last_base_frame_nb = img_status.LastBaseImageReady;
        last_ready_frame_nb = img_status.LastImageReady;

        dt = t - self.m_start_time
        n = last_acq_frame_nb + 1
        acc = self.m_stat_acc
        acc[0] += n
        acc[1] += n**2
        acc[2] += dt
        acc[3] += n * dt
        
        acq_state_changed = False
        if last_ready_frame_nb == self.m_nb_frames - 1:
            self.m_acq_state.set(Core.AcqState.Finished)
            acq_state_changed = True

        pt = t0 = 0
        if n > 1:
            pt = (acc[0] * acc[2] - n * acc[3]) / (acc[0]**2 - n * acc[1])
            t0 = (acc[2] - pt * acc[0]) / n
        if True:
            deb.Always("Last Acquired: %8d, Last Base: %8d, " \
                       "Last Ready: %8d, Status: %s, PT: %.6f, T0: %.6f" %
                       (last_acq_frame_nb, last_base_frame_nb,
                        last_ready_frame_nb, acq_status, pt, t0))

@Core.DEB_GLOBAL_FUNCT
def test_frelon_acc(enable_debug, espia_dev_nb, vert_bin, exp_time,
                    acc_max_exp_time, nb_frames, acc_time_mode):

    if enable_debug:
        Core.DebParams.enableModuleFlags(Core.DebParams.AllFlags)
        Core.DebParams.enableTypeFlags(Core.DebParams.AllFlags)
    else:
        Core.DebParams.disableModuleFlags(Core.DebParams.AllFlags)

    deb.Always("Creating FrelonAcq")
    acq = Frelon.FrelonAcq(espia_dev_nb)
    deb.Trace("Done!")

    cam = acq.getFrelonCamera()
 
    ct = acq.getGlobalControl()
    ct_image = ct.image()
    ct_acq = ct.acquisition()
    
    acq_state = Core.AcqState()
    status_cb = ImageStatusCallback(ct, acq_state) 
    ct.registerImageStatusCallback(status_cb)

    deb.Always("Setting Full Frame Mode - Speed - Chan 3&4")
    cam.setSPB2Config(Frelon.SPB2Speed)
    cam.setFrameTransferMode(Frelon.FFM)
    cam.setInputChan(Frelon.Chan34)
    deb.Trace("Done!")

    frame_dim = acq.getFrameDim(max_dim=True)
    bin = Core.Bin(1, vert_bin)
    deb.Always("Setting binning %s" % bin)
    ct_image.setBin(bin);
    deb.Trace("Done!")

    image_size = frame_dim.getSize()
    nb_cols = image_size.getWidth()
    nb_lines = image_size.getHeight() / vert_bin
    roi = Core.Roi(Core.Point(0, nb_lines - 2), Core.Size(nb_cols, 1));
    deb.Always("Setting RoI %s" % roi)
    ct_image.setRoi(roi);
    deb.Trace("Done!")

    deb.Always("Setting Kinetic RoI mode")
    cam.setRoiMode(Frelon.Kinetic);
    deb.Trace("Done!")

    deb.Always("Setting Acc. mode")
    ct_acq.setAcqMode(Core.Accumulation)
    deb.Trace("Done!")


    deb.Always("Setting %s Acc. Time mode" % acc_time_mode)
    acc_mode = Core.CtAcquisition.Live \
               if acc_time_mode == 'Live' \
               else Core.CtAcquisition.Real
    ct_acq.setAccTimeMode(acc_mode)
    deb.Trace("Done!")

    deb.Always("Setting Acc. max. exp. time %s" % acc_max_exp_time)
    ct_acq.setAccMaxExpoTime(acc_max_exp_time)
    deb.Trace("Done!")

    deb.Always("Setting exp. time %s" % exp_time)
    ct_acq.setAcqExpoTime(exp_time)
    deb.Trace("Done!")

    deb.Always("Setting nb. frames %s" % nb_frames)
    ct_acq.setAcqNbFrames(nb_frames)
    deb.Trace("Done!")

    acc_nb_frames = ct_acq.getAccNbFrames()
    acc_dead_time = ct_acq.getAccDeadTime()
    acc_live_time = ct_acq.getAccLiveTime()
    deb.Always("AccNbFrames: %d, AccDeadTime: %.6f, AccLiveTime: %.6f" %
               (acc_nb_frames, acc_dead_time, acc_live_time))
    
    deb.Always("Preparing acq.")
    ct.prepareAcq()
    deb.Trace("Done!")
    
    deb.Always("Starting acq.")
    status_cb.start()
    ct.startAcq()
    deb.Trace("Done!")

    state_mask = Core.AcqState.Acquiring
    acq_state.waitNot(state_mask)
    deb.Always("Finished!")

    dead_time = cam.getDeadTime()
    read_time = cam.getReadoutTime()
    xfer_time = cam.getTransferTime()
    deb.Always("Dead Time: %.6f, Readout Time: %.6f, Transfer Time: %.6f" %
               (dead_time, read_time, xfer_time))
    
def main(argv):

    enable_debug = False
    espia_dev_nb = 0
    vert_bin = 64
    exp_time = 20e-3
    acc_max_exp_time = 2e-3
    nb_frames = 10
    acc_time_mode = 'Live'
        
    opts, args = getopt.getopt(argv[1:], 'de:v:t:m:n:r')
    for opt, val in opts:
        if opt == '-d':
            enable_debug = True
        if opt == '-e':
            espia_dev_nb = int(val)
        if opt == '-v':
            vert_bin = int(val)
        if opt == '-t':
            exp_time = float(val)
        if opt == '-m':
            acc_max_exp_time = float(val)
        if opt == '-n':
            nb_frames = int(val)
        if opt == '-r':
            acc_time_mode = 'Real'

    test_frelon_acc(enable_debug, espia_dev_nb, vert_bin, exp_time,
                    acc_max_exp_time, nb_frames, acc_time_mode)

if __name__ == '__main__':
    main(sys.argv)
