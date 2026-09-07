#ifndef __GLOBAL_HEADER__
#define __GLOBAL_HEADER__

#include "pluginterfaces/base/fplatform.h"
#include "pluginterfaces/base/funknown.h"

using namespace Steinberg;

namespace Igorski {
namespace VST {

    static const int   ID       = 97151821;
    static const char* NAME     = "Darvaza";
    static const char* VENDOR   = "igorski.nl";

    // generate unique UIDs for these (www.uuidgenerator.net is great for this)

    static const FUID PluginProcessorUID( 0x5F242E0B, 0x955D4A80, 0x85CF461F, 0xAAD2543F );
    static const FUID PluginWithSideChainProcessorUID( 0x955D4A80, 0x85CF461F, 0xAAD2543F, 0x5F242E0B );
    static const FUID PluginControllerUID( 0x85CF461F, 0xAAD2543F, 0x5F242E0B, 0x955D4A80 );

    static const float DEFAULT_SAMPLE_RATE = 44100.f; // updated in vst.cpp#setupProcessing
    static const int DEFAULT_BUFFER_SIZE = 2048; // updated in vst.cpp#setupProcessing

    static const float PI     = 3.141592653589793f;
    static const float TWO_PI = PI * 2.f;

    // plugin properties

    static const float MAX_RECORD_SECONDS = 30.f;
    static const float MIN_PLAYBACK_SPEED = 0.5f;
    static const float MIN_SAMPLE_RATE    = 2000.f;

    // These values are tuned to 44.1 kHz sample rate and will be
    // recalculated to match the host sample recalculated

    static const int NUM_COMBS     = 8;
    static const int NUM_ALLPASSES = 4;

    static const int COMB_TUNINGS[ NUM_COMBS ] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static const int ALLPASS_TUNINGS[ NUM_ALLPASSES ] = { 556, 441, 341, 225 };
}
}

#endif
