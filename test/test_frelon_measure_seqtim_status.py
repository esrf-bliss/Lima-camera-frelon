import sys
import time
from Lima import Core, Frelon


acq = None
cam = None
fser_line = None

def init(frelon_acq_args):
    global acq, cam, fser_line
    acq = Frelon.FrelonAcq(*frelon_acq_args)
    cam = acq.getFrelonCamera()
    fser_line = cam.getSerialLine()

def readStatus():
    return time.time(), cam.getStatus()

def printStatus(t, status):
    comment = ''
    if status == Frelon.Wait:
        comment = '- Idle'
        e = t - t0
        if e and e < 150e-3:
            comment += ' Shorter!'
    print(f'{t - t0:12.6f} ms: 0x{status:02x} {comment}')

frelon_acq_args = [int(x) for x in sys.argv[1:]]
init(frelon_acq_args)
cam.writeRegister(Frelon.ConfigHD, 0)
cam.writeRegister(Frelon.BinVert, 1)
cam.writeRegister(Frelon.ChanMode, 9)

t, status = readStatus()
if status != Frelon.Wait:
    cam.waitStatus(Frelon.Wait, Frelon.StatusMask, 3)
    t, status = readStatus()

cam.writeRegister(Frelon.NbFrames, 3)
cam.writeRegister(Frelon.ExpTime, 1)
cam.writeRegister(Frelon.LatencyTime, 0)
cam.writeRegister(Frelon.ShutEnable, 0)

t, status = readStatus()
t0 = t
printStatus(t, status)
status_list = []
fser_line.sendFmtCmd('S')
while t - t0 < 2:
    prev_status = status
    t, status = readStatus()
    if status != prev_status:
        status_list.append((t, status))
for t, status in status_list:
    printStatus(t, status)
print()

t, status = readStatus()
t0 = t
printStatus(t, status)
fser_line.sendFmtCmd('S')
cam.waitStatus(Frelon.Wait, Frelon.StatusMask, 3)
t, status = readStatus()
printStatus(t, status)
print()
    
