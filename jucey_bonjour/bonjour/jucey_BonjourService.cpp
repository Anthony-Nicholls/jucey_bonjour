
class BonjourDnsService : private juce::Thread
{
public:
    BonjourDnsService (DNSServiceRef ref)
        : juce::Thread {"jucey_Bonjour"}
        , ref {ref, &DNSServiceRefDeallocate}
    {
        startThread();
    }

    ~BonjourDnsService()
    {
        stopThread (1000);
    }

    void stop()
    {
        signalThreadShouldExit();
    }

private:
    void run() override
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000; /* 100ms */

        const int dnsServiceSocket {DNSServiceRefSockFD (ref.get())};
        fd_set readfds;

        while ( ! threadShouldExit())
        {
            FD_ZERO (&readfds);
            FD_SET (dnsServiceSocket, &readfds);

            if (select (dnsServiceSocket + 1, &readfds, nullptr, nullptr, &tv) > 0 && ! threadShouldExit())
            {
                if (FD_ISSET (dnsServiceSocket, &readfds) && ! threadShouldExit())
                    DNSServiceProcessResult (ref.get());
            }
        }

        ref = nullptr;
    }

    std::unique_ptr<_DNSServiceRef_t, decltype(&DNSServiceRefDeallocate)> ref;
};

class BonjourTxtRecord
{
public:
    BonjourTxtRecord()
    {
        TXTRecordCreate (&ref, 0, nullptr);
    }

    BonjourTxtRecord (const BonjourTxtRecord& other)
    {
        TXTRecordCreate (&ref, 0, nullptr);

        for (auto index {0}; index < other.getCount(); ++index)
        {
            const auto item {other.getItemAtIndex (index)};
            setValue (item.key, item.value);
        }
    }

    ~BonjourTxtRecord()
    {
        TXTRecordDeallocate (&ref);
    }

    void clear()
    {
        TXTRecordDeallocate (&ref);
        TXTRecordCreate (&ref, 0, nullptr);

    }

    void copyFrom (uint16_t txtLen, const unsigned char* txtRecord)
    {
        clear();

        for (auto index {0}; index < getCount (txtLen, txtRecord); ++index)
        {
            const auto item {getItemAtIndex (txtLen, txtRecord, index)};
            setValue (item.key, item.value);
        }
    }

    uint16_t getLength() const
    {
        return TXTRecordGetLength (&ref);
    }

    const void* getBytes() const
    {
        return TXTRecordGetBytesPtr (&ref);
    }

    juce::String getValue (const juce::String& key) const
    {
        uint8_t valueLength {0};
        const void* valueBuffer {TXTRecordGetValuePtr (getLength(), getBytes(), key.toRawUTF8(), &valueLength)};
        return juce::String {(const char*) valueBuffer, valueLength};
    }

    jucey::BonjourService::RecordItem getItemAtIndex (int index) const
    {
        return getItemAtIndex (getLength(), getBytes(), index);
    }

    bool containsKey (const juce::String& name) const
    {
        return TXTRecordContainsKey (getLength(), getBytes(), name.toRawUTF8()) == 1;
    }

    int getCount() const
    {
        return getCount (getLength(), getBytes());
    }

    void setValue (const juce::String& key, const juce::String& value)
    {
        // key names must be a maximum of 9 characters
        jassert (key.length() < 10);

        // values must be a maximum of 255 characters
        jassert (value.length() < 256);

        TXTRecordSetValue (&ref, key.toRawUTF8(), (uint8_t) value.length(), value.toRawUTF8());
    }

    void removeValue (const juce::String& key)
    {
        TXTRecordRemoveValue (&ref, key.toRawUTF8());
    }

private:
    static jucey::BonjourService::RecordItem getItemAtIndex (uint16_t txtLen, const void* txtRecord, uint16_t index)
    {
        const uint16_t maxKeyLength {256};
        char keyBuffer[maxKeyLength] {};
        uint8_t valueLength {0};
        const void* valueBuffer {nullptr};

        TXTRecordGetItemAtIndex (txtLen,
                                 txtRecord,
                                 index,
                                 maxKeyLength,
                                 keyBuffer,
                                 &valueLength,
                                 &valueBuffer);

        return {keyBuffer, juce::String {(const char*) valueBuffer, valueLength}};
    }

