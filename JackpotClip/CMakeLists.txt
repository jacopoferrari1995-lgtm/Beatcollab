juce_add_plugin(JackpotClip
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotclip"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jclp
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Clip")

target_sources(JackpotClip PRIVATE Source/ClipProcessor.cpp Source/ClipEditor.cpp)
target_include_directories(JackpotClip PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotClip PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotClip
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(ClipTest PRODUCT_NAME "ClipTest")
  target_include_directories(ClipTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(ClipTest PRIVATE test/main.cpp Source/ClipProcessor.cpp Source/ClipEditor.cpp)
  target_compile_definitions(ClipTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotClip")
  target_link_libraries(ClipTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
