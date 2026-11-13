juce_add_plugin(JackpotTape
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpottape"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jtap
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Tape")

target_sources(JackpotTape PRIVATE
  Source/TapeProcessor.cpp
  Source/TapeEditor.cpp)

target_include_directories(JackpotTape PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotTape PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(JackpotTape
  PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)


if(J60_TEST)
  juce_add_console_app(TapeTest PRODUCT_NAME "TapeTest")
  target_include_directories(TapeTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(TapeTest PRIVATE test/main.cpp Source/TapeProcessor.cpp Source/TapeEditor.cpp)
  target_compile_definitions(TapeTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotTape")
  target_link_libraries(TapeTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
