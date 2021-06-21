/*
   Mixer class

   Copyright (c) 2018 Simon D. Levy

   MIT License
 */

#pragma once

#include "filters.hpp"
#include "motor.hpp"
#include "actuator.hpp"

namespace hf {

    class Mixer : public Actuator {

        friend class Hackflight;
        friend class SerialTask;

        private:

            // Custom mixer data per motor
            typedef struct motorMixer_t {
                int8_t throttle; // T
                int8_t roll; 	 // A
                int8_t pitch;	 // E
                int8_t yaw;	     // R
            } motorMixer_t;

            // Arbitrary
            static const uint8_t MAXMOTORS = 20;

            float _motorsPrev[MAXMOTORS] = {0};

            void writeMotor(uint8_t index, float value)
            {
                _motors->write(index, value);
            }

            void safeWriteMotor(uint8_t index, float value)
            {
                // Avoid sending the motor the same value over and over
                if (_motorsPrev[index] != value) {
                    writeMotor(index, value);
                }

                _motorsPrev[index] = value;
            }

        // XXX protected:
        public:

            Motor * _motors;

            motorMixer_t motorDirections[MAXMOTORS];

            Mixer(Motor * motors, uint8_t nmotors)
            {
                _motors = motors;
                _nmotors = nmotors;

                // set disarmed, previous motor values
                for (uint8_t i = 0; i < nmotors; i++) {
                    motorsDisarmed[i] = 0;
                    _motorsPrev[i] = 0;
                }

            }

            uint8_t _nmotors;

            // This is also use by serial task
            float  motorsDisarmed[MAXMOTORS];

            void begin(void) override
            {
                _motors->begin();
            }

            // This is how we can spin the motors from the GCS
            void runDisarmed(void) override
            {
                for (uint8_t i = 0; i < _nmotors; i++) {
                    safeWriteMotor(i, motorsDisarmed[i]);
                }
            }

            // This helps support servos
            virtual float constrainMotorValue(uint8_t index, float value) 
            {
                (void)index;
                return Filter::constrainMinMax(value, 0, 1);
            }

            // Actuator overrides ----------------------------------------------

            void run(float * demands) override
            {
                // Map throttle demand from [-1,+1] to [0,1]
                demands[DEMANDS_THROTTLE] = (demands[DEMANDS_THROTTLE] + 1) / 2;

                float motorvals[MAXMOTORS];

                for (uint8_t i = 0; i < _nmotors; i++) {

                    motorvals[i] = 
                        (demands[DEMANDS_THROTTLE] * motorDirections[i].throttle + 
                         demands[DEMANDS_ROLL]     * motorDirections[i].roll +     
                         demands[DEMANDS_PITCH]    * motorDirections[i].pitch +   
                         demands[DEMANDS_YAW]      * motorDirections[i].yaw);      
                }

                float maxMotor = motorvals[0];

                for (uint8_t i = 1; i < _nmotors; i++)
                    if (motorvals[i] > maxMotor)
                        maxMotor = motorvals[i];

                for (uint8_t i = 0; i < _nmotors; i++) {

                    // This is a way to still have good gyro corrections if at least one motor reaches its max
                    if (maxMotor > 1) {
                        motorvals[i] -= maxMotor - 1;
                    }

                    // Keep motor values in appropriate interval
                    motorvals[i] = constrainMotorValue(i, motorvals[i]);
                }

                for (uint8_t i = 0; i < _nmotors; i++) {
                    safeWriteMotor(i, motorvals[i]);
                }
            }

            void cut(void) override
            {
                for (uint8_t i = 0; i < _nmotors; i++) {
                    writeMotor(i, 0);
                }
            }

            virtual void setMotorDisarmed(uint8_t index, float value) override
            {
                motorsDisarmed[index] = value;
            }

    }; // class Mixer

} // namespace hf