    static int getCount (uint16_t txtLen, const void* txtRecord)
    {
        return (int) TXTRecordGetCount (txtLen, txtRecord);
    }
    
    TXTRecordRef ref {};
};

juce::Result bonjourResult (DNSServiceErrorType errorCode)
{
    switch (errorCode)
    {
        case kDNSServiceErr_NoError:                    return juce::Result::ok();
        case kDNSServiceErr_Unknown:                    return juce::Result::fail ("bonjour error: Unknown");
        case kDNSServiceErr_NoSuchName:                 return juce::Result::fail ("bonjour error: No such name");
        case kDNSServiceErr_NoMemory:                   return juce::Result::fail ("bonjour error: No memory");
        case kDNSServiceErr_BadParam:                   return juce::Result::fail ("bonjour error: Bad parameter");
        case kDNSServiceErr_BadReference:               return juce::Result::fail ("bonjour error: Bad reference");
        case kDNSServiceErr_BadState:                   return juce::Result::fail ("bonjour error: Bad state");
        case kDNSServiceErr_BadFlags:                   return juce::Result::fail ("bonjour error: Bad flags");
        case kDNSServiceErr_Unsupported:                return juce::Result::fail ("bonjour error: Unsupported");
        case kDNSServiceErr_NotInitialized:             return juce::Result::fail ("bonjour error: Not initialized");
        case kDNSServiceErr_AlreadyRegistered:          return juce::Result::fail ("bonjour error: Already registered");
        case kDNSServiceErr_NameConflict:               return juce::Result::fail ("bonjour error: Name conflict");
        case kDNSServiceErr_Invalid:                    return juce::Result::fail ("bonjour error: Invalid");
        case kDNSServiceErr_Firewall:                   return juce::Result::fail ("bonjour error: Firewall");
        case kDNSServiceErr_Incompatible:               return juce::Result::fail ("bonjour error: Client library incompatible with daemon");
        case kDNSServiceErr_BadInterfaceIndex:          return juce::Result::fail ("bonjour error: Bad interface index");
        case kDNSServiceErr_Refused:                    return juce::Result::fail ("bonjour error: Refused");
        case kDNSServiceErr_NoSuchRecord:               return juce::Result::fail ("bonjour error: No such record");
        case kDNSServiceErr_NoAuth:                     return juce::Result::fail ("bonjour error: No auth");
        case kDNSServiceErr_NoSuchKey:                  return juce::Result::fail ("bonjour error: No such key");
        case kDNSServiceErr_NATTraversal:               return juce::Result::fail ("bonjour error: NAT Traversal");
        case kDNSServiceErr_DoubleNAT:                  return juce::Result::fail ("bonjour error: Double NAT");
        case kDNSServiceErr_BadTime:                    return juce::Result::fail ("bonjour error: Bad time");
        case kDNSServiceErr_BadSig:                     return juce::Result::fail ("bonjour error: Bad signature");
        case kDNSServiceErr_BadKey:                     return juce::Result::fail ("bonjour error: Bad key");
        case kDNSServiceErr_Transient:                  return juce::Result::fail ("bonjour error: Transient");
        case kDNSServiceErr_ServiceNotRunning:          return juce::Result::fail ("bonjour error: Background daemon not running");
        case kDNSServiceErr_NATPortMappingUnsupported:  return juce::Result::fail ("bonjour error: NAT port mapping unsupported: NAT doesn't support PCP, NAT-PMP or UPnP");
        case kDNSServiceErr_NATPortMappingDisabled:     return juce::Result::fail ("bonjour error: NAT port mapping disabled: NAT supports PCP, NAT-PMP or UPnP, but it's disabled by the administrator");
        case kDNSServiceErr_NoRouter:                   return juce::Result::fail ("bonjour error: No router currently configured (probably no network connectivity)");
        case kDNSServiceErr_PollingMode:                return juce::Result::fail ("bonjour error: Polling mode");
        case kDNSServiceErr_Timeout:                    return juce::Result::fail ("bonjour error: Timeout");
        default:                                        return juce::Result::fail ("bonjour error: Unhandled");
    };
}

