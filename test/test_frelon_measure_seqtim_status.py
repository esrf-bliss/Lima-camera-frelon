import sys
import time
from Lima import Core, Frelon


acq = None
cam = None
fser_line = None

nb_frames = 3
exp_time = 1e-3
start_time = 5e-3

def init(frelon_acq_args):
    global acq, cam, fser_line
    acq = Frelon.FrelonAcq(*frelon_acq_args)
    cam = acq.getFrelonCamera()
    fser_line = cam.getSerialLine()

def start():
    start_t0 = time.time()
    fser_line.sendFmtCmd('S')
    e = time.time() - start_t0
    print(f'Started [{e * 1e3:.3f} ms]')

def readStatus():
    return time.time(), cam.getStatus()

def getTotalTime():
    frame_time = exp_time + cam.getTransferTime() + cam.getReadoutTime()
    return nb_frames * frame_time

def printStatus(t, status, check_time=False):
    tot_time = getTotalTime()
    comment = ''
    e = max(0, t - t0)
    is_wait = status == Frelon.Wait
    if is_wait or check_time:
        comment = '- Idle ' if is_wait else '- False'
        if e and e < tot_time - start_time:
            comment += f' Shorter (Expected: {tot_time:.6f})'
    print(f'{e:12.6f} ms: 0x{status:02x} {comment}')

frelon_acq_args = [int(x) for x in sys.argv[1:]]
init(frelon_acq_args)
cam.waitStatus(Frelon.Wait, Frelon.StatusMask, 3)

for m in [9, 10]:
    cam.writeRegister(Frelon.ConfigHD, 0)
    cam.writeRegister(Frelon.BinVert, 1)
    cam.writeRegister(Frelon.ChanMode, m)

    cam.writeRegister(Frelon.NbFrames, nb_frames)
    i = int(exp_time / 1e-3)
    cam.writeRegister(Frelon.ExpTime, i)
    cam.writeRegister(Frelon.TimeUnit, 0)
    cam.writeRegister(Frelon.LatencyTime, 0)
    cam.writeRegister(Frelon.ShutEnable, 0)
    cam.writeRegister(Frelon.ShutCloseTime, 0)

    readout_time = cam.getReadoutTime()
    transfer_time = cam.getTransferTime()
    tot_time = getTotalTime()
    print(f'Running M={m}: I={i}, Readout={readout_time:.6f}, '
          f'Transfer={transfer_time:.6f}, Total={tot_time:.6f}')
    t, status = readStatus()
    status_list = [(t, status)]
    start()
    t0 = time.time()
    while t - t0 < getTotalTime() * 2:
        prev_status = status
        t, status = readStatus()
        if status != prev_status:
            status_list.append((t, status))
    for t, status in status_list:
        printStatus(t, status)
    print()

    t, status = readStatus()
    status_list = [(t, status, False)]
    start()
    t0 = time.time()
    cam.waitStatus(Frelon.Wait, Frelon.StatusMask, 3)
    t, status = readStatus()
    status_list.append((t, status, True))
    cam.waitStatus(Frelon.Wait, Frelon.StatusMask, 3, False, True)
    t, status = readStatus()
    status_list.append((t, status, False))
    for t, status, check_time in status_list:
        printStatus(t, status, check_time)
    print() 
   
