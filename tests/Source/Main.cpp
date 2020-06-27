
#include <JuceHeader.h>

int main (int argc, char* argv[])
{
    juce::UnitTestRunner unitTestRunner;
    unitTestRunner.runAllTests();

    auto numFailures {0};

    for (auto index {0}; index < unitTestRunner.getNumResults(); ++index)
    {
        if (auto result = unitTestRunner.getResult (index))
            numFailures += result->failures;
    }

    return numFailures;
}
