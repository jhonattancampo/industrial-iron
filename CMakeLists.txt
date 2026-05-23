cmake_minimum_required(VERSION 3.25)
project(IndustrialIron VERSION 1.0.0)

set(IPLUG2_DIR "/Users/awesomejohnny/Development/Plugins/iPlug2OOS/iPlug2"
    CACHE PATH "Path to the iPlug2 framework root")

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IndustrialIron.cpp
    IndustrialIron.h
    src/dsp/AsymClipper.cpp
    src/dsp/AsymClipper.h
    resources/resource.h
    src/i18n/StringKeys.h
    src/i18n/Strings_EN.h
    src/i18n/Strings_ES.h
    src/i18n/LocalizedStrings.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  FORMATS
    MINIMAL_PLUGINS
)
