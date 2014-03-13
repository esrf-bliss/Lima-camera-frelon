import time
import signal

from Lima import Core, GLDisplay, Frelon

acq = Frelon.FrelonAcq(0)
ct_control = acq.getGlobalControl()

class FrelonCleanup(GLDisplay.GLForkCallback):
	def execInForked(self):
		global acq
		del acq
		
frelon_cleanup = FrelonCleanup()

gldisplay = GLDisplay.CtSPSGLDisplay(ct_control, [])
gldisplay.addForkCallback(frelon_cleanup)
gldisplay.setSpecArray('GLDisplayTest', 'Frelon')
gldisplay.createWindow()
signal.signal(signal.SIGCHLD, signal.SIG_IGN)

ct_acq = ct_control.acquisition()
ct_acq.setAcqNbFrames(10)
ct_acq.setAcqExpoTime(0.1)

gldisplay.setNorm(0, 200, 0)

ct_control.prepareAcq()
ct_control.startAcq()
ct_status = ct_control.getStatus
while ct_status().AcquisitionStatus == Core.AcqRunning:
        time.sleep(0.2)
        print 'Frame: %d' % ct_status().ImageCounters.LastImageReady

nframes = ct_status().ImageCounters.LastImageReady + 1
print 'Finished: %d' % nframes

print 'Press any key to quit'
raw_input()
