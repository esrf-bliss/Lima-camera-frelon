import PyTango
import time
import sys

ln = '//pulsar:20000/isg/limaccds/frelon1'
fn = '//pulsar:20000/isg/frelon/frelon1'

l = PyTango.DeviceProxy(ln)
f = PyTango.DeviceProxy(fn)

def execCmd(cmd):
  a = f.execSerialCommand(cmd)
  lines = a.split('\r\n')[:-1]
  if len(lines) == 1:
    a = lines[0][4:]
    try:
        return int(a)
    except:
        return a
  return lines

def calibrate(nb_images = 10):
  tango_retries = 10
  t0 = time.time()
  for i in range(tango_retries):
    s = l.State()
  t1 = time.time()
  tango_time = (t1 - t0) / tango_retries
  print 'tango_time = %.3f ms' % (tango_time * 1e3)

  seq_status_retries = 10
  t0 = time.time()
  for i in range(seq_status_retries):
    s = f.seq_status
  t1 = time.time()
  seq_status_time = (t1 - t0) / seq_status_retries
  print 'seq_status_time = %.3f ms' % (seq_status_time * 1e3)

  cmd = '>VER?\r\n'
  t0 = time.time()
  ans = f.execSerialCommand(cmd)
  t1 = time.time()
  ser_line_chr_time = (t1 - t0) / len(cmd + ans)
  print 'ser_line_chr_time = %.3f ms' % (ser_line_chr_time * 1e3)

  image_sizes = l.image_sizes
  image_size = image_sizes[1] * image_sizes[2] * image_sizes[3]
  espia_xfer_time = image_size / 200e6
  print 'espia_xfer_time = %.3f ms' % (espia_xfer_time * 1e3)

  l.acq_expo_time = 1.1e-6
  l.acq_nb_frames = nb_images
  l.acq_trigger_mode = 'INTERNAL_TRIGGER'
  l.prepareAcq()
  t0 = time.time()
  l.startAcq()
  waitAcq()
  t1 = time.time()

  start_time = len('>S\r\n') * ser_line_chr_time
  readout_time = (t1 - t0 - start_time - espia_xfer_time - tango_time) / nb_images
  print 'measured   readout_time = %.3f ms' % (readout_time * 1e3)
  print 'calculated readout_time = %.3f ms' % (float(execCmd('TRD?')) / 1e3)
  return readout_time

def waitAcq(d=False):
  while l.acq_status == 'Running':
    if d:
      sys.stdout.write('%d\r' % (l.last_image_acquired + 1))
      sys.stdout.flush()
  if d:
    sys.stdout.write('%d\n' % (l.last_image_acquired + 1))


calibrate()
