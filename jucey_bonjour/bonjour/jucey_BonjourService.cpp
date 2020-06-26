
#include <dns_sd.h>

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
    BonjourTxtRecord (uint16_t txtLen, const unsigned char* txtRecord)
        : isReadOnly {true}
        , length {txtLen}
        , record {(void*) txtRecord}

    {

    }

    BonjourTxtRecord()
        : isReadOnly {false}
    {
        TXTRecordCreate (&ref, length, record);
    }

    ~BonjourTxtRecord()
    {
        if ( ! isReadOnly)
            TXTRecordDeallocate (&ref);
    }

    uint16_t getLength() const
    {
        return isReadOnly ? length : TXTRecordGetLength (&ref);
    }

    const void* getBytes() const
    {
        return isReadOnly ? record : TXTRecordGetBytesPtr (&ref);
    }

    juce::String getValue (const juce::String& key) const
    {
        uint8_t valueLength {0};
        const void* valueBuffer {TXTRecordGetValuePtr (getLength(), getBytes(), key.toRawUTF8(), &valueLength)};
        return juce::String {(const char*) valueBuffer, valueLength};
    }

    jucey::BonjourService::Item getItemAtIndex (int index) const
    {
        const uint16_t maxKeyLength {256};
        char keyBuffer[maxKeyLength] {};
        uint8_t valueLength {0};
        const void* valueBuffer {nullptr};

        TXTRecordGetItemAtIndex (getLength(),
                                 getBytes(),
                                 (uint16_t) index,
                                 maxKeyLength,
                                 keyBuffer,
                                 &valueLength,
                                 &valueBuffer);

        return {keyBuffer, juce::String {(const char*) valueBuffer, valueLength}};
    }

    bool containsKey (const juce::String& name) const
    {
        return TXTRecordContainsKey (getLength(), getBytes(), name.toRawUTF8()) == 1;
    }

    int getCount() const
    {
        return (int) TXTRecordGetCount (getLength(), getBytes());
    }

    void setValue (const juce::String& key, const juce::String& value)
    {
        if (isReadOnly)
        {
            jassertfalse;
            return;
        }

        TXTRecordSetValue (&ref, key.toRawUTF8(), (uint8_t) value.length(), (const void*) value.toRawUTF8());
    }

    void removeValue (const juce::String& key)
    {
        if (isReadOnly)
        {
            jassertfalse;
            return;
        }

        TXTRecordRemoveValue (&ref, key.toRawUTF8());
    }

