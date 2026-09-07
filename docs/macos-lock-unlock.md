# Optional: sleep / wake on Mac screen lock

Not required to use Tiny Engineer. Extra macOS companion: lock the screen → robot `sleep`, unlock → `wakeup`. Primary integrations stay REST and Cursor ([`integration.md`](integration.md), [`api.md`](api.md)).

Robot must already be on the same Wi-Fi. Base URL: `http://tiny-engineer.local`.

## Overview

A small Swift CLI listens for macOS screen lock / unlock and POSTs `/anim`:

| Event | Request |
| --- | --- |
| Screen locked | `curl -X POST "http://tiny-engineer.local/anim?name=sleep"` |
| Screen unlocked | `curl -X POST "http://tiny-engineer.local/anim?name=wakeup"` |

Verified responses look like:

```json
{"ok":true,"animation":"sleep"}
{"ok":true,"animation":"wakeup"}
```

## How it works

The program observes these macOS **distributed notifications**:

- `com.apple.screenIsLocked`
- `com.apple.screenIsUnlocked`

On each event it runs `/usr/bin/curl` via Foundation `Process`, then stays alive on `RunLoop.main`.

A user LaunchAgent (`RunAtLoad` + `KeepAlive`) starts it after login.

These notification names are commonly used for this purpose but are **not a formally documented public Apple API**. Treat them as an implementation detail that could change in a future macOS version.

## Files

| Path | Role |
| --- | --- |
| `~/screenwatch/screenwatch.swift` | Source |
| `~/screenwatch/screenwatch` | Compiled binary |
| `~/Library/LaunchAgents/local.screenwatch.plist` | LaunchAgent |

`launchd` does **not** expand `~`. The plist `ProgramArguments` must be the **absolute** path to the compiled binary (for example the output of `echo "$HOME/screenwatch/screenwatch"`). Do not put `~` in the plist.

## Installation

```bash
mkdir -p ~/screenwatch
```

Save this as `~/screenwatch/screenwatch.swift`:

```swift
import Darwin
import Foundation

setbuf(stdout, nil)
setbuf(stderr, nil)

func postAnim(name: String) {
    let process = Process()
    process.executableURL = URL(fileURLWithPath: "/usr/bin/curl")
    process.arguments = [
        "-X", "POST",
        "http://tiny-engineer.local/anim?name=\(name)",
    ]
    do {
        try process.run()
        process.waitUntilExit()
        print("posted \(name) status=\(process.terminationStatus)")
        fflush(stdout)
    } catch {
        fputs("curl failed: \(error)\n", stderr)
        fflush(stderr)
    }
}

let center = DistributedNotificationCenter.default()

center.addObserver(
    forName: Notification.Name("com.apple.screenIsLocked"),
    object: nil,
    queue: nil
) { _ in
    print("screen locked")
    fflush(stdout)
    postAnim(name: "sleep")
}

center.addObserver(
    forName: Notification.Name("com.apple.screenIsUnlocked"),
    object: nil,
    queue: nil
) { _ in
    print("screen unlocked")
    fflush(stdout)
    postAnim(name: "wakeup")
}

print("screenwatch listening for lock/unlock")
fflush(stdout)
RunLoop.main.run()
```

Compile:

```bash
swiftc -o ~/screenwatch/screenwatch ~/screenwatch/screenwatch.swift
chmod +x ~/screenwatch/screenwatch
```

To hide curl’s response body, add `-s` `-o` `/dev/null` to `process.arguments` (before `-X`). Keep them off until you have confirmed the endpoints.

## Testing

Confirm the robot answers **before** installing a LaunchAgent:

```bash
curl -X POST "http://tiny-engineer.local/anim?name=sleep"
curl -X POST "http://tiny-engineer.local/anim?name=wakeup"
```

Expect JSON like `{"ok":true,"animation":"sleep"}` / `{"ok":true,"animation":"wakeup"}`. If the hostname stalls, try `curl -4` or the IP on the OLED ([`api.md`](api.md)).

Then run the binary in a terminal (not as a service):

```bash
~/screenwatch/screenwatch
```

You should see `screenwatch listening for lock/unlock`. Lock the screen (Control-Command-Q, or Apple menu → Lock Screen), then unlock. Terminal should print `screen locked` / `screen unlocked` and `posted sleep` / `posted wakeup`. The robot should sleep and wake. Ctrl-C to stop.

Do not load the LaunchAgent until this works.

## LaunchAgent setup

Copy the plist below to `~/Library/LaunchAgents/local.screenwatch.plist`. Replace `/ABSOLUTE/PATH/TO/screenwatch` with the expanded path from `echo "$HOME/screenwatch/screenwatch"`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>local.screenwatch</string>
    <key>ProgramArguments</key>
    <array>
        <string>/ABSOLUTE/PATH/TO/screenwatch</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/tmp/screenwatch.log</string>
    <key>StandardErrorPath</key>
    <string>/tmp/screenwatch-error.log</string>
