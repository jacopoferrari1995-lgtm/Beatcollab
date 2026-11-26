# JACKPOT CYBERNET VOX CHROME: voce cyborg con throw automatici a fine frase
juce_add_binary_data(VoxArt HEADER_NAME VoxArt.h NAMESPACE VoxArt SOURCES Art/RubikMonoOne-Regular.ttf Art/Rajdhani-Bold.ttf Art/ShareTechMono-Regular.ttf)
set_target_properties(VoxArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
juce_add_plugin(JackpotVoxChrome
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotcybernetvoxchrome"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jvxc
  AU_MAIN_TYPE kAudioUnitType_Effect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Cybernet Vox Chrome")

target_sources(JackpotVoxChrome PRIVATE Source/VoxProcessor.cpp Source/VoxEditor.cpp)
target_include_directories(JackpotVoxChrome PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotVoxChrome PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotVoxChrome
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo VoxArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(VoxTest PRODUCT_NAME "VoxTest")
  target_include_directories(VoxTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(VoxTest PRIVATE test/main.cpp Source/VoxProcessor.cpp Source/VoxEditor.cpp)
  target_compile_definitions(VoxTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotVoxChrome")
  target_link_libraries(VoxTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo VoxArt)
endif()
