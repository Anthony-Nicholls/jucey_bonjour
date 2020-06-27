#!/usr/bin/env bash

# clone the JUCE repo
rm -rf JUCE
git clone --depth 1 https://github.com/juce-framework/JUCE.git

# build the projucer
xcodebuild -project ./JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj

# generate the tests xcode project using the projucer
./JUCE/extras/Projucer/Builds/MacOSX/build/Debug/Projucer.app/Contents/MacOS/Projucer --resave ./tests.jucer

# build the tests project
xcodebuild -project ./JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj

# run the tests
./Builds/MacOSX/build/Debug/tests
