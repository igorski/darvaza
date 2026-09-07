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
#ifndef __PLUGIN_PROCESS__H_INCLUDED__
#define __PLUGIN_PROCESS__H_INCLUDED__

#include "global.h"
#include "audiobuffer.h"
#include "automakeupgain.h"
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

    const float DITHER_WORD_LENGTH = pow( 2.0, 15 ); // 15 implies 16-bit depth
    const float DITHER_WI          = 1.0f / DITHER_WORD_LENGTH;
    const float DITHER_DC_OFFSET   = DITHER_WI * 0.5f; // apply in resampling routine to remove DC offset
    const float DITHER_AMPLITUDE   = 0.00003f; // -90 dBFS
    const float SMOOTHING          = 1.5f; // between 1 - 4

    // realtime-safe cross compiler agnostic variation of rand()
    struct Randomizer {
        uint32 state = 0x9E3779B9; // seed

        inline uint32 next() {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            
            return state;
        }

        // in -1 to +1 range
        inline float nextBipolar() {
            return ( float )( int32 ) next() * ( 1.f / 2147483648.f );
        }
    };

    struct ChannelState {
        int remaining = 0;
        int length = 1;
        float sample = 0.f;
        float dither = 0.f;
        float coeff = 1.f;
        float lastSample = 0.f; // last written value
        int filterPointer = 0; // pointer at relevant index in lowpass filtered record buffer
        float filteredPrev = 0.f; // last filtered sample
        float filteredCur = 0.f; // next/current filtered sample
        float readPointer = 0; // read pointer in record buffer (unique per channel because harmonise can move at different speed)
    };

    public:
        PluginProcess( int amountOfChannels, float sampleRate, int maxBufferSize );
        ~PluginProcess();

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        inline float normalisedToDownSampleAmount( float normalised ) {
            return powf( _maxDownSample, 1.f - normalised );
        }

        // setters

        void setHostProperties( float sampleRate, int maxBufferSize );
        void setDryMix( float value );
        void setGateSpeed( float oddSteps, float evenSteps, bool linkGates );
        void randomizeGateSpeed( float randomSteps );

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

        void setResampleRate( float value );
        void setPlaybackRate( float value );
        void setHarmony( float value );
        void enableReverb( bool enabled );
        void enableReverse( bool enabled );

        // child processors

        BitCrusher bitCrusher;
        Limiter limiter;
        Reverb reverb;

    private:
        int _amountOfChannels;
        float _hostSampleRate;
        int _hostBufferSize = 0;
        std::vector<ChannelState> _channelStates;
        std::vector<LowPassFilter> _lowPassFilters;
        std::vector<AutoMakeUpGain> _makeUpGainProcessors;
        std::vector<Reverb> _reverbs;
        std::vector<WaveTable> _waveTables;
        Randomizer _randomizer;

        AudioBuffer* _recordBuffer = nullptr; // buffer used to record incoming signal
        AudioBuffer* _preMixBuffer = nullptr; // buffer used for the pre effect mixing
        float* _scratchBuffer = nullptr; // used for make-up gain processing (reused per channel)
     
        bool _reverbEnabled = false;
        float _dryMix = 0.f;

        bool _linkedGates      = false;
        float _randomizedSpeed = 0.f;
        int _oddInvertProg     = 0;
        int _evenInvertProg    = 0;
        float _oddSteps        = 0;
        float _evenSteps       = 0;
        float _curOddSteps     = 0;
        float _curEvenSteps    = 0;

        void setOddGateSpeed( float speed );
        void setEvenGateSpeed( float speed );

        // read/write pointers for the record buffer used for record and playback
        // note that readPointers are unique per channel, as different playback
        // speeds can be configured for these (e.g. _harmonize) the write pointer
        // will always be equal

        int _writePointer        = 0;
        int _maxRecordBufferSize = 0;

        // cached values for sample accurate calculation of relevant musical positions

        int _writtenMeasureSamples = 0;
        int _fullMeasureSamples    = 0;
        int _halfMeasureSamples    = 0;
        int _beatSamples           = 0;
        int _sixteenthSamples      = 0;

        // tempo related

        double _tempo             = 0.0;
        int32 _timeSigNumerator   = 0;
        int32 _timeSigDenominator = 0;
        float _fullMeasureDuration;

        // related to playback of precorded content (when downsampling or playing at reduced speed)

        void cacheDownSamplingValues();

        float _downSampleNormalised;
        float _downSampleAmount; // 1 == no change (keeps at original sample rate), > 1 provides down sampling
        float _actualDownSampleAmount;
        float _maxDownSample;
        float _targetRate;
        float _playbackRate = VST::MIN_PLAYBACK_SPEED; // 1 == 100% (no change), < 1 is lower playback speed
        int   _sampleIncr;
        bool  _reverse = false;
        float _harmonize = 0.f;
        float _oddPitch = 1.f;
        float _evenPitch = 1.f;

        void cacheMaxDownSample( float sampleRate );
        void setActualDownSampling( float amount );

        inline bool isSlowedDown() {
            return _playbackRate < 1.f || isHarmonized();
        }

        inline bool isDownSampled() {
            return _downSampleAmount > 1.f;
        }

        inline bool isHarmonized() {
            return _harmonize > 0.f;
        }

        inline bool hasRandomizedSpeed() {
            return _randomizedSpeed > 0.f;
        }

        WaveGenerator::WaveForms _gateWaveForm;

        // aligns read and filter pointers with the current _writePointer

        inline void syncFilterPointers() {
            float readPointer = static_cast<float>( _writePointer );

            for ( int c = 0; c < _amountOfChannels; ++c ) {
                _channelStates[ c ].readPointer = readPointer;
                _channelStates[ c ].filterPointer = static_cast<int>( readPointer );
            }
        }
};
}

#include "plugin_process.tcc"

#endif