private:
    const bool isReadOnly {true};
    TXTRecordRef ref {};
    uint16_t length {0};
    void* record {nullptr};
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
                serviceToResolve->pimpl->hostName = hosttarget;
                serviceToResolve->pimpl->broadcastPort = port;
                serviceToResolve->pimpl->txtRecord = std::make_unique<BonjourTxtRecord>(txtLen, txtRecord);
                serviceToResolve->pimpl->resolveAsyncCallback (*serviceToResolve,
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
                serviceToRegister->pimpl->resolveAsyncCallback (*serviceToRegister,
                                                                bonjourResult (errorCode));
            }
        }

        uint32_t interfaceIndex {0};
        juce::String hostName {};
        uint16_t broadcastPort {0};
        std::unique_ptr<BonjourTxtRecord> txtRecord {std::make_unique<BonjourTxtRecord>()};

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
        , pimpl {std::make_unique<BonjourService::Pimpl>()}
    {
        pimpl->interfaceIndex = other.pimpl->interfaceIndex;
        pimpl->hostName = other.pimpl->hostName;
        pimpl->broadcastPort = other.pimpl->broadcastPort;
    }

    BonjourService& BonjourService::operator= (const jucey::BonjourService& other)
    {
        name = other.name;
        type = other.type;
        domain = other.domain;

        pimpl->dnsService = nullptr;
        pimpl->interfaceIndex = other.pimpl->interfaceIndex;
        pimpl->hostName = other.pimpl->hostName;
        pimpl->broadcastPort = other.pimpl->broadcastPort;

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

    juce::var BonjourService::getProperty (const juce::String& name, const juce::var& defaultReturnValue) const
    {
        if ( ! containsProperty (name))
            return defaultReturnValue;
        
        return pimpl->txtRecord->getValue (name);
    }

    BonjourService::Item BonjourService::getItemAtIndex (int index) const
    {
        if (index >= getNumProperties())
        {
            jassertfalse;
            return {};
        }

        return pimpl->txtRecord->getItemAtIndex (index);
    }

    void BonjourService::setProperty (const juce::String& name,
                                      const juce::var& newValue)
    {
        pimpl->txtRecord->setValue (name, newValue);
    }

    void BonjourService::removeProperty (const juce::String& name)
    {
        pimpl->txtRecord->removeValue (name);
    }

    bool BonjourService::containsProperty (const juce::String& name) const
    {
        return pimpl->txtRecord->containsKey (name);
    }

    int BonjourService::getNumProperties() const
    {
        return pimpl->txtRecord->getCount();
    }

    bool BonjourService::isUdp() const
    {
        return type.contains ("._udp");
    }

    bool BonjourService::isTcp() const
    {
        return type.contains ("._tcp");
    }

    void BonjourService::discoverAsync (BonjourService::DiscoverAsyncCallback callback, int interfaceIndex)
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
        {
            pimpl->dnsService = std::make_unique<BonjourDnsService>(ref);
            return;
        }

        callback (*this, false, false, result);
        pimpl->dnsService = nullptr;
    }

    void BonjourService::resolveAsync (jucey::BonjourService::ResolveAsyncCallback callback)
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
        {
            pimpl->dnsService = std::make_unique<BonjourDnsService>(ref);
            return;
        }

        callback (*this, result);
    }

    void BonjourService::registerAsync (jucey::BonjourService::RegisterAsyncCallback callback, int port)
    {
        const auto isBound {isUdp() ? udpSocket.bindToPort (port) : tcpSocket.createListener (port)};

        if ( ! isBound)
        {
            callback (*this, juce::Result::fail ("bonjour error: Failed to bind to port " + juce::String (port)));
            return;
        }

        DNSServiceRef ref {nullptr};
        pimpl->resolveAsyncCallback = callback;

        const auto result {bonjourResult (DNSServiceRegister (&ref,
                                                              0,
                                                              0,
                                                              name.isEmpty() ? nullptr : name.toUTF8(),
                                                              type.toUTF8(),
                                                              domain.isEmpty() ? nullptr : domain.toUTF8(),
                                                              nullptr,
                                                              isUdp() ? udpSocket.getBoundPort() : tcpSocket.getBoundPort(),
                                                              pimpl->txtRecord->getLength(),
                                                              pimpl->txtRecord->getBytes(),
                                                              &Pimpl::registerReply,
                                                              this))};

        if (result.ok())
        {
            pimpl->dnsService = std::make_unique<BonjourDnsService>(ref);
            return;
        }

        callback (*this, result);
    }

    int BonjourService::waitUntilReady (bool readyForReading,
                                        int timeoutMsecs)
    {
        if (isUdp())
            return udpSocket.waitUntilReady (readyForReading, timeoutMsecs);

        if (isTcp())
            return tcpSocket.waitUntilReady (readyForReading, timeoutMsecs);

        jassertfalse;
        return -1;
    }

    int BonjourService::read (void* destBuffer,
                              int maxBytesToRead,
                              bool blockUntilSpecifiedAmountHasArrived)
    {
        if (isUdp())
            return udpSocket.read (destBuffer, maxBytesToRead, blockUntilSpecifiedAmountHasArrived);

        if (isTcp())
        {
            if (auto tcpConnection = std::unique_ptr<juce::StreamingSocket> (tcpSocket.waitForNextConnection()))
                return tcpConnection->read (destBuffer, maxBytesToRead, blockUntilSpecifiedAmountHasArrived);
        }

        jassertfalse;
        return -1;
    }

    int BonjourService::write (const void* sourceBuffer,
                               int numBytesToWrite)
    {
        // The service must be resolved before sending data to it
        jassert (pimpl->hostName.isNotEmpty() && pimpl->broadcastPort > 0);

        if (isUdp())
            return udpSocket.write (pimpl->hostName, pimpl->broadcastPort, sourceBuffer, numBytesToWrite);

        if (isTcp())
        {
            if (tcpSocket.isConnected() || tcpSocket.connect (pimpl->hostName, pimpl->broadcastPort))
                return tcpSocket.write (sourceBuffer, numBytesToWrite);
        }

        jassertfalse;
        return -1;
    }

    BonjourService::Item::Item (const juce::String& key,
                                const juce::String& value)
        : key {key}
        , value {value}
    {

    }

    bool BonjourService::Item::operator== (const Item& other)
    {
        return key == other.key
            && value == other.value;
    }
    
    bool BonjourService::Item::operator!= (const Item& other)
    {
        return key != other.key
            && value != other.value;
    }
}

#include "jucey_BonjourServiceTests.cpp"
