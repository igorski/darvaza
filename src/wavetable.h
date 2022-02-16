/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2022 Igor Zinken - https://www.igorski.nl
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
#ifndef __WAVETABLE_H_INCLUDED__
#define __WAVETABLE_H_INCLUDED__

#include "global.h"

namespace Igorski {
class WaveTable
{
    public:
        WaveTable( int aTableLength, float aFrequency );
        ~WaveTable();

        int tableLength;
        float* getBuffer();
        void setBuffer( float* aBuffer );

        void setFrequency( float aFrequency );
        float getFrequency();

        bool hasContent();

        // accumulators are used to retrieve a sample from the wave table

        float getAccumulator();
        void setAccumulator( float offset );

        /**
         * retrieve a value from the wave table for the current
         * accumulator position, this method also increments
         * the accumulator and keeps it within bounds
         */
        inline float peek()
        {
            // the wave table offset to read from
            int readOffset = ( _accumulator == 0 ) ? 0 : ( int ) ( _accumulator / SR_OVER_LENGTH );

            // increment the accumulators read offset
            _accumulator += _frequency;

            // keep the accumulator in the bounds of the sample frequency
            if ( _accumulator > VST::SAMPLE_RATE )
                _accumulator -= VST::SAMPLE_RATE;

            // return the sample present at the calculated offset within the table
            return _buffer[ readOffset ];
        }

        void cloneTable( WaveTable* waveTable );
        WaveTable* clone();

    protected:
        float* _buffer;       // cached buffer (is a wave table)
        float _accumulator;   // is read offset in wave table buffer
        float SR_OVER_LENGTH;
        float       _frequency;     // frequency (in Hz) of waveform cycle when reading

        float* generateSilentBuffer( int bufferSize );
};
} // E.O namespace Igorski

#endif
