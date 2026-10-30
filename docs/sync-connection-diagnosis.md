# Clipboard connection diagnostics — 2026-10-30

## Observed behavior

The user reported intermittent macOS → Linux clipboard failures and later confirmed that copying and pasting worked again. A synthetic text item was observed on the server with a Mac source, then read intact from Linux through all five advertised text formats. This proves that particular receive path worked; it does not explain every earlier failure or measure Mac-copy-to-Linux-paste latency.

Before the request-cleanup change, a passive 50-second observation recorded 13 successful group-state updates. Consecutive updates were 3.80–4.01 seconds apart. The daemon sleeps three seconds after a successful cycle, so several seconds of waiting can occur even without a dropped connection. The sleep follows the request and clipboard processing; it is not a three-second end-to-end guarantee.

The separate empty-selection defect was fixed in `1f3dba8`: an explicit `wl-paste` “Nothing is copied” response now represents an empty clipboard, while actual read failures remain unreadable state. Local-content priority is preserved.

After deployment, 25 real requests completed without failures or timeouts: minimum 784 ms, median 795 ms, observed p95 916 ms, maximum 922 ms. These timings include lock wait, startup and the request, and are a short sample rather than a long-term stability result.

## Timeout cleanup

The previous `subprocess.run(..., timeout=35)` bounded the direct account-tool process, but did not guarantee termination of its shell/engine descendants. A surviving descendant could retain the shared identity lock and delay later calls. This is a confirmed cleanup defect with a synthetic reproduction, not proof that an orphan caused the user's earlier incident.

Account requests now run in a dedicated process group. On timeout, the daemon terminates that group and reaps the request before retrying. The identity lock still covers the original operation; its scope is not shortened, and no identity is copied. The 35-second request budget and existing three-second success / ten-second failure waits are retained. A full request timeout followed by the failure wait can therefore delay the next attempt by roughly 45 seconds, before its own processing time.

The regression test forks a synthetic child that holds a synthetic lock, expires the request deadline, then verifies that the lock is available. Existing clipboard ordering, retry, acknowledgement and empty-selection regressions also pass.

## Content-free health state

The daemon writes `state/sync-health.json` with owner-only permissions. It records process-lifetime request/failure/timeout counts, consecutive failures, last operation/outcome, wall-clock freshness timestamps and the last 60 request durations measured with a monotonic clock. Lock timeout (exit 73), unavailable runtime (78), process failure, invalid response and request timeout have distinct classifications.

It records no command arguments, response bodies, clipboard content, content hashes, account/device identifiers, tokens or URLs. Diagnostic write failures do not stop synchronization. Counts reset when the daemon restarts; this is bounded current health state, not a persistent event archive.

Request duration includes queueing for the identity lock, runtime startup and the server operation. It is not a separate network RTT measurement. A `group-info` success does not establish a successful upload, clipboard write or remote paste. Image download/decode time and Apple-client foreground behavior remain separate parts of end-to-end latency.

## Deployment and limits

The maintenance machine applied the reviewed changes to its existing private-prefix sync script and restarted only `wetypex-sync.service`. Fcitx and the input engine were not restarted. The prior script was retained for recovery. These source changes are newer than the immutable `0.1.0-rc.2` package; that package is not modified in place.

Short observations cannot certify sleep/wake recovery, long-duration behavior or all device directions. Future failures can be correlated with the bounded health state without asking the user to reproduce private clipboard content. Event-driven local clipboard monitoring and separating local capture from a slow remote request remain potential optimizations; neither has been introduced or claimed as validated here.
