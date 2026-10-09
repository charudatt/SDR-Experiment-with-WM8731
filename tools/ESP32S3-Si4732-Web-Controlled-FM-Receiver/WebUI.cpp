#include "WebUI.h"
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

static WebServer server(80);
static Si4732Controller *gRadio = nullptr;
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Si4732 FM Receiver</title>
<style>
:root{color-scheme:dark;font-family:system-ui,sans-serif}
body{margin:0;background:#101820;color:#eaf1f7}
main{max-width:620px;margin:0 auto;padding:18px}
.card{background:#1b2a36;border:1px solid #334957;border-radius:16px;padding:18px;margin:12px 0}
h1{font-size:1.45rem;margin:0 0 8px}
.muted{color:#aabac6;font-size:.9rem}
.freq{font-size:2.5rem;font-weight:750;letter-spacing:.02em;margin:10px 0}
button,input{font:inherit;border-radius:10px;border:1px solid #587081;padding:11px;background:#263b49;color:#f3f7fa}
button{cursor:pointer;min-height:44px}
button.primary{background:#146b91;border-color:#278ab3}
.controls{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.tuneform{display:flex;gap:8px;margin:12px 0}
.tuneform input{min-width:0;flex:1}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.stat{background:#14212a;border-radius:10px;padding:10px}
.stat b{display:block;font-size:1.15rem}
#message{min-height:1.4em;color:#9bd8f2}
@media(max-width:400px){.freq{font-size:2rem}.stats{grid-template-columns:1fr 1fr}}
</style>
</head>
<body><main>
  <h1>Si4732 FM Receiver</h1>
  <div class="muted">Standalone ESP32-S3 validation · no TFT/OLED</div>
  <section class="card">
    <div class="muted">Receiver readback</div>
    <div id="freq" class="freq">---.-- MHz</div>
    <form id="tuneform" class="tuneform">
      <input id="mhz" type="number" min="64" max="108" step="0.01" value="103.90" aria-label="Frequency in MHz" required>
      <button class="primary" type="submit">SET</button>
    </form>
    <div class="controls">
      <button id="down">STEP −</button>
      <button id="up">STEP +</button>
    </div>
    <p id="message" role="status"></p>
  </section>
  <section class="card">
    <h2>Signal status</h2>
    <div class="stats">
      <div class="stat"><span class="muted">RSSI</span><b id="rssi">—</b><span class="muted">dBµV</span></div>
      <div class="stat"><span class="muted">SNR</span><b id="snr">—</b><span class="muted">dB</span></div>
      <div class="stat"><span class="muted">Channel</span><b id="valid">—</b></div>
    </div>
    <p class="muted">FM step: 100 kHz. Signal validity depends on received RF conditions and the receiver's channel-valid threshold.</p>
  </section>
  <div class="muted">The physical encoder tunes in parallel. This page polls status without reloading.</div>
</main>
<script>
const $=id=>document.getElementById(id);
async function api(path){const r=await fetch(path,{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);return r.json();}
async function refresh(){
 try{
  const s=await api('/api/status');
  $('freq').textContent=(s.readback/100).toFixed(2)+' MHz';
  $('rssi').textContent=s.rssi;
  $('snr').textContent=s.snr;
  $('valid').textContent=s.valid?'YES':'NO';
  if(document.activeElement!==$('mhz'))$('mhz').value=(s.requested/100).toFixed(2);
 }catch(e){$('message').textContent='Status request failed: '+e.message;}
}
async function tuneTo(mhz){
 const q=new URLSearchParams({mhz:String(mhz)});
 const s=await api('/api/tune?'+q.toString());
 $('message').textContent=s.ok?'Tune confirmed: '+(s.readback/100).toFixed(2)+' MHz':'Tune/readback mismatch. Requested '+(s.requested/100).toFixed(2)+' MHz, read '+(s.readback/100).toFixed(2)+' MHz.';
 await refresh();
}
$('tuneform').addEventListener('submit',async e=>{
 e.preventDefault();
 const n=Number($('mhz').value);
 if(!Number.isFinite(n)||n<64||n>108){$('message').textContent='Enter a frequency from 64.00 to 108.00 MHz.';return;}
 try{await tuneTo(Math.round(n*100)/100);}catch(err){$('message').textContent=err.message;}
});
$('down').addEventListener('click',async()=>{try{const s=await api('/api/step?delta=-1');$('message').textContent=s.ok?'Tuned to '+(s.readback/100).toFixed(2)+' MHz':'Tune failed';await refresh();}catch(e){$('message').textContent=e.message;}});
$('up').addEventListener('click',async()=>{try{const s=await api('/api/step?delta=1');$('message').textContent=s.ok?'Tuned to '+(s.readback/100).toFixed(2)+' MHz':'Tune failed';await refresh();}catch(e){$('message').textContent=e.message;}});
refresh();setInterval(refresh,1200);
</script>
</body></html>
)HTML";

static void sendJsonStatus(bool ok, const char *message = "") {
  if (!gRadio) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"radio unavailable\"}");
    return;
  }
  gRadio->refreshSignalQuality();
  char json[256];
  snprintf(json, sizeof(json),
           "{\"ok\":%s,\"message\":\"%s\",\"requested\":%u,\"readback\":%u,\"rssi\":%u,\"snr\":%u,\"valid\":%s,\"ready\":%s}",
           ok ? "true" : "false", message,
           gRadio->requestedFrequency(), gRadio->frequency(),
           gRadio->rssi(), gRadio->snr(),
           gRadio->validChannel() ? "true" : "false",
           gRadio->ready() ? "true" : "false");
  server.send(200, "application/json", json);
}

void webUIBegin(Si4732Controller &radio) {
  gRadio = &radio;
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, []() {
    sendJsonStatus(gRadio && gRadio->ready());
  });
  server.on("/api/tune", HTTP_GET, []() {
    if (!gRadio || !gRadio->ready() || !server.hasArg("mhz")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing frequency or radio not ready\"}");
      return;
    }
    const double mhz = server.arg("mhz").toDouble();
    if (mhz < 64.0 || mhz > 108.0) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"frequency outside 64.00-108.00 MHz\"}");
      return;
    }
    const uint16_t f = (uint16_t)lround(mhz * 100.0);
    const bool ok = gRadio->tune(f);
    sendJsonStatus(ok, ok ? "tuned" : "readback mismatch");
  });
  server.on("/api/step", HTTP_GET, []() {
    if (!gRadio || !gRadio->ready() || !server.hasArg("delta")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing delta or radio not ready\"}");
      return;
    }
    const int delta = server.arg("delta").toInt();
    if (delta != -1 && delta != 1) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"delta must be -1 or 1\"}");
      return;
    }
    int next = (int)gRadio->requestedFrequency() + delta * SI4732_FM_STEP_10KHZ;
    if (next < SI4732_FM_MIN_10KHZ) next = SI4732_FM_MAX_10KHZ;
    if (next > SI4732_FM_MAX_10KHZ) next = SI4732_FM_MIN_10KHZ;
    const bool ok = gRadio->tune((uint16_t)next);
    sendJsonStatus(ok, ok ? "tuned" : "readback mismatch");
  });
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void webUILoop() {
  server.handleClient();
}
