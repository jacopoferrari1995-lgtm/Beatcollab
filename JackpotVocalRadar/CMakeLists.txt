juce_add_plugin(JackpotVocalRadar
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotvocalradar"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jvrd
  VST3_CATEGORIES Fx EQ Dynamics
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Vocal Radar")

target_sources(JackpotVocalRadar PRIVATE Source/VrProcessor.cpp Source/VrEditor.cpp)
target_include_directories(JackpotVocalRadar PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotVocalRadar PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotVocalRadar
  PRIVATE juce::juce_audio_utils JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(VrTest PRODUCT_NAME "VrTest")
  target_include_directories(VrTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(VrTest PRIVATE test/main.cpp Source/VrProcessor.cpp Source/VrEditor.cpp)
  target_compile_definitions(VrTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotVocalRadar")
  target_link_libraries(VrTest PRIVATE juce::juce_audio_utils JackpotFonts JackpotNeo)
endif()
