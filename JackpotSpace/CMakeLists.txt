juce_add_plugin(JackpotSpace
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotspace"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jspc
  VST3_CATEGORIES Fx Reverb Delay
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Space")

target_sources(JackpotSpace PRIVATE Source/SpaceProcessor.cpp Source/SpaceEditor.cpp)
target_include_directories(JackpotSpace PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotSpace PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotSpace
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(SpaceTest PRODUCT_NAME "SpaceTest")
  target_include_directories(SpaceTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(SpaceTest PRIVATE test/main.cpp Source/SpaceProcessor.cpp Source/SpaceEditor.cpp)
  target_compile_definitions(SpaceTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotSpace")
  target_link_libraries(SpaceTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