namespace jucey
{
    struct BonjourService::Pimpl
    {
        static void browseReply (DNSServiceRef sdRef,
                                 DNSServiceFlags flags,
                                 uint32_t interfaceIndex,
                                 DNSServiceErrorType errorCode,
                                 const char* serviceName,
                                 const char* regtype,
                                 const char* replyDomain,
                                 void* context)
        {
            if (auto* serviceToDiscover {static_cast<BonjourService*>(context)})
            {
                BonjourService discoveredService {regtype};
                discoveredService.withName (serviceName);
                discoveredService.withDomain (replyDomain);
                discoveredService.pimpl->interfaceIndex = interfaceIndex;
                serviceToDiscover->pimpl->discoverAsyncCallback (discoveredService,
                                                                 flags & kDNSServiceFlagsAdd,
                                                                 flags & kDNSServiceFlagsMoreComing,
                                                                 bonjourResult (errorCode));
            }
        }

        static void resolveReply (DNSServiceRef sdRef,
                                  DNSServiceFlags flags,
                                  uint32_t interfaceIndex,
                                  DNSServiceErrorType errorCode,
                                  const char* fullname,
                                  const char* hosttarget,
                                  uint16_t port,                                   /* In network byte order */
                                  uint16_t txtLen,
                                  const unsigned char* txtRecord,
                                  void* context)
        {
            if (auto* serviceToResolve {static_cast<BonjourService*>(context)})
            {
                serviceToResolve->pimpl->dnsService->stop();
                serviceToResolve->pimpl->interfaceIndex = interfaceIndex;
                serviceToResolve->pimpl->txtRecord.copyFrom (txtLen, txtRecord);
                serviceToResolve->pimpl->resolveAsyncCallback (*serviceToResolve,
                                                               hosttarget,
                                                               port,
                                                               bonjourResult (errorCode));
            }
        }

        static void registerReply (DNSServiceRef sdRef,
                                   DNSServiceFlags flags,
                                   DNSServiceErrorType errorCode,
                                   const char* name,
                                   const char* regtype,
                                   const char* domain,
                                   void* context)
        {
            if (auto* serviceToRegister {static_cast<BonjourService*>(context)})
            {
                serviceToRegister->name = name;
                serviceToRegister->type = regtype;
                serviceToRegister->domain = domain;
                serviceToRegister->pimpl->registerAsyncCallback (*serviceToRegister,
                                                                 bonjourResult (errorCode));
            }
        }

        Pimpl()
        {

        }

        Pimpl (const Pimpl& other)
            : interfaceIndex {other.interfaceIndex}
            , txtRecord {other.txtRecord}
        {

        }

        void startDnsService (DNSServiceRef ref)
        {
            jassert (ref != nullptr);
            dnsService = std::make_unique<BonjourDnsService>(ref);
        }

        void stopDnsService()
        {
            if (dnsService != nullptr)
                dnsService->stop();
        }

        uint32_t interfaceIndex {0};
        BonjourTxtRecord txtRecord {};

        // These should all be unique per instance even when a copy occurs
        DiscoverAsyncCallback discoverAsyncCallback {nullptr};
        ResolveAsyncCallback resolveAsyncCallback {nullptr};
        RegisterAsyncCallback registerAsyncCallback {nullptr};
        std::unique_ptr<BonjourDnsService> dnsService {nullptr};
    };

    BonjourService::BonjourService()
        : pimpl {std::make_unique<BonjourService::Pimpl>()}
    {

    }

    BonjourService::BonjourService (const juce::String& type)
        : type {type}
        , pimpl {std::make_unique<BonjourService::Pimpl>()}
    {
        jassert (type.startsWith ("_"));
        jassert (isUdp() || isTcp());
    }

    BonjourService::BonjourService (const BonjourService& other)
        : name {other.name}
        , type {other.type}
        , domain {other.domain}
        , pimpl {std::make_unique<BonjourService::Pimpl>(*other.pimpl.get())}
    {

    }

    BonjourService& BonjourService::operator= (const BonjourService& other)
    {
        name = other.name;
        type = other.type;
        domain = other.domain;
        pimpl = std::make_unique<BonjourService::Pimpl>(*other.pimpl.get());

        return *this;
    }

    BonjourService::~BonjourService()
    {
        
    }

    BonjourService& BonjourService::withName (const juce::String& newName)
    {
        name = newName;
        return *this;
    }

