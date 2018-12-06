from Lima import Core, Frelon
import time

class FrelonSavingTest:

    espia_dev_nb = 0
    exp_time = 1e-3
    nb_frames = 5

    saving_dir = '/nobackup/frelonisg12/data/frelon/test'
    saving_prefix = 'test_'
    saving_idx = 1000

    def __init__(self, **kws):
        self.__dict__.update(kws)

        self.acq = Frelon.FrelonAcq(self.espia_dev_nb)
        self.ct = self.acq.getGlobalControl()
        self.ct_acq = self.ct.acquisition()
        self.ct_saving = self.ct.saving()

        self.setupAcq()
        self.setupSaving()

    def __del__(self):
        del self.ct_saving
        del self.ct_acq
        del self.ct

    def setupAcq(self):
        self.ct_acq.setAcqExpoTime(self.exp_time)
        self.ct_acq.setAcqNbFrames(self.nb_frames)

    def setupSaving(self):
        format_list = self.ct_saving.getFormatListAsString()
        print('Available saving format list:')
        print('   %s' % format_list)
        pars = self.ct_saving.getParameters()
        pars.directory = self.saving_dir
        pars.prefix = self.saving_prefix
        pars.nextNumber = self.saving_idx
        print("Warning: Overwrite policy should be used only in tests!")
        pars.overwritePolicy = Core.CtSaving.Overwrite
        self.ct_saving.setParameters(pars)

    def runAcq(self, desc):
        self.ct.prepareAcq()
        acq = self.ct_acq
        eta = ((acq.getAcqExpoTime() + acq.getLatencyTime()) *
               acq.getAcqNbFrames())
        self.ct.startAcq()
        t0 = time.time()
        print("%s: Acq started (ETA: %.2f s) ..." % (desc, eta))
        while self.ct.getStatus().AcquisitionStatus == Core.AcqRunning:
            time.sleep(10e-3)
        t1 = time.time()
        print("Acq finished (elapsed: %.2f s)!" % (t1 - t0))

    def runNoSaveTest(self):
        pars = self.ct_saving.getParameters()
        pars.savingMode = Core.CtSaving.Manual
        self.ct_saving.setParameters(pars)
        self.runAcq('No auto-saving')

    def setupEDFSave(self, frames_per_file=1):
        pars = self.ct_saving.getParameters()
        pars.fileFormat = Core.CtSaving.EDF
        pars.suffix = '.edf'
        pars.framesPerFile = frames_per_file
        pars.savingMode = Core.CtSaving.AutoFrame
        self.ct_saving.setParameters(pars)
        
    def runSaveEDFTest(self):
        self.setupEDFSave(1)
        self.runAcq('Auto-saving in single-image EDF')
        self.setupEDFSave(self.nb_frames)
        self.runAcq('Auto-saving in multi-image EDF')

    def setupHDF5Save(self, saving_format=Core.CtSaving.HDF5):
        pars = self.ct_saving.getParameters()
        pars.fileFormat = saving_format
        pars.suffix = '.h5'
        pars.framesPerFile = self.ct_acq.getAcqNbFrames()
        pars.savingMode = Core.CtSaving.AutoFrame
        self.ct_saving.setParameters(pars)

    def runSaveHDF5Test(self):
        self.setupHDF5Save(Core.CtSaving.HDF5)
        self.runAcq('Auto-saving in HDF5')
        self.setupHDF5Save(Core.CtSaving.HDF5GZ)
        self.runAcq('Auto-saving in HDF5GZ')
        self.setupHDF5Save(Core.CtSaving.HDF5BS)
        self.runAcq('Auto-saving in HDF5BS')

    def run(self):
        self.runNoSaveTest()
        self.runSaveEDFTest()
        self.runSaveHDF5Test()


if __name__ == '__main__':
    test = FrelonSavingTest()
    test.run()
