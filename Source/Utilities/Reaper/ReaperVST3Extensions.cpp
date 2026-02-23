/*
  ==============================================================================

    ReaperVST3Extensions.cpp
    Created: 14 May 2025 9:43:44pm
    Author:  binya

  ==============================================================================
*/

#include "ReaperVST3Extensions.h"

namespace reaper
{
    DEF_CLASS_IID(IReaperHostApplication)
}

juce::String ReaperVST3Extensions::getNamedConfigParam(const juce::String& paramName) const
{
    if (GetSetMediaTrackInfo_String == nullptr || GetTrack == nullptr || trackIndex < 0)
        return {};

    MediaTrack* track = GetTrack(nullptr, trackIndex);

    if (track == nullptr)
        return {};

    // If our track index has not yet been updated, exit early
    if (GetTrackGUID(track) != trackGUID)
        return {};

    constexpr int buffSize = 512;
    char buffer[buffSize] = {};

    bool success = GetSetMediaTrackInfo_String(track, paramName.getCharPointer(), buffer, false);

    if (!success || buffer[0] == 0)
        return {};

    return juce::String::fromUTF8(buffer);
}

void ReaperVST3Extensions::setNamedConfigParam(const juce::String& paramName, const juce::String& value) const
{
    if (GetSetMediaTrackInfo_String == nullptr || GetTrack == nullptr || trackIndex < 0)
        return;

    MediaTrack* track = GetTrack(nullptr, trackIndex);

    if (track == nullptr)
        return;

    // If our track index has not yet been updated, exit early
    if (GetTrackGUID(track) != trackGUID)
        return;

    GetSetMediaTrackInfo_String(track, paramName.getCharPointer(), value.getCharPointer().getAddress(), true);
}

void ReaperVST3Extensions::setIHostApplication(Steinberg::FUnknown* ptr)
{
    if (ptr == nullptr)
        return;

    void* rawInterface = nullptr;

    if (ptr->queryInterface(reaper::IReaperHostApplication::iid, &rawInterface) == Steinberg::kResultOk)
    {
        if (void* fnPtr = static_cast<reaper::IReaperHostApplication*> (rawInterface)->getReaperApi("GetSetMediaTrackInfo_String"))
        {
            GetSetMediaTrackInfo_String = reinterpret_cast<bool (*)(MediaTrack*, const char*, char*, bool)>(fnPtr);
        }

        if (void* fnPtr = static_cast<reaper::IReaperHostApplication*> (rawInterface)->getReaperApi("GetTrack"))
        {
            GetTrack = reinterpret_cast<MediaTrack * (*)(ReaProject*, int)>(fnPtr);
        }

        if (void* fnPtr = static_cast<reaper::IReaperHostApplication*> (rawInterface)->getReaperApi("GetTrackGUID"))
        {
            GetTrackGUID = reinterpret_cast<GUID * (*)(MediaTrack*)>(fnPtr);
        }
    }
}

int32_t ReaperVST3Extensions::queryIEditController(const Steinberg::TUID string, void** obj)
{
#if JAS_VST3_REAPER_INTEGRATION
    if (obj == nullptr)
        return -1;
    
    if (Steinberg::FUnknownPrivate::iidEqual(string, Steinberg::Vst::ChannelContext::IInfoListener::iid))
    {
        *obj = static_cast<Steinberg::Vst::ChannelContext::IInfoListener*>(this);
        return 0;
    }

    *obj = nullptr;
#endif
    return -1;
}

Steinberg::tresult ReaperVST3Extensions::queryInterface(const Steinberg::TUID /*_iid*/, void** /*obj*/)
{
    return Steinberg::kNoInterface;
}

Steinberg::uint32 ReaperVST3Extensions::addRef()
{
    return 1;
}

Steinberg::uint32 ReaperVST3Extensions::release()
{
    return 1;
}

Steinberg::tresult ReaperVST3Extensions::setChannelContextInfos(Steinberg::Vst::IAttributeList* list)
{
    if (!list || GetTrackGUID == nullptr)
        return Steinberg::kResultFalse;

    Steinberg::int64 idx = -1;
    if (list->getInt(Steinberg::Vst::ChannelContext::kChannelIndexKey, idx) == Steinberg::kResultTrue)
    {
        trackIndex = static_cast<int>(idx) - 1;
        trackGUID = GetTrackGUID(GetTrack(nullptr, trackIndex));
    }

    return Steinberg::kResultOk;
}
