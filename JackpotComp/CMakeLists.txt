juce_add_plugin(JackpotComp
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotstudiocomp"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jcmp
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Studio Comp")

target_sources(JackpotComp PRIVATE Source/CompProcessor.cpp Source/CompEditor.cpp)
target_include_directories(JackpotComp PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotComp PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotComp
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(CompTest PRODUCT_NAME "CompTest")
  target_include_directories(CompTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(CompTest PRIVATE test/main.cpp Source/CompProcessor.cpp Source/CompEditor.cpp)
  target_compile_definitions(CompTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotComp")
  target_link_libraries(CompTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