    BonjourService& BonjourService::withDomain (const juce::String& newDomain)
    {
        domain = newDomain;
        return *this;
    }

    juce::String BonjourService::getName() const
    {
        return name;
    }

    juce::String BonjourService::getType() const
    {
        return type;
    }

    juce::String BonjourService::getDomain() const
    {
        return domain;
    }

    juce::var BonjourService::getRecordItemValue (const juce::String& key, const juce::var& defaultReturnValue) const
    {
        if ( ! containsRecordItem (key))
            return defaultReturnValue;
        
        return pimpl->txtRecord.getValue (key);
    }

    BonjourService::RecordItem BonjourService::getRecordItemAtIndex (int index) const
    {
        if (index >= getNumRecordItems())
        {
            jassertfalse;
            return {};
        }

        return pimpl->txtRecord.getItemAtIndex (index);
    }

    void BonjourService::setRecordItemValue (const juce::String& key,
                                             const juce::var& newValue)
    {
        pimpl->txtRecord.setValue (key, newValue);
    }

    void BonjourService::removeRecordItem (const juce::String& key)
    {
        pimpl->txtRecord.removeValue (key);
    }

    bool BonjourService::containsRecordItem (const juce::String& key) const
    {
        return pimpl->txtRecord.containsKey (key);
    }

    int BonjourService::getNumRecordItems() const
    {
        return pimpl->txtRecord.getCount();
    }

    bool BonjourService::isUdp() const
    {
        return type.contains ("._udp");
    }

    bool BonjourService::isTcp() const
    {
        return type.contains ("._tcp");
    }

    juce::Result BonjourService::discoverAsync (BonjourService::DiscoverAsyncCallback callback, int interfaceIndex)
    {
        DNSServiceRef ref {nullptr};
        pimpl->discoverAsyncCallback = callback;

        const auto result {bonjourResult (DNSServiceBrowse (&ref,
                                                            0,
                                                            interfaceIndex,
                                                            type.toUTF8(),
                                                            domain.isEmpty() ? nullptr : domain.toUTF8(),
                                                            &Pimpl::browseReply,
                                                            this))};

        if (result.ok())
            pimpl->startDnsService (ref);

        return result;
    }

    juce::Result BonjourService::resolveAsync (jucey::BonjourService::ResolveAsyncCallback callback)
    {
        DNSServiceRef ref {nullptr};
        pimpl->resolveAsyncCallback = callback;

        const auto result {bonjourResult (DNSServiceResolve (&ref,
                                                             0,
                                                             pimpl->interfaceIndex,
                                                             name.toUTF8(),
                                                             type.toUTF8(),
                                                             domain.toUTF8(),
                                                             &Pimpl::resolveReply,
                                                             this))};

        if (result.ok())
            pimpl->startDnsService (ref);

        return result;
    }

    juce::Result BonjourService::registerAsync (jucey::BonjourService::RegisterAsyncCallback callback, int portToRegisterServiceOn)
    {
        // A socket should be bound to a valid port *before* calling register
        jassert (portToRegisterServiceOn > 0 && portToRegisterServiceOn < 65536);

        DNSServiceRef ref {nullptr};
        pimpl->registerAsyncCallback = callback;

        const auto result {bonjourResult (DNSServiceRegister (&ref,
                                                              0,
                                                              0,
                                                              name.isEmpty() ? nullptr : name.toUTF8(),
                                                              type.toUTF8(),
                                                              domain.isEmpty() ? nullptr : domain.toUTF8(),
                                                              nullptr,
                                                              portToRegisterServiceOn,
                                                              pimpl->txtRecord.getLength(),
                                                              pimpl->txtRecord.getBytes(),
                                                              &Pimpl::registerReply,
                                                              this))};

        if (result.ok())
            pimpl->startDnsService (ref);

        return result;
    }

    BonjourService::RecordItem::RecordItem (const juce::String& key,
                                            const juce::String& value)
        : key {key}
        , value {value}
    {

    }

    bool BonjourService::RecordItem::operator== (const RecordItem& other) const
    {
        return key == other.key
            && value == other.value;
    }
    
    bool BonjourService::RecordItem::operator!= (const RecordItem& other) const
    {
        return key != other.key
            || value != other.value;
    }
}

#include "jucey_BonjourServiceTests.cpp"
