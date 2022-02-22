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

    // input and output buffers can be float or double as defined
    // by the templates SampleType value. Internally we process
    // audio as floats

    SampleType inSample, outSample;
    int i, readIndex;

    float readPointer;
    int writePointer;
    int recordMax = _maxRecordBufferSize - 1;
    int writtenSamples;

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        readPointer  = _readPointer;
        writePointer = _writePointer;

        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

        WaveTable* table = _waveTables.at( c );

        writtenSamples = _writtenMeasureSamples;

        // write input into the record and pre mix buffers (converting to float when necessary)

        for ( i = 0; i < bufferSize; ++i, ++writePointer ) {
            if ( writePointer > recordMax ) {
                writePointer = 0;
            }
            float inSample = ( float ) channelInBuffer[ i ];

            channelRecordBuffer[ writePointer ] = inSample;
            channelPreMixBuffer[ i ] = inSample;
        }

        // run the pre mix effects that require no sample accurate property updates

        bitCrusher->process( channelPreMixBuffer, bufferSize );

        // apply gate and mix the input and processed mix buffer into the output buffer

        for ( i = 0; i < bufferSize; ++i ) {

            // increment the written sample amount to keep track of key positions
            // within the current measure to align the gates to

            if ( ++writtenSamples >= _fullMeasureSamples ) {
                _fullMeasureSamples = 0; // new measure
            }

            // run sample accurate property updates

            if ( writtenSamples % _beatSamples == 0 ) {
                // a beat has passed
                //setGateSpeed( writtenSamples == 0 ? 0.5f : 0.1f, writtenSamples == 0 ? 0.5f : 0.1f );
                reverb->toggleFreeze();
            }

            // before writing to the out buffer we take a snapshot of the current in sample
            // value as VST2 in Ableton Live supplies the same buffer for inBuffer and outBuffer!

            inSample = channelInBuffer[ i ];

            // run the pre mix effects that require sample accurate property updates

            outSample = reverb->processSingle( channelPreMixBuffer[ i ] );

            // open / close the gate
            // note we multiply by .5 and add .5 to make the LFO's bipolar waveform unipolar

            SampleType gateLevel = ( SampleType ) ( table->peek() * .5f + .5f );

            // write the mix buffer for the gate value

            channelOutBuffer[ i ] = ( SampleType ) ( outSample ) * gateLevel;

            // dry mix (e.g. mix in the input signal on the negative gate)

            channelOutBuffer[ i ] += ( inSample * ( 1.0 - gateLevel ));
        }
    }
    // update read/write indices
    _readPointer  = readPointer;
    _writePointer = writePointer;

    _writtenMeasureSamples = writtenSamples;

    // limit the output signal in case its gets hot
    //limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // if the record buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    int idealRecordSize = Calc::secondsToBuffer( MAX_RECORD_SECONDS );
    int recordSize      = idealRecordSize + idealRecordSize % bufferSize;

    if ( _recordBuffer == nullptr || _recordBuffer->bufferSize != recordSize ) {
        delete _recordBuffer;
        _recordBuffer = new AudioBuffer( numInChannels, recordSize );
        _maxRecordBufferSize = recordSize;
    }

    // if the pre mix buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    if ( _preMixBuffer == nullptr || _preMixBuffer->bufferSize != bufferSize ) {
        delete _preMixBuffer;
        _preMixBuffer = new AudioBuffer( numInChannels, bufferSize );
    }
}

}
