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
#include "plugin_process.h"
#include "calc.h"
#include "tablepool.h"
#include "waveforms.h"
#include <math.h>

namespace Igorski {

PluginProcess::PluginProcess( int amountOfChannels, float sampleRate, int maxBufferSize ) {
    _amountOfChannels = amountOfChannels;

    _channelStates.resize( amountOfChannels );
    _lowPassFilters.resize( amountOfChannels );
    _makeUpGainProcessors.resize( amountOfChannels );
    _reverbs.resize( amountOfChannels );
    _waveTables.resize( amountOfChannels );

    _gateWaveForm = WaveGenerator::WaveForms::SINE;
    
    setHostProperties( sampleRate, maxBufferSize );

    // these will be synced to host, see vst.cpp. here we default to 120 BPM in 4/4 time

    setTempo( 120.f, 4, 4 );

    for ( int i = 0; i < amountOfChannels; ++i ) {
        _reverbs.at( i ).setWidth( 1.f );
        _reverbs.at( i ).setRoomSize( 1.f );
    }

    setPlaybackRate( 1.f );
    setResampleRate( 1.f );
}

PluginProcess::~PluginProcess() {
    delete[] _scratchBuffer;
    delete _preMixBuffer;
    delete _recordBuffer;

    TablePool::flush();
}

/* setters */

void PluginProcess::setHostProperties( float sampleRate, int maxBufferSize ) {
    bool hadSampleRateChange = _hostSampleRate != sampleRate;

    cacheMaxDownSample( sampleRate );
    _hostSampleRate = sampleRate;

    if ( hadSampleRateChange ) {
        TablePool::flush();

        TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SINE, sampleRate ),     WaveGenerator::WaveForms::SINE );
        TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::TRIANGLE, sampleRate ), WaveGenerator::WaveForms::TRIANGLE );
        TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SAWTOOTH, sampleRate ), WaveGenerator::WaveForms::SAWTOOTH );
        TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SQUARE, sampleRate ),   WaveGenerator::WaveForms::SQUARE );

        setResampleRate( _downSampleNormalised );

        for ( int c = 0; c < _amountOfChannels; ++c ) {
            _makeUpGainProcessors.at( c ).prepare( sampleRate );
            _reverbs.at( c ).setSampleRate( sampleRate );
            _waveTables.at( c ).clone( TablePool::getTable( _gateWaveForm ));
            _waveTables.at( c ).setSampleRate( sampleRate );
        }
    }

    if ( _hostBufferSize < maxBufferSize ) {
        _hostBufferSize = maxBufferSize;
       
        delete[] _scratchBuffer;
        _scratchBuffer = new float[ _hostBufferSize ];

        // if the record buffer wasn't created yet or the buffer size has changed
        // delete existing buffer and create new one to match properties

        int idealRecordSize = Calc::secondsToBuffer( VST::MAX_RECORD_SECONDS, _hostSampleRate );
        int recordSize      = idealRecordSize + idealRecordSize % _hostBufferSize;

        if ( _recordBuffer == nullptr || _recordBuffer->bufferSize != recordSize ) {
            delete _recordBuffer;
            _recordBuffer = new AudioBuffer( _amountOfChannels, recordSize );
            _maxRecordBufferSize = recordSize;
            resetReadWritePointers();
        }

        // if the pre mix buffer wasn't created yet or the buffer size has changed
        // delete existing buffer and create new one to match properties

        if ( _preMixBuffer == nullptr || _preMixBuffer->bufferSize != _hostBufferSize ) {
            delete _preMixBuffer;
            _preMixBuffer = new AudioBuffer( _amountOfChannels, _hostBufferSize );
        }
    }
}

void PluginProcess::setDryMix( float value )
{
    _dryMix = value;
}

void PluginProcess::setGateSpeed( float oddSteps, float evenSteps, bool linkGates )
{
    bool wasLinked = _linkedGates;

    // set given steps as the "base" steps for the odd/even channels

    _oddSteps  = oddSteps;
    _evenSteps = linkGates ? oddSteps : evenSteps;

    _linkedGates = linkGates;

    if ( hasRandomizedSpeed() ) {
        return; // if speed randomization is active let the process() function update the actual gate speeds
    }

    // in case the gate speeds are newly synchronized, align the oscillator accumulators

    if ( linkGates && !wasLinked ) {
        for ( size_t i = 0; i < _amountOfChannels; ++i ) {
            bool isEvenChannel = ( i % 2 ) == 1;
            if ( isEvenChannel ) {
                _waveTables.at( i ).setAccumulator( _waveTables.at( i - 1 ).getAccumulator() );
            }
        }
    }

    // invoking these sets the actual gate speeds onto the wave tables

    setOddGateSpeed( _oddSteps );
    setEvenGateSpeed( _evenSteps );
}

