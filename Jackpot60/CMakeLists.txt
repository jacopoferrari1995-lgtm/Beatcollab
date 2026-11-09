juce_add_plugin(Jackpot60
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpot60"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jk60
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot 60")

target_sources(Jackpot60 PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/KitGen.cpp)

target_compile_definitions(Jackpot60 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Jackpot60
  PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(J60Test PRODUCT_NAME "J60Test")
  target_sources(J60Test PRIVATE test/main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/KitGen.cpp)
  target_compile_definitions(J60Test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Jackpot60")
  target_link_libraries(J60Test PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp)
endif()
