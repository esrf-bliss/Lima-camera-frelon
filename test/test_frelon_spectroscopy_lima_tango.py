import time
import PyTango
import numpy as np
import struct

domain = 'isg'
server = 'frelon1'
limaccds_name = '%s/limaccds/%s' % (domain, server)
limafrelon_name = '%s/frelon/%s' % (domain, server)

vert_bin = 64
input_channel = None
throw_lines = 5

exp_time = 1.1e-6
slow_nb_frames = 10
fast_nb_frames = 1000

limaccds = PyTango.DeviceProxy(limaccds_name)
limafrelon = PyTango.DeviceProxy(limafrelon_name)

def get_serial_chr_delay():
    # CR-LF terminator is not mandatory at the end of command
    # It is added here for correctness in the real serial string sent
    cmd = 'VER?\r\n'  
    t0 = time.time()
    ans = limafrelon.execSerialCommand(cmd)
    t1 = time.time()
    return (t1 - t0) / len(cmd + ans)

serial_chr_delay = get_serial_chr_delay()

def get_cam_readout_time():
    cmd = 'TRD?'
    ans = limafrelon.execSerialCommand(cmd)
    return float(ans.strip()[4:]) * 1e-6

def get_espia_xfer_time():
    dtype, depth, width, height = limaccds.image_sizes
    image_size_mb = float(width * height * depth) / 1024**2
    xfer_speed_mb = 180
    return image_size_mb / xfer_speed_mb
    
def bench_acq(exp_time, nb_frames):
    limaccds.acq_expo_time = exp_time
    limaccds.acq_nb_frames = nb_frames
    height = limaccds.image_sizes[3]
    if height > 1:
        limaccds.acq_mode = 'SINGLE'
    else:
        limaccds.acq_mode = 'CONCATENATION'
        limaccds.concat_nb_frames = nb_frames
    limaccds.prepareAcq()
    t0 = time.time()
    limaccds.startAcq()
    while limaccds.acq_status == 'Running':
        pass
    t1 = time.time()
    theo_readout = get_cam_readout_time()
    start_cmd = '>S\r\n'
    start_cmd_delay = len(start_cmd) * serial_chr_delay
    espia_xfer_time = get_espia_xfer_time()
    frame_time = (t1 - t0 - start_cmd_delay - espia_xfer_time) / nb_frames
    meas_readout = frame_time - exp_time
    print '  Theory: %.2f ms, Measure: %.2f ms' % (theo_readout / 1e-3,
                                                   meas_readout / 1e-3)
    
def get_throw_lines():
    cmd = 'RLB?'
    ans = limafrelon.execSerialCommand(cmd)
    return int(ans.strip()[4:])

def decode_data_array(s):
    pack_str = '<IHHIIHHHHHHHHIIIIIIII'
    header_len = struct.calcsize(pack_str)
    u = struct.unpack(pack_str, s[:header_len])
    magic = u[0]				# 4 bytes I - magic number
    version = u[1]     				# 2 bytes H - version
    header_header_len = u[2]			# 2 bytes H - this header length
    category = u[3]    				# 4 bytes I - category (enum)
    da_type = u[4]				# 4 bytes I - data type (enum)
    endian = u[5]      				# 2 bytes H - endianness
    nb_dims = u[6]     				# 2 bytes H - nb of dims
    dim = u[7:13]				# 12 bytes H x 6 - dims
    step = u[13:19]				# 24 bytes I x 6 - stepsbytes
    pad = u[19:21]   				# padding 2 x 4 bytes
    if magic != 0x44544159:
        raise ValueError, 'Invalid magic 0x%x' % magic
    if version < 2:
        print 'Warning: using an old version: %d (expected %d)' % (version, 2)
    DataArrayNumpyType = [
        'u1', 'u2', 'u4', 'u8', 'i1', 'i2', 'i4', 'i8', 'f4', 'f8',
    ]
    endian_prefix = '>' if endian else '<'
    np_type = endian_prefix + DataArrayNumpyType[da_type]
    data = np.fromstring(s[header_header_len:], dtype=np.dtype(np_type))
    shape = [dim[i] for i in xrange(nb_dims - 1, -1, -1)]
    return data.reshape(shape)
    
def read_concat_stripes():
    buffer_frames = limaccds.concat_nb_frames
    codec, s = limaccds.readImageSeq(np.array([0, buffer_frames], 'u4'))
    if codec != 'DATA_ARRAY':
        raise RuntimeError, 'Unexpected DevEncoded type %s' % codec
    data = decode_data_array(s)
    lines, height, cols = data.shape
    bigEndian = np.dtype(data.dtype.byteorder + 'i4') == np.dtype('>i4')
    endianess = 'Big' if bigEndian else 'Little' 
    print 'Read %d frames of %d columns, %d lines each [%s-Endian]' % \
          (lines, cols, height, endianess)

limafrelon.input_channel = '1-2-3-4'
print 'Full frame %s:' % limafrelon.input_channel
limaccds.image_bin = np.array((1, 1))
limaccds.image_roi = np.array((0, 0, 0, 0))
bench_acq(exp_time, slow_nb_frames)

dtype, depth, width, height = limaccds.image_sizes
binned_size = height / vert_bin
max_line = binned_size - 1

def run_stripe(input_channel):
    line = 1
    if input_channel == '3-4':
        line = max_line - line
    
    limafrelon.roi_mode = 'SLOW'
    bin = np.array((1, vert_bin))
    roi = np.array((0, line, width, 1))
    limaccds.image_bin = bin
    limaccds.image_roi = roi
    limafrelon.input_channel = input_channel
    mode = 'Stripe %s %s' % (limafrelon.input_channel, limafrelon.roi_mode)
    print '%s:' % mode
    bench_acq(exp_time, slow_nb_frames)
    
    limafrelon.roi_mode = 'FAST'
    mode = 'Stripe %s %s' % (limafrelon.input_channel, limafrelon.roi_mode)
    print '%s:' % mode
    bench_acq(exp_time, slow_nb_frames)
    
    limafrelon.roi_mode = 'KINETIC'
    limafrelon.roi_bin_offset = 0
    real_thrown = get_throw_lines()
    mode = 'Stripe %s %s' % (limafrelon.input_channel, limafrelon.roi_mode)
    print '%s - [%s lines thrown]:' % (mode, real_thrown)
    bench_acq(exp_time, fast_nb_frames)
    
    if input_channel == '1-2':
        roi = np.array((0, 0, width, 1))
        limaccds.image_roi = roi
        limafrelon.roi_bin_offset = throw_lines
    else:
        limafrelon.roi_bin_offset = vert_bin - throw_lines
    
    real_thrown = get_throw_lines()
    mode = 'Stripe %s %s' % (limafrelon.input_channel, limafrelon.roi_mode)
    print '%s - [%s lines thrown]:' % (mode, real_thrown)
    bench_acq(exp_time, fast_nb_frames)

    read_concat_stripes()

for input_channel in ['1-2', '3-4']:
    run_stripe(input_channel)
    
