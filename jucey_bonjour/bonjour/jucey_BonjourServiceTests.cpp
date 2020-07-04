
#if JUCEY_UNIT_TESTS

template<typename SocketType>
bool enableListeningOnSocket (SocketType& socket);

template<>
bool enableListeningOnSocket (juce::DatagramSocket& socket)
{
    return socket.bindToPort (0);
}

template<>
bool enableListeningOnSocket (juce::StreamingSocket& socket)
{
    return socket.createListener (0);
}

template <typename SocketType>
bool sendStringToRemoteSocket (const juce::String& stringToSend,
                               const juce::String& targetHostName,
                               int targetPort);

template<>
bool sendStringToRemoteSocket<juce::DatagramSocket> (const juce::String& stringToSend,
                                                     const juce::String& targetHostName,
                                                     int targetPort)
{
    juce::DatagramSocket socket;

    if (socket.bindToPort (0))
    {
        const auto numBytes {(int) stringToSend.getNumBytesAsUTF8()};
        return socket.write (targetHostName, targetPort, stringToSend.toRawUTF8(), numBytes) == numBytes;
    }

    return false;
}

template<>
bool sendStringToRemoteSocket<juce::StreamingSocket> (const juce::String& stringToSend,
                                                      const juce::String& targetHostName,
                                                      int targetPort)
{
    juce::StreamingSocket socket;

    if (socket.connect (targetHostName, targetPort))
    {
        const auto numBytes {(int) stringToSend.getNumBytesAsUTF8()};
        return socket.write (stringToSend.toRawUTF8(), numBytes) == numBytes;
    }

    return false;
}

template <typename SocketType>
bool expectStringOnSocket (SocketType& socket, const juce::String& stringToExpect);

template<>
bool expectStringOnSocket (juce::DatagramSocket& socket, const juce::String& stringToExpect)
{
    juce::MemoryBlock data {stringToExpect.getNumBytesAsUTF8(), true};

    if (socket.read (data.getData(), (int) data.getSize(), true) == (int) data.getSize())
        return data.toString() == stringToExpect;

    return false;
}

template<>
bool expectStringOnSocket (juce::StreamingSocket& socket, const juce::String& stringToExpect)
{
    if (const auto listenerSocket = std::unique_ptr<juce::StreamingSocket>(socket.waitForNextConnection()))
    {
        juce::MemoryBlock data {stringToExpect.getNumBytesAsUTF8(), true};

        if (listenerSocket->read (data.getData(), (int) data.getSize(), true) == (int) data.getSize())
            return data.toString() == stringToExpect;
    }

    return false;
}

class BonjourServiceTests : private juce::UnitTest
{
public:
    BonjourServiceTests()
        : juce::UnitTest ("BonjourService", "Networking")
    {

    }

    ~BonjourServiceTests()
    {

    }

private:
    jucey::BonjourService runServiceRegistrationTests (jucey::BonjourService& serviceToRegister,
                                                       int portToRegisterServiceOn)
    {
        jucey::BonjourService registeredService;
        juce::WaitableEvent onServiceRegisteredEvent;

        const auto onServiceRegistered = [&](const jucey::BonjourService& service,
                                             const juce::Result& result)
        {
            expect (result.wasOk());
            expect (service.getName() == serviceToRegister.getName());
            expect (service.getType() == serviceToRegister.getType());
            expect (service.getDomain() == serviceToRegister.getDomain());

            for (auto index {0}; index < serviceToRegister.getNumRecordItems(); ++index)
                expect (service.getRecordItemAtIndex (index) == serviceToRegister.getRecordItemAtIndex (index));

            registeredService = service;
            onServiceRegisteredEvent.signal();
        };

        // register the service
        expect (serviceToRegister.registerAsync (onServiceRegistered, portToRegisterServiceOn));
        expect (onServiceRegisteredEvent.wait (1000));

        return registeredService;
    }

    jucey::BonjourService runServiceDiscoveryTests (const jucey::BonjourService& expectedService)
    {
        jucey::BonjourService serviceToDiscover {expectedService};
        jucey::BonjourService discoveredService;
        juce::WaitableEvent onServiceDiscoveredEvent;

        const auto onServiceDiscovered = [&](const jucey::BonjourService& service,
                                             bool isAvailable,
                                             bool isMoreComing,
                                             const juce::Result& result)
        {
            expect (result.wasOk());
            expect (isAvailable);
            expect (service.getType() == expectedService.getType());
            expect (service.getName() == expectedService.getName());
            expect (service.getDomain() == expectedService.getDomain());

            // there shouldn't be any record items until the service is resolved
            expect (service.getNumRecordItems() == 0);

            // we could be registered on multiple interfaces, just take the
            // last one to keep things simple
            if ( ! isMoreComing)
            {
                discoveredService = service;
                onServiceDiscoveredEvent.signal();
            }
        };

        // discover the registered service
        expect (serviceToDiscover.discoverAsync (onServiceDiscovered));
        expect (onServiceDiscoveredEvent.wait (-1));

        return discoveredService;
    }

