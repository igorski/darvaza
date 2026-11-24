/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2026 Igor Zinken - https://www.igorski.nl
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
#include "reverb.h"
#include "calc.h"
#include <math.h>

namespace Igorski {

Reverb::Reverb() {
    setSampleRate( VST::DEFAULT_SAMPLE_RATE );

    setWet     ( INITIAL_WET );
    setRoomSize( INITIAL_ROOM );
    setDry     ( INITIAL_DRY );
    setDamp    ( INITIAL_DAMP );
    setWidth   ( INITIAL_WIDTH );
    setMode    ( INITIAL_MODE );

    // this will initialize the buffers with silence
    mute();
}

Reverb::~Reverb() {
    clearFilters();
}

/* public methods */

void Reverb::setSampleRate( float sampleRate )
{
    _sampleRate = sampleRate;
    setupFilters();
}

void Reverb::process( float* inBuffer, int bufferSize )
{
    // REVERB processing applied onto the temp buffer

    for ( size_t i = 0; i < bufferSize; ++i ) {
        inBuffer[ i ] = processSingle( inBuffer[ i ]);
    }
}

void Reverb::mute()
{
    if ( getMode() >= FREEZE_MODE ) {
        return;
    }

    for ( int i = 0; i < VST::NUM_COMBS; i++ ) {
        _combFilters.at( i ).mute();
    }

    for ( int i = 0; i < VST::NUM_ALLPASSES; i++ ) {
        _allpassFilters.at( i ).mute();
    }
}

float Reverb::getRoomSize()
{
    return ( _roomSize - OFFSET_ROOM ) / SCALE_ROOM;
}

void Reverb::setRoomSize( float value )
{
    _roomSize = ( value * SCALE_ROOM ) + OFFSET_ROOM;
    update();
}

float Reverb::getDamp()
{
    return _damp / SCALE_DAMP;
}

void Reverb::setDamp( float value )
{
    _damp = value * SCALE_DAMP;
    update();
}

float Reverb::getWet()
{
    return _wet / SCALE_WET;
}

void Reverb::setWet( float value )
{
    _wet = value * SCALE_WET;
    update();
}

float Reverb::getDry()
{
    return _dry / SCALE_DRY;
}

void Reverb::setDry( float value )
{
    _dry = value * SCALE_DRY;
}

float Reverb::getWidth()
{
    return _width;
}

void Reverb::setWidth( float value )
{
    _width = value;
    update();
}

float Reverb::getMode()
{
    return ( _mode >= FREEZE_MODE ) ? 1 : 0;
}

void Reverb::setMode( float value )
{
    _mode = value;
    update();
}

void Reverb::toggleFreeze()
{
    setMode( getMode() == 1 ? INITIAL_MODE : FREEZE_MODE );
}

void Reverb::setupFilters()
{
    clearFilters();

    // create filters and buffers per output channel

    // comb filter
    _combFilters.resize( VST::NUM_COMBS );

    for ( int i = 0; i < VST::NUM_COMBS; ++i ) {
        // tune the comb to the host environments sample rate
        int tuning = static_cast<int>((( static_cast<float>( VST::COMB_TUNINGS[ i ]) / 44100.f ) * _sampleRate ));
        int size = tuning + ( /*c **/ STEREO_SPREAD );
        
        _combFilters.at( i ).createBuffer( size );
    }

    // all pass filter

    _allpassFilters.resize( VST::NUM_ALLPASSES );

    for ( int i = 0; i < VST::NUM_ALLPASSES; ++i ) {
        // tune the comb to the host environments sample rate
        int tuning = static_cast<int>((( static_cast<float>( VST::ALLPASS_TUNINGS[ i ] ) / 44100.f ) * _sampleRate ));
        int size = tuning + ( /*c **/ STEREO_SPREAD );

        _allpassFilters.at( i ).createBuffer( size );
    }
}

void Reverb::clearFilters()
{
    _combFilters.clear();
    _allpassFilters.clear();
}

void Reverb::update()
{
    // Recalculate internal values after parameter change

    _wet1 = _wet * ( _width / 2 + 0.5f );
    _wet2 = _wet * (( 1 - _width ) / 2 );

    if ( _mode >= FREEZE_MODE ) {
        _roomSize1 = 1;
        _damp1     = 0;
        _gain      = MUTED;
    }
    else {
        _roomSize1 = _roomSize;
        _damp1     = _damp;
        _gain      = FIXED_GAIN;
    }

    for ( int i = 0; i < VST::NUM_COMBS; i++ ) {
        _combFilters.at( i ).setFeedback( _roomSize1 );
        _combFilters.at( i ).setDamp( _damp1 );
    }
}

}
