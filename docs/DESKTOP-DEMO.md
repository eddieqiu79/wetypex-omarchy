# Desktop interaction demo

This is an isolated, executable Qt Widgets prototype, not a replacement settings application or an installed transfer service. It is intended to evaluate the workflow before connecting it to the existing backend. Its Chinese interface targets the current usability review; documentation is in English.

## Run from source

On a development machine with CMake, a C++17 compiler, and Qt6 Widgets/Test development files already available:

```sh
cmake -S demo/desktop -B build/desktop-demo
cmake --build build/desktop-demo --parallel 2
ctest --test-dir build/desktop-demo --output-on-failure
./build/desktop-demo/wetypex-desktop-demo
```

There is no install target. Building the demo does not build the official engine bridge, download dependencies, obtain runtime assets, or change Fcitx. If dependencies are missing, report them before installing anything.

The window uses the desktop file name / Wayland app ID `org.wetypex.Omarchy.Demo`. It uses a normal decorated, resizable window rather than a frameless fixed-size imitation of another desktop. It does not force floating, position itself, change Hyprland rules, or request always-on-top behavior. The revised visual design references the existing settings interface: grouped cards, a distinct sidebar, green selection/actions, and label/control rows. These are drawn with Qt; no proprietary icons, extracted backgrounds or official UI resources are loaded. The demo uses the user-specified `#24C891` accent and `#282827` main background, with neutral dark surfaces for forms, menus and scrollbars. Text on the green accent is dark for legibility. Candidate previews use the same main dark background. This is local to the demo, not automatic Omarchy theme integration or a change to the installed input method.

For a headless visual preview:

```sh
QT_QPA_PLATFORM=offscreen ./build/desktop-demo/wetypex-desktop-demo --screenshot /tmp/wetypex-demo.png
```

An offscreen render does not establish real Hyprland placement, focus, scaling, or input behavior.

## Try these workflows

1. Open **File transfer** (文件传输). Select a synthetic Mac/iPhone device card. Use **Try sample files** (用示例试一下) or choose/drop local files. The demo shows names, types through filename extensions, byte sizes and a total; selecting or dropping files never sends them. Remove individual selections or clear the list. Folders and non-local URLs are ignored, duplicate canonical paths are deduplicated, and the selection is limited to 100 files.
2. Send becomes enabled only when both device and files are selected. It creates one simulated invitation with the complete file list and total size. Expand **Demo tools** (演示工具) and simulate peer acceptance. The task progresses through connecting and simulated transfer; completion explicitly says no transfer occurred.
3. Select a task card to cancel an active task or retry a failed/rejected/cancelled one. Simulate a connection failure from Demo tools. Cancellation must remain terminal until an explicit retry. Completed cards move into the expandable recent history. Failed tasks remain visible for recovery.
4. Use **Simulate incoming iPhone file** in Demo tools. The selected invitation shows its origin, size and proposed destination, with accept/reject actions. No destination is created. Completed incoming simulations deliberately offer no fake open-file/open-folder action because nothing was saved. Independent task states and the real backend's cancellation/completion semantics remain distinct.
5. Use **Input** to select pinyin/double pinyin/Wubi and their relevant schemes, smart spelling, all 19 fuzzy-pinyin pairs, punctuation, default language, emoji and standalone mode. Apply to demo memory or revert unapplied changes. The four requested fuzzy pairs are demo presets. No live engine configuration is read or written, and no independent correction-strength or continuous-pinyin control is invented.
6. Inspect **Voice**, **Common phrases and clipboard**, **Appearance** and **Shortcuts**. Voice uses a placeholder microphone and does not record. Phrase editing operates on synthetic in-memory records. Candidate size, label style, count and orientation affect a synthetic preview. Keyboard switches do not register global shortcuts. Pages retain independent in-memory apply/revert snapshots.
7. **Cross-device** retains the actual clipboard/dictionary/phrase categories, plus synthetic devices. Pairing and immediate-sync actions are disabled because they are not connected. Text and image synchronization are no longer represented as two existing server flags. The demo's update button stays disabled to preserve network isolation; the existing production settings source has a separate explicit release checker. See the [source-backed content inventory](SETTINGS-CONTENT.md) for mappings and limitations.

