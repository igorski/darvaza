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
#include "plugin_process.h"
#include "calc.h"
#include "tablepool.h"
#include <math.h>

namespace Igorski {

PluginProcess::PluginProcess( int amountOfChannels ) {
    _amountOfChannels = amountOfChannels;

    _maxDownSample = VST::SAMPLE_RATE / MIN_SAMPLE_RATE;

    // cache the waveforms (as sample rate is known to be accurate on PluginProcess construction)

    TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SINE ),     WaveGenerator::WaveForms::SINE );
    TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::TRIANGLE ), WaveGenerator::WaveForms::TRIANGLE );
    TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SAWTOOTH ), WaveGenerator::WaveForms::SAWTOOTH );
    TablePool::setTable( WaveGenerator::generate( WaveGenerator::WaveForms::SQUARE ),   WaveGenerator::WaveForms::SQUARE );

    createGateTables( WaveGenerator::WaveForms::TRIANGLE );

    // these will be synced to host, see vst.cpp. here we default to 120 BPM in 4/4 time

    _tempo               = 120.f;
    _fullMeasureDuration = 2.f;
    _timeSigNumerator    = 4;
    _timeSigDenominator  = 4;

    // child processors that can work on any audio channel
    // (e.g. don't maintain the last channel-specific state)

    bitCrusher = new BitCrusher( 8, .5f, .5f );
    limiter    = new Limiter( 10.f, 500.f, .6f );

    // child processors and properties that work on individual channels

    _lastSamples  = new float[ amountOfChannels ];
    _readPointers = new float[ amountOfChannels ];

    for ( int i = 0; i < amountOfChannels; ++i ) {
        _lastSamples[ i ]  = 0.f;
        _readPointers[ i ] = 0.f;

        _lowPassFilters.push_back( new LowPassFilter());

        Reverb* reverb = new Reverb();
        reverb->setWidth( 1.f );
        reverb->setRoomSize( 1.f );

        _reverbs.push_back( reverb );
    }

    setPlaybackRate( 1.f );
    setResampleRate( 1.f );

    // will be lazily created in the process function
    _recordBuffer = nullptr;
    _preMixBuffer = nullptr;
}

PluginProcess::~PluginProcess() {
    delete bitCrusher;
    delete limiter;

    delete[] _lastSamples;
    delete[] _readPointers;

    while ( _lowPassFilters.size() > 0 ) {
        delete _lowPassFilters.at( 0 );
        _lowPassFilters.erase( _lowPassFilters.begin() );
    }

    while ( _reverbs.size() > 0 ) {
        delete _reverbs.at( 0 );
        _reverbs.erase( _reverbs.begin() );
    }

    delete _preMixBuffer;
    delete _recordBuffer;

    clearGateTables();

    TablePool::flush();
}

/* setters */

void PluginProcess::setGateSpeed( float oddSteps, float evenSteps )
{
    // (1.f / measureDuration value) converts seconds to cycles in Hertz
    float oddValue  = 1.f / ( _fullMeasureDuration / Calc::gateSubdivision( oddSteps ));
    float evenValue = 1.f / ( _fullMeasureDuration / Calc::gateSubdivision( evenSteps ));

    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        _waveTables.at( i )->setFrequency(( i + 1 ) % 2 == 0 ? evenValue : oddValue );
    }
}

void PluginProcess::resetReadWritePointers()
{
    _writePointer = 0;

    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        _readPointers[ i ] = 0.f;
    }

    for ( auto waveTable : _waveTables ) {
        waveTable->setAccumulator( 0 );
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
    float scaledAmount = Calc::scale( value, 1.f, _maxDownSample - 1.f ) + 1.f;

    if ( scaledAmount == _downSampleAmount ) {
        return; // don't trigger changes if value is the same
    }
    bool wasDownSampled = isDownSampled();
    _downSampleAmount   = scaledAmount;

    _fSampleIncr = std::max( 1.f, floor( _downSampleAmount ));
    _sampleIncr  = ( int ) _fSampleIncr;

    // update the lowpass filters to the appropriate cutoff

    float ratio = 1.f + ( _downSampleAmount / _maxDownSample );
    for ( auto lowPassFilter : _lowPassFilters ) {
        lowPassFilter->setRatio( ratio );
    }

    // if down sampling is deactivated and there is no playback slowdown taking place:
    // sync the read pointer with the write pointer

    if ( wasDownSampled && !isDownSampled() && !isSlowedDown() ) {
        for ( size_t i = 0; i < _amountOfChannels; ++i ) {
            _readPointers[ i ] = ( float ) _writePointer;
        }
    }
}

void PluginProcess::setPlaybackRate( float value )
{
    // rate is in 0 - 1 range, playback rate speed support is between 0.5 (half speed) - 1.0f (full speed)
    // note we invert the value as a higher value should imply a higher slowdown

    float scaledAmount = Calc::scale( abs( value - 1.f ), 1, MIN_PLAYBACK_SPEED ) + MIN_PLAYBACK_SPEED;

    if ( scaledAmount == _playbackRate ) {
        return; // don't trigger changes if value is the same
    }

    bool wasSlowedDown = isSlowedDown();
    _playbackRate      = scaledAmount;

    // if slowdown is deactivated sync the read pointer with the write pointer

    if ( wasSlowedDown && !isSlowedDown() ) {
        for ( size_t i = 0; i < _amountOfChannels; ++i ) {
            _readPointers[ i ] = ( float ) _writePointer;
        }
    }
}

void PluginProcess::enableReverb( bool enabled )
{
    _reverbEnabled = enabled;
}

void PluginProcess::enableHarmonize( bool enabled )
{
    _harmonize = enabled;
}

/* other */

void PluginProcess::setTempo( double tempo, int32 timeSigNumerator, int32 timeSigDenominator, float oddSteps, float evenSteps )
{
    if ( _tempo == tempo && _timeSigNumerator == timeSigNumerator && _timeSigDenominator == timeSigDenominator ) {
        return; // no change
    }

    _fullMeasureDuration = ( 60.f / _tempo ) * _timeSigDenominator; // seconds per measure
    _fullMeasureSamples  = Calc::secondsToBuffer( _fullMeasureDuration );
    _beatSamples         = ceil( _fullMeasureSamples / _timeSigDenominator );
    _sixteenthSamples    = ceil( _fullMeasureSamples / 16 );

    _timeSigNumerator   = timeSigNumerator;
    _timeSigDenominator = timeSigDenominator;
    _tempo              = tempo;

    setGateSpeed( oddSteps, evenSteps );
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
    clearGateTables();

    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        _waveTables.push_back( TablePool::getTable( waveForm )->clone() );
    }
}

void PluginProcess::syncGates() {

}

/* private methods */

void PluginProcess::clearGateTables() {
    while ( _waveTables.size() > 0 ) {
        delete _waveTables.at( 0 );
        _waveTables.erase( _waveTables.begin() );
    }
}

}
