/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2026 Igor Zinken - https://www.igorski.nl
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
#include "wavetable.h"

namespace Igorski {

/* constructor / destructor */

WaveTable::WaveTable( int tableLength, float frequency )
{
    _tableLength = tableLength;
    _accumulator = 0.f;
    _buffer      = generateSilentBuffer( _tableLength );
    setFrequency( frequency );
    setSampleRate( VST::DEFAULT_SAMPLE_RATE );
}

WaveTable::~WaveTable()
{
    delete[] _buffer;
}

/* public methods */

void WaveTable::setSampleRate( float sampleRate )
{
    _sampleRate = sampleRate;
    _sampleRateOverLength = sampleRate / static_cast<float>( _tableLength );
}

void WaveTable::setFrequency( float frequency )
{
    _frequency = frequency;
}

float WaveTable::getFrequency()
{
    return _frequency;
}

bool WaveTable::hasContent()
{
    for ( int i = 0; i < _tableLength; ++i )
    {
        if ( _buffer[ i ] != 0.0 ) {
            return true;
        }
    }
    return false;
}

void WaveTable::setAccumulator( float value )
{
    _accumulator = value;
}

float WaveTable::getAccumulator()
{
    return _accumulator;
}

float* WaveTable::getBuffer()
{
    return _buffer;
}

void WaveTable::setBuffer( float* aBuffer )
{
    if ( _buffer != nullptr ) {
        delete[] _buffer;
    }
    _buffer = aBuffer;
}

void WaveTable::cloneTable( WaveTable* waveTable )
{
    if ( _tableLength != waveTable->_tableLength )
    {
        delete[] _buffer;
        _tableLength = waveTable->_tableLength;
        _buffer = generateSilentBuffer( _tableLength );
    }

    for ( int i = 0; i < _tableLength; ++i ) {
        _buffer[ i ] = waveTable->_buffer[ i ];
    }
}

void WaveTable::clone( WaveTable* source )
{
    _frequency = source->getFrequency();
    _accumulator = source->getAccumulator();
    cloneTable( source );
}

float* WaveTable::generateSilentBuffer( int bufferSize )
{
    float* out = new float[ bufferSize ];
    memset( out, 0, bufferSize * sizeof( float )); // zero bits should equal 0.0f

    return out;
}

} // E.O namespace Igorski
