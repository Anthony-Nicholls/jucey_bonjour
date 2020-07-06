# jucey_bonjour
A JUCE module wrapper for Apple's zero-configuration protocol Bonjour

## Register (advertise) a service
```cpp
jucey::BonjourService serviceToRegister {"_type._udp", "My Service", "local"};
juce::DatagramSocket udpSocket;
udpSocket.bindToPort(0);

const auto onServiceRegistered = [](const jucey::BonjourService& service,
                                    const juce::Result& result)
{
    if (result.wasOK())
    {
        std::cout << "Registered service:" << std::endl;
        std::cout << service.getType() << std::endl;
        std::cout << service.getName() << std::endl;
        std::cout << service.getDomain() << std::endl;
    }
}

serviceToRegister.registerAsync (onServiceRegistered, udpSocket);
```

## Discover (browse for) a service
```cpp
jucey::BonjourService serviceToDiscover {"_type._udp"};

const auto onServiceDiscovered = [](const jucey::BonjourService& service,
                                    bool isAvailable,
                                    bool isMoreComing,
                                    const juce::Result& result)
{
    if (result.wasOK())
    {
        std::cout << (isAvailable ? "Added service:" : "Removed service:") << std::endl;
        std::cout << service.getType() << std::endl;
        std::cout << service.getName() << std::endl;
        std::cout << service.getDomain() << std::endl;
    }
};

serviceToRegister.discoverAsync (onServiceRegistered, udpSocket);
```

## Resolve a service
```cpp
jucey::BonjourService serviceToResolve {"_type._udp"};

const auto onServiceResolved = [](const jucey::BonjourService& service,
                                  bool isAvailable,
                                  bool isMoreComing,
                                  const juce::Result& result)
{
    if (result.wasOK())
    {
        std::cout << (isAvailable ? "Added service:" : "Removed service:") << std::endl;
        std::cout << service.getType() << std::endl;
        std::cout << service.getName() << std::endl;
        std::cout << service.getDomain() << std::endl;
    }
};

serviceToRegister.discoverAsync (onServiceRegistered, udpSocket);
```

