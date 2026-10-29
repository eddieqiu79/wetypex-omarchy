# Settings content and implementation inventory

This inventory supports the desktop demo's **configurable content**, not only its visual style. It was reviewed on 2026-10-29 against repository baseline `92b35d3c01667a26ef1bb4d10546333a7178d033` plus the local updater-policy draft. The reference is the existing WeTypeX settings implementation in this repository, not a claim that every current official Mac/iPhone option has been reproduced.

No account, clipboard, personal settings or device state was read for this review. Source wiring is not a behavioral or end-to-end acceptance result.

## Evidence levels

- **UI/schema:** an option is presented by the existing custom settings window or Fcitx configuration schema.
- **Consumer:** source reads the option and uses it in an engine request, candidate presentation, keyboard handler, synchronization command or another operation.
- **Runtime validation:** its effects have been checked with the actual runtime and affected applications/devices. This inventory does not establish this level for every option.
- **Demo:** an interactive control changes memory or a synthetic preview only. It is not connected to the live configuration or engine.

The custom window writes `wetypex.json` and requests `ReloadAddonConfig` in [`src/settings/main.cpp`](../src/settings/main.cpp). The Fcitx-facing schema is [`src/plugin/config.hpp`](../src/plugin/config.hpp); import/export and consumers are in [`src/plugin/wetype.cpp`](../src/plugin/wetype.cpp). A future real settings adapter must preserve the existing mapping and report write/reload failures.

## Content to retain

| Existing section | Configurable content | Implementation evidence and demo treatment |
| --- | --- | --- |
| Input method | Pinyin, double pinyin, Wubi; seven double-pinyin schemes (Ziranma, Sogou, Microsoft, Xiaohe, Pinyin Jiajia, Ziguang, Smart ABC); Wubi 86/98/New Century | `input_mode`, `double_pinyin_scheme`, `wubi_solution` are imported/exported and mapped into engine requests. Only show relevant scheme controls for the selected input method. |
| Wubi options | Pinyin mixing, unique-candidate commit, commit on next code, wildcard hints | `wubi_pinyin`, `wubi_unique_commit`, `wubi_next_commit`, `wubi_wildcard_comment` are forwarded to the bridge and written into its session configuration. Do not imply effects have been tested for every scheme. |
| Smart spelling | One enabled/disabled setting | `smart_input` is consumed by the engine bridge. Preserve the actual switch; do not invent correction-strength or continuous-pinyin sliders. See the dedicated explanation below. |
| Emoji and expression recommendations | WeChat expressions, Emoji, kaomoji, large expressions, symbol expressions | The custom UI has five switches. The schema also has the `emoji_recommend` master flag. The plugin gates WeChat expressions and large expressions by the target program. A generic demo preview must not imply every application supports them. |
| Punctuation | Slash/question key mapping to the Chinese enumeration comma; numeric punctuation conversion; paired punctuation completion | `slash_punctuation`, `symbol_auto_change`, `symbol_auto_pair` have plugin consumers. Preserve individual switches and explanations. |
| Default input language | Chinese or English | `default_language` is read when setting an input context's language on activation (`wetype.cpp`, around line 2096). It is not a language-learning preference or a global system-language selector. |
| Fuzzy pinyin | Nineteen independent pairs | Retain the complete list below. The demo may start with the user's requested four-pair preset, clearly labelled as a demo preset rather than a readout of the actual engine configuration. |
| Standalone mode | Disable network-dependent functionality | Existing settings disable pairing/transfer/sync controls when standalone is set. The plugin derives `networkEnabled_` from `standalone`. Coordinating the independent sync service remains a separate integration responsibility; do not promise that a demo switch stops all real networking. |
| Voice | Start/hold shortcut enablement, microphone selection, four punctuation modes, smart polish | `voice_stream.py` consumes microphone, smart-polish and punctuation settings. The custom UI enumerates PipeWire microphones, which the isolated demo must not do. Offer an explicitly synthetic/default microphone entry. Voice is not evaluated merely by changing its controls. |
| Common phrases | List, add, edit and remove a phrase with its shortcut | Custom settings call engine `hotword_list` and `hotword_set`. The demo should use only synthetic in-memory records and reject empty records rather than touching the real phrase store. |
| Input-method clipboard | Display copied text inside the input method's clipboard history | `clipboard_enabled` is used by `recordClipboard` in the plugin. This is a local-history setting, distinct from cross-device clipboard synchronization. Demo history must remain synthetic. |
| Appearance | Candidate font size 10–18; system/light/dark candidate theme; client/candidate preedit placement; WeType/Fcitx candidate-number style | Custom UI and schema expose these choices. Real theme application updates Fcitx classic UI configuration; demo changes should affect a synthetic candidate preview only. |
| Additional Fcitx appearance options | Candidate count 3–9 and horizontal/vertical layout | Present in `config.hpp` and consumed by candidate lists in `wetype.cpp` (`setPageSize`, `setLayoutHint`), even though the original custom settings window does not expose both. Label these as Linux/Fcitx additions rather than official Mac parity. |
| Keyboard shortcuts | Language switching, AI/V mode, voice start/hold, full/half width, punctuation, simplified/traditional, page navigation, second/third candidate selection | Original custom window mostly enables predefined shortcuts. The Fcitx schema also exposes configurable key lists. Use the actual schema defaults, not decorative key labels from the old UI; avoid claiming conflict detection until Hyprland/Fcitx conflict inspection exists. |
| Cross-device | Clipboard synchronization, personal dictionary synchronization, common phrase synchronization, request synchronization now | Three independent server mask bits exist: clipboard = 1, phrase = 2, dictionary = 4. Preserve unrelated bits when updating them. See limitations below. |
| Device management | Paired device names/version, association workflow, local association code, file-transfer entry | Source uses sync-state/account operations. Demo devices must be explicitly synthetic; paired is not synonymous with currently reachable. Pair/unpair controls must not fabricate real identities or codes. |
| File transfer | Choose peer, choose files, display invitation/connection/transfer outcome | The real transfer process owns the transport session. The demo can model actions and failures but does not establish real cancellation, background reception or file verification. |
| Mobile client | Links to the official mobile download page | This is information/navigation, not an input setting. The isolated demo need not open a browser or download anything. |
| Version and feedback | Adapter version, runtime boundary, fork update source and feedback destination | The local updater-policy draft replaces the inherited upstream installer with manual fork-package guidance. Automatic updating is not currently available. Do not present the legacy `auto_update` schema flag as an effective updater. |

