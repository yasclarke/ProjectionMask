cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

project(ProjectionMask VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(ProjectionMask
    COMPANY_NAME     "Yaseen Clarke"
    PRODUCT_NAME     "Projection Mask"
    BUNDLE_ID        "com.yaseenclarke.projectionmask"
    PLIST_TO_MERGE   "<plist><dict><key>LSMinimumSystemVersion</key><string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string></dict></plist>"
)

target_sources(ProjectionMask PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MaskModel.cpp
    Source/MaskRasterizer.cpp
    Source/CornerEditor.cpp
    Source/DisplayOutputWindow.cpp
    Source/WindowLevel.mm
    Source/DeckLinkOutput.cpp
    # Loads DeckLinkAPI.framework (installed by Desktop Video) at runtime
    ThirdParty/DeckLinkSDK/DeckLinkAPIDispatch.cpp
)

target_include_directories(ProjectionMask PRIVATE Source)
# Vendored SDK: treat as a system include so its warnings stay quiet
target_include_directories(ProjectionMask SYSTEM PRIVATE ThirdParty/DeckLinkSDK)
set_source_files_properties(ThirdParty/DeckLinkSDK/DeckLinkAPIDispatch.cpp PROPERTIES COMPILE_OPTIONS "-w")

target_compile_definitions(ProjectionMask PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="Projection Mask"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(ProjectionMask
    PRIVATE
        juce::juce_gui_extra
        "-framework CoreFoundation"
        "-framework AppKit"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
