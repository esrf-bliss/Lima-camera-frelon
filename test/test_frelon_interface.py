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
from Lima import Core, Espia, Frelon
import time
import numpy as N

Data_UNDEF  = 0
Data_UINT8  = 1
Data_INT8   = 2
Data_UINT16 = 3
Data_INT16  = 4
Data_UINT32 = 5
Data_INT32  = 6
Data_UINT64 = 7
Data_INT64  = 8
Data_FLOAT  = 9
Data_DOUBLE = 10
	

Core.DEB_GLOBAL(Core.DebModTest)

class SoftRoiCallback( Core.Processlib.TaskEventCallback ):

	Core.DEB_CLASS(Core.DebModTest, "SoftRoiCallback")

	DataType2ImageType = {
		N.int8:   Core.Bpp8,
		N.uint8:  Core.Bpp8,
		N.int16:  Core.Bpp16,
		N.uint16: Core.Bpp16,
		N.int32:  Core.Bpp32,
		N.uint32: Core.Bpp32
	}

	@Core.DEB_MEMBER_FUNCT
	def __init__(self, hw_inter, buffer_save, acq_state):
		Core.Processlib.TaskEventCallback.__init__(self)
		self.m_hw_inter = hw_inter
		self.m_buffer_save = buffer_save
		self.m_acq_state = acq_state

	@Core.DEB_MEMBER_FUNCT
	def finished(self, data):
		finfo, fdim = self.data2FrameInfo(data)
		self.m_buffer_save.writeFrame(finfo)

		hw_sync = self.m_hw_inter.getHwCtrlObj(Core.HwCap.Sync)
		nb_frames = hw_sync.getNbFrames()
		if finfo.acq_frame_nb == nb_frames - 1:
			self.m_acq_state.set(Core.AcqState.Finished)

	@Core.DEB_MEMBER_FUNCT
	def data2FrameInfo(self, data):
		arr = data.buffer
		arr_type = arr.dtype.type
		arr_height, arr_width = arr.shape
		
		image_type = self.DataType2ImageType[arr_type]

		buffer_ctrl = self.m_hw_inter.getHwCtrlObj(Core.HwCap.Buffer)
		start_ts = buffer_ctrl.getStartTimestamp()

		fdim = Core.FrameDim(arr_width, arr_height, image_type)
		timestamp = Core.Timestamp(data.timestamp)
		valid_pixels = Core.Point(fdim.getSize()).getArea()

		ownership = Core.HwFrameInfoType.Managed
		finfo = Core.HwFrameInfoType(data.frameNumber, arr, timestamp, 
					     valid_pixels, ownership)
		return finfo, fdim

	
class TestFrameCallback( Core.HwFrameCallback ):

	Core.DEB_CLASS(Core.DebModTest, "TestFrameCallback")

	ImageType2DataType = {
		Core.Bpp8:  Data_UINT8,
		Core.Bpp16: Data_UINT16,
		Core.Bpp32: Data_UINT32
	}
	
	@Core.DEB_MEMBER_FUNCT
	def __init__(self, hw_inter, soft_roi, buffer_save, acq_state):
		Core.HwFrameCallback.__init__(self)
		self.m_hw_inter = hw_inter
		self.m_soft_roi = soft_roi
		self.m_acq_state = acq_state
		self.m_roi_task = Core.Processlib.Tasks.SoftRoi()
		self.m_roi_cb   = SoftRoiCallback(hw_inter, buffer_save, 
						  acq_state)

	@Core.DEB_MEMBER_FUNCT
	def newFrameReady(self, frame_info):
		msg  = 'acq_frame_nb=%d, ' % frame_info.acq_frame_nb
		data = self.frameInfo2Data(frame_info)
		buffer = data.buffer
		msg += 'frame_dim=%dx%dx%d, ' % \
		       (buffer.shape[1], buffer.shape[0], buffer.dtype.itemsize)
		msg += 'frame_timestamp=%.6f, ' % frame_info.frame_timestamp
		msg += 'valid_pixels=%d' % frame_info.valid_pixels
		deb.Always("newFrameReady: %s" % msg)

		if self.m_soft_roi.isActive():
			pass
		else:
			self.m_roi_cb.finished(data)
			
		return True

	@Core.DEB_MEMBER_FUNCT
	def frameInfo2Data(self, frame_info):
		data = Core.Processlib.Data()
		data.buffer = frame_info.frame_data
		data.frameNumber = frame_info.acq_frame_nb
		data.timestamp = frame_info.frame_timestamp
		
		return data


