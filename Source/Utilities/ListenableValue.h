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

private:
    juce::CriticalSection mutex;
    T value;
    std::vector<ValueListener<T>*> listeners;
};

/** An attachment connecting juce::Button to a ListenableValue<bool>. */
class ToggleButtonAttachment : public ValueListener<bool>, public juce::Button::Listener
{
public:
    ToggleButtonAttachment(juce::Button& toggleButton, ListenableValue<bool>& listenableValue) : button(toggleButton), value(listenableValue)
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
            value = button.getToggleState();
    }

    void valueChanged(ListenableValue<bool>& source, bool newValue) override
    {
        button.setToggleState(newValue, juce::dontSendNotification);
    }

    juce::Button& button;
    ListenableValue<bool>& value;
};