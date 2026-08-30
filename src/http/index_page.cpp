#include "http/index_page.h"

#include "http/json.h"

namespace {

static const char INDEX_HTML[] = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tiny Engineer</title>
<style>
:root{--bg:#faf9f7;--text:#1a1a1a;--muted:#666;--accent:#e85d04;--accent-hover:#d45304;--card:#fff;--border:#e0ddd8;--success:#2d6a4f;--error:#c1121f;--loading:#555}
*{box-sizing:border-box}
body{font-family:system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--text);max-width:42rem;margin:0 auto;padding:0 1rem 3rem;line-height:1.5}
a{color:var(--accent);text-decoration:none}
a:hover{text-decoration:underline}
nav{display:flex;gap:.25rem;flex-wrap:wrap;padding:1rem 0;border-bottom:1px solid var(--border);margin-bottom:1.5rem;position:sticky;top:0;background:var(--bg);z-index:10}
nav a{padding:.4rem .75rem;border-radius:.35rem;color:var(--muted);font-size:.9rem;font-weight:500;text-decoration:none}
nav a:hover{background:#eee;color:var(--text);text-decoration:none}
nav a.active{background:var(--accent);color:#fff}
.view{display:none}
.view.active{display:block}
.hero{margin-bottom:2rem}
.hero h1{font-size:1.75rem;margin:0 0 .5rem;font-weight:700}
.hero p{color:var(--muted);margin:0;font-size:1.05rem}
.info-strip{background:var(--card);border:1px solid var(--border);border-radius:.5rem;padding:.6rem 1rem;margin:1rem 0;font-size:.85rem;color:var(--muted)}
.cards{display:grid;gap:.85rem;margin:1.5rem 0;grid-template-columns:1fr}
@media(min-width:480px){.cards{grid-template-columns:1fr 1fr}}
.card{display:block;background:var(--card);border:1px solid var(--border);border-radius:.6rem;padding:1.1rem 1.25rem;text-decoration:none;color:inherit;transition:box-shadow .15s,border-color .15s,transform .15s}
.card:hover{border-color:var(--accent);box-shadow:0 2px 8px rgba(0,0,0,.06);text-decoration:none}
.card h3{margin:0 0 .35rem;font-size:1rem;color:var(--accent)}
.card p{margin:0;font-size:.9rem;color:var(--muted)}
.card-github{background:var(--accent);border-color:var(--accent);color:#fff;box-shadow:0 2px 10px rgba(232,93,4,.28)}
.card-github h3{color:#fff}
.card-github p{color:rgba(255,255,255,.85)}
.card-github:hover{background:var(--accent-hover);border-color:var(--accent-hover);box-shadow:0 4px 14px rgba(232,93,4,.35);transform:translateY(-1px);text-decoration:none}
.page-title{font-size:1.35rem;margin:0 0 .5rem}
.page-desc{color:var(--muted);margin:0 0 1.25rem;font-size:.95rem}
#status{padding:.65rem 1rem;border-radius:.4rem;margin-bottom:1rem;font-size:.9rem;display:none}
#status.show{display:block}
#status.loading{background:#eee;color:var(--loading)}
#status.ok{background:#d8f3dc;color:var(--success)}
#status.err{background:#fde8e8;color:var(--error)}
.badge{display:inline-block;background:#eee;padding:.25rem .6rem;border-radius:.3rem;font-size:.85rem;margin-bottom:1rem}
.badge strong{color:var(--accent)}
.btn-grid{display:grid;gap:.75rem}
@media(min-width:400px){.btn-grid{grid-template-columns:1fr 1fr}}
.btn{display:block;width:100%;padding:.75rem 1rem;background:var(--card);border:1px solid var(--border);border-radius:.5rem;cursor:pointer;text-align:left;font:inherit;color:inherit;transition:border-color .15s,background .15s}
.btn:hover:not(:disabled){border-color:var(--accent);background:#fff8f4}
.btn:disabled{opacity:.5;cursor:not-allowed}
.btn-title{font-weight:600;display:block;margin-bottom:.15rem}
.btn-hint{font-size:.8rem;color:var(--muted)}
.btn-primary{background:var(--accent);color:#fff;border-color:var(--accent);text-align:center;font-weight:600;margin-top:1rem}
.btn-primary:hover:not(:disabled){background:var(--accent-hover);border-color:var(--accent-hover)}
.test-card{background:var(--card);border:1px solid var(--border);border-radius:.5rem;padding:1rem;margin-bottom:.75rem;display:flex;align-items:center;justify-content:space-between;gap:1rem}
.test-card p{margin:0;font-size:.9rem;color:var(--muted);flex:1}
.test-card .btn{width:auto;min-width:7rem;text-align:center;padding:.55rem 1rem}
.form-group{margin-bottom:1rem}
.form-group label{display:block;font-weight:600;margin-bottom:.35rem;font-size:.9rem}
.form-group select,.form-group input[type=number],.form-group input[type=text]{width:100%;padding:.5rem .65rem;border:1px solid var(--border);border-radius:.35rem;font:inherit;background:var(--card)}
.form-group input[type=range]{width:100%;margin:.5rem 0}
.range-row{display:flex;align-items:center;gap:1rem}
.range-row input[type=number]{width:5rem}
.toggle-row{display:flex;align-items:center;gap:.65rem}
.toggle-row input[type=checkbox]{width:1.15rem;height:1.15rem;accent-color:var(--accent)}
.toggle-row label{margin:0;font-weight:600;font-size:.9rem}
.hint{font-size:.8rem;color:var(--muted);margin-top:.25rem}
table{border-collapse:collapse;width:100%;margin-bottom:1rem;font-size:.85rem}
th,td{border:1px solid var(--border);padding:.35rem .5rem;text-align:left;vertical-align:top}
th{background:#f4f4f4}
code{background:#f4f4f4;padding:.1rem .3rem;border-radius:.2rem;font-size:.8rem}
footer{margin-top:2.5rem;padding-top:1rem;border-top:1px solid var(--border);font-size:.85rem;color:var(--muted);text-align:center}
#auth-gate{display:none;position:fixed;inset:0;background:var(--bg);z-index:100;padding:2rem 1rem;align-items:center;justify-content:center}
#auth-gate.show{display:flex}
#auth-gate .auth-box{width:100%;max-width:22rem;background:var(--card);border:1px solid var(--border);border-radius:.6rem;padding:1.5rem}
#auth-gate h1{font-size:1.35rem;margin:0 0 .5rem}
#auth-gate p{color:var(--muted);margin:0 0 1.25rem;font-size:.95rem}
#auth-gate .form-group{margin-bottom:1rem}
#auth-error{display:none;color:var(--error);font-size:.85rem;margin-bottom:.75rem}
#auth-error.show{display:block}
#reboot-gate{display:none;position:fixed;inset:0;background:var(--bg);z-index:100;padding:2rem 1rem;align-items:center;justify-content:center}
#reboot-gate.show{display:flex}
#reboot-gate .auth-box{width:100%;max-width:22rem;background:var(--card);border:1px solid var(--border);border-radius:.6rem;padding:1.5rem}
#reboot-gate h1{font-size:1.35rem;margin:0 0 .5rem}
#reboot-gate p{color:var(--muted);margin:0 0 1.25rem;font-size:.95rem}
#reboot-gate p:last-child{margin-bottom:0}
body.locked nav,body.locked #status,body.locked .view,body.locked footer{display:none!important}
.form-group input[type=password]{width:100%;padding:.5rem .65rem;border:1px solid var(--border);border-radius:.35rem;font:inherit;background:var(--card)}
.token-row{display:flex;gap:.5rem;align-items:stretch}
.token-row input[type=password],.token-row input[type=text]{flex:1}
.btn-token-toggle{width:auto;min-width:7rem;text-align:center;padding:.5rem .75rem;font-size:.85rem;flex-shrink:0;align-self:stretch}
.config-section{background:var(--card);border:1px solid var(--border);border-radius:.6rem;padding:1rem 1.15rem;margin-bottom:.85rem}
.config-section-head{display:flex;align-items:center;justify-content:space-between;gap:.75rem;margin-bottom:.85rem}
.config-section h3{margin:0;font-size:1rem;color:var(--accent)}
.config-section-desc{font-size:.85rem;color:var(--muted);margin:-.35rem 0 .85rem}
.config-section .form-group:last-child{margin-bottom:0}
.apply-badge{font-size:.7rem;font-weight:600;padding:.2rem .5rem;border-radius:.25rem;white-space:nowrap;flex-shrink:0}
.apply-now{background:#d8f3dc;color:var(--success)}
.apply-reboot{background:#fff3cd;color:#856404}
.field-head{display:flex;align-items:center;justify-content:space-between;gap:.5rem;margin-bottom:.35rem}
.field-head label{margin:0;font-weight:600;font-size:.9rem}
#config-welcome-motion-hint{display:none}
.btn-danger{border-color:var(--error);color:var(--error);text-align:center;font-weight:600}
.btn-danger:hover:not(:disabled){border-color:var(--error);background:#fde8e8}
.config-danger{border-color:#f5c2c7;margin-top:1rem}
body.setup-mode nav,body.setup-mode footer{display:none!important}
body.setup-mode #config-form,body.setup-mode .config-danger{display:none!important}
body:not(.setup-mode) #wifi-config-section{display:none!important}
</style>
</head>
<body>
<div id="auth-gate">
<div class="auth-box">
<h1>Access token required</h1>
<p>Enter the device access token to use the control panel.</p>
<form id="auth-form">
<div class="form-group">
<label for="auth-token">Access token</label>
<input type="password" id="auth-token" autocomplete="current-password" required>
</div>
<div id="auth-error">Invalid access token.</div>
<button type="submit" class="btn btn-primary" style="margin-top:0">Unlock</button>
</form>
</div>
</div>
<div id="reboot-gate">
<div class="auth-box">
<h1>Factory reset done</h1>
<p>Settings and WiFi credentials are cleared. Power-cycle or press the device reset button.</p>
<p>Then join the robot WiFi shown on the OLED and open the setup page to configure home WiFi.</p>
</div>
</div>
<nav>
<a href="/" data-nav="/">Home</a>
<a href="/animations" data-nav="/animations">Animations</a>
<a href="/tests" data-nav="/tests">Tests</a>
<a href="/servo" data-nav="/servo">Servo</a>
<a href="/config" data-nav="/config">Config</a>
<a href="/api" data-nav="/api">API</a>
</nav>

<div id="status"></div>

<section id="view-home" class="view">
<div class="hero">
<h1>Tiny Engineer</h1>
<p>A desk robot that acts out your AI coding assistant while it works.</p>
</div>
<div id="health-info" class="info-strip">Loading status&hellip;</div>
<div class="cards">
<a class="card" href="/animations"><h3>Animations</h3><p>Pick a gesture &mdash; typing, reading, thinking, and more.</p></a>
<a class="card" href="/servo"><h3>Servo control</h3><p>Move individual servos to any angle.</p></a>
<a class="card" href="/tests"><h3>Hardware tests</h3><p>Try the speaker, screen, LEDs, and servo sweep.</p></a>
<a class="card" href="/config"><h3>Config</h3><p>Device name, timeouts, volume, boot behavior, serial logging, and API token. WiFi is set in setup mode after factory reset.</p></a>
<a class="card" href="/api"><h3>API reference</h3><p>Full endpoint list, parameters, and curl-friendly docs.</p></a>
<a class="card card-github" href="https://github.com/jamro/tiny-engineer" target="_blank" rel="noopener"><h3>GitHub docs &rarr;</h3><p>Build guide, wiring, and full project docs.</p></a>
</div>
</section>

<section id="view-api" class="view">
<h2 class="page-title">API reference</h2>
<p class="page-desc">JSON HTTP API on port 80. Optional Bearer auth when an access token is configured. Test routes are POST only. <code>GET /auth</code> is always public.</p>
<table>
<tr><th>Method</th><th>Path</th><th>Description</th></tr>
<tr><td>GET</td><td><code>/</code></td><td>Web control panel</td></tr>
<tr><td>GET</td><td><code>/animations</code></td><td>Animations page</td></tr>
<tr><td>GET</td><td><code>/tests</code></td><td>Hardware tests page</td></tr>
<tr><td>GET</td><td><code>/servo</code></td><td>Servo control page</td></tr>
<tr><td>GET</td><td><code>/config</code></td><td>Config page</td></tr>
<tr><td>GET</td><td><code>/api</code></td><td>This API reference</td></tr>
<tr><td>GET</td><td><code>/auth</code></td><td>Whether API auth is required (always public)</td></tr>
<tr><td>GET</td><td><code>/health</code></td><td>Health JSON, no side effects</td></tr>
<tr><td>GET</td><td><code>/settings</code></td><td>Persistent settings</td></tr>
<tr><td>POST</td><td><code>/settings</code></td><td>Update settings (see parameters below)</td></tr>
<tr><td>POST</td><td><code>/settings/reset</code></td><td>Factory reset all settings to defaults</td></tr>
<tr><td>GET</td><td><code>/anim</code></td><td>Current animation name</td></tr>
<tr><td>POST</td><td><code>/anim</code></td><td>Set animation (see parameters below)</td></tr>
<tr><td>POST</td><td><code>/test/audio</code></td><td>Play tone test</td></tr>
<tr><td>POST</td><td><code>/test/audio/bell</code></td><td>Play bell WAV from LittleFS</td></tr>
<tr><td>POST</td><td><code>/test/screen</code></td><td>OLED demo</td></tr>
<tr><td>POST</td><td><code>/test/movement</code></td><td>All servos exercise</td></tr>
<tr><td>POST</td><td><code>/test/led</code></td><td>RGB LED cycle</td></tr>
<tr><td>POST</td><td><code>/test/servo</code></td><td>Move one servo (see parameters below)</td></tr>
</table>
<p>POST <code>/settings</code> &mdash; query params (at least one required):</p>
<table>
<tr><th>Param</th><th>Type</th><th>Range</th></tr>
<tr><td><code>sleep_timeout</code></td><td>integer</td><td>1&ndash;1440 minutes (positive)</td></tr>
<tr><td><code>hostname</code></td><td>string</td><td>1&ndash;31 chars, letters/digits/hyphen</td></tr>
<tr><td><code>volume</code></td><td>integer</td><td>0&ndash;100 percent (speaker gain)</td></tr>
<tr><td><code>welcome</code></td><td>integer</td><td>0 or 1 (boot welcome animation)</td></tr>
<tr><td><code>serial_log</code></td><td>integer</td><td>0 or 1 (USB serial debug logging)</td></tr>
<tr><td><code>continuous_timeout</code></td><td>integer</td><td>1&ndash;1440 minutes</td></tr>
<tr><td><code>loading</code></td><td>string</td><td><code>progress</code> or <code>sleep_inertia</code> (boot screen; next reboot)</td></tr>
<tr><td><code>access_token</code></td><td>string</td><td>0&ndash;64 printable ASCII; empty clears (disables auth)</td></tr>
<tr><td><code>wifi_ssid</code></td><td>string</td><td>1&ndash;32 chars; setup AP only; requires <code>wifi_password</code></td></tr>
<tr><td><code>wifi_password</code></td><td>string</td><td>0&ndash;63 chars; setup AP only; tested before save</td></tr>
</table>
<p>WiFi credentials can only be set in setup AP mode. During setup (credentials not saved), control APIs return <strong>503</strong> <code>wifi not configured</code>. Change WiFi later via factory reset.</p>
<p>POST <code>/anim</code> &mdash; query param <code>name</code>:</p>
<table>
<tr><th>Value</th><th>Description</th></tr>
<tr><td><code>none</code></td><td>Idle pose</td></tr>
<tr><td><code>typing</code></td><td>Typing gesture</td></tr>
<tr><td><code>reading</code></td><td>Reading gesture</td></tr>
<tr><td><code>thinking</code></td><td>Thinking gesture</td></tr>
<tr><td><code>ring</code></td><td>One-shot bell gesture</td></tr>
<tr><td><code>welcome</code></td><td>One-shot hello gesture</td></tr>
<tr><td><code>attention</code></td><td>Input-request gesture + audio</td></tr>
<tr><td><code>error</code></td><td>Obstacle gesture + audio</td></tr>
<tr><td><code>abort</code></td><td>Abort gesture + audio</td></tr>
<tr><td><code>wakeup</code></td><td>One-shot sleep-inertia wake</td></tr>
<tr><td><code>sleep</code></td><td>Close eyes and sleep</td></tr>
</table>
<p>POST <code>/test/servo</code> &mdash; query params:</p>
<table>
<tr><th>Param</th><th>Type</th><th>Range</th></tr>
<tr><td><code>index</code></td><td>integer</td><td>0&ndash;4</td></tr>
<tr><td><code>angle</code></td><td>number</td><td>0&ndash;180</td></tr>
</table>
</section>

<section id="view-animations" class="view">
<h2 class="page-title">Animations</h2>
<p class="page-desc">Set the robot&rsquo;s current gesture. Continuous animations keep going until changed or timed out.</p>
<div id="anim-current" class="badge">Current: <strong>&hellip;</strong></div>
<div class="btn-grid" id="anim-buttons">
<button class="btn" data-anim="none"><span class="btn-title">Idle</span><span class="btn-hint">Rest pose</span></button>
<button class="btn" data-anim="typing"><span class="btn-title">Typing</span><span class="btn-hint">Continuous</span></button>
<button class="btn" data-anim="reading"><span class="btn-title">Reading</span><span class="btn-hint">Continuous</span></button>
<button class="btn" data-anim="thinking"><span class="btn-title">Thinking</span><span class="btn-hint">Continuous</span></button>
<button class="btn" data-anim="ring"><span class="btn-title">Bell</span><span class="btn-hint">One-shot</span></button>
<button class="btn" data-anim="welcome"><span class="btn-title">Welcome</span><span class="btn-hint">One-shot</span></button>
<button class="btn" data-anim="wakeup"><span class="btn-title">Wakeup</span><span class="btn-hint">Sleep inertia</span></button>
<button class="btn" data-anim="sleep"><span class="btn-title">Sleep</span><span class="btn-hint">Close + OLED off</span></button>
<button class="btn" data-anim="attention"><span class="btn-title">Attention</span><span class="btn-hint">+ audio, 1 min hold</span></button>
<button class="btn" data-anim="error"><span class="btn-title">Error</span><span class="btn-hint">+ audio, 1 min hold</span></button>
<button class="btn" data-anim="abort"><span class="btn-title">Abort</span><span class="btn-hint">One-shot + audio</span></button>
</div>
</section>

<section id="view-servo" class="view">
<h2 class="page-title">Servo control</h2>
<p class="page-desc">Move a single servo to a target angle.</p>
<form id="servo-form">
<div class="form-group">
<label for="servo-index">Servo</label>
<select id="servo-index">
<option value="0">0 &mdash; Head</option>
<option value="1">1 &mdash; Neck</option>
<option value="2">2 &mdash; Left hand</option>
<option value="3">3 &mdash; Right hand</option>
<option value="4">4 &mdash; Body</option>
</select>
<p id="servo-range-hint" class="hint">Safe range: 60&ndash;130&deg;</p>
</div>
<div class="form-group">
<label for="servo-angle">Angle</label>
<div class="range-row">
<input type="range" id="servo-slider" min="0" max="180" value="90">
<input type="number" id="servo-angle" min="0" max="180" value="90">
</div>
</div>
<button type="submit" class="btn btn-primary">Move servo</button>
</form>
</section>

<section id="view-tests" class="view">
<h2 class="page-title">Hardware tests</h2>
<p class="page-desc">Run built-in diagnostics. Each test blocks until complete.</p>
<div class="test-card"><p>Play ascending tones through the speaker.</p><button class="btn" data-test="/test/audio">Audio tones</button></div>
<div class="test-card"><p>Play the bell WAV file from storage.</p><button class="btn" data-test="/test/audio/bell">Bell sound</button></div>
<div class="test-card"><p>Draw a demo pattern on the OLED screen.</p><button class="btn" data-test="/test/screen">OLED screen</button></div>
<div class="test-card"><p>Exercise all five servos through a sweep.</p><button class="btn" data-test="/test/movement">All servos</button></div>
<div class="test-card"><p>Cycle the onboard RGB LED through colors.</p><button class="btn" data-test="/test/led">RGB LED</button></div>
</section>

<section id="view-config" class="view">
<h2 class="page-title" id="config-page-title">Config</h2>
<p class="page-desc" id="config-page-desc">Saved to flash. Most changes apply right away.</p>
<div id="wifi-config-section" class="config-section">
<div class="config-section-head"><h3>WiFi</h3><span class="apply-badge apply-now">Immediate</span></div>
<p id="wifi-config-status" class="hint">Loading WiFi status&hellip;</p>
<div id="wifi-change-fields">
<div class="form-group">
<label for="config-wifi-ssid">Network name (SSID)</label>
<input type="text" id="config-wifi-ssid" maxlength="32" autocomplete="off">
</div>
<div class="form-group">
<label for="config-wifi-password">Password</label>
<div class="token-row">
<input type="password" id="config-wifi-password" maxlength="63" autocomplete="new-password">
<button type="button" id="config-wifi-password-toggle" class="btn btn-token-toggle">Show</button>
</div>
<p class="hint">Leave empty only for open networks.</p>
</div>
<button type="button" id="config-wifi-connect" class="btn btn-primary" style="margin-top:0">Connect to WiFi</button>
</div>
</div>
<form id="config-form">
<div class="config-section">
<div class="config-section-head"><h3>Network</h3><span class="apply-badge apply-reboot">After reboot</span></div>
<div class="form-group">
<label for="config-hostname">Hostname</label>
<input type="text" id="config-hostname" maxlength="31" pattern="[A-Za-z0-9]([A-Za-z0-9-]{0,29}[A-Za-z0-9])?" required>
<p class="hint">Letters, digits, hyphen &mdash; no .local</p>
</div>
</div>
<div class="config-section">
<div class="config-section-head"><h3>Timeouts</h3><span class="apply-badge apply-now">Immediate</span></div>
<div class="form-group">
<label for="config-sleep">Sleep timeout (minutes)</label>
<input type="number" id="config-sleep" min="1" max="1440" step="1" required>
<p class="hint">Minutes in animation <code>none</code> before OLED blanks</p>
</div>
<div class="form-group">
<label for="config-continuous">Continuous anim timeout (minutes)</label>
<input type="number" id="config-continuous" min="1" max="1440" step="1" required>
<p class="hint">Max time for typing / reading / thinking</p>
</div>
</div>
<div class="config-section">
<div class="config-section-head"><h3>Audio</h3><span class="apply-badge apply-now">Immediate</span></div>
<div class="form-group">
<label for="config-volume">Volume <span id="config-volume-label">70%</span></label>
<div class="range-row">
<input type="range" id="config-volume-slider" min="0" max="100" value="70">
<input type="number" id="config-volume" min="0" max="100" step="1" value="70" required>
</div>
</div>
</div>
<div class="config-section">
<h3>Boot behavior</h3>
<p class="config-section-desc">Loading screen changes take effect on next boot.</p>
<div class="form-group">
<div class="field-head"><label for="config-loading">Boot loading screen</label><span class="apply-badge apply-reboot">After reboot</span></div>
<select id="config-loading">
<option value="progress">Progress bar</option>
<option value="sleep_inertia">Sleep inertia (no IP display)</option>
</select>
<p id="config-welcome-motion-hint" class="hint">Head motion during boot needs Welcome enabled.</p>
</div>
<div class="form-group">
<div class="field-head"><span>Welcome on boot</span><span class="apply-badge apply-now">Immediate</span></div>
<div class="toggle-row">
<input type="checkbox" id="config-welcome" checked>
<label for="config-welcome">Play welcome when Wi-Fi connects</label>
</div>
</div>
<div class="form-group">
<div class="field-head"><span>Serial logging</span><span class="apply-badge apply-now">Immediate</span></div>
<div class="toggle-row">
<input type="checkbox" id="config-serial-log">
<label for="config-serial-log">Log to USB serial</label>
</div>
</div>
</div>
<div class="config-section">
<h3>Security</h3>
<div class="form-group">
<label for="config-access-token">Access token</label>
<div class="token-row">
<input type="password" id="config-access-token" maxlength="64" autocomplete="new-password" placeholder="Enter access token">
<button type="button" id="config-access-token-toggle" class="btn btn-token-toggle" hidden>Remove token</button>
</div>
<p id="config-access-token-status" class="hint">Auth disabled</p>
</div>
</div>
<button type="submit" class="btn btn-primary">Save settings</button>
</form>
<div class="config-section config-danger">
<h3>Factory reset</h3>
<p class="hint">Erases all saved settings and restores defaults. WiFi credentials are cleared too. Power-cycle the device to reopen setup AP mode and configure WiFi again.</p>
<button type="button" id="config-factory-reset" class="btn btn-danger">Factory reset</button>
</div>
</section>

<footer>
<a href="https://github.com/jamro/tiny-engineer" target="_blank" rel="noopener">github.com/jamro/tiny-engineer</a>
</footer>

<script>
var SERVO_RANGES=[[60,130],[40,130],[50,140],[40,130],[40,130]];
var TOKEN_KEY="te_access_token";
var ACCESS_TOKEN_MASK="********";
var accessTokenConfigured=false;
var accessTokenMaskActive=false;
var accessTokenClearPending=false;
var busy=false;
var healthTimer=null;
var lastHealthUptimeMs=0;
var uiUnlocked=false;
var provisioningMode=false;
var wifiConfigured=false;
var statusEl=document.getElementById("status");
function getStoredToken(){
  try{return sessionStorage.getItem(TOKEN_KEY)||"";}catch(e){return"";}
}
function setStoredToken(token){
  try{
    if(token)sessionStorage.setItem(TOKEN_KEY,token);
    else sessionStorage.removeItem(TOKEN_KEY);
  }catch(e){}
}
function apiFetch(path,opts){
  opts=opts||{};
  var headers=Object.assign({},opts.headers||{});
  var token=getStoredToken();
  if(token)headers["Authorization"]="Bearer "+token;
  return fetch(path,Object.assign({},opts,{headers:headers}));
}
function setStatus(msg,type){
  statusEl.textContent=msg;
  statusEl.className="show"+(type?" "+type:"");
}
function clearStatus(){
  statusEl.className="";
  statusEl.textContent="";
}
function setBusy(on){
  busy=on;
  document.querySelectorAll(".btn,[type=submit]").forEach(function(b){b.disabled=on;});
}
function showAuthGate(show){
  document.getElementById("auth-gate").classList.toggle("show",show);
  document.getElementById("reboot-gate").classList.remove("show");
  document.body.classList.toggle("locked",show);
  if(show){
    document.getElementById("auth-error").classList.remove("show");
    document.getElementById("auth-token").value="";
    document.getElementById("auth-token").focus();
  }
}
function showRebootGate(show){
  document.getElementById("reboot-gate").classList.toggle("show",show);
  document.getElementById("auth-gate").classList.remove("show");
  document.body.classList.toggle("locked",show);
  if(show){
    uiUnlocked=false;
    stopHealthPolling();
  }
}
function enterApp(){
  uiUnlocked=true;
  showAuthGate(false);
  showRebootGate(false);
  syncSetupUi(function(){
    if(provisioningMode||!wifiConfigured){
      showPage("/config");
      var ssidField=document.getElementById("config-wifi-ssid");
      if(ssidField)ssidField.focus();
    }else{
      showPage(location.pathname);
    }
    updateServoHint();
  });
}
function syncSetupUi(done){
  fetch("/health").then(function(r){return r.json();}).then(function(j){
    if(j.ok){
      provisioningMode=!!j.provisioning;
      wifiConfigured=!!j.wifi_configured;
      var inSetup=provisioningMode||!wifiConfigured;
      document.body.classList.toggle("setup-mode",inSetup);
      var title=document.getElementById("config-page-title");
      var desc=document.getElementById("config-page-desc");
      if(title)title.textContent=inSetup?"WiFi setup":"Config";
      if(desc){
        desc.textContent=inSetup
          ?"Enter your home WiFi network. The robot tests the connection before saving."
          :"Saved to flash. Most changes apply right away.";
      }
    }
    if(done)done();
  }).catch(function(){if(done)done();});
}
function syncWifiStatusFromSettings(j){
  var status=document.getElementById("wifi-config-status");
  if(!status)return;
  status.textContent="Enter the network name and password for the WiFi you want the robot to join.";
}
function showPage(path){
  if(!uiUnlocked)return;
  var map={"/":"view-home","/animations":"view-animations","/servo":"view-servo","/tests":"view-tests","/config":"view-config","/api":"view-api"};
  var id=map[path]||"view-home";
  document.querySelectorAll(".view").forEach(function(v){v.classList.remove("active");});
  document.getElementById(id).classList.add("active");
  document.querySelectorAll("nav a").forEach(function(a){
    a.classList.toggle("active",a.getAttribute("data-nav")===path||(path==="/"&&a.getAttribute("data-nav")==="/"));
  });
  if(id==="view-animations") refreshAnim();
  if(id==="view-config") loadSettings();
  if(id==="view-home") startHealthPolling();
  else stopHealthPolling();
}
function startHealthPolling(){
  stopHealthPolling();
  loadHealth();
  healthTimer=setInterval(loadHealth,1000);
}
function stopHealthPolling(){
  if(healthTimer){clearInterval(healthTimer);healthTimer=null;}
}
function apiPost(path){
  if(busy)return Promise.reject();
  setBusy(true);
  setStatus("Running\u2026","loading");
  return apiFetch(path,{method:"POST"}).then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false){
      setStatus("Done.","ok");
    }else{
      setStatus(res.data.error||"Request failed","err");
    }
    return res;
  }).catch(function(){
    setStatus("Network error","err");
  }).finally(function(){setBusy(false);});
}
function refreshAnim(){
  apiFetch("/anim").then(function(r){return r.json();}).then(function(j){
    if(j.ok) document.querySelector("#anim-current strong").textContent=j.animation;
  }).catch(function(){});
}
function formatUptime(ms){
  var s=Math.floor(ms/1000);
  if(s<60)return s+" s";
  var m=Math.floor(s/60);s%=60;
  if(m<60)return m+" min "+s+" s";
  var h=Math.floor(m/60);m%=60;
  if(h<24)return h+" h "+m+" min";
  var d=Math.floor(h/24);h%=24;
  return d+" d "+h+" h";
}
function formatBytes(n){
  if(n>=1048576)return(n/1048576).toFixed(1)+" MB";
  if(n>=1024)return Math.round(n/1024)+" KB";
  return n+" B";
}
function loadHealth(){
  apiFetch("/health").then(function(r){return r.json();}).then(function(j){
    var el=document.getElementById("health-info");
    if(!j.ok){el.textContent="Could not load status.";return;}
    if(j.uptime_ms!=null)lastHealthUptimeMs=j.uptime_ms;
    var ip=j.wifi&&j.wifi.connected?j.wifi.ip:"offline";
    var heapPct=j.heap_size?Math.round((1-j.free_heap/j.heap_size)*100):0;
    var line="IP: "+ip+" \u00b7 Uptime: "+formatUptime(j.uptime_ms)+" \u00b7 Heap: "+heapPct+"% used ("+formatBytes(j.free_heap)+" free)";
    if(typeof j.cpu_temp_c==="number")line+=" \u00b7 Temp: "+j.cpu_temp_c.toFixed(1)+" \u00b0C";
    el.textContent=line;
  }).catch(function(){
    document.getElementById("health-info").textContent="Could not load status.";
  });
}
function updateServoHint(){
  var idx=parseInt(document.getElementById("servo-index").value,10);
  var r=SERVO_RANGES[idx];
  document.getElementById("servo-range-hint").textContent="Safe range: "+r[0]+"\u2013"+r[1]+"\u00b0";
}
document.getElementById("servo-slider").addEventListener("input",function(){
  document.getElementById("servo-angle").value=this.value;
});
document.getElementById("servo-angle").addEventListener("input",function(){
  document.getElementById("servo-slider").value=this.value;
});
document.getElementById("servo-index").addEventListener("change",updateServoHint);
document.getElementById("servo-form").addEventListener("submit",function(e){
  e.preventDefault();
  if(busy)return;
  var idx=document.getElementById("servo-index").value;
  var angle=document.getElementById("servo-angle").value;
  setBusy(true);
  setStatus("Moving servo\u2026","loading");
  apiFetch("/test/servo?index="+idx+"&angle="+angle,{method:"POST"})
  .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false){
      setStatus("Servo "+idx+" moved to "+angle+"\u00b0.","ok");
    }else{
      setStatus(res.data.error||"Move failed","err");
    }
  }).catch(function(){setStatus("Network error","err");})
  .finally(function(){setBusy(false);});
});
function setConfigVolume(v){
  var n=parseInt(v,10);
  if(isNaN(n))n=70;
  if(n<0)n=0;
  if(n>100)n=100;
  document.getElementById("config-volume").value=n;
  document.getElementById("config-volume-slider").value=n;
  document.getElementById("config-volume-label").textContent=n+"%";
}
function syncAccessTokenUi(){
  var field=document.getElementById("config-access-token");
  var toggle=document.getElementById("config-access-token-toggle");
  var status=document.getElementById("config-access-token-status");
  if(accessTokenClearPending){
    field.value="";
    field.disabled=true;
    field.placeholder="Token will be removed on Save";
    toggle.hidden=false;
    toggle.textContent="Undo";
    status.textContent="Will remove on save";
  }else if(accessTokenConfigured){
    field.disabled=false;
    field.placeholder="Enter a new token to replace";
    if(accessTokenMaskActive)field.value=ACCESS_TOKEN_MASK;
    toggle.hidden=false;
    toggle.textContent="Remove token";
    status.textContent="Auth enabled \u2014 click field to replace";
  }else{
    field.disabled=false;
    if(!accessTokenMaskActive)field.value="";
    field.placeholder="Enter access token";
    toggle.hidden=true;
    status.textContent="Auth disabled";
  }
}
function updateWelcomeMotionHint(){
  var loading=document.getElementById("config-loading").value;
  var welcome=document.getElementById("config-welcome").checked;
  document.getElementById("config-welcome-motion-hint").style.display=(loading==="sleep_inertia"&&!welcome)?"block":"none";
}
function buildSaveMessage(prevHost,prevLoading,res){
  var savedHost=res.data.hostname||prevHost;
  var savedLoading=res.data.loading==="sleep_inertia"?"sleep_inertia":"progress";
  var hostChanged=savedHost!==prevHost||!!res.data.reboot_required;
  var loadingChanged=savedLoading!==prevLoading;
  if(!hostChanged&&!loadingChanged)return "Settings saved.";
  if(hostChanged&&loadingChanged)return "Saved. Hostname and loading screen apply after reboot.";
  if(hostChanged)return "Saved. Hostname applies after reboot.";
  return "Saved. Loading screen applies after reboot.";
}
function setAccessTokenFromServer(tokenSet){
  accessTokenConfigured=!!tokenSet;
  accessTokenClearPending=false;
  accessTokenMaskActive=accessTokenConfigured;
  syncAccessTokenUi();
}
document.getElementById("config-volume-slider").addEventListener("input",function(){
  setConfigVolume(this.value);
});
document.getElementById("config-volume").addEventListener("input",function(){
  setConfigVolume(this.value);
});
document.getElementById("config-access-token").addEventListener("focus",function(){
  if(accessTokenMaskActive){
    this.value="";
    accessTokenMaskActive=false;
  }
});
document.getElementById("config-access-token").addEventListener("blur",function(){
  if(accessTokenClearPending||!accessTokenConfigured)return;
  if(this.value.trim()===""){
    accessTokenMaskActive=true;
    syncAccessTokenUi();
  }
});
document.getElementById("config-access-token-toggle").addEventListener("click",function(){
  if(accessTokenClearPending){
    accessTokenClearPending=false;
    accessTokenMaskActive=true;
  }else{
    accessTokenClearPending=true;
    accessTokenMaskActive=false;
  }
  syncAccessTokenUi();
});
document.getElementById("config-loading").addEventListener("change",updateWelcomeMotionHint);
document.getElementById("config-welcome").addEventListener("change",updateWelcomeMotionHint);
function loadSettings(){
  apiFetch("/settings").then(function(r){return r.json();}).then(function(j){
    if(!j.ok)return;
    wifiConfigured=!!j.wifi_configured;
    syncWifiStatusFromSettings(j);
    document.getElementById("config-hostname").value=j.hostname||"";
    document.getElementById("config-sleep").value=j.sleep_timeout;
    document.getElementById("config-continuous").value=j.continuous_timeout!=null?j.continuous_timeout:5;
    setConfigVolume(j.volume!=null?j.volume:70);
    document.getElementById("config-welcome").checked=j.welcome!==false;
    document.getElementById("config-serial-log").checked=!!j.serial_log;
    document.getElementById("config-loading").value=j.loading==="sleep_inertia"?"sleep_inertia":"progress";
    setAccessTokenFromServer(!!j.access_token_set);
    updateWelcomeMotionHint();
  }).catch(function(){setStatus("Could not load settings","err");});
}
document.getElementById("config-wifi-password-toggle").addEventListener("click",function(){
  var field=document.getElementById("config-wifi-password");
  var show=field.type==="password";
  field.type=show?"text":"password";
  this.textContent=show?"Hide":"Show";
});
document.getElementById("config-wifi-connect").addEventListener("click",function(){
  if(busy)return;
  var ssid=document.getElementById("config-wifi-ssid").value.trim();
  var password=document.getElementById("config-wifi-password").value;
  if(!ssid){
    setStatus("Enter a WiFi network name.","err");
    return;
  }
  setBusy(true);
  setStatus("Testing WiFi credentials\u2026","loading");
  var url="/settings?wifi_ssid="+encodeURIComponent(ssid)+"&wifi_password="+encodeURIComponent(password);
  apiFetch(url,{method:"POST"})
  .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false&&res.data.wifi_connect_success){
      document.getElementById("config-wifi-password").value="";
      wifiConfigured=true;
      provisioningMode=false;
      document.body.classList.remove("setup-mode");
      syncSetupUi();
      syncWifiStatusFromSettings(res.data);
      var next="WiFi connected. Rejoin your home network";
      if(res.data.wifi_ip)next+=" and open http://"+res.data.wifi_ip;
      if(res.data.wifi_hostname)next+=" or http://"+res.data.wifi_hostname;
      next+=".";
      setStatus(next,"ok");
    }else{
      setStatus(res.data.error||"WiFi connection failed","err");
    }
  }).catch(function(){setStatus("Network error","err");})
  .finally(function(){setBusy(false);});
});
document.getElementById("config-form").addEventListener("submit",function(e){
  e.preventDefault();
  if(busy)return;
  var prevHost=document.getElementById("config-hostname").value.trim();
  var prevLoading=document.getElementById("config-loading").value;
  var host=document.getElementById("config-hostname").value.trim();
  var sleep=document.getElementById("config-sleep").value;
  var continuous=document.getElementById("config-continuous").value;
  var volume=document.getElementById("config-volume").value;
  var welcome=document.getElementById("config-welcome").checked?1:0;
  var serialLog=document.getElementById("config-serial-log").checked?1:0;
  var loading=document.getElementById("config-loading").value;
  var newToken=document.getElementById("config-access-token").value;
  var wasClearPending=accessTokenClearPending;
  var url="/settings?sleep_timeout="+encodeURIComponent(sleep)+"&hostname="+encodeURIComponent(host)+"&volume="+encodeURIComponent(volume)+"&welcome="+welcome+"&serial_log="+serialLog+"&continuous_timeout="+encodeURIComponent(continuous)+"&loading="+encodeURIComponent(loading);
  if(wasClearPending)url+="&access_token=";
  else if(!accessTokenMaskActive&&newToken)url+="&access_token="+encodeURIComponent(newToken);
  setBusy(true);
  setStatus("Saving\u2026","loading");
  apiFetch(url,{method:"POST"})
  .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false){
      document.getElementById("config-hostname").value=res.data.hostname||host;
      document.getElementById("config-sleep").value=res.data.sleep_timeout;
      document.getElementById("config-continuous").value=res.data.continuous_timeout!=null?res.data.continuous_timeout:continuous;
      setConfigVolume(res.data.volume!=null?res.data.volume:volume);
      document.getElementById("config-welcome").checked=res.data.welcome!==false;
      document.getElementById("config-serial-log").checked=!!res.data.serial_log;
      document.getElementById("config-loading").value=res.data.loading==="sleep_inertia"?"sleep_inertia":"progress";
      if(wasClearPending)setStoredToken("");
      else if(!accessTokenMaskActive&&newToken)setStoredToken(newToken);
      setAccessTokenFromServer(!!res.data.access_token_set);
      updateWelcomeMotionHint();
      setStatus(buildSaveMessage(prevHost,prevLoading,res),"ok");
    }else{
      setStatus(res.data.error||"Save failed","err");
    }
  }).catch(function(){setStatus("Network error","err");})
  .finally(function(){setBusy(false);});
});
document.getElementById("config-factory-reset").addEventListener("click",function(){
  if(busy)return;
  if(!confirm("Reset all settings to factory defaults? WiFi credentials will be cleared. Power-cycle the device to reopen setup AP mode and configure WiFi again."))return;
  setBusy(true);
  setStatus("Resetting\u2026","loading");
  apiFetch("/settings/reset",{method:"POST"})
  .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false){
      setStoredToken("");
      setAccessTokenFromServer(false);
      clearStatus();
      showRebootGate(true);
    }else{
      setStatus(res.data.error||"Factory reset failed","err");
    }
  }).catch(function(){setStatus("Network error","err");})
  .finally(function(){setBusy(false);});
});
document.querySelectorAll("[data-anim]").forEach(function(btn){
  btn.addEventListener("click",function(){
    if(busy)return;
    var name=this.getAttribute("data-anim");
    setBusy(true);
    setStatus("Setting animation\u2026","loading");
    apiFetch("/anim?name="+name,{method:"POST"})
    .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
    .then(function(res){
      if(res.ok&&res.data.ok!==false){
        setStatus("Animation: "+res.data.animation,"ok");
        document.querySelector("#anim-current strong").textContent=res.data.animation;
      }else{
        setStatus(res.data.error||"Failed","err");
      }
    }).catch(function(){setStatus("Network error","err");})
    .finally(function(){setBusy(false);});
  });
});
document.querySelectorAll("[data-test]").forEach(function(btn){
  btn.addEventListener("click",function(){
    apiPost(this.getAttribute("data-test"));
  });
});
document.getElementById("auth-form").addEventListener("submit",function(e){
  e.preventDefault();
  var token=document.getElementById("auth-token").value;
  if(!token)return;
  setStoredToken(token);
  document.getElementById("auth-error").classList.remove("show");
  apiFetch("/settings").then(function(r){
    if(r.status===401){
      setStoredToken("");
      document.getElementById("auth-error").classList.add("show");
      return;
    }
    enterApp();
  }).catch(function(){
    setStoredToken("");
    document.getElementById("auth-error").classList.add("show");
  });
});
function bootUi(){
  document.body.classList.add("locked");
  fetch("/auth").then(function(r){return r.json();}).then(function(j){
    if(!j.ok||!j.required){
      enterApp();
      return;
    }
    var token=getStoredToken();
    if(!token){
      showAuthGate(true);
      return;
    }
    apiFetch("/settings").then(function(r){
      if(r.status===401){
        setStoredToken("");
        showAuthGate(true);
        return;
      }
      enterApp();
    }).catch(function(){
      showAuthGate(true);
    });
  }).catch(function(){
    enterApp();
  });
}
document.addEventListener("DOMContentLoaded",bootUi);
</script>
</body>
</html>
)html";

}  // namespace

void sendIndexPage(WebServer& server) {
  httpSendHtml(server, 200, INDEX_HTML);
}
