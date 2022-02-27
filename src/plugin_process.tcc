/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2022 Igor Zinken - https://www.igorski.nl
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

    int32 i;
    SampleType dryMix = ( SampleType ) _dryMix;

    float readPointer, tmpSample;
    int writePointer;
    int writtenSamples;

    int recordMax     = _maxRecordBufferSize - 1;
    int maxBufferPos  = bufferSize - 1;
    int maxReadOffset = _writePointer + maxBufferPos; // never read beyond the range of the current incoming input

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    bool playFromRecordBuffer = isSlowedDown() || isDownSampled();

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        bool isOddChannel = ( c % 2 ) == 0;

        readPointer  = _readPointers[ c ];
        writePointer = _writePointer;

        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

        WaveTable* table = _waveTables.at( c );

        writtenSamples = _writtenMeasureSamples;

        // 1. write incoming input into the record and pre mix buffers (converting to float when necessary)

        for ( i = 0; i < bufferSize; ++i, ++writePointer ) {
            if ( writePointer > recordMax ) {
                writePointer = 0;
            }
            tmpSample = ( float ) channelInBuffer[ i ];

            channelRecordBuffer[ writePointer ] = tmpSample;
            channelPreMixBuffer[ i ]            = tmpSample;
        }

        // 2. in case we should play at a custom rate from the record buffer
        // fill the pre mix buffer with the appropriate slowed down recorded content

        if ( playFromRecordBuffer ) {

            float nextSample, curSample, outSample;
            int r1 = 0, r2 = 0, t, t2;
            float incr, frac, s1, s2;

            // calculate iterator size when reading from recorded buffer
            // this is determined by the down sampling amount (defined in _fSampleIncr)
            // and further more by the playback rate (for playback speed)
            // in _harmonize mode, the playback rate is determined by the desired pitch shift

            if ( _harmonize ) {
                incr = _fSampleIncr * Calc::pitchDown( isOddChannel ? 2 : 5 );
            } else {
                incr = _fSampleIncr * _playbackRate;
            }

            LowPassFilter* lowPassFilter = _lowPassFilters.at( c );
            float lastSample = _lastSamples[ c ];

            i = 0;
            while ( i < bufferSize ) {
                t  = ( int ) readPointer;
                t2 = std::min( recordMax, t + _sampleIncr );

                // this fractional is in the 0 - 1 range

                frac = /*readPointer - t;*/ 0.f;

                s1 = channelRecordBuffer[ t ];
                s2 = channelRecordBuffer[ t2 ];

                // we apply a lowpass filter to prevent interpolation artefacts

                curSample = lowPassFilter->applySingle( s1 + ( s2 - s1 ) * frac );
                outSample = curSample * .5f;

                int start = i;
                for ( int32 l = std::min( bufferSize, start + _sampleIncr ); i < l; ++i ) {
                    r2 = r1;
                    r1 = rand();

                    nextSample = outSample + lastSample;
                    lastSample = nextSample * .25f;

                    // write sample into the output buffer, corrected for DC offset and dithering applied

                    channelPreMixBuffer[ i ] = nextSample + DITHER_DC_OFFSET + DITHER_AMPLITUDE * ( r1 - r2 );
                }

                if (( readPointer += incr ) > maxReadOffset ) {
                    // don't go to 0.f but align with current write offset to play currently incoming audio
                    readPointer = ( float ) _writePointer;
                }
            }
            _lastSamples[ c ] = lastSample;
        }

        // 3. run the pre mix effects that require no sample accurate property updates

        bitCrusher->process( channelPreMixBuffer, bufferSize );

        // 4. apply gate and mix the input and processed mix buffer into the output buffer

        Reverb* reverb = _reverbs.at( c );

        for ( i = 0; i < bufferSize; ++i ) {

            // increment the written sample amount to keep track of key positions
            // within the current measure to align the gates to

            if ( ++writtenSamples >= _fullMeasureSamples ) {
                writtenSamples = 0; // new measure
            }

            // if gate speed inversion is enabled, count the progress
            // and advance the speeds every half measure

            if ( _randomizeSpeed ) {
                if ( isOddChannel && ( ++_oddInvertProg >= _halfMeasureSamples )) {
                    _oddInvertProg = 0;
                    setOddGateSpeed( Calc::cap( _curOddSteps == _oddSteps ? _oddSteps * 2 : _oddSteps ));
                } else if ( !isOddChannel && ( ++_evenInvertProg >= _halfMeasureSamples )) {
                    _evenInvertProg = 0;
                    setEvenGateSpeed( Calc::cap( _curEvenSteps == _evenSteps ? _oddSteps * 2 : _oddSteps ));
                }
            }

            // run sample accurate property updates

            if (( writtenSamples % _beatSamples ) == 0 ) {
                // a beat has passed
                if ( c == 0 ) {
                    // global parameters (gate speed, etc.) should only be toggled once per loop
                    //setGateSpeed( writtenSamples == 0 ? 0.5f : 0.1f, writtenSamples == 0 ? 0.5f : 0.1f );
                }
                reverb->toggleFreeze();
            }

            // open / close the gate
            // note we multiply by .5 and add .5 to make the LFO's bipolar waveform unipolar

            SampleType gateLevel = ( SampleType ) ( table->peek() * .5f + .5f );

            tmpSample = channelPreMixBuffer[ i ];

            // run the pre mix effects that require sample accurate property updates

            if ( _reverbEnabled ) {
                tmpSample = reverb->processSingle( tmpSample );
            }

            // blend in the effect mix buffer for the gates value

            channelOutBuffer[ i ] = ( SampleType ) ( tmpSample ) * gateLevel;

            // blend in the dry signal (mixed to the negative of the gated signal)
            channelOutBuffer[ i ] += (( channelInBuffer[ i ] * ( 1.0 - gateLevel )) * dryMix );
        }
        // end of processing for channel

        // update read index
        _readPointers[ c ] = readPointer;
    }

    // update write indices

    _writePointer          = writePointer;
    _writtenMeasureSamples = writtenSamples;

    // limit the output signal in case its gets hot
    limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // variable block size for a smaller block should not require new record buffers
    // only create these when the last size was smaller than the current
    if ( bufferSize <= _lastBufferSize ) {
        return;
    }

    _lastBufferSize = bufferSize;

    // if the record buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    int idealRecordSize = Calc::secondsToBuffer( MAX_RECORD_SECONDS );
    int recordSize      = idealRecordSize + idealRecordSize % bufferSize;

    if ( _recordBuffer == nullptr || _recordBuffer->bufferSize != recordSize ) {
        delete _recordBuffer;
        _recordBuffer = new AudioBuffer( numInChannels, recordSize );
        _maxRecordBufferSize = recordSize;
        resetReadWritePointers();
    }

    // if the pre mix buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    if ( _preMixBuffer == nullptr || _preMixBuffer->bufferSize != bufferSize ) {
        delete _preMixBuffer;
        _preMixBuffer = new AudioBuffer( numInChannels, bufferSize );
    }
}

}
