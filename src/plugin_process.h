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
#ifndef __PluginProcess__H_INCLUDED__
#define __PluginProcess__H_INCLUDED__

#include "global.h"
#include "audiobuffer.h"
#include "bitcrusher.h"
#include "limiter.h"
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
        void setDryMix( float value );
        void setWetMix( float value );

        // child processors

        BitCrusher* bitCrusher;
        Limiter* limiter;

    private:
        std::vector<WaveTable*> _waveTables;

        AudioBuffer* _recordBuffer; // buffer used to record incoming signal
        AudioBuffer* _preMixBuffer;  // buffer used for the pre effect mixing

        // read/write pointers for the record buffer used for record and playback

        float _readPointer;
        int _writePointer;
        int _maxRecordBufferSize;

        float _dryMix;
        float _wetMix;
        int _amountOfChannels;

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. the buffers are pooled so this can be called upon each process
        // cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );
};
}

#include "plugin_process.tcc"

#endif
