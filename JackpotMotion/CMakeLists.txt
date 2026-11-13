juce_add_plugin(JackpotMotion
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotmotion"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jmot
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Motion")

target_sources(JackpotMotion PRIVATE Source/MotionProcessor.cpp Source/MotionEditor.cpp)
target_include_directories(JackpotMotion PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotMotion PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotMotion
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(MotionTest PRODUCT_NAME "MotionTest")
  target_include_directories(MotionTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(MotionTest PRIVATE test/main.cpp Source/MotionProcessor.cpp Source/MotionEditor.cpp)
  target_compile_definitions(MotionTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotMotion")
  target_link_libraries(MotionTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
