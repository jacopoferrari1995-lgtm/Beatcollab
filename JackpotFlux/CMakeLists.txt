juce_add_plugin(JackpotFlux
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotflux"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jflx
  VST3_CATEGORIES Instrument Generator
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Flux")

target_sources(JackpotFlux PRIVATE Source/FluxProcessor.cpp Source/FluxEditor.cpp)
target_include_directories(JackpotFlux PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotFlux PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotFlux
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(FluxTest PRODUCT_NAME "FluxTest")
  target_include_directories(FluxTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(FluxTest PRIVATE test/main.cpp Source/FluxProcessor.cpp Source/FluxEditor.cpp)
  target_compile_definitions(FluxTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotFlux")
  target_link_libraries(FluxTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
