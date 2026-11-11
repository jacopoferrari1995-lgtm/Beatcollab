juce_add_plugin(JackpotMix
  COMPANY_NAME "Jackpot.wav"
  BUNDLE_ID "com.jackpotwav.jackpotmixology"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Jkpt
  PLUGIN_CODE Jmix
  AU_MAIN_TYPE kAudioUnitType_Effect
  FORMATS ${JP_FORMATS}
  PRODUCT_NAME "Jackpot Mixology")

juce_add_binary_data(JackpotMixArt HEADER_NAME PreArt.h NAMESPACE PreArt SOURCES
  Art/knob_red.png Art/knob_grey.png Art/knob_black.png Art/chicken.png
  Art/jewel_red.png Art/jewel_red_on.png Art/jewel_amber.png Art/jewel_amber_on.png Art/jewel_green.png Art/jewel_green_on.png
  Art/toggle_up.png Art/toggle_down.png)
set_target_properties(JackpotMixArt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

set(MIX_INC ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/JackpotButter/Source ${CMAKE_SOURCE_DIR}/JackpotComp/Source
  ${CMAKE_SOURCE_DIR}/JackpotClip/Source ${CMAKE_SOURCE_DIR}/JackpotTube/Source ${CMAKE_SOURCE_DIR}/JackpotAlter/Source
  ${CMAKE_SOURCE_DIR}/JackpotMotion/Source ${CMAKE_SOURCE_DIR}/JackpotMaster/Source)
set(MIX_SRC Source/MixModules.cpp Source/MixProcessor.cpp Source/MixPresets.cpp Source/MixEditor.cpp)

target_sources(JackpotMix PRIVATE ${MIX_SRC})
target_include_directories(JackpotMix PRIVATE ${MIX_INC})
target_compile_definitions(JackpotMix PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(JackpotMix
  PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotMixArt
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(J60_TEST)
  juce_add_console_app(MixTest PRODUCT_NAME "MixTest")
  target_include_directories(MixTest PRIVATE ${MIX_INC})
  target_sources(MixTest PRIVATE test/main.cpp ${MIX_SRC})
  target_compile_definitions(MixTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="JackpotMixology")
  target_link_libraries(MixTest PRIVATE juce::juce_audio_utils juce::juce_dsp JackpotFonts JackpotMixArt)
endif()
