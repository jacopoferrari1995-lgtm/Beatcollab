juce_add_plugin(JackpotFX
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotfx"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jfx8
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot FX")

target_sources(JackpotFX PRIVATE Source/FxProcessor.cpp Source/FxEditor.cpp)
target_include_directories(JackpotFX PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotFX PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotFX
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotFxSamples JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(FxTest PRODUCT_NAME "FxTest")
  target_include_directories(FxTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(FxTest PRIVATE test/main.cpp Source/FxProcessor.cpp Source/FxEditor.cpp)
  target_compile_definitions(FxTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotFX")
  target_link_libraries(FxTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotFxSamples JackpotNeo)
endif()
