juce_add_plugin(JackpotMeter
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotmeteringhub"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jmtr
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Metering Hub")

target_sources(JackpotMeter PRIVATE Source/MeterProcessor.cpp Source/MeterEditor.cpp)
target_include_directories(JackpotMeter PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotMeter PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotMeter
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(MeterTest PRODUCT_NAME "MeterTest")
  target_include_directories(MeterTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(MeterTest PRIVATE test/main.cpp Source/MeterProcessor.cpp Source/MeterEditor.cpp)
  target_compile_definitions(MeterTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotMeter")
  target_link_libraries(MeterTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
