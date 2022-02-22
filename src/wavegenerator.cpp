/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2022 Igor Zinken - https://www.igorski.nl
 *
 * wave table generation adapted from sources by Matt @ hackmeopen.com
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
#include "wavegenerator.h"
#include <math.h>
#include <cmath>

namespace Igorski {
namespace WaveGenerator
{
    WaveTable* generate( int tableSize, WaveForms waveformType )
    {
        WaveTable* waveTable = new WaveTable( tableSize, 440.f );

        float* outputBuffer = waveTable->getBuffer();

        int tempValue, partials;
        float frequency, gibbs, sample, tmp, maxValue;
        float power, baseFrequency = 440, nyquist = ( float ) VST::SAMPLE_RATE / 2.0;

        // every other MIDI note (127 values)
        for ( int i = -69; i <= 58; i += 2 )
        {
            power       = i / 12.0;
            tempValue   = baseFrequency * pow( 2, power );
            frequency   = round( tempValue );
            partials    = nyquist / frequency;
            maxValue    = 0.0;

            for ( int t = 0; t < tableSize; t++ )
            {
                sample = 0.0, tmp = 0.0;

                // summing of the Fourier series for harmonics up to half the sample rate (Nyquist frequency)
                for ( int s = 1; s <= partials; ++s )
                {
                    // smoothing of sharp transitions
                    gibbs  = cos(( float )( s - 1.0 ) * VST::PI / ( 2.0 * ( float ) partials ));
                    gibbs *= gibbs;

                    // generation of the waveform
                    switch ( waveformType )
                    {
                        case WaveForms::SINE:
                            sample += gibbs * sin(( float ) s * VST::TWO_PI * ( float ) t / tableSize );
                            tmp     = sample;
                            break;

                        case WaveForms::TRIANGLE:
                            sample += sin(( float ) s * VST::TWO_PI * ( float ) t / tableSize );
                            tmp     = 1.0 - ( float ) ( std::abs( sample - 0.5 )) * 2.0;
                            break;

                        case WaveForms::SAWTOOTH:
                            sample += gibbs * ( 1.0 / ( float ) s ) * sin(( float ) s * VST::TWO_PI * ( float ) t / tableSize );
                            tmp     = sample;
                            break;

                        case WaveForms::SQUARE:
                            sample += sin(( float ) s * VST::TWO_PI * ( float ) t / tableSize );
                            tmp     = ( sample >= 0.0 ) ? 1.0 : -1.0;
                            break;
                    }
                }
                outputBuffer[ t ] = tmp;

                if ( tmp > maxValue ) {
                    maxValue = tmp;
                }
            }
            float factor = 1.0 / maxValue;

            // normalize values
            for ( int j = 0; j < tableSize; ++j ) {
                outputBuffer[ j ] *= factor;
            }
        }
        return waveTable;
    }
}

} // E.O namespace Igorski
