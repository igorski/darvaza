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
#include "lowpassfilter.h"
#include "reverb.h"
#include "wavegenerator.h"
#include "wavetable.h"
#include "snd.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class PluginProcess {

    // dithering constants

    const float DITHER_WORD_LENGTH = pow( 2.0, 15 );        // 15 implies 16-bit depth
    const float DITHER_WI          = 1.0f / DITHER_WORD_LENGTH;
    const float DITHER_DC_OFFSET   = DITHER_WI * 0.5f;      // apply in resampling routine to remove DC offset
    const float DITHER_AMPLITUDE   = DITHER_WI / RAND_MAX;  // 2 LSB

    public:
        static constexpr float MAX_RECORD_SECONDS = 30.f;
        static constexpr float MIN_PLAYBACK_SPEED = .5f;
        static constexpr float MIN_SAMPLE_RATE    = 2000.f;

        PluginProcess( int amountOfChannels );
        ~PluginProcess();

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        // setters

        void setGateSpeed( float oddSteps, float evenSteps );

        // others

        // synchronize the gate tempo with the host
        // tempo is in BPM, time signature provided as: timeSigNumerator / timeSigDenominator (e.g. 3/4)
        // returns true when tempo has updated, false to indicate no change was made

        bool setTempo( double tempo, int32 timeSigNumerator, int32 timeSigDenominator );

        // resets gate envelope to 0 position (e.g. on sequencer stop/start)

        void syncGates();

        // assigns the appropriate WaveTables to each gate

        void createGateTables( float normalizedWaveFormType );

        void resetReadWritePointers();
        void resetGates();
        void clearRecordBuffer();

        void setPlaybackRate( float value );
        void setResampleRate( float value );
        void enableReverb( bool enabled );
        void enableHarmonize( bool enabled );

        // child processors

        BitCrusher* bitCrusher;
        Limiter* limiter;
        Reverb* reverb;

    private:
        int _amountOfChannels;
        std::vector<WaveTable*> _waveTables;

        AudioBuffer* _recordBuffer; // buffer used to record incoming signal
        AudioBuffer* _preMixBuffer; // buffer used for the pre effect mixing
        int _lastBufferSize = 0;    // size of the last buffer used when generating the _recordBuffer

        bool _reverbEnabled = false;

        // read/write pointers for the record buffer used for record and playback
        // note that readPointers are unique per channel, as different playback
        // speeds can be configured for these (e.g. _harmonize) the write pointer
        // will always be equal

        int _writePointer        = 0;
        int _maxRecordBufferSize = 0;
        float* _readPointers;

        // cached values for sample accurate calculation of relevant musical positions

        int _writtenMeasureSamples = 0;
        int _fullMeasureSamples    = 0;
        int _beatSamples           = 0;
        int _sixteenthSamples      = 0;

        // tempo related

        double _tempo             = 0.0;
        int32 _timeSigNumerator   = 0;
        int32 _timeSigDenominator = 0;
        float _fullMeasureDuration;

        // related to playback of precorded content (when downsampling or playing at reduced speed)

        float* _lastSamples; // last written sample, per channel
        float _downSampleAmount = 0.f; // 1 == no change (original sample rate), > 1 provides down sampling
        float _maxDownSample;
        float _playbackRate = MIN_PLAYBACK_SPEED; // 1 == 100% (no change), < 1 is lower playback speed
        float _fSampleIncr;
        int   _sampleIncr;
        bool  _harmonize = false;

        std::vector<LowPassFilter*> _lowPassFilters;
        std::vector<Reverb*> _reverbs;

        inline bool isSlowedDown() {
            return _playbackRate < 1.f || _harmonize;
        }

        inline bool isDownSampled() {
            return _downSampleAmount > 1.f;
        }

        void clearGateTables();
        WaveGenerator::WaveForms _gateWaveForm;

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. the buffers are pooled so this can be called upon each process
        // cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );
};
}

#include "plugin_process.tcc"

#endif
