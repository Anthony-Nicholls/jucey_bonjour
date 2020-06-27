
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
        beginTest ("Bonjour Network Test: " + type);

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
        serviceToRegister.setProperty ("keyA", "valueA");
        serviceToRegister.setProperty ("keyB", "valueB");

        // register the service
        serviceToRegister.registerAsync (serviceRegistered);
        expect (serviceRegisteredEvent.wait (1000));

        // discover the registered service
        serviceToDiscover.discoverAsync (serviceDiscovered);
        expect (serviceDiscoveredEvent.wait (1000));

        // the discovered service shouldn't have any properties yet
        expect (serviceToDiscover.getNumProperties() == 0);

        // resolve the discovered service
        serviceToResolve.resolveAsync (serviceResolved);
        expect (serviceResolvedEvent.wait (1000));

        // check that the resolved service has the correct properties
        expect (serviceToResolve.getNumProperties() == 2);
        expect (serviceToResolve.getProperty ("keyA") == "valueA");
        expect (serviceToResolve.getProperty ("keyB") == "valueB");

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

    void runTest() override
    {
        runBonjourNetworkTests ("_" + juce::Uuid().toString() + "._udp");
        runBonjourNetworkTests ("_" + juce::Uuid().toString() + "._tcp");
    }
};

static BonjourServiceTests bonjourServiceTests;

#endif // JUCEY_UNIT_TESTS
