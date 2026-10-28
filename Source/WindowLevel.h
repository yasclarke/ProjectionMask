#pragma once

// Raises a borderless output window above the menu bar and Dock on its
// display (JUCE's always-on-top level sits below the menu bar on macOS).
void raiseWindowAboveMenuBar (void* nativeViewHandle);
