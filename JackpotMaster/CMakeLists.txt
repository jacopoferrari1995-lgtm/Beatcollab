juce_add_plugin(JackpotMaster
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotmaster"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jmst
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Master")

target_sources(JackpotMaster PRIVATE Source/MasterProcessor.cpp Source/MasterEditor.cpp Source/MasterEasyView.cpp)
target_include_directories(JackpotMaster PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotMaster PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotMaster
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(MasterTest PRODUCT_NAME "MasterTest")
  target_include_directories(MasterTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(MasterTest PRIVATE test/main.cpp Source/MasterProcessor.cpp Source/MasterEditor.cpp Source/MasterEasyView.cpp)
  target_compile_definitions(MasterTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotMaster")
  target_link_libraries(MasterTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
