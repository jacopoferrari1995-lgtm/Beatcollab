cmake_minimum_required(VERSION 3.22)
project(JackpotPlugins VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
  set(JP_FORMATS AU)
else()
  set(JP_FORMATS Standalone)
endif()

add_subdirectory(JUCE)
juce_add_binary_data(JackpotFonts HEADER_NAME JackpotFontsData.h NAMESPACE JackpotFontsData SOURCES
  Common/Fonts/Rajdhani-Medium.ttf Common/Fonts/Rajdhani-SemiBold.ttf Common/Fonts/Rajdhani-Bold.ttf
  Common/Fonts/ShareTechMono-Regular.ttf Common/Fonts/Michroma-Regular.ttf)
set_target_properties(JackpotFonts PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(Jackpot60)
add_subdirectory(JackpotTape)
add_subdirectory(JackpotPad)
add_subdirectory(JackpotButter)
add_subdirectory(JackpotComp)
add_subdirectory(JackpotMeter)