    void runServiceResolutionTests (const jucey::BonjourService& serviceToResolve,
                                    juce::String& targetHostName,
                                    int& targetPort)
    {
        jucey::BonjourService resolvedService;
        juce::WaitableEvent onServiceResolvedEvent;

        const auto onServiceResolved = [&](const jucey::BonjourService& service,
                                           const juce::String& hostName,
                                           int port,
                                           const juce::Result& result)
        {
            expect (result.wasOk());
            expect (result.wasOk());
            expect (service.getName() == serviceToResolve.getName());
            expect (service.getType() == serviceToResolve.getType());
            expect (service.getDomain() == serviceToResolve.getDomain());

            for (auto index {0}; index < serviceToResolve.getNumRecordItems(); ++index)
                expect (service.getRecordItemAtIndex (index) == serviceToResolve.getRecordItemAtIndex (index));

            targetHostName = hostName;
            targetPort = port;
            onServiceResolvedEvent.signal();
        };

        jucey::BonjourService serviceToResolveCopy {serviceToResolve};
        expect (serviceToResolveCopy.resolveAsync (onServiceResolved));
        expect (onServiceResolvedEvent.wait (1000));
    }

    template <typename SocketType>
    void runBonjourNetworkTests (const juce::String& serviceTypeToTest)
    {
        beginTest ("Network Test: " + serviceTypeToTest);

        SocketType localSocket;
        expect (enableListeningOnSocket (localSocket));

        jucey::BonjourService serviceToRegister {serviceTypeToTest};
        serviceToRegister.withName ("JUCEY Test Service");
        serviceToRegister.withDomain ("local");
        serviceToRegister.setRecordItemValue ("keyA", "valueA");
        serviceToRegister.setRecordItemValue ("keyB", "valueB");

        const auto registeredService (runServiceRegistrationTests (serviceToRegister, localSocket.getBoundPort()));
        const auto discoveredService (runServiceDiscoveryTests (registeredService));

        juce::String targetHostName {};
        int targetPort {-1};
        runServiceResolutionTests (discoveredService, targetHostName, targetPort);
        expect (localSocket.getBoundPort() == targetPort);

        // send a random string using the target host name and port retrieved
        // from the resolved service
        // note that the 'remote' socket in this case is actually the local one
        // we registered when we called `runServiceRegistrationTests()`
        const auto randomString {juce::Uuid().toString()};
        expect (sendStringToRemoteSocket<SocketType>(randomString, targetHostName, targetPort));

        // check the data was recieved on the local socket we registered
        expect (expectStringOnSocket (localSocket, randomString));
    }

    void runDefaultConstructorTests()
    {
        beginTest ("Default Constructor");
        jucey::BonjourService service;
        expect (service.getType().isEmpty());
        expect (service.getName().isEmpty());
        expect (service.getDomain().isEmpty());
        expect (service.isUdp() == false);
        expect (service.isTcp() == false);
        expect (service.getNumRecordItems() == 0);
    }

    void runUdpConstructorTests()
    {
        beginTest ("UDP Constructor");
        jucey::BonjourService service {"_type._udp"};
        expect (service.getType() == "_type._udp");
        expect (service.getName().isEmpty());
        expect (service.getDomain().isEmpty());
        expect (service.isUdp() == true);
        expect (service.isTcp() == false);
        expect (service.getNumRecordItems() == 0);
    }

    void runTcpConstructorTests()
    {
        beginTest ("TCP Constructor");
        jucey::BonjourService service {"_type._tcp"};
        expect (service.getType() == "_type._tcp");
        expect (service.getName().isEmpty());
        expect (service.getDomain().isEmpty());
        expect (service.isUdp() == false);
        expect (service.isTcp() == true);
        expect (service.getNumRecordItems() == 0);
    }

    void runCopyConstructorTests()
    {
        beginTest ("Copy Constructor");
        jucey::BonjourService originalService {"_type._tcp"};
        originalService.withName ("name");
        originalService.withDomain ("domain");
        originalService.setRecordItemValue ("keyA", "valueA");
        originalService.setRecordItemValue ("keyB", "valueB");

        jucey::BonjourService copiedService {originalService};
        expect (copiedService.getType() == originalService.getType());
        expect (copiedService.getName() == originalService.getName());
        expect (copiedService.getDomain() == originalService.getDomain());
        expect (copiedService.isUdp() == originalService.isUdp());
        expect (copiedService.isTcp() == originalService.isTcp());
        expect (copiedService.getNumRecordItems() == originalService.getNumRecordItems());
        expect (copiedService.getRecordItemAtIndex (0) == originalService.getRecordItemAtIndex (0));
        expect (copiedService.getRecordItemAtIndex (1) == originalService.getRecordItemAtIndex (1));
    }

    void runRecordItemTests()
    {
        beginTest ("Record Items");

        jucey::BonjourService service;
        service.setRecordItemValue ("keyA", "valueA");
        service.setRecordItemValue ("keyB", "valueB");
        service.setRecordItemValue ("keyC", "valueC");

        expect (service.getNumRecordItems() == 3);
        expect (service.containsRecordItem ("keyA") == true);
        expect (service.containsRecordItem ("keyB") == true);
        expect (service.containsRecordItem ("keyC") == true);
        expect (service.containsRecordItem ("keyD") == false);

        service.removeRecordItem ("keyB");

        expect (service.getNumRecordItems() == 2);
        expect (service.containsRecordItem ("keyA") == true);
        expect (service.containsRecordItem ("keyB") == false);
        expect (service.containsRecordItem ("keyC") == true);
        expect (service.containsRecordItem ("keyD") == false);
    }

    void runTest() override
    {
        runDefaultConstructorTests();
        runUdpConstructorTests();
        runTcpConstructorTests();
        runCopyConstructorTests();
        runRecordItemTests();

        runBonjourNetworkTests<juce::DatagramSocket> ("_test._udp");
        runBonjourNetworkTests<juce::StreamingSocket> ("_test._tcp");
    }
};

static BonjourServiceTests bonjourServiceTests;

#endif // JUCEY_UNIT_TESTS
