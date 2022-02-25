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
#include "waveforms.h"
#include <math.h>
#include <cmath>

using namespace Igorski::VST;

namespace Igorski {
namespace WaveGenerator
{
    WaveTable* generate( WaveForms waveformType )
    {
        WaveTable* waveTable = new WaveTable( TABLE_SIZE, 440.f );

        float* outputBuffer = waveTable->getBuffer();

        if ( VST::SAMPLE_RATE <= ( WAVEFORM_CACHE_SAMPLE_RATE * 2 )) {
            // when the sample rate is in roughly the same ballpark as the cached tables we
            // just assign the cached contents directly to the WaveTable and skip runtime
            // rendering (as it is CPU heavy)
            auto waveformTable = TABLE_SINE;
            switch ( waveformType ) {
                default:
                    break;
                case WaveForms::TRIANGLE:
                    waveformTable = TABLE_TRIANGLE;
                    break;
                case WaveForms::SAWTOOTH:
                    waveformTable = TABLE_SAW;
                    break;
                case WaveForms::SQUARE:
                    waveformTable = TABLE_SQUARE;
                    break;
            }

            for ( int i = 0; i < TABLE_SIZE; i++ ) {
                outputBuffer[ i ] = waveformTable[ i ];
            }
            return waveTable;
        }

        int tempValue, partials;
        float frequency, gibbs, sample, tmp, maxValue;
        float power, baseFrequency = 440;
        // note we cap the max sample rate (the Steinberg validator test goes into million Hz territory...)
        // for now it's safe to assume no VST will be used in gHz sampling rate projects...
        float nyquist = fminf( 384000.f, VST::SAMPLE_RATE  ) / 2.f;

        // every other MIDI note (127 values)
        for ( int i = -69; i <= 58; i += 2 )
        {
            power       = i / 12.0;
            tempValue   = baseFrequency * pow( 2, power );
            frequency   = round( tempValue );
            partials    = nyquist / frequency;
            maxValue    = 0.0;

            // unique to triangle generation

            float delta      = 1.f / (( float ) TABLE_SIZE / 2 );
            float lastSample = 0.0;

            for ( int t = 0; t < TABLE_SIZE; t++ )
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
                            sample += gibbs * sin(( float ) s * VST::TWO_PI * ( float ) t / TABLE_SIZE );
                            tmp     = sample;
                            break;

                        case WaveForms::TRIANGLE:
                            sample += gibbs * sin(( float ) s * VST::TWO_PI * ( float ) t / TABLE_SIZE );
                            tmp     = lastSample + (( sample >= lastSample ) ? delta : -delta );
                            break;

                        case WaveForms::SAWTOOTH:
                            sample += gibbs * ( 1.0 / ( float ) s ) * sin(( float ) s * VST::TWO_PI * ( float ) t / TABLE_SIZE );
                            tmp     = sample;
                            break;

                        case WaveForms::SQUARE:
                            // regular sine generation
                            sample += gibbs * sin(( float ) s * VST::TWO_PI * ( float ) t / TABLE_SIZE );
                            // snap to extremes
                            tmp = ( sample >= 0.0 ) ? 1.0 : -1.0;
                            break;
                    }
                    lastSample = sample;
                }
                outputBuffer[ t ] = tmp;

                maxValue = fmax( abs( tmp ), maxValue );
            }

            if ( waveformType == WaveForms::SQUARE ) {
                // square waves are very violent, when smoothing is specified
                // apply gentle fades around the start/end of the waveform
                int smoothRange = 4;
                float factor = 1.0 / smoothRange;
                if ( TABLE_SIZE >= ( smoothRange * 2 )) {
                    int fadeOutPos = TABLE_SIZE - smoothRange;
                    int tableEnd   = TABLE_SIZE - 1;
                    for ( int j = 0; j < TABLE_SIZE; ++j ) {
                        // gentle fade in
                        if ( j < smoothRange ) {
                            outputBuffer[ j ] *= ( j == 0 ? 0 : j * factor );
                        }
                        // gentle fade out
                        if ( j >= fadeOutPos ) {
                            outputBuffer[ j ] *= (( tableEnd - j ) * factor );
                        }
                    }
                }
            } else {
                // normalize values for all other waveforms to keep them in the -1 to +1 range
                float factor = 1.0 / maxValue;
                for ( int j = 0; j < TABLE_SIZE; ++j ) {
                    outputBuffer[ j ] *= factor;
                }
            }
        }
        return waveTable;
    }
}

} // E.O namespace Igorski
