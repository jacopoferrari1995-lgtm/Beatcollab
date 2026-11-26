# JACKPOT TAG FORGE (serie Cybernet): dalla frase registrata al tag del produttore
juce_add_binary_data(TagArt HEADER_NAME TagArt.h NAMESPACE TagArt SOURCES Art/Bungee-Regular.ttf)
set_target_properties(TagArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
juce_add_plugin(JackpotTagForge
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpottagforge"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jtag
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Tag Forge")

target_sources(JackpotTagForge PRIVATE Source/TagProcessor.cpp Source/TagEditor.cpp)
target_include_directories(JackpotTagForge PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotTagForge PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotTagForge
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo TagArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(TagTest PRODUCT_NAME "TagTest")
  target_include_directories(TagTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(TagTest PRIVATE test/main.cpp Source/TagProcessor.cpp Source/TagEditor.cpp)
  target_compile_definitions(TagTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotTagForge")
  target_link_libraries(TagTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo TagArt)
endif()
