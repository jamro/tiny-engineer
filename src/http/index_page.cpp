#include "http/index_page.h"

#include "http/json.h"
#include "sleep.h"

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
.cards{display:grid;gap:1rem;margin:1.5rem 0}
@media(min-width:480px){.cards{grid-template-columns:1fr 1fr}}
.card{display:block;background:var(--card);border:1px solid var(--border);border-radius:.6rem;padding:1.1rem 1.25rem;text-decoration:none;color:inherit;transition:box-shadow .15s,border-color .15s}
.card:hover{border-color:var(--accent);box-shadow:0 2px 8px rgba(0,0,0,.06);text-decoration:none}
.card h3{margin:0 0 .35rem;font-size:1rem;color:var(--accent)}
.card p{margin:0;font-size:.9rem;color:var(--muted)}
.github-link{display:inline-block;margin:1rem 0;padding:.5rem 1rem;background:var(--card);border:1px solid var(--border);border-radius:.4rem;font-weight:500}
.github-link:hover{border-color:var(--accent);text-decoration:none}
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
.hint{font-size:.8rem;color:var(--muted);margin-top:.25rem}
table{border-collapse:collapse;width:100%;margin-bottom:1rem;font-size:.85rem}
th,td{border:1px solid var(--border);padding:.35rem .5rem;text-align:left;vertical-align:top}
th{background:#f4f4f4}
code{background:#f4f4f4;padding:.1rem .3rem;border-radius:.2rem;font-size:.8rem}
footer{margin-top:2.5rem;padding-top:1rem;border-top:1px solid var(--border);font-size:.85rem;color:var(--muted);text-align:center}
</style>
</head>
<body>
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
<a class="card" href="/api"><h3>API reference</h3><p>Full endpoint list, parameters, and curl-friendly docs.</p></a>
<a class="card" href="/animations"><h3>Animations</h3><p>Pick a gesture &mdash; typing, reading, thinking, and more.</p></a>
<a class="card" href="/tests"><h3>Hardware tests</h3><p>Try the speaker, screen, LEDs, and servo sweep.</p></a>
<a class="card" href="/servo"><h3>Servo control</h3><p>Move individual servos to any angle.</p></a>
<a class="card" href="/config"><h3>Config</h3><p>Hostname, sleep timeout, and speaker volume (saved in flash).</p></a>
</div>
<a class="github-link" href="https://github.com/jamro/tiny-engineer" target="_blank" rel="noopener">Full docs &amp; build guide on GitHub &rarr;</a>
</section>

<section id="view-api" class="view">
<h2 class="page-title">API reference</h2>
<p class="page-desc">JSON HTTP API on port 80. Test routes are POST only.</p>
<table>
<tr><th>Method</th><th>Path</th><th>Description</th></tr>
<tr><td>GET</td><td><code>/</code></td><td>Web control panel</td></tr>
<tr><td>GET</td><td><code>/animations</code></td><td>Animations page</td></tr>
<tr><td>GET</td><td><code>/tests</code></td><td>Hardware tests page</td></tr>
<tr><td>GET</td><td><code>/servo</code></td><td>Servo control page</td></tr>
<tr><td>GET</td><td><code>/config</code></td><td>Config page</td></tr>
<tr><td>GET</td><td><code>/api</code></td><td>This API reference</td></tr>
<tr><td>GET</td><td><code>/health</code></td><td>Health JSON, no side effects</td></tr>
<tr><td>GET</td><td><code>/settings</code></td><td>Persistent settings</td></tr>
<tr><td>POST</td><td><code>/settings</code></td><td>Update settings (see parameters below)</td></tr>
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
<tr><td><code>sleep_timeout</code></td><td>integer</td><td>5&ndash;3600 seconds</td></tr>
<tr><td><code>hostname</code></td><td>string</td><td>1&ndash;31 chars, letters/digits/hyphen</td></tr>
<tr><td><code>volume</code></td><td>integer</td><td>0&ndash;100 percent (speaker gain)</td></tr>
</table>
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
<p class="page-desc">Set the robot&rsquo;s current gesture. Looping animations repeat until changed.</p>
<div id="anim-current" class="badge">Current: <strong>&hellip;</strong></div>
<div class="btn-grid" id="anim-buttons">
<button class="btn" data-anim="none"><span class="btn-title">Idle</span><span class="btn-hint">Rest pose</span></button>
<button class="btn" data-anim="typing"><span class="btn-title">Typing</span><span class="btn-hint">Loop</span></button>
<button class="btn" data-anim="reading"><span class="btn-title">Reading</span><span class="btn-hint">Loop</span></button>
<button class="btn" data-anim="thinking"><span class="btn-title">Thinking</span><span class="btn-hint">Loop</span></button>
<button class="btn" data-anim="ring"><span class="btn-title">Bell</span><span class="btn-hint">One-shot</span></button>
<button class="btn" data-anim="welcome"><span class="btn-title">Welcome</span><span class="btn-hint">One-shot</span></button>
<button class="btn" data-anim="attention"><span class="btn-title">Attention</span><span class="btn-hint">+ audio</span></button>
<button class="btn" data-anim="error"><span class="btn-title">Error</span><span class="btn-hint">+ audio</span></button>
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
<h2 class="page-title">Config</h2>
<p class="page-desc">Settings are stored in flash and survive reboot. Sleep timeout and volume apply immediately; hostname needs a reboot.</p>
<form id="config-form">
<div class="form-group">
<label for="config-hostname">Hostname</label>
<input type="text" id="config-hostname" maxlength="31" pattern="[A-Za-z0-9]([A-Za-z0-9-]{0,29}[A-Za-z0-9])?" required>
<p class="hint">mDNS name without .local (letters, digits, hyphen). Default: tiny-engineer</p>
</div>
<div class="form-group">
<label for="config-sleep">Sleep timeout (seconds)</label>
<input type="number" id="config-sleep" min="5" max="3600" step="1" required>
<p class="hint">Idle time before OLED blanks when animation is none. Default: 60</p>
</div>
<div class="form-group">
<label for="config-volume">Volume <span id="config-volume-label">70%</span></label>
<div class="range-row">
<input type="range" id="config-volume-slider" min="0" max="100" value="70">
<input type="number" id="config-volume" min="0" max="100" step="1" value="70" required>
</div>
<p class="hint">Speaker gain for tones and WAV playback. Default: 70</p>
</div>
<button type="submit" class="btn btn-primary">Save settings</button>
</form>
</section>

<footer>
<a href="https://github.com/jamro/tiny-engineer" target="_blank" rel="noopener">github.com/jamro/tiny-engineer</a>
</footer>

<script>
var SERVO_RANGES=[[60,130],[40,130],[50,140],[40,130],[40,130]];
var busy=false;
var healthTimer=null;
var statusEl=document.getElementById("status");
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
function showPage(path){
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
  return fetch(path,{method:"POST"}).then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
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
  fetch("/anim").then(function(r){return r.json();}).then(function(j){
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
  fetch("/health").then(function(r){return r.json();}).then(function(j){
    var el=document.getElementById("health-info");
    if(!j.ok){el.textContent="Could not load status.";return;}
    var ip=j.wifi&&j.wifi.connected?j.wifi.ip:"offline";
    var heapPct=j.heap_size?Math.round((1-j.free_heap/j.heap_size)*100):0;
    el.textContent="IP: "+ip+" \u00b7 Uptime: "+formatUptime(j.uptime_ms)+" \u00b7 Heap: "+heapPct+"% used ("+formatBytes(j.free_heap)+" free)";
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
  fetch("/test/servo?index="+idx+"&angle="+angle,{method:"POST"})
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
document.getElementById("config-volume-slider").addEventListener("input",function(){
  setConfigVolume(this.value);
});
document.getElementById("config-volume").addEventListener("input",function(){
  setConfigVolume(this.value);
});
function loadSettings(){
  fetch("/settings").then(function(r){return r.json();}).then(function(j){
    if(!j.ok)return;
    document.getElementById("config-hostname").value=j.hostname||"";
    document.getElementById("config-sleep").value=j.sleep_timeout;
    setConfigVolume(j.volume!=null?j.volume:70);
  }).catch(function(){});
}
document.getElementById("config-form").addEventListener("submit",function(e){
  e.preventDefault();
  if(busy)return;
  var host=document.getElementById("config-hostname").value.trim();
  var sleep=document.getElementById("config-sleep").value;
  var volume=document.getElementById("config-volume").value;
  setBusy(true);
  setStatus("Saving\u2026","loading");
  fetch("/settings?sleep_timeout="+encodeURIComponent(sleep)+"&hostname="+encodeURIComponent(host)+"&volume="+encodeURIComponent(volume),{method:"POST"})
  .then(function(r){return r.json().then(function(j){return{ok:r.ok,data:j};});})
  .then(function(res){
    if(res.ok&&res.data.ok!==false){
      document.getElementById("config-hostname").value=res.data.hostname||host;
      document.getElementById("config-sleep").value=res.data.sleep_timeout;
      setConfigVolume(res.data.volume!=null?res.data.volume:volume);
      if(res.data.reboot_required){
        setStatus("Saved. Hostname applies after reboot (RESET or power cycle).","ok");
      }else{
        setStatus("Settings saved.","ok");
      }
    }else{
      setStatus(res.data.error||"Save failed","err");
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
    fetch("/anim?name="+name,{method:"POST"})
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
document.addEventListener("DOMContentLoaded",function(){
  showPage(location.pathname);
  updateServoHint();
});
</script>
</body>
</html>
)html";

}  // namespace

void sendIndexPage(WebServer& server) {
  touchApiActivity();
  httpSendHtml(server, 200, INDEX_HTML);
}
