# Local GUI connection diagnosis (2026-10-29)

The isolated `/tmp` GUI deployment could not list devices, while the existing private-prefix synchronization service remained functional. The settings UI reported a server-confirmation error when changing sync switches.

## Confirmed cause

The isolated engine host failed its read-only `network-info` probe with a missing `libc++.so.1` loader dependency. This happens before an account request can reach the server. The maintenance host does not expose libc++ through the system loader cache. Its existing private deployment includes the verified libc++/libc++abi libraries and a launcher that sets the support library search path; the isolated staging install originally omitted those additions.

Providing those same open-source dependencies from the already verified local build sysroot, with a support-directory launcher, fixed the isolated deployment. The candidate's own account tool then returned `ok=true` and five device records. Only success/count metadata was inspected; no clipboard body or identity was logged, no new pairing was created, and existing synchronization services were not restarted.

This does not establish that the published Arch package has the same missing-dependency defect: its PKGBUILD declares libc++ as a dependency. Real-device acceptance remains necessary. The Arch workflow now also invokes the installed account tool's read-only native `network-info` probe, covering dependencies loaded with `dlopen` that ordinary host `ldd` and package file checks missed.

## Related UI issues

- Single-instance activation does not replace the environment of an existing process. Reissuing a command with overrides while its old window is still open does not apply those overrides.
- The transfer window opens the real settings application, which retains the original page structure. It is not the standalone design demo and does not use a second pairing identity.
- The standalone setting disables cross-device controls. It was subsequently confirmed disabled; continuing failures were not explained by that switch.
- Local process failure was incorrectly described as an unconfirmed server change. The account wrapper now emits a structured runtime-stage failure before touching account state, and GUI errors distinguish local startup failure from an actual unconfirmed server response.
- GUI account deadlines now allow the existing 20-second identity-lock wait plus the bounded account operation, instead of killing the process at the lock-wait boundary.

The missing-dependency regression uses a synthetic failing host and proves that the account directory is not created. A successful device-list request is not a completed file transfer or a tested sync-preference change; those remain separate checks. No sync switches were changed automatically during diagnosis.

## Subsequent phone-transfer credential failure

The user subsequently confirmed bidirectional Linux/macOS file transfer in the isolated native GUI, but reported that the phone path displayed “Unable to save phone security credentials.” This is a user-reported desktop acceptance result, not a hash comparison or validation of the immutable release package on a clean host.

Code inspection found that the phone path marked a dispatch as handled **before** checking and saving the peer certificate. The same error message covered both an absent certificate and an actual local write failure. If a certificate arrived in a later response with the same dispatch, the duplicate-dispatch guard prevented recovery. No live certificate or account identity was inspected to reach this finding; the exact response that triggered the user's failure was not captured, so a delayed certificate remains a possible cause rather than a confirmed server behavior.

The fix waits for missing credentials within the existing connection deadline, rejects malformed base64 explicitly, and marks a dispatch handled only after both files have been saved. A failed write remains retryable. Certificate permissions are restricted to the owner before committing the file. The UI distinguishes waiting for a peer, an invalid response, and a local save error without displaying certificate contents. Synthetic regression coverage exercises missing and malformed credentials, a local write failure, successful recovery with the same dispatch, and suppression of a duplicate after launch.

After staging the fix from `95315f80a9431822461c5e2f476ade1950df65de`, the user confirmed Linux ↔ iPhone file transfer in both directions. Together with the earlier Linux ↔ macOS confirmation, all four explicit file-transfer directions now have user-reported acceptance in the isolated native GUI. No cross-device hash comparison, full desktop package migration, sleep/network recovery, or long-duration validation was reported. These changes do not restart the input method or clipboard service, create a new pairing, or modify the published immutable `v0.1.0-rc.1` assets.
