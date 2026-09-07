/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2026 Igor Zinken - https://www.igorski.nl
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
#include "calc.h"

namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    if ( bufferSize <= 0 ) {
        return; // Variable Block Size unit test
    }

    ScopedNoDenormals noDenormals;

    // input and output buffers can be float or double as defined
    // by the templates SampleType value. Internally we process
    // audio as floats

    int32 i, l;
    SampleType dryMix = static_cast<SampleType>( _dryMix );

    float readPointer, tmpSample;
    int writePointer;
    int writtenSamples;

    int recordMax     = _maxRecordBufferSize - 1;
    int maxBufferPos  = bufferSize - 1;
    int maxReadOffset = _writePointer + maxBufferPos; // never read beyond the range of the current incoming input

    bool playFromRecordBuffer = isSlowedDown() || isDownSampled();
    bool randomizeSpeed = hasRandomizedSpeed();
    bool harmonize = isHarmonized();

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        bool isOddChannel = ( c % 2 ) == 0;

        writePointer = _writePointer;

        ChannelState &state = _channelStates[ c ];
        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

        WaveTable &table = _waveTables.at( c );
        writtenSamples = _writtenMeasureSamples;

        // 1. write incoming input into the record and pre mix buffers (converting to float when necessary)

        for ( i = 0; i < bufferSize; ++i, ++writePointer ) {
            if ( writePointer > recordMax ) {
                writePointer = 0;
            }
            tmpSample = static_cast<float>( channelInBuffer[ i ] );

            channelRecordBuffer[ writePointer ] = tmpSample;
            channelPreMixBuffer[ i ]            = tmpSample;
        }

        // 2. in case we should play at a custom rate from the record buffer
        // fill the pre mix buffer with the appropriate slowed down recorded content

        if ( playFromRecordBuffer ) {

            float curSample;
            int r1 = 0, r2 = 0, t;
            float incr, frac, s1, s2;

            // calculate iterator size when reading from recorded buffer
            // this is determined by the down sampling amount (defined in fSampleIncr)
            // and further more by the playback rate (for playback speed)
            // in harmonize mode, the playback rate is determined by the desired pitch shift

            if ( harmonize ) {
                incr = static_cast<float>( _sampleIncr ) * ( isOddChannel ? _oddPitch : _evenPitch );
            } else {
                incr = static_cast<float>( _sampleIncr ) * _playbackRate;
            }

            LowPassFilter &lowPassFilter = _lowPassFilters.at( c );
            
            readPointer = state.readPointer;
            int filterPointer = state.filterPointer;
            float filteredPrev = state.filteredPrev;
            float filteredCur = state.filteredCur;

            float lastSample = state.lastSample;

            i = 0;
            while ( i < bufferSize ) {
                if ( state.remaining == 0 ) {
                    t = static_cast<int>( readPointer );
                    frac = readPointer - t;
                    
                    // advance the anti alias filter at host rate through every sample we will read
                    // up to and including t + 1 so the interpolation uses filtered values

                    while ( filterPointer <= t + 1 ) {
                        int idx = filterPointer;
                        if ( idx > recordMax ) {
                            idx -= _maxRecordBufferSize;
                        }
                        filteredPrev = filteredCur;
                        filteredCur = /* lowPassFilter.applySingle( */ channelRecordBuffer[ idx ]; // );
                        ++filterPointer;
                    }

                    // filteredPrev is sample at t, filteredCur is sample at t + 1

                    curSample = filteredPrev + ( filteredCur - filteredPrev ) * frac;

                    state.length = _sampleIncr;
                    state.remaining = state.length;
                    state.sample = curSample * .667f;
                    state.coeff = std::min( 1.f, SMOOTHING / static_cast<float>( state.length ));

                    r2 = r1;
                    r1 = _randomizer.nextBipolar();
                    state.dither = DITHER_AMPLITUDE * ( r1 - r2 );

                    float incr = static_cast<float>( state.length ) * _playbackRate;
                    
                    if ( _reverse ) {
                        if (( readPointer -= incr ) < 0 ) {
                            readPointer = maxReadOffset;
                            filterPointer = static_cast<int>( readPointer );
                        }
                    } else {
                        if (( readPointer += incr ) > maxReadOffset ) {
                            // don't go to 0.f but align with last write offset to play currently incoming audio
                            readPointer = harmonize ? 0.f : static_cast<float>( writePointer );
                            filterPointer = static_cast<int>( readPointer );
                        }
                    }
                }

                for ( l = i + std::min( bufferSize - i, state.remaining ); i < l; ++i ) {

                    lastSample += ( state.sample - lastSample ) * state.coeff;
                    
                    // write sample into the output buffer, corrected for DC offset and dithering applied
                    channelPreMixBuffer[ i ] = lastSample + DITHER_DC_OFFSET + state.dither;

                    // catch denormals
                    UNDENORMALISE( channelPreMixBuffer[ i ]);

                    --state.remaining;
                }
            }
            // update channel state
            state.lastSample = lastSample;
            state.filterPointer = filterPointer;
            state.filteredPrev = filteredPrev;
            state.filteredCur = filteredCur;
        }

        // 3. run the pre mix effects that require no sample accurate property updates

        bitCrusher.process( channelPreMixBuffer, bufferSize );

        // 4. apply gate and mix the input and processed mix buffer into the output buffer

        Reverb &reverb = _reverbs.at( c );

        for ( i = 0; i < bufferSize; ++i ) {

            // increment the written sample amount to keep track of key positions
            // within the current measure to align the gates to

            if ( ++writtenSamples >= _fullMeasureSamples ) {
                writtenSamples = 0; // new measure
            }

            // if gate speed inversion is enabled, count the progress
            // and advance the speeds every half measure

            if ( randomizeSpeed ) {
                if ( isOddChannel && ( ++_oddInvertProg >= _halfMeasureSamples )) {
                    _oddInvertProg = 0;
                    setOddGateSpeed( _curOddSteps == _oddSteps ? _randomizedSpeed : _oddSteps );
                } else if ( !isOddChannel && ( ++_evenInvertProg >= _halfMeasureSamples )) {
                    _evenInvertProg = 0;
                    setEvenGateSpeed( _curEvenSteps == _evenSteps ? _randomizedSpeed : _evenSteps );
                }
            }

            // run sample accurate property updates

            if (( writtenSamples % _beatSamples ) == 0 ) {
                // a beat has passed
                if ( c == 0 ) {
                    // global parameters (gate speed, etc.) should only be toggled once per loop
                    //setGateSpeed( writtenSamples == 0 ? 0.5f : 0.1f, writtenSamples == 0 ? 0.5f : 0.1f );
                }
                reverb.toggleFreeze();
            }

            // open / close the gate
            // note we multiply by .5 and add .5 to make the LFO's bipolar waveform unipolar

            SampleType gateLevel = static_cast<SampleType>( table.peek() * .5f + .5f );

            tmpSample = channelPreMixBuffer[ i ];

            // run the pre mix effects that require sample accurate property updates

            if ( _reverbEnabled ) {
                tmpSample = reverb.processSingle( tmpSample );
            }

            // blend in the effect mix buffer for the gates value

            channelOutBuffer[ i ] = Calc::capSample( static_cast<SampleType>( tmpSample ) * gateLevel );

            // blend in the dry signal (mixed to the negative of the gated signal)
            channelOutBuffer[ i ] += (( channelInBuffer[ i ] * ( 1.f - gateLevel )) * dryMix );
        }
        // end of processing for channel

        // update read index
        state.readPointer = readPointer;
    }

    // update write indices

    _writePointer          = writePointer;
    _writtenMeasureSamples = writtenSamples;

    // limit the output signal in case its gets hot
    limiter.process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

}
