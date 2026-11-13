juce_add_plugin(JackpotAlter
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotalterdrive"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jalt
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Alter Drive")

target_sources(JackpotAlter PRIVATE Source/AlterProcessor.cpp Source/AlterEditor.cpp)
target_include_directories(JackpotAlter PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotAlter PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotAlter
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(AlterTest PRODUCT_NAME "AlterTest")
  target_include_directories(AlterTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(AlterTest PRIVATE test/main.cpp Source/AlterProcessor.cpp Source/AlterEditor.cpp)
  target_compile_definitions(AlterTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotAlter")
  target_link_libraries(AlterTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