class MaxImageSizeCallback( Core.HwMaxImageSizeCallback ):

	Core.DEB_CLASS(Core.DebModTest, "MaxImageSizeCallback")
	
	@Core.DEB_MEMBER_FUNCT
	def maxImageSizeChanged(self, size, image_type):
		fdim = Core.FrameDim(size, image_type)
		msg = "size=%sx%s, image_type=%s, depth=%d" % \
		      (size.getWidth(), size.getHeight(), image_type, \
		       fdim.getDepth())
		deb.Always("MaxImageSizeChanged: %s" % msg)


@Core.DEB_GLOBAL_FUNCT
def main(argv):

	deb.Always("Creating Espia.Dev")
	edev = Espia.Dev(0)

	deb.Always("Creating Espia.Acq")
	acq = Espia.Acq(edev)

	acqstat = acq.getStatus()
	deb.Always("Whether the Acquisition is running : %s" %
		       acqstat.running)

	deb.Always("Creating Espia.BufferMgr")
	buffer_cb_mgr = Espia.BufferMgr(acq)

	deb.Always("Creating BufferCtrlMgr")
	buffer_mgr = Core.BufferCtrlMgr(buffer_cb_mgr)

	deb.Always("Creating Espia.SerialLine")
	eser_line = Espia.SerialLine(edev)

	deb.Always("Creating Frelon.Camera")
	cam = Frelon.Camera(eser_line)

	deb.Always("Creating the Hw Interface ... ")
	hw_inter = Frelon.Interface(acq, buffer_mgr, cam)

	deb.Always("Creating HW BufferSave")
	buffer_save = Core.HwBufferSave(Core.HwBufferSave.EDF, "img", 0, 
					".edf", True, 1)

	deb.Always("Getting HW detector info")
	hw_det_info = hw_inter.getHwCtrlObj(Core.HwCap.DetInfo)

	deb.Always("Getting HW buffer")
	hw_buffer = hw_inter.getHwCtrlObj(Core.HwCap.Buffer)

	deb.Always("Getting HW Sync")
	hw_sync = hw_inter.getHwCtrlObj(Core.HwCap.Sync)

	deb.Always("Getting HW Bin")
	hw_bin = hw_inter.getHwCtrlObj(Core.HwCap.Bin)

	deb.Always("Getting HW RoI")
	hw_roi = hw_inter.getHwCtrlObj(Core.HwCap.Roi)

	mis_cb = MaxImageSizeCallback()
	hw_det_info.registerMaxImageSizeCallback(mis_cb)

	deb.Always("Setting FTM")
	cam.setFrameTransferMode(Frelon.FTM)
	deb.Always("Setting FFM")
	cam.setFrameTransferMode(Frelon.FFM)
	
	soft_roi = Core.Roi()
	acq_state = Core.AcqState()
	deb.Always("Creating a TestFrameCallback")
	cb = TestFrameCallback(hw_inter, soft_roi, buffer_save, acq_state)

	do_reset = False
	if do_reset:
		deb.Always("Reseting hardware ...")
		hw_inter.reset(HwInterface.HardReset)
		deb.Always("  Done!")

	size = hw_det_info.getMaxImageSize()
	image_type = hw_det_info.getCurrImageType()
	frame_dim = Core.FrameDim(size, image_type)

	bin = Core.Bin(Core.Point(1))
	hw_bin.setBin(bin)

	#Roi set_roi, real_roi;
	#set_hw_roi(hw_roi, set_roi, real_roi, soft_roi);

	effect_frame_dim = Core.FrameDim(frame_dim)  # was (frame_dim / bin)
	hw_buffer.setFrameDim(effect_frame_dim)
	hw_buffer.setNbBuffers(10)
	hw_buffer.registerFrameCallback(cb)

	hw_sync.setExpTime(2)
	hw_sync.setNbFrames(3)

	deb.Always("Starting Acquisition")
	acq_state.set(Core.AcqState.Acquiring)
	hw_inter.startAcq()

	deb.Always("Waiting acq finished...")
	acq_state.waitNot(Core.AcqState.Acquiring)
	deb.Always("Acq finished!!")

	deb.Always("Stopping Acquisition")
	hw_inter.stopAcq()

	deb.Always("This is the End...")


if __name__ == '__main__':
	main(sys.argv)
