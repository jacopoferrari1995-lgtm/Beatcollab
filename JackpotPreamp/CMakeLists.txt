juce_add_plugin(JackpotPreamp
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotpreamp"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jpre
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Preamp EQ")

# manopole e comandi renderizzati in 3D (Blender Cycles), fotogramma per fotogramma
juce_add_binary_data(JackpotPreampArt HEADER_NAME PreArt.h NAMESPACE PreArt SOURCES
  Art/knob_red.png Art/knob_grey.png Art/knob_black.png Art/chicken.png
  Art/jewel_red.png Art/jewel_red_on.png Art/jewel_amber.png Art/jewel_amber_on.png Art/jewel_green.png Art/jewel_green_on.png
  Art/toggle_up.png Art/toggle_down.png)
set_target_properties(JackpotPreampArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(JackpotPreamp PRIVATE Source/PreProcessor.cpp Source/PreEditor.cpp)
target_include_directories(JackpotPreamp PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/JackpotButter/Source)
target_compile_definitions(JackpotPreamp PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotPreamp
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotPreampArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(PreTest PRODUCT_NAME "PreTest")
  target_include_directories(PreTest PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/JackpotButter/Source)
  target_sources(PreTest PRIVATE test/main.cpp Source/PreProcessor.cpp Source/PreEditor.cpp)
  target_compile_definitions(PreTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotPreamp")
  target_link_libraries(PreTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotPreampArt)
endif()
