{--
  Copyright(C) 2021 Simon D.Levy

  MIT License
--}

-- import Hackflight2(hackflight)
-- import Mixer(quadXAPMixer)
import RatePid(rateController)
import YawPid(yawController)
import LevelPid(levelController)
import AltHoldPid(altHoldController)
import PosHoldPid(posHoldController)

-- import SimReceiver
-- import SimSensor

main :: IO ()

main = let 
           rate = rateController 0.225    -- Kp
                                 0.001875 -- Ki
                                 0.375    -- Kd
                                 0.4      -- windupMax
                                 40       -- maxDegreesPerSecond

           yaw = yawController 2.0 -- Kp
                               0.1 -- Ki
                               0.4 -- windupMax

           level = levelController 0.2 -- Kp
                                   45  -- maxAngleDegrees

           altHold = altHoldController 0.75 -- Kp
                                       1.5  -- Ki
                                       0.4  -- windupMax
                                       2.5  -- pilotVelZMax
                                       0.2  -- stickDeadband

           posHold = posHoldController 0.5 -- Kp
                                       0.2 -- stickDeadband

       in print "hello"