# omp-dx

A high-performance, server-authoritative 2D rendering and interactive user interface system for open.mp servers and SA-MP clients. The system consists of a server-side component (omp-dx.dll) and a client-side ASI plugin (omp-dx.asi) that communicate via custom RPCs to render dynamic, responsive, and hardware-accelerated user interfaces directly within the Direct3D9 loop.

## Videos

[![Video 1](https://img.youtube.com/vi/yEzTO345jnQ/maxresdefault.jpg)](https://www.youtube.com/watch?v=yEzTO345jnQ)

[![Video 2](https://img.youtube.com/vi/lnRDpxthiMc/maxresdefault.jpg)](https://www.youtube.com/watch?v=lnRDpxthiMc)

[![Video 3](https://img.youtube.com/vi/PePrjPJONPg/maxresdefault.jpg)](https://www.youtube.com/watch?v=PePrjPJONPg)

## Repository Structure

```text
omp-dx/
├── client/                      # Client-side (ASI Plugin - C++)
│   ├── source/                  # D3D9 Hooking, WndProc handler, and widget rendering engine
│   │   ├── Plugin.cpp           # D3D9 Present Hook, WndProc input routing, and widget drawing
│   │   ├── Plugin.h             # Element definitions, font handlers, and state schemas
│   │   └── dllmain.cpp          # DLL entry point
│   ├── libs/                    # Third-party dependencies (RakHook, samp-api)
│   └── CMakeLists.txt           # CMake configuration for building the ASI plugin
│
├── server/                      # Server-side (open.mp Component & Pawn API)
│   ├── main.cpp                 # Server component entry point
│   ├── dx-component.cpp         # RPC handling and Pawn event dispatcher
│   ├── dx-renderer.cpp          # RPC serialization functions
│   ├── natives.cpp              # Native definitions for the Pawn scripting interface
│   ├── omp-dx.inc               # Pawn include file containing natives and callback forwards
│   └── CMakeLists.txt           # CMake configuration for building the open.mp component
│
└── test/                        # Pawn Test Suite (Modular Scripting API Validation)
    ├── test_dx.pwn              # Main entry script routing hooks to modules
    └── modules/                 # Modular subsystems
        ├── test_defs.inc        # State declarations and configurations
        ├── test_speedo.inc      # Speedometer HUD logic
        ├── test_dash.inc        # Diagnostic and GPS panel logic
        ├── test_widgets.inc     # Standard form widget logic
        ├── test_features.inc    # Premium visual effect widgets logic
        └── test_misc.inc        # Shape and image drawing helpers
```

---

## Installation

To install pre-built release versions of `omp-dx` without compiling from source:

### 1. Client Setup
1. Download `omp-dx.asi` from the repository releases section.
2. Place `omp-dx.asi` into your Grand Theft Auto: San Andreas game root directory.
3. Ensure you have an ASI Loader (such as `vorbisFile.dll`) in your game directory to automatically load the plugin.

### 2. Server Setup
1. Download `omp-dx.dll` from the repository releases section.
2. Place `omp-dx.dll` into your open.mp server's `components/` folder.
3. Copy `omp-dx.inc` from the `server/` directory of the source repository to your Pawn include path (e.g. `qawno/include/`).
4. Include `omp-dx` in your server scripts:
   ```pawn
   #include <omp-dx>
   ```

---

## Compilation and Deployment

### 1. Client-Side Plugin (`omp-dx.asi`)
The client component hooks the Direct3D9 device context and intercepts input window messages to render widgets and process cursor events locally.

* **Requirements:** Visual Studio 2022 (with Desktop Development with C++ workload).
* **Compilation Steps:**
  1. Open a command prompt or terminal in the `client/` directory.
  2. Generate the build files using CMake:
     ```bash
     cmake -B build -A Win32
     ```
  3. Compile the release binary:
     ```bash
     cmake --build build --config Release
     ```
  4. The compiled plugin will be located at `client/bin/Release/omp-dx.asi`.
* **Deployment:**
  * Copy `omp-dx.asi` into the Grand Theft Auto: San Andreas root directory.
  * Copy the generated `omp-dx/` folder next to the ASI. By default it contains bundled fonts in `omp-dx/fonts/`.
  * Ensure a working ASI Loader (e.g., `vorbisFile.dll`) is installed in the game client directory.

### 2. Server-Side Component (`omp-dx.dll`)
The server component integrates into the open.mp modular host, providing the Pawn API and managing network communications with clients.

* **Requirements:** CMake 3.19+, Conan 1.53+, Visual Studio 2022.
* **Compilation Steps:**
  1. Open a command prompt or terminal in the `server/` directory.
  2. Generate the build files (Win32 architecture is required as open.mp operates on a 32-bit server architecture):
     ```bash
     cmake -G "Visual Studio 17 2022" -A Win32 -B build
     ```
  3. Compile the component:
     ```bash
     cmake --build build --config Release
     ```
  4. The compiled DLL will be located at `server/build/Release/omp-dx.dll`.
* **Deployment:**
  * Copy `omp-dx.dll` to your open.mp server's `components/` directory.
  * Update the server `config.json` file to load `omp-dx` under the components/plugins list.

### 3. Pawn Integration
* Copy `server/omp-dx.inc` into your Pawn compiler's include directory (e.g., `pawno/include/` or `qawno/include/`).
* Copy the `test/` directory to your development environment.
* Open `test/test_dx.pwn` in your Pawn compiler, make sure the include files in `test/modules/` are accessible, and compile it. Load the resulting `test_dx.amx` as a filterscript in your open.mp server.

---

## Pawn Test Suite and Verification

The repository contains a highly structured, modular test suite inside the `test/` directory. This test suite validates all available visual primitives, complex widgets, dynamic assets, sound overlapping, and local drag-and-drop mechanics.

### Modular Architecture
*   **`test_dx.pwn`**: The main entry point script routing standard Pawn player callbacks (such as `OnPlayerClickDX`, `OnPlayerChangeDXSlider`, and `OnPlayerDragDX`) directly to module-specific event handlers.
*   **`modules/test_defs.inc`**: Unified header declaring macros for element coordinates, ID allocations, and player state variables (e.g., fuel level, speed limiter values, engine states).
*   **`modules/test_speedo.inc`**: Implements a real-time dial needle speedometer, fuel gauge level, seatbelt status flashing animation, and interactive control buttons (Engine, Lights, and Lock) rendering in the bottom-right corner.
*   **`modules/test_dash.inc`**: Implements an interactive GPS Map ring dashboard with compass vectors, engine temp metrics, custom input limiters, and hardware-clipping live alert feeds.
*   **`modules/test_widgets.inc`**: Implements a multi-tab settings panel demonstrating sliders, comboboxes, listviews, password fields, and radial progress synchronization.
*   **`modules/test_features.inc`**: Validates dynamic color picker spectrum arrays, scrollable containers with mousewheel clipping layers, vector line graphs, dynamic audio playbacks, and vector icons.
*   **`modules/test_misc.inc`**: Handles basic primitive render checks including geometric outlines, color gradient panels, rounded corners, custom textures, and hardware scissor-clipping validation.

### In-Game Commands
Once the `test_dx` filterscript is loaded on your open.mp server, the following commands are available to test the features:
*   `/dxspeed`: Toggles the real-time Speedometer HUD in the bottom-right corner. Use the cursor to interact with Engine, Lights, and Lock buttons.
*   `/dxshow` / `/dxhide`: Opens/closes the Vehicle Diagnostics Dashboard in the center of the screen, allowing speed limiter values to be updated via text inputs.
*   `/dxwidgets` / `/dxhidewidgets`: Opens/closes the Classic Interactive Widgets Panel for adjusting values, combobox dropdown selections, and tab transitions.
*   `/dxnew` / `/dxhidenew`: Opens/closes the Advanced Features Panel to interact with color pickers, scrollable blocks, overlapping sound clips, and vector graphs.
*   `/dximage`: Downloads and renders dynamic texture assets.
*   `/dxshapes` / `/dxshapes2` / `/dxhideall2`: Visualizes line thicknesses, polygon triangles, custom circle parameters, gradient rectangles, and rounded cards.
*   `/dxclip`: Renders clipping boundaries demonstrating scissor mask alignments.
*   `/dxhideall`: Instantly destroys and clears all active UI elements on the screen.

---

## Core Capabilities and Architecture

* **Network-Optimized Vector Rendering**: Server-directed commands for basic primitives (rectangles, rounded rectangles, gradient rectangles, circles, triangles, lines, clips, icons, and textures) are serialized and transmitted through dedicated client-server RPCs (RPC 190 and 192), minimizing bandwidth usage while enabling custom UI components.
* **Interactive UI Widgets**: Built-in stateful widgets including buttons, checkboxes, input text fields, sliders, comboboxes, listviews, tab panels, radial menus, inventory slots, color pickers, and scrollable containers.
* **Parent-Child Hierarchy**: Supports logical grouping where child widgets inherit parent transforms. When a parent element is dragged or animated, all child elements are updated client-side instantly with zero network delay and zero CPU overhead.
* **Direct3D9 Device Reset Resilience**: Handles transitions such as Alt-Tab window toggling, resolution modifications, and graphic settings adjustments without visual degradation. Direct3D9 texture surfaces are safely cleared and re-created while font references remain persistently registered in memory, preventing the common "square box" font placeholder bug.
* **Hardware-Accelerated Effects**: Supports high-performance features including multi-layered dropshadows, single-pass circular progress indicators, and pixel-shader based background blur (glassmorphism) rendered directly within the D3D9 viewport.
* **Bundled Font and Vector Asset Loading**: Supports safe runtime registration of pre-installed `.ttf`/`.otf` files from the client-side `omp-dx/fonts/` folder. Servers request a font by family name and local file name; arbitrary remote font downloads are intentionally blocked.
* **Robust Input Handler**: Windows message loop hook (WndProc) coordinates and scales mouse operations, accounting for OS DPI scaling offsets and windowed mode resolution adjustments to ensure pixel-perfect hover and click detections.
* **Integrated Audio Manager**: Asynchronously downloads, caches, and plays audio resources over HTTP using a multi-channel overlapping sound player, providing sub-second execution times and minimizing network re-downloads.
* **Network RPC Throttling**: Interactive UI transitions (such as sliders and drags) are computed locally at the client’s display rate (up to 1000Hz+), while state synchronization packets sent to the server are throttled to a maximum of 60Hz, preventing packet congestion and jitter.

---

## Scripting Reference

### Natives

#### DX_DrawRectangle
```pawn
native bool:DX_DrawRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color);
```
Renders a 2D flat rectangle on the screen.
* `playerid`: The ID of the target player.
* `elementid`: A unique ID to identify the UI element.
* `x`, `y`: Screen coordinates (top-left corner).
* `w`, `h`: Width and height of the rectangle in pixels.
* `color`: Color value in ARGB format (e.g., 0xFFFFFFFF for solid white).
* Returns `true` on success, `false` otherwise.

#### DX_DrawGradientRectangle
```pawn
native bool:DX_DrawGradientRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, colorTL, colorTR, colorBL, colorBR);
```
Renders a rectangle with a multi-directional color gradient.
* `colorTL`, `colorTR`, `colorBL`, `colorBR`: ARGB colors for the Top-Left, Top-Right, Bottom-Left, and Bottom-Right corners.

#### DX_DrawRoundedRectangle
```pawn
native bool:DX_DrawRoundedRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, Float:radius, color);
```
Renders a rectangle with rounded corners.
* `radius`: Radius of the corner rounding in pixels.

#### DX_DrawText
```pawn
native bool:DX_DrawText(playerid, elementid, const text[], Float:x, Float:y, color, Float:scale, const font[] = "");
```
Renders text at specified screen coordinates. Supports BBCode-style color formatting (e.g., `"{FF0000}Red {00FF00}Green"`).
* `text`: String to display.
* `scale`: Scaling factor for the text size.
* `font`: Optional registered font name (defaults to default UI font).

#### DX_DrawButton
```pawn
native bool:DX_DrawButton(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:scale, const text[], const font[] = "");
```
Creates an interactive button that triggers `OnPlayerClickDX` when clicked. Automatically manages hover color transition effects.

#### DX_DrawCheckbox
```pawn
native bool:DX_DrawCheckbox(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, bool:checked, Float:scale, const label[], const font[] = "");
```
Creates a stateful checkbox that triggers `OnPlayerToggleDXCheckbox`.
* `checked`: Initial selection state.

#### DX_DrawInput
```pawn
native bool:DX_DrawInput(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:scale, const defaultText[], const placeholder[], const font[] = "");
```
Creates a text input field. Triggers `OnPlayerDXInputSubmit` when the player presses Enter.
* `defaultText`: Initial text content.
* `placeholder`: Text displayed when the field is empty.

#### DX_DrawSlider
```pawn
native bool:DX_DrawSlider(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:value, const font[] = "");
```
Renders an interactive horizontal slider. Triggers `OnPlayerChangeDXSlider` on modification.
* `value`: Initial value ratio between `0.0` (0%) and `1.0` (100%).

#### DX_DrawComboBox
```pawn
native bool:DX_DrawComboBox(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const options[], const font[] = "");
```
Creates a dropdown menu selection widget. Triggers `OnPlayerSelectDXComboBox`.
* `selectedIndex`: Index of the option selected by default (0-indexed).
* `options`: Options separated by commas (e.g., `"Option 1,Option 2,Option 3"`).

#### DX_DrawListView
```pawn
native bool:DX_DrawListView(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const items[], const font[] = "");
```
Renders a scrollable vertical selection list. Triggers `OnPlayerSelectDXListView`.
* `items`: Item values separated by commas.

#### DX_DrawTabPanel
```pawn
native bool:DX_DrawTabPanel(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const tabs[], const font[] = "");
```
Renders an interactive horizontal tab selector. Triggers `OnPlayerSelectDXTab`.
* `tabs`: Comma-separated tab labels.

#### DX_DrawImage
```pawn
native bool:DX_DrawImage(playerid, elementid, const url[], Float:x, Float:y, Float:w, Float:h, color = 0xFFFFFFFF);
```
Downloads asynchronously over HTTP, caches, and renders a 2D image.
* `url`: Direct path to the image asset (PNG/JPG).

#### DX_DrawIcon
```pawn
native bool:DX_DrawIcon(playerid, elementid, Float:x, Float:y, Float:size, const iconName[], color, const font[] = "FontAwesome");
```
Renders a crisp vector icon from a loaded font library.
* `iconName`: The name of the icon code inside the specified font family (e.g., `"heart"`).
* `font`: The font family name. Must be preloaded via `DX_LoadFont`. Defaults to `"FontAwesome"`.

#### DX_DrawLine
```pawn
native bool:DX_DrawLine(playerid, elementid, Float:x1, Float:y1, Float:x2, Float:y2, Float:thickness, color);
```
Draws a 2D line between two screen points.

#### DX_DrawCircle
```pawn
native bool:DX_DrawCircle(playerid, elementid, Float:x, Float:y, Float:radius, color, Float:thickness = 0.0);
```
Draws a circle. If `thickness` is greater than `0.0`, an outlined circle is drawn instead of a filled one.

#### DX_DrawTriangle
```pawn
native bool:DX_DrawTriangle(playerid, elementid, Float:x1, Float:y1, Float:x2, Float:y2, Float:x3, Float:y3, color);
```
Draws a filled polygon triangle using hardware vertices.

#### DX_DrawCircularProgress
```pawn
native bool:DX_DrawCircularProgress(playerid, elementid, Float:x, Float:y, Float:radius, Float:progress, color, Float:thickness);
```
Renders a circular speedometer or ring indicator.
* `progress`: Value ratio between `0.0` (0%) and `1.0` (100%).

#### DX_DrawShadow
```pawn
native bool:DX_DrawShadow(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:size = 8.0, Float:offset = 4.0);
```
Applies a hardware-optimized gradient-layered box shadow around a rectangular area.

#### DX_DrawGraph
```pawn
native bool:DX_DrawGraph(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, const Float:values[], numValues, Float:maxVal);
```
Draws a real-time line graph plotting an array of floating-point values.

#### DX_DrawInventorySlot
```pawn
native bool:DX_DrawInventorySlot(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, const iconUrl[], const label[], amount);
```
Creates an inventory slot containing an image icon, label, and numerical counter. Supports item dragging and swapping.

#### DX_DrawTexturedProgressBar
```pawn
native bool:DX_DrawTexturedProgressBar(playerid, elementid, Float:x, Float:y, Float:w, Float:h, const bgTextureUrl[], const fillTextureUrl[], Float:progress, color = 0xFFFFFFFF);
```
Renders a progress bar utilizing customized texture URLs for both its background and foreground layers.

#### DX_DrawRadialMenu
```pawn
native bool:DX_DrawRadialMenu(playerid, elementid, Float:x, Float:y, Float:radius, color, selectedIndex, const items[], const icons[]);
```
Renders an interactive circular pie menu. Triggers `OnPlayerSelectRadialItem`.
* `items`: Comma-separated labels.
* `icons`: Comma-separated asset URLs.

#### DX_DrawScrollContainer
```pawn
native bool:DX_DrawScrollContainer(playerid, elementid, Float:x, Float:y, Float:w, Float:h, Float:contentHeight, Float:scrollVal, color);
```
Renders a container with an interactive vertical scroll bar. Triggers `OnPlayerScrollDXContainer`.
* `contentHeight`: Real vertical height of the scroll content in pixels.
* `scrollVal`: Current vertical scroll position percentage from `0.0` to `1.0`.

#### DX_DrawColorPicker
```pawn
native bool:DX_DrawColorPicker(playerid, elementid, Float:x, Float:y, Float:w, Float:h, selectedColor);
```
Renders an interactive RGB gradient color picker spectrum. Triggers `OnPlayerSelectDXColor`.

#### DX_Destroy
```pawn
native bool:DX_Destroy(playerid, elementid);
```
Removes a specific UI element for a player.

#### DX_ClearAll
```pawn
native bool:DX_ClearAll(playerid);
```
Removes all active UI elements for a player.

#### DX_LoadFont
```pawn
native bool:DX_LoadFont(playerid, const fontFamily[], const fileName[]);
```
Registers a `.ttf` or `.otf` font. The server first looks for `fileName` in `fonts/`, `omp-dx/fonts/`, then `components/omp-dx/fonts/` on the server and transfers it to the client cache with size/checksum validation. If it is not present server-side, the client falls back to its bundled `omp-dx/fonts/` folder. `FontAwesome`, `Outfit`, `Poppins`, and `JetBrains Mono` are bundled and loaded by default.
* `fontFamily`: Family name used later in draw calls (e.g., `"Outfit"`).
* `fileName`: Safe file name only, such as `"Outfit.ttf"`. Paths, `..`, URLs, and unsupported extensions are rejected.
* Security: the client only accepts fonts whose SHA-256, family, and file name are listed in `omp-dx/fonts/font-allowlist.txt`.

Example:
```pawn
DX_LoadFont(playerid, "Outfit", "Outfit.ttf");
DX_DrawText(playerid, 10, "Hello", 320.0, 180.0, -1, 1.0, "Outfit");
```

For server-provided fonts, place the file on the server, for example:
```text
open.mp server/
  fonts/
    MyFont.ttf
```

Then call:
```pawn
DX_LoadFont(playerid, "MyFont", "MyFont.ttf");
```

The client allowlist format is:
```text
SHA256|FontFamily|FileName
```

For example:
```text
FC7287273E66929776E2BA54F144FE699080BEC29F61BF649D70D871468AEADE|Outfit|Outfit.ttf
```

If a server needs a new font, add the font file to the server, compute its SHA-256, add the matching line to `client/omp-dx/fonts/font-allowlist.txt`, and ship the updated `omp-dx/fonts/font-allowlist.txt` with the client plugin. Fonts missing from this manifest are rejected by default; users can request new entries through an issue.

#### DX_PlaySound
```pawn
native bool:DX_PlaySound(playerid, const url[]);
```
Downloads, locally caches, and plays a dynamic multichannel overlapping sound.
* `url`: Direct HTTP link to the `.mp3` or `.wav` audio track.

#### DX_SetDraggable
```pawn
native bool:DX_SetDraggable(playerid, elementid, bool:draggable);
```
Enables or disables local drag interaction. Triggers `OnPlayerDragDX` callback upon coordinate modifications.

#### DX_SetParent
```pawn
native bool:DX_SetParent(playerid, elementid, parentid);
```
Links an element as a child of another. Child elements move relative to their parents instantly on the client side.

#### DX_SetTooltip
```pawn
native bool:DX_SetTooltip(playerid, elementid, const tooltipText[]);
```
Binds an automated hover popup tooltip message to a specific UI widget.

#### DX_SetInputPassword
```pawn
native bool:DX_SetInputPassword(playerid, elementid, bool:enable);
```
Toggles password mode on input boxes, masking letters locally as asterisks while maintaining text integrity in server sync calls.

#### DX_SetBlurBehind
```pawn
native bool:DX_SetBlurBehind(playerid, elementid, bool:enable);
```
Enables pixel-shader based glassmorphism background blurring inside the boundaries of a given element.

#### DX_SetClipArea
```pawn
native bool:DX_SetClipArea(playerid, elementid, Float:x, Float:y, Float:w, Float:h);
```
Defines a bounded rectangular clipping viewport limiting child render pipelines.

#### DX_Animate
```pawn
native bool:DX_Animate(playerid, elementid, Float:targetX, Float:targetY, Float:targetW, Float:targetH, Float:targetAlpha, durationMs, easingType);
```
Triggers a hardware-interpolated transition for position, size, and transparency.
* `easingType`: Interpolation curve calculation code (0 = Linear, 1 = EaseIn, 2 = EaseOut, 3 = EaseInOut).

#### Getters
```pawn
native bool:DX_GetScreenSize(playerid, &Float:width, &Float:height);
native bool:DX_GetCheckboxState(playerid, elementid, &bool:checked);
native bool:DX_GetInputText(playerid, elementid, text[], len = sizeof(text));
native bool:DX_GetSliderValue(playerid, elementid, &Float:value);
native bool:DX_GetComboBoxIndex(playerid, elementid, &index);
native bool:DX_GetListViewIndex(playerid, elementid, &index);
native bool:DX_GetTabActive(playerid, elementid, &index);
native bool:DX_GetColorPickerColor(playerid, elementid, &color);
native bool:DX_GetScrollContainerVal(playerid, elementid, &Float:value);
native bool:DX_IsReady(playerid);
```
Various API routines to query current state metrics and player readiness securely on the server side.

---

### Callbacks

#### OnPlayerClickDX
```pawn
forward OnPlayerClickDX(playerid, elementid);
```
Triggered when a player clicks a clickable widget (e.g., button, image, rectangle).

#### OnPlayerToggleDXCheckbox
```pawn
forward OnPlayerToggleDXCheckbox(playerid, elementid, bool:checked);
```
Triggered when a player toggles the state of a checkbox.

#### OnPlayerDXInputSubmit
```pawn
forward OnPlayerDXInputSubmit(playerid, elementid, const text[]);
```
Triggered when a player submits text inside an input field by pressing Enter.

#### OnPlayerChangeDXSlider
```pawn
forward OnPlayerChangeDXSlider(playerid, elementid, Float:value);
```
Triggered when a player changes the position of a slider.

#### OnPlayerSelectDXComboBox
```pawn
forward OnPlayerSelectDXComboBox(playerid, elementid, index);
```
Triggered when a player selects a dropdown option from a ComboBox.

#### OnPlayerSelectDXListView
```pawn
forward OnPlayerSelectDXListView(playerid, elementid, index);
```
Triggered when a player clicks a list item inside a ListView.

#### OnPlayerSelectDXTab
```pawn
forward OnPlayerSelectDXTab(playerid, elementid, index);
```
Triggered when a player selects a tab from a TabPanel.

#### OnPlayerDragDX
```pawn
forward OnPlayerDragDX(playerid, elementid, Float:x, Float:y);
```
Triggered when a player drags an element designated as draggable.

#### OnPlayerSwapDXSlots
```pawn
forward OnPlayerSwapDXSlots(playerid, sourceElementid, targetElementid);
```
Triggered when a player drags and drops one inventory slot element over another.

#### OnPlayerSelectRadialItem
```pawn
forward OnPlayerSelectRadialItem(playerid, elementid, index);
```
Triggered when a player selects a segment in a Radial Menu.

#### OnPlayerSelectDXColor
```pawn
forward OnPlayerSelectDXColor(playerid, elementid, color);
```
Triggered when a player selects a color on a Color Picker spectrum.

#### OnPlayerScrollDXContainer
```pawn
forward OnPlayerScrollDXContainer(playerid, elementid, Float:scrollVal);
```
Triggered when a player scrolls within a Scroll Container.

#### OnPlayerDXReady
```pawn
forward OnPlayerDXReady(playerid);
```
Triggered when the player's client-side DirectX eklenti (ASI plugin) has successfully initialized, hooked the D3D9 renderer, and is ready to draw custom elements.

