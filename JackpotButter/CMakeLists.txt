juce_add_plugin(JackpotButter
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotbutterrack"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jbtr
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Butter Rack")

target_sources(JackpotButter PRIVATE
  Source/ButterDSP.cpp
  Source/ButterProcessor.cpp
  Source/ButterEditor.cpp)

target_include_directories(JackpotButter PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotButter PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(JackpotButter
  PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)



if(J60_TEST)
  juce_add_console_app(ButterTest PRODUCT_NAME "ButterTest")
  target_include_directories(ButterTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(ButterTest PRIVATE test/main.cpp Source/ButterDSP.cpp Source/ButterProcessor.cpp Source/ButterEditor.cpp)
  target_compile_definitions(ButterTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotButter")
  target_link_libraries(ButterTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