void PluginProcess::setOddGateSpeed( float steps ) {
    _curOddSteps = steps;

    // (1.f / measureDuration value) converts seconds to cycles in Hertz
    float value  = 1.f / ( _fullMeasureDuration / Calc::gateSubdivision( steps ));
    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        bool isOddChannel = ( i % 2 ) == 0;
        if ( isOddChannel ) {
            _waveTables.at( i ).setFrequency( value );
        }
    }
}

void PluginProcess::setEvenGateSpeed( float steps ) {
    _curEvenSteps = steps;

    // (1.f / measureDuration value) converts seconds to cycles in Hertz
    float value  = 1.f / ( _fullMeasureDuration / Calc::gateSubdivision( steps ));
    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        bool isEvenChannel = ( i % 2 ) == 1;
        if ( isEvenChannel ) {
            _waveTables.at( i ).setFrequency( value );
        }
    }
}

void PluginProcess::cacheMaxDownSample( float sampleRate )
{
    _maxDownSample = sampleRate / VST::MIN_SAMPLE_RATE;
}

void PluginProcess::setActualDownSampling( float value )
{
    bool wasDownSampled     = isDownSampled();
    _actualDownSampleAmount = value;
    cacheDownSamplingValues();

    // if down sampling is deactivated and there is no playback slowdown taking place:
    // sync the read pointer with the write pointer

    if ( wasDownSampled && !isDownSampled() && !isSlowedDown() ) {
        syncFilterPointers();
    }
}

void PluginProcess::cacheDownSamplingValues()
{
    _sampleIncr = static_cast<int>( roundf( _actualDownSampleAmount ));
    _sampleIncr = std::max( 1, std::min( _sampleIncr, static_cast<int>( _maxDownSample )));

    // update the lowpass filters to the appropriate cutoff

    float ratio = 1.f + ( _actualDownSampleAmount / _maxDownSample );
    for ( int c = 0; c < _amountOfChannels; ++c ) {
        _lowPassFilters.at( c ).setRatio( ratio );
    }
}

void PluginProcess::randomizeGateSpeed( float randomSteps )
{
    _randomizedSpeed = randomSteps;

    // when enabling the speed inversion, synchronize it with the current measure progress
    if ( hasRandomizedSpeed() ) {
        _oddInvertProg  = _writtenMeasureSamples;
        _evenInvertProg = _writtenMeasureSamples;
    }
}

void PluginProcess::resetReadWritePointers()
{
    _writePointer = 0;
    syncFilterPointers();
}

void PluginProcess::resetGates()
{
    for ( auto waveTable : _waveTables ) {
        waveTable.setAccumulator( 0 );
    }
}

void PluginProcess::clearRecordBuffer()
{
    if ( _recordBuffer != nullptr ) {
        _recordBuffer->silenceBuffers();
    }
}

void PluginProcess::setResampleRate( float value )
{
    float normalisedDownSampleAmount = normalisedToDownSampleAmount( value );
    
    // no change in either resample rate or host sample rate value
    // no need to trigger changes

    if ( normalisedDownSampleAmount == _downSampleAmount ) {
        return;
    }

    bool wasDownSampled = isDownSampled();
    float tempRatio = _actualDownSampleAmount / std::max( 0.000000001f, _downSampleAmount );

    // this should reflect the value visible in the UI
    _targetRate = VST::MIN_SAMPLE_RATE + value * ( _hostSampleRate - VST::MIN_SAMPLE_RATE );
    
    _downSampleNormalised = value;
    _downSampleAmount = normalisedDownSampleAmount;

    setActualDownSampling( _downSampleAmount );

    // if down sampling is deactivated and there is no playback slowdown taking place:
    // sync the read pointer with the write pointer

    if ( wasDownSampled && !isDownSampled() && !isSlowedDown() ) {
        for ( size_t c = 0; c < _amountOfChannels; ++c ) {
            _channelStates[ c ].readPointer = static_cast<float>( _writePointer );
        }
    }
}

