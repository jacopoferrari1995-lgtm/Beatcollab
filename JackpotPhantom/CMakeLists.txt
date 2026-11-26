# JACKPOT CYBERNET PHANTOM: voci fantasma che escono dalla tua voce
juce_add_binary_data(PhantomArt HEADER_NAME PhantomArt.h NAMESPACE PhantomArt SOURCES Art/KronaOne.ttf Art/VT323-Regular.ttf Art/Rajdhani-Bold.ttf)
set_target_properties(PhantomArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
juce_add_plugin(JackpotPhantom
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotcybernetphantom"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jphn
  AU_MAIN_TYPE kAudioUnitType_Effect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Cybernet Phantom")

target_sources(JackpotPhantom PRIVATE Source/PhantomProcessor.cpp Source/PhantomEditor.cpp)
target_include_directories(JackpotPhantom PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotPhantom PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotPhantom
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo PhantomArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(PhantomTest PRODUCT_NAME "PhantomTest")
  target_include_directories(PhantomTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(PhantomTest PRIVATE test/main.cpp Source/PhantomProcessor.cpp Source/PhantomEditor.cpp)
  target_compile_definitions(PhantomTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotPhantom")
  target_link_libraries(PhantomTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo PhantomArt)
endif()
