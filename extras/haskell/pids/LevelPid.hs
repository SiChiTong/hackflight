{--
  PID controller for roll/pitch angle

  Copyright(C) 2021 Simon D.Levy

  MIT License
--}

module LevelPid(levelController)

where

import VehicleState
import PidControl
import Demands
import Utils(deg2rad)

levelController :: Double -> Double -> PidController

levelController kp maxAngleDegrees =
    PidController (levelClosure kp maxAngleDegrees) NoState

levelClosure :: Double -> Double -> PidFun
levelClosure kp maxAngleDegrees =

    \vehicleState -> \demands -> \_controllerState ->

    let 
        -- Maximum roll pitch demand is +/-0.5, so to convert demand to
        -- angle for error computation, we multiply by the folling amount:
        demandScale = 2 * deg2rad(maxAngleDegrees)

    -- Return updated demands and controller state
    in (Demands (Demands.throttle demands) 

                -- New roll demand
                (kp * (Demands.roll demands) * demandScale -
                      (VehicleState.phi vehicleState))

                -- Pitch demand is nose-down positive, so we negate
                -- pitch-forward (nose-down negative) to reconcile them
                -- and get new pitch demand
                (kp * (Demands.pitch demands) * demandScale +
                      (VehicleState.theta vehicleState))

                (Demands.yaw demands),

        NoState)
