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
#ifndef __TABLEPOOL_H_INCLUDED__
#define __TABLEPOOL_H_INCLUDED__

#include "wavetable.h"
#include "wavegenerator.h"
#include <map>

namespace Igorski {
class TablePool
{
    public:

        // retrieves the pooled table for given waveformType

        static WaveTable* getTable( WaveGenerator::WaveForms waveformType );

        // stores the given WaveTable for the given waveform type inside the pool.
        // if the table was empty and the waveformType exists inside the WaveGenerator,
        // the tables buffer contents are generated on the fly
        // returns boolean success

        static bool setTable( WaveTable* waveTable, WaveGenerator::WaveForms waveformType );

        // query whether the pool has a WaveTable for given waveformType

        static bool hasTable( WaveGenerator::WaveForms waveformType );

        // removes the reference to WaveTables of given waveformType
        // also clears the table contents from memory

        static bool removeTable( WaveGenerator::WaveForms waveformType );

        // clears all registered WaveTables and their contents

        static void flush();

    private:

        static std::map<WaveGenerator::WaveForms, WaveTable*> _cachedTables;
};
} // E.O namespace Igorski

#endif
