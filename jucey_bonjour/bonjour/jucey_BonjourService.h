
#pragma once

namespace jucey
{
    class BonjourService
    {
    public:
        BonjourService();
        BonjourService (const juce::String& type);
        BonjourService (const BonjourService& other);
        ~BonjourService();

        BonjourService& withName (const juce::String& newName);
        BonjourService& withDomain (const juce::String& newDomain);

        juce::String getName() const;
        juce::String getType() const;
        juce::String getDomain() const;

        struct Item
        {
            Item() = default;
            
            Item (const juce::String& key,
                  const juce::String& value);

            bool operator== (const Item& other);
            bool operator!= (const Item& other);

            const juce::String key;
            const juce::String value;

            JUCE_LEAK_DETECTOR (Item)
        };

        juce::var getProperty (const juce::String& name, const juce::var& defaultReturnValue = {}) const;
        Item getItemAtIndex (int index) const;
        
        void setProperty (const juce::String& name, const juce::var& newValue);
        void removeProperty (const juce::String& name);

        bool containsProperty (const juce::String& name) const;
        int getNumProperties() const;

        bool isUdp() const;
        bool isTcp() const;

        using DiscoverAsyncCallback = std::function<void(const BonjourService& service, bool isAvailable, bool isMoreComing, const juce::Result& result)>;
        using ResolveAsyncCallback = std::function<void(const BonjourService& service, const juce::Result& result)>;
        using RegisterAsyncCallback = std::function<void(const BonjourService& service, const juce::Result& result)>;

        void discoverAsync (DiscoverAsyncCallback callback, int interfaceIndex = 0);
        void resolveAsync (ResolveAsyncCallback callback);
        void registerAsync (RegisterAsyncCallback callback, int port = 0);

        int waitUntilReady (bool readyForReading, int timeoutMsecs);

        int read (void* destBuffer, int maxBytesToRead,
                  bool blockUntilSpecifiedAmountHasArrived);

        int write (const void* sourceBuffer, int numBytesToWrite);

        BonjourService& operator= (const BonjourService& other);
        bool operator== (const BonjourService& other) const;
        bool operator!= (const BonjourService& other) const;

    private:
        juce::String name {};
        juce::String type {};
        juce::String domain {};

        juce::DatagramSocket udpSocket;
        juce::StreamingSocket tcpSocket;
        
        class Pimpl;
        std::unique_ptr<Pimpl> pimpl;

        JUCE_LEAK_DETECTOR (BonjourService)
    };
}
