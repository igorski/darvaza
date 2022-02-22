/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2022 Igor Zinken - https://www.igorski.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __REVERB__H_INCLUDED__
#define __REVERB__H_INCLUDED__

#include "global.h"
#include "audiobuffer.h"
#include "comb.h"
#include "allpass.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class Reverb {

    struct CombFilter {
        std::vector<Comb*> filters;
        std::vector<float*> buffers;

        ~CombFilter() {
            while ( !filters.empty() ) {
                delete filters.at( 0 );
                filters.erase( filters.begin() );
            }
            while ( !buffers.empty() ) {
                delete buffers.at( 0 );
                buffers.erase( buffers.begin() );
            }
        }
    };

    struct AllPassFilter {
        std::vector<AllPass*> filters;
        std::vector<float*> buffers;

        ~AllPassFilter() {
            while ( !filters.empty() ) {
                delete filters.at( 0 );
                filters.erase( filters.begin() );
            }
            while ( !buffers.empty() ) {
                delete buffers.at( 0 );
                buffers.erase( buffers.begin() );
            }
        }
    };

    static constexpr float MAX_RECORD_TIME_MS = 5000.f;
    static constexpr float MUTED              = 0;
    static constexpr float FIXED_GAIN         = 0.015f;
    static constexpr float SCALE_WET          = 1.f;
    static constexpr float SCALE_DRY          = 1.f;
    static constexpr float SCALE_DAMP         = 0.4f;
    static constexpr float SCALE_ROOM         = 0.28f;
    static constexpr float OFFSET_ROOM        = 0.7f;
    static constexpr float INITIAL_ROOM       = 0.5f;
    static constexpr float INITIAL_DAMP       = 0.5f;
    static constexpr float INITIAL_WET        = 1 / SCALE_WET;
    static constexpr float INITIAL_DRY        = 0.5;
    static constexpr float INITIAL_WIDTH      = 1;
    static constexpr float INITIAL_MODE       = 0;
    static constexpr float FREEZE_MODE        = 0.5f;
    static constexpr int STEREO_SPREAD        = 23;

    public:
        Reverb();
        ~Reverb();

        // apply effect to incoming sampleBuffer contents

        void process( float* inBuffer, int bufferSize );

        inline float processSingle( float inputSample ) {

            // ---- REVERB process

            float processedSample = 0;
            inputSample *= _gain;

            // accumulate comb filters in parallel

            for ( int i = 0; i < VST::NUM_COMBS; i++ ) {
                processedSample += _combFilter->filters.at( i )->process( inputSample );
            }

            // feed through all pass filters in series

            for ( int i = 0; i < VST::NUM_ALLPASSES; i++ ) {
                processedSample = _allpassFilter->filters.at( i )->process( processedSample );
            }

            // wet mix (e.g. the reverberated signal) and dry mix (e.g. mix in the input signal)
            return ( processedSample * _wet1 ) + ( inputSample * _dry );
        }

        void mute();
        void setRoomSize( float value );
        float getRoomSize();
        void setDamp( float value );
        float getDamp();
        void setWet( float value );
        float getWet();
        void setDry( float value );
        float getDry();
        float getWidth();
        void setWidth( float value );
        float getMode();
        void setMode( float value );
        void toggleFreeze();

    private:
        int  _amountOfChannels;

        void setupFilters(); // generates comb and allpass filter buffers
        void clearFilters(); // frees memory allocated to comb and allpass filter buffers
        void update();

        float _gain;
        float _roomSize, _roomSize1;
        float _damp, _damp1;
        float _wet, _wet1, _wet2;
        float _dry;
        float _width;
        float _mode;

        CombFilter*     _combFilter   = nullptr;
        AllPassFilter* _allpassFilter = nullptr;
};
}

#endif
