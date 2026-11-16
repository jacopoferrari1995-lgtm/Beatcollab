juce_add_plugin(JackpotSampleDNA
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotsampledna"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jdna
  VST3_CATEGORIES Instrument Sampler
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Sample DNA")

target_sources(JackpotSampleDNA PRIVATE Source/DnaProcessor.cpp Source/DnaEditor.cpp)
target_include_directories(JackpotSampleDNA PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotSampleDNA PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotSampleDNA
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(DnaTest PRODUCT_NAME "DnaTest")
  target_include_directories(DnaTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(DnaTest PRIVATE test/main.cpp Source/DnaProcessor.cpp Source/DnaEditor.cpp)
  target_compile_definitions(DnaTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotSampleDNA")
  target_link_libraries(DnaTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
