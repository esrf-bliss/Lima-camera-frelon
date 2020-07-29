import sys
from Lima import Core, Frelon
from collections import namedtuple

acq = None
cam = None
ct_control = None
ct_image = None

RoiConfig = namedtuple('RoiConfig',
                       ['enable', 'fast', 'kinetic', 'line_begin', 'line_width'],
                       defaults=[0, 0, 0, 0, 1])

ImageConfig = namedtuple('ImageConfig', ['config_hd', 'bin_vert', 'chan_mode'])

def init(frelon_acq_args):
    global acq, cam, ct_control, ct_image
    acq = Frelon.FrelonAcq(*frelon_acq_args)
    cam = acq.getFrelonCamera()
    ct_control = acq.getGlobalControl()
    ct_image = ct_control.image()

def toUSec(x):
    return int(x * 1e6)

def measure(check_expected=False):
    expected = None
    needed_seqtim_measure = False
    if check_expected:
        needed_seqtim_measure = cam.needSeqTimMeasure()
        if needed_seqtim_measure:
            expected = (0, 0)
        else:
            expected = (toUSec(cam.getReadoutTime()),
                        toUSec(cam.getTransferTime()))

    image_config = getImageConfig()
    st = cam.measureSeqTimValues()
    readout_us, transfer_us = toUSec(st.readout_time), toUSec(st.transfer_time)
    ok = not expected or (readout_us, transfer_us) == expected
    if ok:
        status = 'OK'
    else:
        status = ('Camera needed SeqTim measurement'
                  if needed_seqtim_measure
                  else f'Mismatch: expected {expected}')
    print(f'Config {image_config}: '
          f'readout_time={readout_us:8d} us, '
          f'transfer_time={transfer_us:8d} us: {status}')

def setRoi(roi_config):
    cam.writeRegister(Frelon.RoiEnable, roi_config.enable)
    cam.writeRegister(Frelon.RoiFast, roi_config.fast)
    cam.writeRegister(Frelon.RoiKinetic, roi_config.kinetic)
    cam.writeRegister(Frelon.RoiLineBegin, roi_config.line_begin)
    cam.writeRegister(Frelon.RoiLineWidth, roi_config.line_width)

def setImageConfig(image_config):
    cam.writeRegister(Frelon.ConfigHD, image_config.config_hd)
    cam.writeRegister(Frelon.BinVert, image_config.bin_vert)
    cam.writeRegister(Frelon.ChanMode, image_config.chan_mode)
    cam.writeRegister(Frelon.NbLinesXfer, 0)
    cam.writeRegister(Frelon.ShutElecSelect, 0)

def getImageConfig():
    return ImageConfig(cam.readRegister(Frelon.ConfigHD),
                       cam.readRegister(Frelon.BinVert),
                       cam.readRegister(Frelon.ChanMode))

def testRois():
    image_config = ImageConfig(0, 1, 10)
    setImageConfig(image_config)

    print('Testing RoIs: no RoI')
    no_roi_config = RoiConfig()
    setRoi(no_roi_config)
    measure()

    print('Testing RoIs: Slow RoI')
    slow_roi_config = RoiConfig(enable=1, line_width=960)
    setRoi(slow_roi_config)
    measure()

    print('Testing RoIs: Fast RoI')
    fast_roi_config = RoiConfig(enable=1, fast=1, line_width=960)
    setRoi(fast_roi_config)
    measure()

    print('Testing RoIs: Kinetic RoI')
    kinetic_roi_config = RoiConfig(kinetic=1, line_width=960)
    setRoi(kinetic_roi_config)
    measure()

def calibrateImageConfigs():
    no_roi_config = RoiConfig()
    setRoi(no_roi_config)

    image_config_list = [ImageConfig(config_hd, bin_vert, chan_mode)
                         for config_hd in (0, 1)
                         for chan_mode in (9, 10)
                         for bin_vert in (1, 2, 3)]
    for image_config in image_config_list:
        setImageConfig(image_config)
        measure(True)


def main():
    debug = False
    if debug:
        Core.DebParams.setTypeFlags(0xffff)

    frelon_acq_args = [int(x) for x in sys.argv[1:]]
    init(frelon_acq_args)
    if not cam.getModel().has(Frelon.Model.SeqTim):
        print('This script can only run on Frelons with SeqTim measurement')
        exit(1)

    testRois()

    calibrateImageConfigs()


if __name__ == '__main__':
    try:
        main()
    except Core.Exception as e:
        print(e)
        exit(1)

