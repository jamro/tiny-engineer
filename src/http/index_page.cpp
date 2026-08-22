#include "http/index_page.h"

#include "http/json.h"
#include "sleep.h"

namespace {

static const char INDEX_HTML[] = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tiny Engineer API</title>
<style>
body{font-family:system-ui,sans-serif;max-width:48rem;margin:2rem auto;padding:0 1rem;line-height:1.4}
h1,h2{font-size:1.25rem;margin:1.5rem 0 .75rem}
p{color:#444}
table{border-collapse:collapse;width:100%;margin-bottom:1rem}
th,td{border:1px solid #ccc;padding:.4rem .6rem;text-align:left;vertical-align:top}
th{background:#f4f4f4}
code{background:#f4f4f4;padding:.1rem .3rem;border-radius:.2rem}
</style>
</head>
<body>
<h1>Tiny Engineer HTTP API</h1>
<p>JSON API on port 80. Test routes are POST only.</p>
<table>
<tr><th>Method</th><th>Path</th><th>Description</th></tr>
<tr><td>GET</td><td><code>/</code></td><td>This endpoint index</td></tr>
<tr><td>GET</td><td><code>/health</code></td><td>Health JSON, no side effects</td></tr>
<tr><td>GET</td><td><code>/anim</code></td><td>Current animation name</td></tr>
<tr><td>POST</td><td><code>/anim</code></td><td>Set animation (see parameters below)</td></tr>
<tr><td>POST</td><td><code>/test/audio</code></td><td>Play tone test</td></tr>
<tr><td>POST</td><td><code>/test/audio/bell</code></td><td>Play bell WAV from LittleFS</td></tr>
<tr><td>POST</td><td><code>/test/screen</code></td><td>OLED demo</td></tr>
<tr><td>POST</td><td><code>/test/movement</code></td><td>All servos exercise</td></tr>
<tr><td>POST</td><td><code>/test/led</code></td><td>RGB LED cycle</td></tr>
<tr><td>POST</td><td><code>/test/servo</code></td><td>Move one servo (see parameters below)</td></tr>
</table>
<h2>POST /anim</h2>
<p>Query param <code>name</code>:</p>
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
</table>
<h2>POST /test/servo</h2>
<p>Query params:</p>
<table>
<tr><th>Param</th><th>Type</th><th>Range</th></tr>
<tr><td><code>index</code></td><td>integer</td><td>0&ndash;4</td></tr>
<tr><td><code>angle</code></td><td>number</td><td>0&ndash;180</td></tr>
</table>
</body>
</html>
)html";

}  // namespace

void sendIndexPage(WebServer& server) {
  touchApiActivity();
  httpSendHtml(server, 200, INDEX_HTML);
}
