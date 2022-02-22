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
#ifndef __PLUGIN_PROCESS__H_INCLUDED__
#define __PLUGIN_PROCESS__H_INCLUDED__

#include "global.h"
#include "audiobuffer.h"
#include "bitcrusher.h"
#include "limiter.h"
#include "reverb.h"
#include "wavetable.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class PluginProcess {

    static constexpr float MAX_RECORD_SECONDS = 30.f;
    static constexpr float MIN_PLAYBACK_SPEED = .5f;

    public:
        PluginProcess( int amountOfChannels );
        ~PluginProcess();

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        // setters

        void setGateSpeed( float evenSpeed, float oddSpeed );

        // others

        // synchronize the gate tempo with the host
        // tempo is in BPM, time signature provided as: timeSigNumerator / timeSigDenominator (e.g. 3/4)

        void setTempo( double tempo, int32 timeSigNumerator, int32 timeSigDenominator, float evenSteps, float oddSteps );

        // resets gate envelope to 0 position (e.g. on sequencer stop/start)

        void syncGates();

        // assigns the appropriate WaveTables to each gate

        void createGateTables( float normalizedWaveFormType );

        void resetReadWritePointers();
        void clearRecordBuffer();

        // child processors

        BitCrusher* bitCrusher;
        Limiter* limiter;
        Reverb* reverb;

    private:
        std::vector<WaveTable*> _waveTables;

        AudioBuffer* _recordBuffer; // buffer used to record incoming signal
        AudioBuffer* _preMixBuffer; // buffer used for the pre effect mixing

        // read/write pointers for the record buffer used for record and playback

        float _readPointer       = 0.f;
        int _writePointer        = 0;
        int _maxRecordBufferSize = 0;

        int _amountOfChannels;
        int _writtenMeasureSamples = 0;
        int _fullMeasureSamples    = 0;
        int _beatSamples           = 0;
        int _sixteenthSamples      = 0;

        double _tempo;
        int32 _timeSigNumerator;
        int32 _timeSigDenominator;
        float _fullMeasureDuration;

        void clearGateTables();
        float _gateTableType = -1.f;

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. the buffers are pooled so this can be called upon each process
        // cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );
};
}

#include "plugin_process.tcc"

#endif
