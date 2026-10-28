# Projection Mask

A small macOS app for making projection masks for an ATEM video switcher.

Pin the four corners of a quadrilateral over your projection surface while watching a live preview on the projector: a white quad with a 1px red outline on a black background. When it lines up, export a PNG mask for the ATEM: opaque black outside and transparent inside, with no outline.

## Features

- Four independently pinned corners. Drag them, type exact pixel positions, or nudge the selected corner with the arrow keys (1px, shift for 10px). Tab cycles corners and alt-drag gives fine control
- Mask resolutions of 1280 × 720, 1920 × 1080 and 3840 × 2160. Set it to match your ATEM's video format
- An editable preview in the main window
- One output menu covering every connected display, plus any Blackmagic Desktop Video devices (UltraStudio, DeckLink, etc.) with a choice of video format
  - Display outputs are full screen and borderless at the display's native resolution, so the outline is a true 1px line. Double-click or press Esc to close
  - Blackmagic outputs send 8-bit 4:2:2 Rec.709 video
- Every output and the exported mask come from the same pixel-exact rasteriser, so what you line up is exactly what you export. Outline pixels count as inside the mask
- Save and load setups (`.pmask`). The last setup is restored automatically on launch

## Using the mask on an ATEM

In ATEM Software Control, open the **Media** page and drop the exported PNG into a still slot. Then use that media player as a key source: fill plus key, from the PNG's alpha channel. A downstream key works well for a mask that stays on top of everything.

## Installing

Runs on macOS 15 Sequoia or later, on both Apple Silicon and Intel Macs.

Download the latest `.dmg` from [Releases](../../releases), open it, and drag **Projection Mask** into Applications.

The app isn't signed with a paid Apple Developer ID, so Gatekeeper will refuse to open it on first launch. Right-click (or Control-click) the app in Applications and choose **Open**, then confirm the dialog. You only need to do this once. On macOS 15 you may instead need to go to **System Settings → Privacy & Security** and click **Open Anyway**.

Blackmagic output needs [Desktop Video](https://www.blackmagicdesign.com/support/family/capture-and-playback) installed. Without it, the app still works with displays.

## Building from source

Requires CMake 3.22+ and the Xcode command line tools. JUCE is fetched automatically. The DeckLink SDK headers are included in `ThirdParty/DeckLinkSDK` under Blackmagic's licence.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The built app is at `build/ProjectionMask_artefacts/Release/Projection Mask.app`. To build a release `.dmg`, run `scripts/make-dmg.sh`.

## Licence

MIT. See [LICENSE](LICENSE). The DeckLink SDK headers are © Blackmagic Design and carry their own licence.