void PluginProcess::setPlaybackRate( float value )
{
    // rate is in 0 - 1 range, playback rate speed support is between 0.5 (half speed) - 1.0f (full speed)
    // note we invert the value as a higher value should imply a higher slowdown

    float scaledAmount = Calc::scale( abs( value - 1.f ), 1, VST::MIN_PLAYBACK_SPEED ) + VST::MIN_PLAYBACK_SPEED;

    if ( scaledAmount == _playbackRate ) {
        return; // don't trigger changes if value is the same
    }

    bool wasSlowedDown = isSlowedDown();
    _playbackRate      = scaledAmount;

    // if slowdown is deactivated sync the read pointer with the write pointer

    if ( wasSlowedDown && !isSlowedDown() ) {
        for ( size_t c = 0; c < _amountOfChannels; ++c ) {
            _channelStates[ c ].readPointer = static_cast<float>( _writePointer );
        }
    }
}

void PluginProcess::setHarmony( float value )
{
    _harmonize = value;

    if ( !isHarmonized() ) {
        return;
    }

    // determine scale by integral value

    float odd  = 1.f;
    float even = 1.f;
    int scaled = static_cast<int>( round( 5.f * value ));
    switch ( scaled ) {
        // major
        default:
        case 0:
            odd  = -1; // major 7th
            even = -8; // major 3rd
            break;
        // mixolydian
        case 1:
            odd  = -2; // minor 7th
            even = -8; // major 3rd
            break;
        // augmented
        case 2:
            odd  = -4; // augmented 5th
            even = -8; // major 3rd
            break;
        // "neutral"
        case 3:
            odd  = -5; // 5th
            even = -10; // 2nd
            break;
        // minor
        case 4:
            odd = -2; // minor 7th
            even = -9; // minor 3rd
            break;
        // diminished
        case 5:
            odd = -6; // diminished 5th / tritone
            even = -9; // minor 3rd
            break;
    }
    _oddPitch  = Calc::pitchDown( odd );
    _evenPitch = Calc::pitchDown( even );
}

void PluginProcess::enableReverb( bool enabled )
{
    _reverbEnabled = enabled;
}

void PluginProcess::enableReverse( bool enabled )
{
    _reverse = enabled;
}

/* other */

bool PluginProcess::setTempo( double tempo, int32 timeSigNumerator, int32 timeSigDenominator )
{
    if ( _tempo == tempo && _timeSigNumerator == timeSigNumerator && _timeSigDenominator == timeSigDenominator ) {
        return false; // no change
    }

    _timeSigNumerator   = timeSigNumerator;
    _timeSigDenominator = timeSigDenominator;
    _tempo              = tempo;

    _fullMeasureDuration = ( 60.f / _tempo ) * _timeSigDenominator; // seconds per measure
    _fullMeasureSamples  = Calc::secondsToBuffer( _fullMeasureDuration, _hostSampleRate );
    _halfMeasureSamples  = ceil( _fullMeasureSamples / 2 );
    _beatSamples         = ceil( _fullMeasureSamples / _timeSigDenominator );
    _sixteenthSamples    = ceil( _fullMeasureSamples / 16 );

    return true;
}

void PluginProcess::createGateTables( float normalizedWaveFormType ) {
    WaveGenerator::WaveForms waveForm = WaveGenerator::WaveForms::SINE;

    if ( normalizedWaveFormType >= 0.75f ) {
        waveForm = WaveGenerator::WaveForms::SQUARE;
    } else if ( normalizedWaveFormType >= 0.5f ) {
        waveForm = WaveGenerator::WaveForms::SAWTOOTH;
    } else if ( normalizedWaveFormType >= 0.25f ) {
        waveForm = WaveGenerator::WaveForms::TRIANGLE;
    }

    if ( waveForm == _gateWaveForm ) {
        return; // don't update when tables haven't changed
    }

    _gateWaveForm = waveForm;

    // we keep the wave tables as they are and update their buffer contents
    // (this keeps accumulator position and thus phase as-is)

    float* newBuffer = TablePool::getTable( waveForm )->getBuffer();

    for ( size_t c = 0; c < _amountOfChannels; ++c ) {
        float* buffer = _waveTables.at( c ).getBuffer();
        for ( size_t i = 0; i < Igorski::VST::TABLE_SIZE; ++i ) {
            buffer[ i ] = newBuffer[ i ];
        }
    }
}

void PluginProcess::syncGates() {

}

}
