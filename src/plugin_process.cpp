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

    // cache the waveforms (as sample rate is known to be accurate on PluginProcess construction)

    TablePool::setTable( WaveGenerator::generate( 512, WaveGenerator::WaveForms::SINE ),     WaveGenerator::WaveForms::SINE );
    TablePool::setTable( WaveGenerator::generate( 512, WaveGenerator::WaveForms::TRIANGLE ), WaveGenerator::WaveForms::TRIANGLE );
    TablePool::setTable( WaveGenerator::generate( 512, WaveGenerator::WaveForms::SAWTOOTH ), WaveGenerator::WaveForms::SAWTOOTH );
    TablePool::setTable( WaveGenerator::generate( 512, WaveGenerator::WaveForms::SQUARE ),   WaveGenerator::WaveForms::SQUARE );

    createGateTables( WaveGenerator::WaveForms::TRIANGLE );

    // these will be synced to host, see vst.cpp. here we default to 120 BPM in 4/4 time

    _tempo               = 120.f;
    _fullMeasureDuration = 2.f;
    _timeSigNumerator    = 4;
    _timeSigDenominator  = 4;

    // create the child processors

    bitCrusher = new BitCrusher( 8, .5f, .5f );
    limiter    = new Limiter( 10.f, 500.f, .6f );
    reverb     = new Reverb();

    // will be lazily created in the process function
    _recordBuffer = nullptr;
    _preMixBuffer = nullptr;
}

PluginProcess::~PluginProcess() {
    delete bitCrusher;
    delete limiter;
    delete reverb;

    delete _preMixBuffer;
    delete _recordBuffer;

    clearGateTables();

    TablePool::flush();
}

/* setters */

void PluginProcess::setGateSpeed( float evenSteps, float oddSteps )
{
    // 1.f / value converts seconds to cycles in Hertz
    float evenValue = 1.f / ( _fullMeasureDuration / (( 31.f * evenSteps ) + 1.f ));
    float oddValue  = 1.f / ( _fullMeasureDuration / (( 31.f * oddSteps )  + 1.f ));

    for ( size_t i = 0; i < _amountOfChannels; ++i ) {
        _waveTables.at( i )->setFrequency(( i + 1 ) % 2 == 0 ? evenValue : oddValue );
    }
}

void PluginProcess::resetReadWritePointers()
{
    _readPointer  = 0.f;
    _writePointer = 0;

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

/* other */

void PluginProcess::setTempo( double tempo, int32 timeSigNumerator, int32 timeSigDenominator, float evenSteps, float oddSteps )
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

    setGateSpeed( evenSteps, oddSteps );
}

void PluginProcess::createGateTables( float normalizedWaveFormType ) {
    if ( _gateTableType == normalizedWaveFormType ) {
        return;
    }
    _gateTableType = normalizedWaveFormType;
    clearGateTables();

    WaveGenerator::WaveForms waveForm = WaveGenerator::WaveForms::SINE;

    if ( normalizedWaveFormType > 0.75f ) {
        waveForm = WaveGenerator::WaveForms::SQUARE;
    } else if ( normalizedWaveFormType > 0.5f ) {
        waveForm = WaveGenerator::WaveForms::SAWTOOTH;
    } else if ( normalizedWaveFormType > 0.25f ) {
        waveForm = WaveGenerator::WaveForms::TRIANGLE;
    }

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
