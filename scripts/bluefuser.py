#!/usr/bin/env python3

# for Bluetooth
BT_ADDR = "00:06:66:73:e3:a6"
BT_PORT = 1

# for plotting
BARO_RANGE    = 20
SONAR_RANGE   = 200

import numpy as np
from altitude_fuser import ASL_EKF, ASL_Plotter
import threading
from math import sin, pi

# ground-truth AGL to sonar measurement, empirically determined:
# see http://diydrones.com/profiles/blogs/altitude-hold-with-mb1242-sonar
def sonarfun( agl):

    return 0.933 * agl - 2.894


BARO_BASELINE = 97420

class Bluetooth_ASL_EKF(ASL_EKF):

    def __init__(self):

        ASL_EKF.__init__(self)

    def getBaroBaseline(self):

        return BARO_BASELINE

class Bluetooth_ASLPlotter(ASL_Plotter):

    def __init__(self):

        ASL_Plotter.__init__(self, Bluetooth_ASL_EKF())
        self.count = 0

    def getSensors(self):

        LOOPSIZE = 5000

        # Model up-and-down motion with a sine wave
        self.count = (self.count + 1) % LOOPSIZE
        sine = sin(self.count/float(LOOPSIZE) * 2 * pi)

        baro  = BARO_BASELINE + sine * BARO_RANGE

        # Add noise to simulated sonar at random intervals
        sonar = sonarfun(50*(1-sine)) + (50 if np.random.rand()>0.9 else 0)

        return baro, sonar

if __name__ == '__main__':

    plotter = Bluetooth_ASLPlotter()

    thread = threading.Thread(target=plotter.update)
    thread.daemon = True

    thread.start()
    plotter.start()
