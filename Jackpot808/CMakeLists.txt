juce_add_plugin(Jackpot808
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpot808studio"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE J808
  VST3_CATEGORIES Fx Distortion Dynamics
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot 808 Studio")

target_sources(Jackpot808 PRIVATE Source/S8Processor.cpp Source/S8Editor.cpp)
target_include_directories(Jackpot808 PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(Jackpot808 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(Jackpot808
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(S8Test PRODUCT_NAME "S8Test")
  target_include_directories(S8Test PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(S8Test PRIVATE test/main.cpp Source/S8Processor.cpp Source/S8Editor.cpp)
  target_compile_definitions(S8Test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Jackpot808")
  target_link_libraries(S8Test PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
