juce_add_plugin(JackpotDust
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotdust"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jdst
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Dust")

target_sources(JackpotDust PRIVATE Source/DustProcessor.cpp Source/DustEditor.cpp)
target_include_directories(JackpotDust PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotDust PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotDust
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(DustTest PRODUCT_NAME "DustTest")
  target_include_directories(DustTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(DustTest PRIVATE test/main.cpp Source/DustProcessor.cpp Source/DustEditor.cpp)
  target_compile_definitions(DustTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotDust")
  target_link_libraries(DustTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
