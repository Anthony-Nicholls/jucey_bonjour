
/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

 ID:                 jucey_bonjour
 vendor:             jucey
 version:            0.0.0
 name:               JUCEY bonjour classes
 description:        Classes to create and interact with bonjour services
 website:            https://github.com/Anthony-Nicholls/jucey_bonjour
 license:            MIT

 dependencies:       juce_core

 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
/** Config: JUCEY_UNIT_TESTS

    If enabled this will add all the module unit tests to the UnitTestRunner.
 */
#ifndef JUCEY_UNIT_TESTS
 #define JUCEY_UNIT_TESTS 0
#endif // JUCE_UNIT_TESTS

#include "bonjour/jucey_BonjourService.h"
