# JACKPOT CHOIR: cori e armonie dalla voce (scala o note MIDI), PSOLA con formanti al loro posto
juce_add_plugin(JackpotChoir
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotchoir"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jcho
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  VST3_CATEGORIES Fx Pitch Shift
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Choir")

target_sources(JackpotChoir PRIVATE Source/ChoirProcessor.cpp Source/ChoirEditor.cpp)
target_include_directories(JackpotChoir PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotChoir PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotChoir
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(ChoirTest PRODUCT_NAME "ChoirTest")
  target_include_directories(ChoirTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(ChoirTest PRIVATE test/main.cpp Source/ChoirProcessor.cpp Source/ChoirEditor.cpp)
  target_compile_definitions(ChoirTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotChoir")
  target_link_libraries(ChoirTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo)
endif()
