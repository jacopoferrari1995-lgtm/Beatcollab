# JACKPOT CYBERNET CREW: gang vocals da una voce sola, ogni voce un personaggio
juce_add_binary_data(CrewArt HEADER_NAME CrewArt.h NAMESPACE CrewArt SOURCES Art/Bungee-Regular.ttf Art/BebasNeue-Regular.ttf)
set_target_properties(CrewArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
juce_add_plugin(JackpotCrew
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotcybernetcrew"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jcrw
  AU_MAIN_TYPE kAudioUnitType_Effect
  VST3_CATEGORIES Fx
  FORMATS ${JP_FORMATS} VST3
  PRODUCT_NAME "Jackpot Cybernet Crew")

target_sources(JackpotCrew PRIVATE Source/CrewProcessor.cpp Source/CrewEditor.cpp)
target_include_directories(JackpotCrew PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_compile_definitions(JackpotCrew PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotCrew
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo CrewArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(CrewTest PRODUCT_NAME "CrewTest")
  target_include_directories(CrewTest PRIVATE ${CMAKE_SOURCE_DIR}/Common)
  target_sources(CrewTest PRIVATE test/main.cpp Source/CrewProcessor.cpp Source/CrewEditor.cpp)
  target_compile_definitions(CrewTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotCrew")
  target_link_libraries(CrewTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotNeo CrewArt)
endif()
