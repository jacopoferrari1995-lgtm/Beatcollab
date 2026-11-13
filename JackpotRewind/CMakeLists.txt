juce_add_plugin(JackpotRewind
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotrewind"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jrwd
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Rewind")

target_sources(JackpotRewind PRIVATE Source/RewindProcessor.cpp Source/RewindEditor.cpp)
target_include_directories(JackpotRewind PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotRewind PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotRewind
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(RewindTest PRODUCT_NAME "RewindTest")
  target_include_directories(RewindTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(RewindTest PRIVATE test/main.cpp Source/RewindProcessor.cpp Source/RewindEditor.cpp)
  target_compile_definitions(RewindTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotRewind")
  target_link_libraries(RewindTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
