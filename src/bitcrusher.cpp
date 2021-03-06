/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2022 Igor Zinken - https://www.igorski.nl
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
#include "bitcrusher.h"
#include "global.h"
#include "calc.h"
#include <limits.h>
#include <math.h>

namespace Igorski {

/* constructor */

BitCrusher::BitCrusher( float amount, float inputMix, float outputMix )
{
    setAmount   ( amount );
    setInputMix ( inputMix );
    setOutputMix( outputMix );
}

BitCrusher::~BitCrusher()
{
    // nowt
}

/* public methods */

void BitCrusher::process( float* inBuffer, int bufferSize )
{
    // sound should not be crushed ? do nothing
    if ( !isActive() ) {
        return;
    }
    
    int bitsPlusOne = _bits + 1;

    for ( int i = 0; i < bufferSize; ++i )
    {
        short input = ( short ) (( inBuffer[ i ] * _inputMix ) * SHRT_MAX );
        short prevent_offset = ( short )( -1 >> bitsPlusOne );
        input &= ( -1 << ( 16 - _bits ));
        inBuffer[ i ] = (( input + prevent_offset ) * _outputMix ) / SHRT_MAX;
    }
}

/* setters */

void BitCrusher::setAmount( float value )
{
    // note we invert the value as a higher value implies less bit rate reduction
    _amount = abs( value - 1.f );

    calcBits();
}

void BitCrusher::setInputMix( float value )
{
    _inputMix = Calc::cap( value );
}

void BitCrusher::setOutputMix( float value )
{
    _outputMix = Calc::cap( value );
}

/* private methods */

void BitCrusher::calcBits()
{
    // scale float to 1 - 16 bit range
    _bits = ( int ) floor( Calc::scale( _amount, 1, 15 )) + 1;
}

}
