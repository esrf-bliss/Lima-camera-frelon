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

Core.DEB_GLOBAL(Core.DebModTest)

class DeadTimeChangedCallback(Frelon.Camera.DeadTimeChangedCallback):

    Core.DEB_CLASS(Core.DebModTest, "DeadTimeChangedCallback")

    @Core.DEB_MEMBER_FUNCT
    def __init__(self, cam):
        Frelon.Camera.DeadTimeChangedCallback.__init__(self)
        self.m_cam = cam
        
    @Core.DEB_MEMBER_FUNCT
    def deadTimeChanged(self, dead_time):
        cam = self.m_cam
        deb.Always("dead_time=%.6f, readout_time=%.6f, xfer_time=%.6f" %
                   (dead_time, cam.getReadoutTime(), cam.getTransferTime()))


def main():
    edev_nr = 0
    enable_debug = False
    
    if len(sys.argv) > 1:
        edev_nr = int(sys.argv[1])

    if enable_debug:
        Core.DebParams.enableModuleFlags(Core.DebParams.AllFlags)
        Core.DebParams.enableTypeFlags(Core.DebParams.AllFlags)
    else:
        Core.DebParams.disableModuleFlags(Core.DebParams.AllFlags)

    edev = Espia.Dev(edev_nr)
    eserline = Espia.SerialLine(edev)
    cam = Frelon.Camera(eserline)
    dt_cb = DeadTimeChangedCallback(cam)
    cam.registerDeadTimeChangedCallback(dt_cb)
    cam.setFrameTransferMode(Frelon.FTM)
    cam.setFrameTransferMode(Frelon.FFM)
    
if __name__ == '__main__':
    main()
