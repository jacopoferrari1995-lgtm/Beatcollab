# JACKPOT BASS: generatore MIDI di linee di basso (808, basso, synth) con suono interno per ascoltare
juce_add_plugin(JackpotBass
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotbass"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jbas
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  VST3_CATEGORIES Instrument Generator
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Bass")
target_sources(JackpotBass PRIVATE Source/BassProcessor.cpp Source/BassEditor.cpp)
# la teoria (scale, accordi, giri, mood) e la corda Karplus-Strong vengono da Guitar
target_include_directories(JackpotBass PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/JackpotGuitar/Source)
target_compile_definitions(JackpotBass PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotBass
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(BassTest PRODUCT_NAME "BassTest")
  target_include_directories(BassTest PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/JackpotGuitar/Source)
  target_sources(BassTest PRIVATE test/main.cpp Source/BassProcessor.cpp Source/BassEditor.cpp)
  target_compile_definitions(BassTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotBass")
  target_link_libraries(BassTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats JackpotFonts JackpotNeo)
endif()
