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
