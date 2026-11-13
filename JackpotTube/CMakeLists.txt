juce_add_plugin(JackpotTube
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpottube"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jtbx
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Tube Box")

target_sources(JackpotTube PRIVATE Source/TubeProcessor.cpp Source/TubeEditor.cpp)
target_include_directories(JackpotTube PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotTube PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotTube
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(TubeTest PRODUCT_NAME "TubeTest")
  target_include_directories(TubeTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(TubeTest PRIVATE test/main.cpp Source/TubeProcessor.cpp Source/TubeEditor.cpp)
  target_compile_definitions(TubeTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotTube")
  target_link_libraries(TubeTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
