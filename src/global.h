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

    extern float SAMPLE_RATE; // set upon initialization, see vst.cpp

    static const float PI     = 3.141592653589793f;
    static const float TWO_PI = PI * 2.f;

    // These values are tuned to 44.1 kHz sample rate and will be
    // recalculated to match the host sample recalculated

    static const int NUM_COMBS     = 8;
    static const int NUM_ALLPASSES = 4;

    static const int COMB_TUNINGS[ NUM_COMBS ] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    static const int ALLPASS_TUNINGS[ NUM_ALLPASSES ] = { 556, 441, 341, 225 };
}
}

#endif
