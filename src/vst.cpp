/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Igor Zinken - https://www.igorski.nl
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
#include "global.h"
#include "vst.h"
#include "paramids.h"
#include "calc.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include <stdio.h>

namespace Igorski {

float VST::SAMPLE_RATE = 44100.f; // updated in setupProcessing()

//------------------------------------------------------------------------
// Plugin Implementation
//------------------------------------------------------------------------
Darvaza::Darvaza()
: pluginProcess( nullptr )
// , outputGainOld( 0.f )
, currentProcessMode( -1 ) // -1 means not initialized
{
    // register its editor class (the same as used in vstentry.cpp)
    setControllerClass( VST::PluginControllerUID );

    // should be created on setupProcessing, this however doesn't fire for Audio Unit using auval?
    pluginProcess = new PluginProcess( 2 );
}

//------------------------------------------------------------------------
Darvaza::~Darvaza()
{
    // free all allocated resources
    delete pluginProcess;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::initialize( FUnknown* context )
{
    //---always initialize the parent-------
    tresult result = AudioEffect::initialize( context );
    // if everything Ok, continue
    if ( result != kResultOk )
        return result;

    //---create Audio In/Out buses------
    addAudioInput ( STR16( "Stereo In" ),  SpeakerArr::kStereo );
    addAudioOutput( STR16( "Stereo Out" ), SpeakerArr::kStereo );

    //---create Event In/Out buses (1 bus with only 1 channel)------
    addEventInput( STR16( "Event In" ), 1 );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::terminate()
{
    // nothing to do here yet...except calling our parent terminate
    return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::setActive (TBool state)
{
    if (state)
        sendTextMessage( "Darvaza::setActive (true)" );
    else
        sendTextMessage( "Darvaza::setActive (false)" );

    // reset output level meter
    // outputGainOld = 0.f;

    // call our parent setActive
    return AudioEffect::setActive( state );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::process( ProcessData& data )
{
    // In this example there are 4 steps:
    // 1) Read inputs parameters coming from host (in order to adapt our model values)
    // 2) Read inputs events coming from host (note on/off events)
    // 3) Apply the effect using the input buffer into the output buffer

    //---1) Read input parameter changes-----------
    IParameterChanges* paramChanges = data.inputParameterChanges;
    if ( paramChanges )
    {
        int32 numParamsChanged = paramChanges->getParameterCount();
        // for each parameter which are some changes in this audio block:
        for ( int32 i = 0; i < numParamsChanged; i++ )
        {
            IParamValueQueue* paramQueue = paramChanges->getParameterData( i );
            if ( paramQueue )
            {
                ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                switch ( paramQueue->getParameterId())
                {
// --- AUTO-GENERATED PROCESS START

                    case kOddSpeedId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fOddSpeed = ( float ) value;
                        break;

                    case kEvenSpeedId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fEvenSpeed = ( float ) value;
                        break;

                    case kLinkGatesId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fLinkGates = ( float ) value;
                        break;

                    case kWaveformId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fWaveform = ( float ) value;
                        break;

                    case kResampleRateId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fResampleRate = ( float ) value;
                        break;

                    case kPlaybackRateId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fPlaybackRate = ( float ) value;
                        break;

                    case kReverbId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fReverb = ( float ) value;
                        break;

                    case kHarmonizeId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fHarmonize = ( float ) value;
                        break;

                    case kReverseId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fReverse = ( float ) value;
                        break;

                    case kBitDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fBitDepth = ( float ) value;
                        break;

                    case kRandomSpeedId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fRandomSpeed = ( float ) value;
                        break;

                    case kDryMixId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fDryMix = ( float ) value;
                        break;

// --- AUTO-GENERATED PROCESS END

                    case kBypassId:
						if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value) == kResultTrue ) {
							_bypass = value >= 0.5f;
						}
						break;
                }
                syncModel();
            }
        }
    }

    // according to docs: processing context (optional, but most welcome)

    if ( data.processContext != nullptr ) {
        bool wasPlaying = isPlaying;

        // when host starts sequencer, ensure the process read pointer and write pointers are reset
        // and the gate waveforms are reset

        isPlaying = data.processContext->state & ProcessContext::kPlaying;

        if ( !wasPlaying && isPlaying ) {
            pluginProcess->resetReadWritePointers();
            pluginProcess->resetGates();
        }

        // clear the record buffers on sequencer start / stop to prevent slowed down playback from
        // reading old data that has been recorded into its "future"

        if ( wasPlaying != isPlaying ) {
            pluginProcess->clearRecordBuffer();
        }

        if ( pluginProcess->setTempo( data.processContext->tempo,
            data.processContext->timeSigNumerator, data.processContext->timeSigDenominator )) {
            pluginProcess->setGateSpeed( fOddSpeed, fEvenSpeed, Calc::toBool( fLinkGates ));
        }
    }

    //---2) Read input events-------------
//    IEventList* eventList = data.inputEvents;


    //-------------------------------------
    //---3) Process Audio---------------------
    //-------------------------------------

    if ( data.numInputs == 0 || data.numOutputs == 0 )
    {
        // nothing to do
        return kResultOk;
    }

    int32 numInChannels  = data.inputs[ 0 ].numChannels;
    int32 numOutChannels = data.outputs[ 0 ].numChannels;

    // --- get audio buffers----------------
    uint32 sampleFramesSize = getSampleFramesSizeInBytes( processSetup, data.numSamples );
    void** in  = getChannelBuffersPointer( processSetup, data.inputs [ 0 ] );
    void** out = getChannelBuffersPointer( processSetup, data.outputs[ 0 ] );

    bool isDoublePrecision = ( data.symbolicSampleSize == kSample64 );

	if ( _bypass )
	{
        // bypass mode, ensure output equals input

		for ( int32 i = 0; i < numInChannels; i++ ) {
			if ( in[ i ] != out[ i ]) {
				memcpy( out[ i ], in[ i ], sampleFramesSize );
			}
		}
	}
	else
    {
        // process the incoming sound!

        if ( isDoublePrecision ) {
            // 64-bit samples, e.g. Reaper64
            pluginProcess->process<double>(
                ( double** ) in, ( double** ) out, numInChannels, numOutChannels,
                data.numSamples, sampleFramesSize
            );
        }
        else {
            // 32-bit samples, e.g. Ableton Live...
            pluginProcess->process<float>(
                ( float** ) in, ( float** ) out, numInChannels, numOutChannels,
                data.numSamples, sampleFramesSize
            );
        }
    }

    // output flags

    data.outputs[ 0 ].silenceFlags = false; // there should always be output
    
    // float outputGain = pluginProcess->limiter->getLinearGR();
    //---4) Write output parameter changes-----------
    // IParameterChanges* outParamChanges = data.outputParameterChanges;
    // // a new value of VuMeter will be sent to the host
    // // (the host will send it back in sync to our controller for updating our editor)
    // if ( !isDoublePrecision && outParamChanges && outputGainOld != outputGain ) {
    //     int32 index = 0;
    //     IParamValueQueue* paramQueue = outParamChanges->addParameterData( kVuPPMId, index );
    //     if ( paramQueue )
    //         paramQueue->addPoint( 0, outputGain, index );
    // }
    // outputGainOld = outputGain;
    return kResultOk;
}

//------------------------------------------------------------------------
tresult Darvaza::receiveText( const char* text )
{
    // received from Controller
    fprintf( stderr, "[Darvaza] received: " );
    fprintf( stderr, "%s", text );
    fprintf( stderr, "\n" );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::setState( IBStream* state )
{
    // called when we load a preset, the model has to be reloaded

// --- AUTO-GENERATED SETSTATE START

    float savedOddSpeed = 0.f;
    if ( state->read( &savedOddSpeed, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedEvenSpeed = 0.f;
    if ( state->read( &savedEvenSpeed, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedLinkGates = 0.f;
    if ( state->read( &savedLinkGates, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedWaveform = 0.f;
    if ( state->read( &savedWaveform, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedResampleRate = 0.f;
    if ( state->read( &savedResampleRate, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedPlaybackRate = 0.f;
    if ( state->read( &savedPlaybackRate, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedReverb = 0.f;
    if ( state->read( &savedReverb, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedHarmonize = 0.f;
    if ( state->read( &savedHarmonize, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedReverse = 0.f;
    if ( state->read( &savedReverse, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedBitDepth = 0.f;
    if ( state->read( &savedBitDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedRandomSpeed = 0.f;
    if ( state->read( &savedRandomSpeed, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedDryMix = 0.f;
    if ( state->read( &savedDryMix, sizeof ( float )) != kResultOk )
        return kResultFalse;

// --- AUTO-GENERATED SETSTATE END

    int32 savedBypass = 0;
    if ( state->read( &savedBypass, sizeof ( int32 )) != kResultOk )
        return kResultFalse;

#if BYTEORDER == kBigEndian

// --- AUTO-GENERATED SETSTATE SWAP START
    SWAP_32( savedOddSpeed )
    SWAP_32( savedEvenSpeed )
    SWAP_32( savedLinkGates )
    SWAP_32( savedWaveform )
    SWAP_32( savedResampleRate )
    SWAP_32( savedPlaybackRate )
    SWAP_32( savedReverb )
    SWAP_32( savedHarmonize )
    SWAP_32( savedReverse )
    SWAP_32( savedBitDepth )
    SWAP_32( savedRandomSpeed )
    SWAP_32( savedDryMix )

// --- AUTO-GENERATED SETSTATE SWAP END

    SWAP_32( savedBypass );

#endif

// --- AUTO-GENERATED SETSTATE APPLY START
    fOddSpeed = savedOddSpeed;
    fEvenSpeed = savedEvenSpeed;
    fLinkGates = savedLinkGates;
    fWaveform = savedWaveform;
    fResampleRate = savedResampleRate;
    fPlaybackRate = savedPlaybackRate;
    fReverb = savedReverb;
    fHarmonize = savedHarmonize;
    fReverse = savedReverse;
    fBitDepth = savedBitDepth;
    fRandomSpeed = savedRandomSpeed;
    fDryMix = savedDryMix;

// --- AUTO-GENERATED SETSTATE APPLY END

    _bypass = savedBypass > 0;

    syncModel();

    // Example of using the IStreamAttributes interface
    FUnknownPtr<IStreamAttributes> stream (state);
    if ( stream )
    {
        IAttributeList* list = stream->getAttributes ();
        if ( list )
        {
            // get the current type (project/Default..) of this state
            String128 string = {0};
            if ( list->getString( PresetAttributes::kStateType, string, 128 * sizeof( TChar )) == kResultTrue )
            {
                UString128 tmp( string );
                char ascii[128];
                tmp.toAscii( ascii, 128 );
                if ( !strncmp( ascii, StateType::kProject, strlen( StateType::kProject )))
                {
                    // we are in project loading context...
                }
            }

            // get the full file path of this state
            TChar fullPath[1024];
            memset( fullPath, 0, 1024 * sizeof( TChar ));
            if ( list->getString( PresetAttributes::kFilePathStringType,
                 fullPath, 1024 * sizeof( TChar )) == kResultTrue )
            {
                // here we have the full path ...
            }
        }
    }
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::getState( IBStream* state )
{
    // here we save the model values

// --- AUTO-GENERATED GETSTATE START
    float toSaveOddSpeed = fOddSpeed;
    float toSaveEvenSpeed = fEvenSpeed;
    float toSaveLinkGates = fLinkGates;
    float toSaveWaveform = fWaveform;
    float toSaveResampleRate = fResampleRate;
    float toSavePlaybackRate = fPlaybackRate;
    float toSaveReverb = fReverb;
    float toSaveHarmonize = fHarmonize;
    float toSaveReverse = fReverse;
    float toSaveBitDepth = fBitDepth;
    float toSaveRandomSpeed = fRandomSpeed;
    float toSaveDryMix = fDryMix;

// --- AUTO-GENERATED GETSTATE END

    int32 toSaveBypass = _bypass ? 1 : 0;

#if BYTEORDER == kBigEndian

// --- AUTO-GENERATED GETSTATE SWAP START
    SWAP_32( toSaveOddSpeed )
    SWAP_32( toSaveEvenSpeed )
    SWAP_32( toSaveLinkGates )
    SWAP_32( toSaveWaveform )
    SWAP_32( toSaveResampleRate )
    SWAP_32( toSavePlaybackRate )
    SWAP_32( toSaveReverb )
    SWAP_32( toSaveHarmonize )
    SWAP_32( toSaveReverse )
    SWAP_32( toSaveBitDepth )
    SWAP_32( toSaveRandomSpeed )
    SWAP_32( toSaveDryMix )

// --- AUTO-GENERATED GETSTATE SWAP END

    SWAP_32( toSaveBypass );

#endif

// --- AUTO-GENERATED GETSTATE APPLY START
    state->write( &toSaveOddSpeed, sizeof( float ));
    state->write( &toSaveEvenSpeed, sizeof( float ));
    state->write( &toSaveLinkGates, sizeof( float ));
    state->write( &toSaveWaveform, sizeof( float ));
    state->write( &toSaveResampleRate, sizeof( float ));
    state->write( &toSavePlaybackRate, sizeof( float ));
    state->write( &toSaveReverb, sizeof( float ));
    state->write( &toSaveHarmonize, sizeof( float ));
    state->write( &toSaveReverse, sizeof( float ));
    state->write( &toSaveBitDepth, sizeof( float ));
    state->write( &toSaveRandomSpeed, sizeof( float ));
    state->write( &toSaveDryMix, sizeof( float ));

// --- AUTO-GENERATED GETSTATE APPLY END

    state->write( &toSaveBypass, sizeof( int32 ));

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::setupProcessing( ProcessSetup& newSetup )
{
    // called before the process call, always in a disabled state (not active)

    // here we keep a trace of the processing mode (offline,...) for example.
    currentProcessMode = newSetup.processMode;

    VST::SAMPLE_RATE = newSetup.sampleRate;

    syncModel();

    return AudioEffect::setupProcessing( newSetup );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::setBusArrangements( SpeakerArrangement* inputs,  int32 numIns,
                                                 SpeakerArrangement* outputs, int32 numOuts )
{
    if ( numIns == 1 && numOuts == 1 )
    {
        // the host wants Mono => Mono (or 1 channel -> 1 channel)
        if ( SpeakerArr::getChannelCount( inputs[0])  == 1 &&
             SpeakerArr::getChannelCount( outputs[0]) == 1 )
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at( 0 ));
            if ( bus )
            {
                // check if we are Mono => Mono, if not we need to recreate the buses
                if ( bus->getArrangement() != inputs[0])
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Mono In" ),  inputs[0] );
                    addAudioOutput( STR16( "Mono Out" ), inputs[0] );
                }
                return kResultOk;
            }
        }
        // the host wants something else than Mono => Mono, in this case we are always Stereo => Stereo
        else
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at(0));
            if ( bus )
            {
                tresult result = kResultFalse;

                // the host wants 2->2 (could be LsRs -> LsRs)
                if ( SpeakerArr::getChannelCount(inputs[0]) == 2 && SpeakerArr::getChannelCount( outputs[0]) == 2 )
                {
                    removeAudioBusses();
                    addAudioInput  ( STR16( "Stereo In"),  inputs[0] );
                    addAudioOutput ( STR16( "Stereo Out"), outputs[0]);
                    result = kResultTrue;
                }
                // the host want something different than 1->1 or 2->2 : in this case we want stereo
                else if ( bus->getArrangement() != SpeakerArr::kStereo )
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Stereo In"),  SpeakerArr::kStereo );
                    addAudioOutput( STR16( "Stereo Out"), SpeakerArr::kStereo );
                    result = kResultFalse;
                }
                return result;
            }
        }
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::canProcessSampleSize( int32 symbolicSampleSize )
{
    if ( symbolicSampleSize == kSample32 )
        return kResultTrue;

    // we support double processing
    if ( symbolicSampleSize == kSample64 )
        return kResultTrue;

    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Darvaza::notify( IMessage* message )
{
    if ( !message )
        return kInvalidArgument;

    if ( !strcmp( message->getMessageID(), "BinaryMessage" ))
    {
        const void* data;
        uint32 size;
        if ( message->getAttributes ()->getBinary( "MyData", data, size ) == kResultOk )
        {
            // we are in UI thread
            // size should be 100
            if ( size == 100 && (( char* ) data )[ 1 ] == 1 ) // yeah...
            {
                fprintf( stderr, "[Darvaza] received the binary message!\n" );
            }
            return kResultOk;
        }
    }

    return AudioEffect::notify( message );
}

void Darvaza::syncModel()
{
    // forward the protected model values onto the plugin process and related processors
    // NOTE: when dealing with "bool"-types, use Calc::toBool() to determine on/off
    pluginProcess->createGateTables( fWaveform ); // should come before gate speed updates
    pluginProcess->setGateSpeed( fOddSpeed, fEvenSpeed, Calc::toBool( fLinkGates ));
    pluginProcess->randomizeGateSpeed( fRandomSpeed );
    pluginProcess->bitCrusher->setAmount( fBitDepth );
    pluginProcess->setResampleRate( fResampleRate );
    pluginProcess->setPlaybackRate( fPlaybackRate );
    pluginProcess->enableReverse( Calc::toBool( fReverse ));
    pluginProcess->setHarmony( fHarmonize );
    pluginProcess->enableReverb( Calc::toBool( fReverb ));
    pluginProcess->setDryMix( fDryMix );
}

}