## Smart spelling, correction and continuous pinyin

The source has a concrete correction-related path:

1. The settings window writes `smart_input`.
2. The plugin reads it and includes it in outgoing engine requests (`wetype.cpp`, around lines 504 and 1730).
3. [`src/backend/service.hpp`](../src/backend/service.hpp), around line 435, enables session configuration bytes `0x08` through `0x0c` when it is on. The source comment identifies transpose, neighbouring-key, insertion and skip correction.

This is evidence of a consumed control, not proof that the reverse-engineered mapping behaves exactly like every official release. No separate continuous-pinyin switch, correction-strength control, or candidate-ranking slider was found in these settings paths. Continuous phrase composition and candidate quality should be evaluated with representative typing sequences against the actual runtime. Do not add plausible-looking controls with no backend mapping.

## Complete fuzzy-pinyin list

The existing UI and bridge map all of the following pairs:

| Display | Configuration key |
| --- | --- |
| n/l | `fuzzy_nl` |
| r/l | `fuzzy_rl` |
| h/f | `fuzzy_hf` |
| g/k | `fuzzy_gk` |
| an/ang | `fuzzy_an_ang` |
| ian/iang | `fuzzy_ian_iang` |
| uan/uang | `fuzzy_uan_uang` |
| c/ch | `fuzzy_c_ch` |
| s/sh | `fuzzy_s_sh` |
| z/zh | `fuzzy_z_zh` |
| hui/fei | `fuzzy_hui_fei` |
| en/eng | `fuzzy_en_eng` |
| in/ing | `fuzzy_in_ing` |
| on/ong | `fuzzy_on_ong` |
| huang/wang | `fuzzy_huang_wang` |
| un/ong | `fuzzy_un_ong` |
| un/iong | `fuzzy_un_iong` |
| an/ai | `fuzzy_an_ai` |
| eng/ong | `fuzzy_eng_ong` |

All are false in the generic schema. The user's requested `z/zh`, `c/ch`, `s/sh`, `in/ing` combination is a specific preference, not an upstream default.

## Shortcut defaults and grouping

Use `config.hpp` as the source of truth for actual default bindings:

- Language switch: Shift enabled; Ctrl optional. Custom language-switch key lists also exist.
- AI assistant: `=`; V mode: `v`.
- Voice start: `Control+Super+Shift_L`; hold: `Control+Super_L`.
- Full/half width: `Shift+Space`; punctuation: `Control+Period`; simplified/traditional: `Control+Shift+F`.
- Candidate paging: minus/equal and brackets enabled by default; comma/period and Shift+Tab/Tab optional. Additional previous/next keys are supported by the schema.
- Second/third candidate: semicolon/quote or left/right Ctrl are optional; custom key lists are also present.

The old custom window has ambiguous decorative key labels (including a backslash icon where the schema says Space). Do not copy those labels as authoritative bindings. Whether a shortcut reaches Fcitx depends on desktop and application bindings; no new global Hyprland bindings should be installed by the demo.

## Cross-device and file settings boundaries

`src/settings/main.cpp` and `applyDeviceFunctions()` in the plugin use the server mask described above. `scripts/fcitx5-wetypex-syncd.sh.in` gates clipboard synchronization with bit 1. The reviewed implementation does **not** expose separate server flags for text and image sync. Separate local text/image controls would require a deliberate local policy implementation; they should not be represented as already implemented backend switches.

The current receive destination is selected by `WETYPE_TRANSFER_DOWNLOAD_DIR`, falling back to the desktop download directory plus `/WeTypeX` (`src/transfer/main.cpp`, `startSession`, around line 445). It is passed to the child transport as `WETYPE_FLURRY_DOWNLOAD_DIR`. A persistent JSON receive-directory setting is not currently wired into this path. A demo directory selector is therefore a proposed UI/adapter addition until integrated and tested; it must not silently suggest that the live destination changed.

The existing cross-device settings have a failure-handling gap: local flags are written before the remote request, while a failed request restores controls from the initial snapshot without necessarily restoring local state. A production replacement needs transactional state, current capability/status refresh, and explicit pending/failed/successful application states.

## Demo acceptance scope

The expanded demo should retain the option inventory, mode-dependent enablement, apply/revert behavior, linked values in repeated views, synthetic phrase CRUD, candidate preview, and the existing simulated transfer flow. Expose limitations next to unsupported or proposed controls. Verify these UI behaviors without loading the official engine, live settings, microphone devices, account state or clipboard.

Replacing a sparse prototype with a richer settings surface does not make the controls live. Backend integration, actual engine effects, service coordination, global-shortcut conflicts and each official-device direction remain distinct validation work.
