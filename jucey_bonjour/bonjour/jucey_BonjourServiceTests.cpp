
#if JUCEY_UNIT_TESTS

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
    void runBonjourNetworkTests (const juce::String& type)
    {
        beginTest ("Network Test: " + type);

        jucey::BonjourService serviceToRegister {type};
        serviceToRegister.withName ("JUCEY Bonjour Service Test");
        serviceToRegister.withDomain ("local");

        jucey::BonjourService serviceToDiscover {serviceToRegister};
        jucey::BonjourService serviceToResolve;

        juce::WaitableEvent serviceRegisteredEvent;
        juce::WaitableEvent serviceDiscoveredEvent;
        juce::WaitableEvent serviceResolvedEvent;

        const auto serviceResolved = [&](const jucey::BonjourService& service,
                                         const juce::Result& result)
        {
            expect (result.wasOk());
            serviceResolvedEvent.signal();
        };

        const auto serviceDiscovered = [&](const jucey::BonjourService& service,
                                           bool isAvailable,
                                           bool isMoreComing,
                                           const juce::Result& result)
        {
            expect (result.wasOk());
            expect (isAvailable);

            if ( ! isMoreComing)
            {
                serviceToResolve = service;
                serviceDiscoveredEvent.signal();
            }
        };

        const auto serviceRegistered = [&](const jucey::BonjourService& service,
                                           const juce::Result& result)
        {
            expect (result.wasOk());
            serviceRegisteredEvent.signal();
        };

        // add some propperties before registering the service
        serviceToRegister.setRecordItemValue ("keyA", "valueA");
        serviceToRegister.setRecordItemValue ("keyB", "valueB");

        // register the service
        serviceToRegister.registerAsync (serviceRegistered);
        expect (serviceRegisteredEvent.wait (1000));

        // discover the registered service
        serviceToDiscover.discoverAsync (serviceDiscovered);
        expect (serviceDiscoveredEvent.wait (1000));

        // the discovered service shouldn't have any properties yet
        expect (serviceToDiscover.getNumRecordItems() == 0);

        // resolve the discovered service
        serviceToResolve.resolveAsync (serviceResolved);
        expect (serviceResolvedEvent.wait (1000));

        // check that the resolved service has the correct properties
        expect (serviceToResolve.getNumRecordItems() == 2);
        expect (serviceToResolve.getRecordItemValue ("keyA") == "valueA");
        expect (serviceToResolve.getRecordItemValue ("keyB") == "valueB");

        // send random data using the resolved service
        const auto dataSent {juce::Uuid().toString()};
        const auto dataSize {dataSent.getNumBytesAsUTF8()};
        const auto dataSizeInt {static_cast<int>(dataSize)};
        expect (serviceToResolve.write (dataSent.toRawUTF8(), dataSizeInt) == dataSizeInt);

        // check the same data was recieved by the registered service
        juce::MemoryBlock dataRecieved {dataSize, true};
        expect (serviceToRegister.read (dataRecieved.getData(), dataSizeInt, true) == dataSizeInt);
        expect (dataRecieved.toString() == dataSent);
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

        // TODO: Re-enable these tests
        // These tests are being skipped as they currently require UI
        // interaction on macOS

        // runBonjourNetworkTests ("_test._udp");
        // runBonjourNetworkTests ("_test._tcp");
    }
};

static BonjourServiceTests bonjourServiceTests;

#endif // JUCEY_UNIT_TESTS
