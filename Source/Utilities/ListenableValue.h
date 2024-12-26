/*
  ==============================================================================

    ListenableAtomic.h
    Created: 3 Jun 2024 11:50:52pm
    Author:  binya

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <vector>

#include "../PluginParameters.h"

template <typename T>
class ListenableValue;

/** A listener for a ListenableAtomic or a ListenableMutex */
template <typename T>
class ValueListener
{
public:
    virtual ~ValueListener() = default;
    virtual void valueChanged(ListenableValue<T>& source, T newValue) = 0;
};

template <typename T>
class ListenableValue
{
public:
    virtual ~ListenableValue() = default;

    virtual T load() const = 0;
    virtual ListenableValue& store(T newValue) = 0;

    virtual void addListener(ValueListener<T>* listener) = 0;
    virtual void removeListener(ValueListener<T>* listener) = 0;

    virtual ListenableValue& operator=(T newValue) = 0;
};

/** A thread-safe atomic value that can be listened to. */
template <typename T>
class ListenableAtomic final : public ListenableValue<T>
{
public:
    ListenableAtomic() : value() {}

    ListenableAtomic(T initial) : value(initial) {}

    ListenableAtomic& store(T newValue) override
    {
        value = newValue;
        for (auto listener : listeners)
        {
            listener->valueChanged(*this, newValue);
        }
        return *this;
    }

    T load() const override
    {
        return value;
    }

    void addListener(ValueListener<T>* listener) override
    {
        listeners.push_back(listener);
    }

    void removeListener(ValueListener<T>* listener) override
    {
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }

    ListenableAtomic& operator=(T newValue) override
    {
        return store(newValue);
    }

    operator T()
    {
        return load();
    }

    bool operator==(const ListenableAtomic& other) const
    {
        return &other == this;
    }

    bool operator==(T newValue) const
    {
        return newValue == this->value;
    }

private:
    std::atomic<T> value;
    std::vector<ValueListener<T>*> listeners;
};

/** A thread-safe value that uses locks, for use when atomic operations are not possible. */
template <typename T>
class ListenableMutex final : public ListenableValue<T>
{
public:
    ListenableMutex() : value() {}

    ListenableMutex(T initial) : value(initial) {}

    ListenableMutex& store(T newValue) override
    {
        {  // Lock scope
            juce::ScopedLock lock(mutex);
            value = newValue;
        }
        for (auto listener : listeners)
        {
            listener->valueChanged(*this, newValue);
        }
        return *this;
    }

    T load() const override
    {
        juce::ScopedLock lock(mutex);
        return value;
    }

    void addListener(ValueListener<T>* listener) override
    {
        listeners.push_back(listener);
    }

    void removeListener(ValueListener<T>* listener) override
    {
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }

    ListenableMutex& operator=(T newValue) override
    {
        return store(newValue);
    }

    operator T()
    {
        return load();
    }

    bool operator==(const ListenableMutex& other) const
    {
        return &other == this;
    }

    bool operator==(T newValue) const
    {
        return newValue == this->value;
    }

private:
    juce::CriticalSection mutex;
    T value;
    std::vector<ValueListener<T>*> listeners;
};

/** A dummy parameter that can be used to notify the host of state updates that it otherwise has no knowledge of. */
class UIDummyParam final
{
public:
    explicit UIDummyParam(const juce::AudioProcessorValueTreeState& apvts, const juce::String& dummyParamID) :
        dummyParam(apvts.getParameter(dummyParamID))
    {
    }

    void sendUIUpdate() const
    {
        dummyParam->beginChangeGesture();
        dummyParam->setValueNotifyingHost(1.f - dummyParam->getValue());
        dummyParam->endChangeGesture();
    }

private:
    juce::RangedAudioParameter* dummyParam{ nullptr };
};

/** An attachment connecting juce::Button to a ListenableValue<bool>. */
class ToggleButtonAttachment final : public ValueListener<bool>, public juce::Button::Listener
{
public:
    ToggleButtonAttachment(juce::Button& toggleButton, ListenableValue<bool>& listenableValue, UIDummyParam* dummyParam = nullptr) :
        button(toggleButton), value(listenableValue), hostDummy(dummyParam)
    {
        button.setToggleState(value.load(), juce::dontSendNotification);
        button.addListener(this);
        value.addListener(this);
    }

    ~ToggleButtonAttachment() override
    {
        button.removeListener(this);
        value.removeListener(this);
    }

private:
    void buttonClicked(juce::Button*) override
    {
        if (button.getToggleState() != value.load())
        {
            value = button.getToggleState();
            if (hostDummy != nullptr)
                hostDummy->sendUIUpdate();
        }
    }

    void valueChanged(ListenableValue<bool>& /* source */, bool newValue) override
    {
        button.setToggleState(newValue, juce::sendNotificationSync);
    }

    juce::Button& button;
    ListenableValue<bool>& value;

    UIDummyParam* hostDummy{ nullptr };
};