Keyboard navigation: `Ctrl+1` through `Ctrl+8` switch pages (transfer, input, voice, phrases, appearance, shortcuts, cross-device, updates); `Ctrl+O` opens a file chooser; `Ctrl+Enter` creates a simulated send task on the transfer page; `Tab` / `Shift+Tab` traverse controls; `Space` activates a focused button or checkbox; `Esc` closes the demo. `--page 2` opens the input page directly. The file chooser and drop handler read only selected-file metadata (name, size and canonical path for in-memory deduplication): no file contents are read, uploaded, or persisted by the demo. Choosing a file alone never creates a transfer task.

All settings, selected-file metadata and simulated tasks remain in process memory. Closing the window discards them and ends simulation. The application does not access the clipboard, account directories, identity locks, control sockets, network, backend executables or service manager. A desktop-provided file chooser may maintain its own recent-location state; that is separate from the demo model.

## Findings behind the prototype

The reviewed baseline is `92b35d3c01667a26ef1bb4d10546333a7178d033`:

- Settings use a fixed 715×569 frameless window and custom drag controls (`src/settings/original_windows.hpp`). The transfer window is also frameless. The prototype delegates window management to the compositor and uses layouts.
- The transfer entry labelled as device association actually selects an existing peer, then establishes a session before asking for files (`src/transfer/main.cpp`). The prototype makes file and target selection explicit before the invitation.
- Multiple transfer events update one shared status/progress display. The prototype gives each task a state and actions; it does not claim the real backend already supports its model.
- Closing the real transfer window shuts down its owned session. The prototype clearly states its lifetime. Moving real jobs into a persistent service requires a separate session-ownership change and device tests.
- Some settings operations block the GUI while waiting for engine startup or process output. The future adapter must perform these requests asynchronously, report timeouts and restore editable state after failure.
- The inherited updater selected original upstream binaries. The accompanying source change disables that route and explains manual fork-package updates; see [UPDATES.md](UPDATES.md). It does not change the updater already installed on this machine.

## Before connecting real services

Keep the existing protocol implementation and canonical identity. Extract an asynchronous adapter around its requests/events, with stable task IDs and explicit invitation, connection, transfer, verification and saved-result states. Show success only after the backend confirms complete output, and show the actual destination. Do not infer progress or completion from an invitation acknowledgement.

Real cancellation must stop the corresponding transport and prevent late callbacks from reviving a cancelled task; retry must create a fresh protocol attempt. Incoming invitations need validated metadata, a user acceptance decision and safe destination handling. Do not automatically upload copied file URIs. No file-clipboard support is introduced by this demo.

Further work includes single-instance activation, handling closure with live transfers, asynchronous capability/status discovery, transactional settings writes with errors, theme integration, large-text/scaling/accessibility checks, notifications, and package upgrade/rollback validation. Real keyboard/compositor behavior, three-device interoperability and failure recovery remain separate acceptance layers.

## Local validation of this draft

On 2026-10-29, with Qt 6.11.2 and CMake 4.4.3, the standalone build succeeded in `/tmp/wetypex-desktop-demo-build`. Its expanded QtTest suite passed nine behavioral test slots (plus setup/cleanup) using the offscreen platform. The suite covers invitation gating, terminal cancellation, retry reset, invalid actions, independent jobs, eight-page keyboard navigation, selection-dependent actions, mode-specific settings, in-memory apply/revert, nineteen fuzzy pairs, phrase deletion/revert, candidate previews, multi-file selection, local-URL drops, deduplication and explicit-send gating. Offscreen previews were rendered and visually inspected.

The demo was also launched using `QT_QPA_PLATFORM=wayland`. Hyprland reported `org.wetypex.Omarchy.Demo` as mapped, visible and not using XWayland. This establishes native window launch only; it is not a completed manual usability, scaling or device-interoperability acceptance test.

The updater policy tests, existing offline sync regression and setup lifecycle regression passed. The workflow includes the demo build/tests and release/update policy tests; consult the Actions run for the exact commit for CI results. The live input method, clipboard services and pairing were not replaced or restarted.
