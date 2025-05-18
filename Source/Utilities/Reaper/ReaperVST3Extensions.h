/*
  ==============================================================================

    ReaperVST3Extensions.h
    Created: 14 May 2025 9:38:25pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "reaper-plugins/reaper_plugin.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"

namespace reaper
{
    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wzero-as-null-pointer-constant",
        "-Wunused-parameter",
        "-Wnon-virtual-dtor")
    JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100)

    using namespace Steinberg;
    using INT_PTR = juce::pointer_sized_int;
    using uint32 = Steinberg::uint32;

    #include "reaper-plugins/reaper_vst3_interfaces.h"

    JUCE_END_IGNORE_WARNINGS_MSVC
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE
}

class ReaperVST3Extensions final : public juce::VST3ClientExtensions, public Steinberg::Vst::ChannelContext::IInfoListener
{
public:
    ReaperVST3Extensions() = default;
    ~ReaperVST3Extensions() override = default;

    juce::String getNamedConfigParam(const juce::String& paramName) const;
    void setNamedConfigParam(const juce::String& paramName, const juce::String& value) const;

    void setIHostApplication(Steinberg::FUnknown* ptr) override;
    int32_t queryIEditController(const Steinberg::TUID, void** obj) override;

    Steinberg::tresult PLUGIN_API setChannelContextInfos(Steinberg::Vst::IAttributeList* list) override;

    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override;
    Steinberg::uint32 PLUGIN_API addRef() override;
    Steinberg::uint32 PLUGIN_API release() override;

private:
    // https://github.com/justinfrankel/reaper-sdk/blob/main/sdk/reaper_plugin_functions.h
    bool (*GetSetMediaTrackInfo_String)(MediaTrack* tr, const char* parmname, char* stringNeedBig, bool setNewValue);
    MediaTrack* (*GetTrack)(ReaProject* proj, int trackidx) { nullptr };
    GUID* (*GetTrackGUID)(MediaTrack* tr) { nullptr };

    GUID* trackGUID{ nullptr };
    int trackIndex{ -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReaperVST3Extensions)
};
