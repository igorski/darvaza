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
#include "tablepool.h"

namespace Igorski {

std::map<WaveGenerator::WaveForms, WaveTable*> TablePool::_cachedTables;

WaveTable* TablePool::getTable( WaveGenerator::WaveForms waveformType )
{
    std::map<WaveGenerator::WaveForms, WaveTable*>::iterator it = _cachedTables.find( waveformType );

    if ( it != _cachedTables.end())
    {
        // table existed, load the pooled version
        return ( WaveTable* )( it->second );
    }
    return nullptr;
}

bool TablePool::setTable( WaveTable* waveTable, WaveGenerator::WaveForms waveformType )
{
    // don't set a table for the same type twice

    if ( hasTable( waveformType )) {
        return false;
    }
    std::map<WaveGenerator::WaveForms, WaveTable*>::iterator it = _cachedTables.find( waveformType );

    // insert the generated table into the pools table map
    _cachedTables.insert( std::pair<WaveGenerator::WaveForms, WaveTable*>( waveformType, waveTable ));

    return true;
}

bool TablePool::hasTable( WaveGenerator::WaveForms waveformType )
{
    std::map<WaveGenerator::WaveForms, WaveTable*>::iterator it = _cachedTables.find( waveformType );
    return it != _cachedTables.end();
}

bool TablePool::removeTable( WaveGenerator::WaveForms waveformType )
{
    std::map<WaveGenerator::WaveForms, WaveTable*>::iterator it = _cachedTables.find( waveformType );

    if ( it != _cachedTables.end())
    {
        delete ( WaveTable* )( it->second );

        _cachedTables.erase( it );

        return true;
    }
    return false;
}

void TablePool::flush()
{
    std::map<WaveGenerator::WaveForms, WaveTable*>::iterator it;

    for ( it = _cachedTables.begin(); it != _cachedTables.end(); it++ )
    {
        delete ( WaveTable* )( it->second );
    }
    _cachedTables.clear();
}

} // E.O namespace Igorski