</dict>
</plist>
```

Validate:

```bash
plutil -lint ~/Library/LaunchAgents/local.screenwatch.plist
```

Load **as the logged-in user**. Do **not** use `sudo` with `launchctl bootstrap gui/$(id -u) …` — that targets the user GUI domain and fails or loads into the wrong session.

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/local.screenwatch.plist
```

Unload:

```bash
launchctl bootout gui/$(id -u)/local.screenwatch
```

Status:

```bash
launchctl print gui/$(id -u)/local.screenwatch
```

Logs:

```bash
tail -f /tmp/screenwatch.log /tmp/screenwatch-error.log
```

After login (or immediately, because `RunAtLoad` is true) you should see `screenwatch listening for lock/unlock` in `/tmp/screenwatch.log`.

## Verification

1. `launchctl print gui/$(id -u)/local.screenwatch` shows `state = running` (not only `spawn scheduled`).
2. Lock, then unlock. `/tmp/screenwatch.log` should contain `screen locked` / `screen unlocked` and `posted sleep` / `posted wakeup`.
3. Robot plays sleep then wakeup. Independent check:

```bash
curl http://tiny-engineer.local/anim
```

Successful POSTs return JSON such as `{"ok":true,"animation":"sleep"}` and `{"ok":true,"animation":"wakeup"}`.

## Troubleshooting

**`state = spawn scheduled`**

launchd has accepted the job but has not kept a running process. Usual causes: wrong `ProgramArguments` path, binary not executable, or the process exiting immediately and being retried. Check `path =`, `state =`, and last exit status in `launchctl print`. Confirm:

```bash
ls -l "$(echo "$HOME/screenwatch/screenwatch")"
~/screenwatch/screenwatch
```

The foreground run must stay up until you Ctrl-C.

**`last exit code = 78` (`EX_CONFIG`)**

launchd could not configure/exec the job. Almost always a bad executable path in the plist (missing file, still using `~`, or a directory instead of the binary). `ProgramArguments[0]` must be an existing executable file. Re-lint the plist, fix the path, `bootout`, then `bootstrap` again.

**`Bootstrap failed: 5: Input/output error`**

The job is usually **already loaded**, or you mixed user/root domains. Do **not** retry with `sudo`. Print, then bootout, then bootstrap:

```bash
launchctl print gui/$(id -u)/local.screenwatch
launchctl bootout gui/$(id -u)/local.screenwatch
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/local.screenwatch.plist
```

Also happens if the plist path is wrong or unreadable by the user.

**Incorrect executable path in the plist**

`launchd` does not expand `~`. Use `echo "$HOME/screenwatch/screenwatch"` and paste that absolute path as the only `ProgramArguments` string. After editing:

```bash
plutil -lint ~/Library/LaunchAgents/local.screenwatch.plist
launchctl bootout gui/$(id -u)/local.screenwatch
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/local.screenwatch.plist
```

**Empty log output (stdout buffering)**

If lock/unlock works but `/tmp/screenwatch.log` stays empty, Swift’s stdout is block-buffered when not a TTY. The sample calls `setbuf(stdout, nil)` / `fflush`. Rebuild if you omitted those. `StandardErrorPath` still catches `fputs` failures.

**Verify curl endpoints independently**

If the helper runs but the robot does not move, bypass Swift:

```bash
curl -v -X POST "http://tiny-engineer.local/anim?name=sleep"
curl -v -X POST "http://tiny-engineer.local/anim?name=wakeup"
```

A reachable board returns `{"ok":true,"animation":"sleep"}` / `{"ok":true,"animation":"wakeup"}`. Failures here are network, mDNS, or API auth — not launchd. If `access_token` is set, curl needs `Authorization: Bearer …` ([`api.md`](api.md)).

## Uninstall / disable

```bash
launchctl bootout gui/$(id -u)/local.screenwatch
rm ~/Library/LaunchAgents/local.screenwatch.plist
```

Optional: `rm -r ~/screenwatch` and the log files under `/tmp/`. After `bootout` the helper is gone until you `bootstrap` again; deleting the plist is enough to keep it from returning at login.

## Notes and limitations

- Optional extra. Tiny Engineer does not need this to work with Cursor, REST, or the web UI.
- `com.apple.screenIsLocked` / `com.apple.screenIsUnlocked` are widely used but unofficial. They may change in a future macOS.
- Lock via Control-Command-Q (or Lock Screen) is the event this watches. Closing the lid may sleep the Mac before the helper can POST.
- To discard curl bodies: add `-s` `-o` `/dev/null` to the `Process` arguments.
- Same-network, port 80. Optional Bearer token: [`api.md`](api.md).
