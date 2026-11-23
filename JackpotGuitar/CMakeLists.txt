# JACKPOT GUITAR: generatore MIDI di parti di chitarra (accordi, ritmica, arpeggio, lead) con chitarra interna per ascoltare
juce_add_plugin(JackpotGuitar
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotguitar"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jgtr
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  VST3_CATEGORIES Instrument Generator
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Guitar")
target_sources(JackpotGuitar PRIVATE Source/GtrProcessor.cpp Source/GtrEditor.cpp)
target_include_directories(JackpotGuitar PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotGuitar PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotGuitar
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(GtrTest PRODUCT_NAME "GtrTest")
  target_include_directories(GtrTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(GtrTest PRIVATE test/main.cpp Source/GtrProcessor.cpp Source/GtrEditor.cpp)
  target_compile_definitions(GtrTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotGuitar")
  target_link_libraries(GtrTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats JackpotFonts JackpotNeo)
endif()
