/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2022 Igor Zinken - https://www.igorski.nl
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

WaveTable::WaveTable( int aTableLength, float aFrequency )
{
    tableLength  = aTableLength;
    _accumulator = 0.0;
    _buffer      = generateSilentBuffer( tableLength );
    setFrequency( aFrequency );

    _sampleRateOverLength = ( float ) VST::SAMPLE_RATE / ( float ) tableLength;
}

WaveTable::~WaveTable()
{
    delete[] _buffer;
}

/* public methods */

void WaveTable::setFrequency( float aFrequency )
{
    _frequency = aFrequency;
}

float WaveTable::getFrequency()
{
    return _frequency;
}

bool WaveTable::hasContent()
{
    for ( int i = 0; i < tableLength; ++i )
    {
        if ( _buffer[ i ] != 0.0 )
            return true;
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
    if ( _buffer != nullptr )
        delete[] _buffer;

    _buffer = aBuffer;
}

void WaveTable::cloneTable( WaveTable* waveTable )
{
    if ( tableLength != waveTable->tableLength )
    {
        delete[] _buffer;
        tableLength = waveTable->tableLength;
        _buffer     = generateSilentBuffer( tableLength );
    }

    for ( int i = 0; i < tableLength; ++i ) {
        _buffer[ i ] = waveTable->_buffer[ i ];
    }
}

WaveTable* WaveTable::clone()
{
    WaveTable* out    = new WaveTable( tableLength, _frequency );
    out->_accumulator = _accumulator;
    out->cloneTable( this );

    return out;
}

float* WaveTable::generateSilentBuffer( int bufferSize )
{
    float* out = new float[ bufferSize ];
    memset( out, 0, bufferSize * sizeof( float )); // zero bits should equal 0.0f

    return out;
}

} // E.O namespace Igorski
