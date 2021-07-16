/*
   Abstract RC receiver class

   Copyright (c) 2018 Simon D. Levy

   MIT License
 */

#pragma once

#include <stdint.h>
#include <math.h>

#include "HF_demands.hpp"

#include <RFT_openloop.hpp>

namespace hf {

    class Receiver : public rft::OpenLoopController {

        friend class Hackflight;
        friend class SerialTask;
        friend class PidTask;

        private: 

            const float THROTTLE_MARGIN = 0.1f;
            const float CYCLIC_EXPO     = 0.65f;
            const float CYCLIC_RATE     = 0.90f;
            const float THROTTLE_EXPO   = 0.20f;
            const float AUX_THRESHOLD   = 0.4f;

            float _demandScale = 0;

            float adjustCommand(float command, uint8_t channel)
            {
                command /= 2;

                if (rawvals[_channelMap[channel]] < 0) {
                    command = -command;
                }

                return command;
            }

            float applyCyclicFunction(float command)
            {
                return rcFun(command, CYCLIC_EXPO, CYCLIC_RATE);
            }

            float makePositiveCommand(uint8_t channel)
            {
                return fabs(rawvals[_channelMap[channel]]);
            }

            static float rcFun(float x, float e, float r)
            {
                return (1 + e*(x*x - 1)) * x * r;
            }

            // [-1,+1] -> [0,1] -> [-1,+1]
            float throttleFun(float x)
            {
                float mid = 0.5;
                float tmp = (x + 1) / 2 - mid;
                float y = tmp>0 ? 1-mid : (tmp<0 ? mid : 1);
                return (mid + tmp*(1-THROTTLE_EXPO + THROTTLE_EXPO * (tmp*tmp) / (y*y))) * 2 - 1;
            }

        protected: 

            // maximum number of channels that any receiver will send (of which we'll use six)
            static const uint8_t MAXCHAN = 8;

            uint8_t _aux1State = 0;
            uint8_t _aux2State = 0;

            // channel indices
            enum {
                CHANNEL_THROTTLE, 
                CHANNEL_ROLL,    
                CHANNEL_PITCH,  
                CHANNEL_YAW,   
                CHANNEL_AUX1,
                CHANNEL_AUX2
            };

            uint8_t _channelMap[6] = {0};

            // These must be overridden for each receiver
            virtual bool gotNewFrame(void) = 0;
            virtual void readRawvals(void) = 0;

            // Software trim
            float _trimRoll = 0;
            float _trimPitch = 0;
            float _trimYaw = 0;

            // Raw receiver values in [-1,+1]
            float rawvals[MAXCHAN] = {0};  

            // Demands (throttle, roll, pitch, yaw)
            float _demands[4] = {};

            float getRawval(uint8_t chan)
            {
                return rawvals[_channelMap[chan]];
            }

            /**
              * channelMap: throttle, roll, pitch, yaw, aux, arm
              */
            Receiver(const uint8_t channelMap[6],
                     const float demandScale,
                     const float trim[3] = NULL) 
            { 
                for (uint8_t k=0; k<6; ++k) {
                    _channelMap[k] = channelMap[k];
                }

                if (trim) {
                    _trimRoll  = trim[0];
                    _trimPitch = trim[1];
                    _trimYaw   = trim[2];
                }

                _demandScale = demandScale;
            }

            virtual bool ready(void) override
            {
                // Wait till there's a new frame
                if (!gotNewFrame()) return false;

                // Read raw channel values
                readRawvals();

                // Convert raw [-1,+1] to absolute value
                _demands[DEMANDS_ROLL]  = makePositiveCommand(CHANNEL_ROLL);
                _demands[DEMANDS_PITCH] = makePositiveCommand(CHANNEL_PITCH);
                _demands[DEMANDS_YAW]   = makePositiveCommand(CHANNEL_YAW);

                // Apply expo nonlinearity to roll, pitch
                _demands[DEMANDS_ROLL]  = applyCyclicFunction(_demands[DEMANDS_ROLL]);
                _demands[DEMANDS_PITCH] = applyCyclicFunction(_demands[DEMANDS_PITCH]);

                // Put sign back on command, yielding [-0.5,+0.5]
                _demands[DEMANDS_ROLL]  = adjustCommand(_demands[DEMANDS_ROLL], CHANNEL_ROLL);
                _demands[DEMANDS_PITCH] = adjustCommand(_demands[DEMANDS_PITCH], CHANNEL_PITCH);
                _demands[DEMANDS_YAW]   = adjustCommand(_demands[DEMANDS_YAW], CHANNEL_YAW);

                // Add in software trim
                _demands[DEMANDS_ROLL]  += _trimRoll;
                _demands[DEMANDS_PITCH] += _trimPitch;
                _demands[DEMANDS_YAW]   += _trimYaw;

                // Pass throttle demand through exponential function
                _demands[DEMANDS_THROTTLE] = throttleFun(rawvals[_channelMap[CHANNEL_THROTTLE]]);

                // Multiply by demand scale
                _demands[DEMANDS_ROLL] *= _demandScale;
                _demands[DEMANDS_PITCH] *= _demandScale;
                _demands[DEMANDS_YAW] *= _demandScale;

                // Store auxiliary switch state
                _aux1State = getRawval(CHANNEL_AUX1) >= 0.0 ? (getRawval(CHANNEL_AUX1) > AUX_THRESHOLD ? 2 : 1) : 0;
                _aux2State = getRawval(CHANNEL_AUX2) >= AUX_THRESHOLD ? 1 : 0;

                // Got a new frame
                return true;

            }  // ready

            virtual void getDemands(float * demands) override
            {
                memcpy(demands, _demands, sizeof(_demands));
            }

            virtual bool inactive(void) override
            {
                return getRawval(CHANNEL_THROTTLE) < -1 + THROTTLE_MARGIN;
            }

            virtual bool inArmedState(void) override
            {
                return _aux1State > 0;
            }

            virtual uint8_t getModeIndex(void) override
            {
                return _aux2State;
            }

    }; // class Receiver

} // namespace