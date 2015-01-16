import fnmatch
import glob
import logging
from os import path
import os
from stat import S_ISREG, ST_MODE, ST_MTIME
import subprocess
import sys


__author__ = 'Oliver Schmidts'


class JPSRunTestDriver(object):

    def __init__(self, testnumber, argv0):
        self.SUCCESS = 0
        self.FAILURE = 1

        # check if testnumber is an int
        assert isinstance(testnumber, float)
        self.testno = testnumber
        self.logfile = "log_test_%d.txt" % self.testno

        # touch file if not already there
        f = open(self.logfile)
        f.close()
        logging.basicConfig(filename=self.logfile, level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s')
        self.HOME = path.expanduser("~")

        # only allow path and strings as path directory name
        assert path.exists(argv0)
        assert isinstance(argv0, str)
        self.DIR = os.path.dirname(os.path.realpath(argv0))
        self.CWD = os.getcwd()
        self.FILE = os.path.join(self.DIR, "master_ini.xml")

    def run_test(self, testfunction, *args):
        assert hasattr(testfunction, '__call__')
        self.__configure()
        executable = self.__find_executable()
        for inifile in self.inifiles:
            self.__execute_test(executable, inifile, testfunction, *args)
        return

    def __configure(self):
        if self.CWD != self.DIR:
            logging.info("working dir is %s. Change to %s" % (os.getcwd(), self.DIR))
            os.chdir(self.DIR)
        logging.info("change directory to ..")
        os.chdir("..")
        logging.info("call makeini.py with -f %s" % self.FILE)
        subprocess.call(["python", "makeini.py", "-f", "%s" % self.FILE])
        os.chdir(self.DIR)
        # -------- get directory of the code TRUNK
        os.chdir("../..")
        self.trunk = os.getcwd()
        os.chdir(self.DIR)
        lib_path = os.path.abspath(os.path.join(self.trunk, "Utest"))
        sys.path.append(lib_path)
        logging.info("change directory back to %s" % self.DIR)

        # initialise the inputfiles for jpscore
        self.geofile = os.path.join(self.DIR, "geometry.xml")
        self.inifiles = glob.glob(os.path.join("inifiles", "*.xml"))
        if not path.exists(self.geofile):
            logging.critical("geofile <%s> does not exist" % self.geofile)
            exit(self.FAILURE)
        for inifile in self.inifiles:
            if not path.exists(inifile):
                logging.critical("inifile <%s> does not exist" % inifile)
                exit(self.FAILURE)
        return

    def __find_executable(self):
        executable = os.path.join(self.trunk, "bin", "jpscore")

        # fix for windows
        if not path.exists(executable):
            matches = []
            for root, dirname, filenames in os.walk(os.path.join(self.trunk, 'bin')):
                for filename in fnmatch.filter(filenames, 'jpscore.exe'):
                    matches.append(os.path.join(root, filename))
            if len(matches) == 0:
                logging.critical("executable <%s> or jpscore.exe does not exist yet." % executable)
                exit(self.FAILURE)
            elif len(matches) > 1:
                matches = ((os.stat(file_path), file_path) for file_path in matches)
                matches = ((stat[ST_MTIME], file_path) for stat, file_path in matches if S_ISREG(stat[ST_MODE]))
                matches = sorted(matches)
            executable = matches[0]
        # end fix for windows

        return executable

    def __execute_test(self, executable, inifile, testfunction, *args):
        cmd = "%s --inifile=%s"%(executable, inifile)
        logging.info('start simulating with exe=<%s>' % cmd)
        subprocess.call([executable, "--inifile=%s" % inifile])
        logging.info('end simulation ...\n--------------\n')
        trajfile = os.path.join("trajectories", "traj" + inifile.split("ini")[2])
        logging.info('trajfile = <%s>' % trajfile)
        if not path.exists(trajfile):
            logging.critical("trajfile <%s> does not exist" % trajfile)
            exit(self.FAILURE)
        testfunction(inifile, trajfile, *args)
        return