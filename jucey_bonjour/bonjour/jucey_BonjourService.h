
#pragma once

namespace jucey
{
    class BonjourService
    {
    public:
        BonjourService();
        BonjourService (const juce::String& type,
                        const juce::String& name = {},
                        const juce::String& domain = {});
        BonjourService (const BonjourService& other);
        ~BonjourService();

        juce::String getName() const;
        juce::String getType() const;
        juce::String getDomain() const;

        struct RecordItem
        {
            RecordItem() = default;
            
            RecordItem (const juce::String& key,
                        const juce::String& value);

            bool operator== (const RecordItem& other) const;
            bool operator!= (const RecordItem& other) const;

            const juce::String key;
            const juce::String value;
            
            JUCE_LEAK_DETECTOR (RecordItem)
        };

        juce::var getRecordItemValue (const juce::String& key, const juce::var& defaultReturnValue = {}) const;
        RecordItem getRecordItemAtIndex (int index) const;
        
        void setRecordItemValue (const juce::String& key, const juce::var& newValue);
        void removeRecordItem (const juce::String& key);

        bool containsRecordItem (const juce::String& key) const;
        int getNumRecordItems() const;

        bool isUdp() const;
        bool isTcp() const;

        using DiscoverAsyncCallback = std::function<void(const BonjourService& service, bool isAvailable, bool isMoreComing, const juce::Result& result)>;
        using ResolveAsyncCallback = std::function<void(const BonjourService& service, const juce::String& hostName, int port, const juce::Result& result)>;
        using RegisterAsyncCallback = std::function<void(const BonjourService& service, const juce::Result& result)>;

        juce::Result discoverAsync (DiscoverAsyncCallback callback, int interfaceIndex = 0);
        juce::Result resolveAsync (ResolveAsyncCallback callback);
        juce::Result registerAsync (RegisterAsyncCallback callback, int portToRegisterServiceOn);
        juce::Result registerAsync (RegisterAsyncCallback callback, const juce::DatagramSocket& socketToRegisterServiceOn);
        juce::Result registerAsync (RegisterAsyncCallback callback, const juce::StreamingSocket& socketToRegisterServiceOn);

        BonjourService& operator= (const BonjourService& other);

    private:
        juce::String type {};
        juce::String name {};
        juce::String domain {};
        
        class Pimpl;
        std::unique_ptr<Pimpl> pimpl;

        JUCE_LEAK_DETECTOR (BonjourService)
    };
}
