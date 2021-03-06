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
#ifndef __PARAMIDS_HEADER__
#define __PARAMIDS_HEADER__

enum
{
    // ids for all visual controls
    // these identifiers are mapped to the UI in plugin.uidesc
    // and consumed by controller.cpp to update the model

// --- AUTO-GENERATED START
    kOddSpeedId = 0,    // Odd channel speed
    kEvenSpeedId = 1,    // Even channel speed
    kLinkGatesId = 2,    // Link gates
    kWaveformId = 3,    // Door
    kResampleRateId = 4,    // Regret
    kPlaybackRateId = 5,    // Sorrow
    kReverbId = 6,    // Dwell
    kHarmonizeId = 7,    // Weep
    kReverseId = 8,    // Recast
    kBitDepthId = 9,    // Torture
    kRandomSpeedId = 10,    // Randomize closing speed
    kDryMixId = 11,    // Entry

// --- AUTO-GENERATED END

	kBypassId, // bypass process
    kVuPPMId // for the Vu value return to host
};

#endif
