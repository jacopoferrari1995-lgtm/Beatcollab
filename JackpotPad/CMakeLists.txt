if(APPLE)
  set(JP_PAD_FORMATS AU VST3)
else()
  set(JP_PAD_FORMATS Standalone)
endif()

juce_add_plugin(JackpotPad
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotpad"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jpad
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  FORMATS ${JP_PAD_FORMATS}
  PRODUCT_NAME "Jackpot Sampler")

target_sources(JackpotPad PRIVATE
  Source/PadProcessor.cpp
  Source/PadEditor.cpp
  Source/KitGen.cpp
  Source/Synth.cpp
  Source/Kits.cpp
  Source/Patterns.cpp
  Source/Generator.cpp
  Source/Library.cpp
  Source/SampleScan.cpp)

target_include_directories(JackpotPad PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotPad PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(JackpotPad
  PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp JackpotFonts JackpotFxSamples JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)


if(J60_TEST)
  juce_add_console_app(PadTest PRODUCT_NAME "PadTest")
  target_include_directories(PadTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(PadTest PRIVATE test/main.cpp Source/PadProcessor.cpp Source/PadEditor.cpp Source/KitGen.cpp Source/Synth.cpp Source/Kits.cpp Source/Patterns.cpp Source/Generator.cpp Source/Library.cpp Source/SampleScan.cpp)
  target_compile_definitions(PadTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotPad")
  target_link_libraries(PadTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotFxSamples JackpotNeo)
endif()
