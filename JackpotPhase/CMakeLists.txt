juce_add_plugin(JackpotPhase
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotphase"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jphs
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Phase")

target_sources(JackpotPhase PRIVATE Source/PhaseProcessor.cpp Source/PhaseEditor.cpp)
target_include_directories(JackpotPhase PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotPhase PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotPhase
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(PhaseTest PRODUCT_NAME "PhaseTest")
  target_include_directories(PhaseTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(PhaseTest PRIVATE test/main.cpp Source/PhaseProcessor.cpp Source/PhaseEditor.cpp)
  target_compile_definitions(PhaseTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotPhase")
  target_link_libraries(PhaseTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts)
endif()
