# JACKPOT CYBERNET CITY EDGERUNNER: la voce dentro la citta' (radio pirata, megafono, chiamata, ologramma, club sotto la pioggia, metro)
juce_add_binary_data(CityArt HEADER_NAME CityArt.h NAMESPACE CityArt SOURCES Art/Monoton-Regular.ttf Art/Rajdhani-Bold.ttf Art/ShareTechMono-Regular.ttf)
set_target_properties(CityArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
juce_add_plugin(JackpotCity
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotcybernetcity"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jcty
  AU_MAIN_TYPE kAudioUnitType_Effect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Cybernet City Edgerunner")

target_sources(JackpotCity PRIVATE Source/CityProcessor.cpp Source/CityEditor.cpp)
target_include_directories(JackpotCity PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotCity PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotCity
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo CityArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(CityTest PRODUCT_NAME "CityTest")
  target_include_directories(CityTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(CityTest PRIVATE test/main.cpp Source/CityProcessor.cpp Source/CityEditor.cpp)
  target_compile_definitions(CityTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotCity")
  target_link_libraries(CityTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo CityArt)
endif()
