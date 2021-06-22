/*
   Hackflight sketch for Tlear Butterfly with USFS IMU, DSMX receiver, and
   standard motors

   Additional libraries needed:

       https://github.com/simondlevy/EM7180
       https://github.com/simondlevy/CrossPlatformDataBus
       https://github.com/simondlevy/SpektrumDSM 

    Copyright (C) 2021 Simon D. Levy
 
   MIT License
 */

#include "hackflight.hpp"
#include "mixers/quadxmw.hpp"
#include "pidcontrollers/rate.hpp"
#include "pidcontrollers/yaw.hpp"
#include "pidcontrollers/level.hpp"
#include "sensors/usfs.hpp"

#include <rft_boards/realboards/arduino/butterfly.hpp>
#include <rft_motors/rotary/brushless.hpp>

#include "receivers/mock.hpp"

// Receiver ============================================================================

static hf::MockReceiver receiver;

// Board ================================================================================

// static rft::TinyPico board;
static rft::Butterfly board;

// Motors  ==============================================================================

static rft::BrushlessMotorEsp32 motor1 = rft::BrushlessMotorEsp32(11);
static rft::BrushlessMotorEsp32 motor2 = rft::BrushlessMotorEsp32(9);
static rft::BrushlessMotorEsp32 motor3 = rft::BrushlessMotorEsp32(8);
static rft::BrushlessMotorEsp32 motor4 = rft::BrushlessMotorEsp32(5);

// Mixer ================================================================================

static hf::MixerQuadXMW mixer = hf::MixerQuadXMW(&motor1, &motor2, &motor3, &motor4);

// Hackflight object ====================================================================

static hf::Hackflight h = hf::Hackflight(&board, &receiver, &mixer);

// PID controllers ======================================================================

static hf::RatePid ratePid = hf::RatePid(0.04, 0.002, 0.01);
static hf::YawPid yawPid = hf::YawPid(0.10, 0.01);
static hf::LevelPid levelPid = hf::LevelPid(0.20);

// Sensors ==============================================================================

static hf::UsfsGyrometer gyrometer;
static hf::UsfsQuaternion quaternion; // not really a sensor, but we treat it like one!

// Setup ==============================================================================

void setup(void)
{
    // USFS piggyback: use pins 4,3 for power, ground
    rft::ArduinoBoard::powerPins(4, 3);
    delay(100);

    // Start I^2C
    Wire.begin(TWI_PINS_6_7);

    // Add sensors
    h.addSensor(&quaternion);
    h.addSensor(&gyrometer);

    // Add PID controllers
    h.addPidController(&levelPid);
    h.addPidController(&ratePid);
    h.addPidController(&yawPid);

    // Adjust trim
    receiver.setTrim(0, 0.05, 0);

    // Start Hackflight firmware
    h.begin();
}

// Loop ===============================================================================

void loop(void)
{
    h.update();
}
