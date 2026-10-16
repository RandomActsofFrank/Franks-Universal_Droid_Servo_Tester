#pragma once
#include <Arduino.h>

// Dark UDC mockup UI for the Servo & LED Tester (served from PROGMEM).

static const char kWebUiHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SERVO &amp; LED TESTER</title>
<link rel="icon" type="image/png" href="/favicon.png">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@700;800&family=Exo+2:wght@300;400&display=swap" rel="stylesheet">
<style>
:root{
  --bg:#070b14;--bg2:#0c1424;--panel:#121a2c;--panel2:#182338;--border:#2a3650;
  --text:#e8eef8;--muted:#8b9bb4;--sub:#8A9FB1;--cyan:#2fd0e8;--cyan-dim:#1a8fa3;
  --white:#f5f7fb;--danger:#ff6b7a;--radius:12px;
}
*{box-sizing:border-box}
html,body{margin:0;padding:0;background:
  radial-gradient(1200px 500px at 10% -10%,rgba(47,208,232,.12),transparent 55%),
  radial-gradient(900px 400px at 90% 0%,rgba(60,90,180,.14),transparent 50%),
  linear-gradient(180deg,var(--bg2),var(--bg));
  color:var(--text);font-family:"Segoe UI",ui-sans-serif,system-ui,sans-serif;min-height:100%}
body{max-width:1100px;margin:0 auto;padding:1.25rem 1.1rem 2.5rem}
.header{display:flex;align-items:center;justify-content:space-between;gap:1.25rem;margin-bottom:1rem}
.titles{min-width:0;flex:1 1 auto}
.titles h1{margin:0;font-family:"Orbitron","Michroma","Segoe UI",sans-serif;font-size:clamp(1.35rem,3.2vw,2rem);
  font-weight:800;letter-spacing:.04em;text-transform:uppercase;line-height:1.05;
  background:linear-gradient(180deg,#f7f9fc 0%,#d7dee8 55%,#b8c2d0 100%);
  -webkit-background-clip:text;background-clip:text;color:transparent;
  filter:drop-shadow(0 2px 2px rgba(0,0,0,.55)) drop-shadow(0 1px 0 rgba(0,0,0,.35))}
.titles .sub{margin:.45rem 0 0;font-family:"Exo 2","Segoe UI",sans-serif;font-weight:300;
  color:var(--sub);font-size:clamp(.62rem,1.5vw,.78rem);letter-spacing:.22em;text-transform:uppercase}
.banner{flex:0 0 auto;height:78px;width:auto;max-width:min(330px,46vw);object-fit:contain;
  display:block;filter:drop-shadow(0 4px 12px rgba(0,0,0,.35))}
@media(max-width:520px){.banner{height:60px;max-width:min(200px,48vw)}}
.footer{display:flex;align-items:center;justify-content:space-between;gap:.75rem;flex-wrap:wrap;
  margin-top:1rem;color:var(--muted);font-size:.72rem;letter-spacing:.04em}
.footer .ver{white-space:nowrap}
.footer .status{margin:0;flex:1 1 auto;min-width:12rem;text-align:left;font-size:.72rem}
.servo-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:.9rem;margin-bottom:1.1rem}
@media(max-width:860px){.servo-grid{grid-template-columns:repeat(2,1fr)}}
@media(max-width:520px){.servo-grid{grid-template-columns:1fr}}
.card{background:linear-gradient(180deg,var(--panel2),var(--panel));border:1px solid var(--border);
  border-radius:var(--radius);padding:1rem 1.05rem 1.05rem;box-shadow:0 10px 28px rgba(0,0,0,.28)}
.card-head{display:flex;align-items:center;justify-content:space-between;gap:.5rem;margin-bottom:.55rem}
.card-title{font-size:.95rem;font-weight:700;letter-spacing:.04em}
.card-title .gpio{font-size:.72rem;font-weight:600;color:var(--muted);letter-spacing:.02em;margin-left:.15rem}
.btn-reset{display:inline-flex;align-items:center;gap:.35rem;padding:.32rem .7rem;border-radius:999px;
  border:1px solid #3d4f6e;background:transparent;color:var(--text);font:inherit;font-size:.72rem;
  font-weight:600;letter-spacing:.04em;cursor:pointer;text-transform:uppercase}
.btn-reset:hover{border-color:var(--cyan);color:var(--cyan)}
.btn-reset svg{width:13px;height:13px}
.pos-row{display:flex;align-items:baseline;gap:.65rem;margin:.15rem 0 .55rem;
  font-variant-numeric:tabular-nums}
.pct{font-size:1.85rem;font-weight:800;color:var(--cyan);line-height:1}
.us{color:var(--white);font-size:.92rem;font-weight:600}
input[type=range]{width:100%;accent-color:var(--cyan);margin:0}
.scale{display:flex;justify-content:space-between;color:var(--muted);font-size:.68rem;
  letter-spacing:.04em;margin-top:.35rem;font-weight:600}
.sweep-row{display:grid;grid-template-columns:repeat(3,1fr);gap:.4rem;margin-top:.65rem}
.btn-tool{padding:.38rem .35rem;border-radius:8px;border:1px solid var(--border);background:#0d1422;
  color:var(--text);font:inherit;font-size:.68rem;font-weight:700;letter-spacing:.03em;
  text-transform:uppercase;cursor:pointer}
.btn-tool:hover{border-color:var(--cyan);color:var(--cyan)}
.btn-tool.active{border-color:var(--cyan);color:var(--cyan);background:rgba(47,208,232,.1)}
.btn-tool .sub{display:block;margin-top:.12rem;font-size:.62rem;font-weight:600;color:var(--muted);
  text-transform:none;letter-spacing:.02em}
.speed-field input[type=range]{margin-top:.15rem}
.check-field{justify-content:flex-end;padding-bottom:.15rem}
.check-field .check{display:flex;align-items:center;gap:.55rem;padding:.55rem .65rem;border-radius:8px;
  border:1px solid var(--border);background:#0d1422;min-height:2.4rem}
.check-field input[type=checkbox]{width:1.05rem;height:1.05rem;accent-color:var(--cyan);margin:0;flex:0 0 auto}
.check-field .check span{font-size:.78rem;letter-spacing:.04em;color:var(--text);font-weight:700;
  text-transform:uppercase}
.led-panel{background:linear-gradient(180deg,#152038,#101826);border:1px solid var(--border);
  border-radius:var(--radius);padding:1.15rem 1.2rem 1.25rem;box-shadow:0 12px 32px rgba(0,0,0,.3)}
.led-panel h2{margin:0 0 1rem;font-size:.95rem;letter-spacing:.12em;font-weight:800}
.led-panel h2 .gpio{font-size:.78rem;font-weight:600;color:var(--muted);letter-spacing:.04em}
.led-controls{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:.85rem .95rem;margin-bottom:1rem}
@media(max-width:860px){.led-controls{grid-template-columns:repeat(2,1fr)}}
@media(max-width:520px){.led-controls{grid-template-columns:1fr}}
label.field{display:flex;flex-direction:column;gap:.35rem;font-size:.72rem;color:var(--muted);
  letter-spacing:.06em;font-weight:700;text-transform:uppercase}
input[type=number],select{font:inherit;padding:.5rem .65rem;border-radius:8px;border:1px solid var(--border);
  background:#0d1422;color:var(--text)}
input[type=number]:focus,select:focus{outline:2px solid rgba(47,208,232,.28);border-color:var(--cyan-dim)}
.color-block{grid-column:span 1}
.swatch-row{display:flex;align-items:center;gap:.65rem}
.swatch{width:36px;height:36px;border-radius:8px;border:1px solid var(--border);flex:0 0 auto;
  box-shadow:inset 0 0 0 1px rgba(255,255,255,.06)}
.rgb-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:.4rem;margin-top:.45rem;flex:1}
.rgb-grid label{font-size:.62rem;color:var(--muted);letter-spacing:.04em;display:flex;flex-direction:column;gap:.2rem}
.rgb-grid input{min-width:0;padding:.35rem .4rem;font-size:.8rem}
.strand-wrap{margin-top:.35rem}
.strand-label{font-size:.68rem;color:var(--muted);letter-spacing:.08em;font-weight:700;
  text-transform:uppercase;margin-bottom:.45rem}
.strand{display:flex;gap:2px;height:22px;padding:4px;border-radius:8px;background:#0a101c;
  border:1px solid var(--border);overflow:hidden}
.strand .px{flex:1;min-width:2px;border-radius:2px;background:#1a2233;transition:background .08s linear}
.status{color:var(--muted)}
.status.ok{color:var(--cyan)}
.status.err{color:var(--danger)}
</style>
</head>
<body>
<header class="header">
  <div class="titles">
    <h1>SERVO &amp; LED TESTER</h1>
    <div class="sub">REAL-TIME SERVO AND NEOPIXEL CONTROL</div>
  </div>
  <img class="banner" src="/banner.png" alt="Universal Droid Control" width="400" height="148">
</header>

<section class="servo-grid" id="servos"></section>

<section class="led-panel">
  <h2 id="ledPanelTitle">WS2811 / NEOPIXEL TESTER</h2>
  <div class="led-controls">
    <label class="field">LED count
      <input type="number" id="ledCount" min="1" max="300" value="60">
    </label>
    <label class="field">Brightness %
      <input type="number" id="ledBright" min="0" max="100" value="80">
    </label>
    <label class="field">Color order
      <select id="ledOrder">
        <option value="RGB">RGB</option>
        <option value="GRB" selected>GRB</option>
        <option value="RBG">RBG</option>
        <option value="BRG">BRG</option>
        <option value="GBR">GBR</option>
        <option value="BGR">BGR</option>
      </select>
    </label>
    <label class="field check-field">Invert Direction
      <div class="check"><input type="checkbox" id="ledInvert" aria-label="Invert Direction"></div>
    </label>
    <label class="field">Effect
      <select id="ledEffect"></select>
    </label>
    <div class="color-block">
      <label class="field">Color 1</label>
      <div class="swatch-row">
        <div class="swatch" id="sw1"></div>
        <div class="rgb-grid">
          <label>R<input type="number" id="c1r" min="0" max="255" value="255"></label>
          <label>G<input type="number" id="c1g" min="0" max="255" value="0"></label>
          <label>B<input type="number" id="c1b" min="0" max="255" value="0"></label>
        </div>
      </div>
    </div>
    <div class="color-block">
      <label class="field">Color 2</label>
      <div class="swatch-row">
        <div class="swatch" id="sw2"></div>
        <div class="rgb-grid">
          <label>R<input type="number" id="c2r" min="0" max="255" value="0"></label>
          <label>G<input type="number" id="c2g" min="0" max="255" value="0"></label>
          <label>B<input type="number" id="c2b" min="0" max="255" value="255"></label>
        </div>
      </div>
    </div>
    <label class="field speed-field">Speed <span id="ledSpeedVal">50%</span>
      <input type="range" id="ledSpeed" min="0" max="100" value="50">
    </label>
  </div>
  <div class="strand-wrap">
    <div class="strand-label">LED strand visualizer</div>
    <div class="strand" id="strand"></div>
  </div>
</section>
<footer class="footer">
  <span class="status" id="status">Connecting…</span>
  <span class="ver">UDC Servo &amp; LED Tester · v2026.10-42.07</span>
</footer>

<script>
const EFFECTS=[
  {v:"solid",l:"Solid"},{v:"rgb",l:"RGB step"},{v:"rainbow_chase",l:"Rainbow chase"},
  {v:"chase",l:"Chase"},{v:"dj",l:"DJ Light"},{v:"fade",l:"Fade"},{v:"flow",l:"Flow Stripe"},
  {v:"gradient",l:"Gradient"},{v:"pacifica",l:"Pacifica"},{v:"rainbow",l:"Rainbow"},
  {v:"twinkle",l:"Twinkle"},{v:"fire",l:"Fire"},{v:"breathe",l:"Breathe"},{v:"theater",l:"Theater"},
  {v:"comet",l:"Comet"},{v:"wipe",l:"Color wipe"},{v:"scanner",l:"Scanner"},{v:"police",l:"Police bar"}
];
const LS_KEY="udcServoLedTester.v2";
const SERVO_DEBOUNCE_MS=35;
const LED_DEBOUNCE_MS=65;
let servoPct=[50,50,50,50,50,50];
let servoMin=[0,0,0,0,0,0];
let servoMax=[100,100,100,100,100,100];
let servoSweeping=[false,false,false,false,false,false];
let servoPins=[0,0,0,0,0,0];
let ledGpio=0;
let ledState={count:60,brightness:80,speed:50,invert:false,effect:"solid",colorOrder:"GRB",
  color1:{r:255,g:0,b:0},color2:{r:0,g:0,b:255}};
let servoTimers={}; let ledTimer=null; let syncing=false; let vizT0=performance.now();

function setLedPanelTitle(){
  const el=document.getElementById("ledPanelTitle");
  if(!el)return;
  el.innerHTML=ledGpio>0
    ? 'WS2811 / NEOPIXEL TESTER <span class="gpio">('+ledGpio+')</span>'
    : "WS2811 / NEOPIXEL TESTER";
}

function clamp(n,a,b){n=Number(n);if(!isFinite(n))return a;return Math.max(a,Math.min(b,Math.round(n)));}
function pctToUs(p){return 500+Math.round(clamp(p,0,100)*2000/100);}
function setStatus(msg,cls){const el=document.getElementById("status");if(!el)return;el.textContent=msg;el.className="status"+(cls?" "+cls:"");}

function saveLocal(){
  try{
    localStorage.setItem(LS_KEY,JSON.stringify({
      servos:servoPct,servoMin:servoMin,servoMax:servoMax,led:ledState
    }));
  }catch(e){}
}
function loadLocal(){
  try{
    const raw=localStorage.getItem(LS_KEY);if(!raw)return;
    const o=JSON.parse(raw);
    if(Array.isArray(o.servos)&&o.servos.length===6){
      servoPct=o.servos.map(v=>clamp(v,0,100));
    }
    if(Array.isArray(o.servoMin)&&o.servoMin.length===6){
      servoMin=o.servoMin.map(v=>clamp(v,0,100));
    }
    if(Array.isArray(o.servoMax)&&o.servoMax.length===6){
      servoMax=o.servoMax.map(v=>clamp(v,0,100));
    }
    if(o.led){
      ledState={
        count:clamp(o.led.count,1,300),
        brightness:clamp(o.led.brightness,0,100),
        speed:clamp(o.led.speed!=null?o.led.speed:50,0,100),
        invert:!!o.led.invert,
        effect:String(o.led.effect||"solid"),
        colorOrder:String(o.led.colorOrder||"GRB").toUpperCase(),
        color1:{r:clamp(o.led.color1&&o.led.color1.r,0,255),g:clamp(o.led.color1&&o.led.color1.g,0,255),b:clamp(o.led.color1&&o.led.color1.b,0,255)},
        color2:{r:clamp(o.led.color2&&o.led.color2.r,0,255),g:clamp(o.led.color2&&o.led.color2.g,0,255),b:clamp(o.led.color2&&o.led.color2.b,0,255)}
      };
    }
  }catch(e){}
}

async function postJson(url,body){
  const r=await fetch(url,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  if(!r.ok)throw new Error("HTTP "+r.status);
  return r;
}

function servoLabel(id){
  const gpio=servoPins[id-1];
  if(gpio>0){return "SERVO "+id+' <span class="gpio">('+gpio+')</span>';}
  return "SERVO "+id;
}
function updateLimitLabels(id){
  const i=id-1;
  const minEl=document.getElementById("minLbl"+id);
  const maxEl=document.getElementById("maxLbl"+id);
  const sw=document.getElementById("sweep"+id);
  if(minEl)minEl.textContent=servoMin[i]+"%";
  if(maxEl)maxEl.textContent=servoMax[i]+"%";
  if(sw){
    sw.classList.toggle("active",!!servoSweeping[i]);
    sw.textContent=servoSweeping[i]?"Sweeping…":"Sweep";
  }
}
function renderServos(){
  const root=document.getElementById("servos");
  root.innerHTML="";
  for(let i=0;i<6;i++){
    const id=i+1; const p=servoPct[i]; const us=pctToUs(p);
    const card=document.createElement("div"); card.className="card";
    card.innerHTML=
      '<div class="card-head"><div class="card-title">'+servoLabel(id)+'</div>'+
      '<button type="button" class="btn-reset" data-reset="'+id+'">'+
      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">'+
      '<path d="M3 12a9 9 0 1 0 3-6.7"/><polyline points="3 4 3 10 9 10"/></svg>Reset</button></div>'+
      '<div class="pos-row"><div class="pct" id="pct'+id+'">'+p+'%</div>'+
      '<div class="us" id="us'+id+'">'+us+' μs</div></div>'+
      '<input type="range" min="0" max="100" value="'+p+'" id="sl'+id+'" data-id="'+id+'">'+
      '<div class="scale"><span>0%</span><span>50%</span><span>100%</span></div>'+
      '<div class="sweep-row">'+
      '<button type="button" class="btn-tool" data-setmin="'+id+'">Set Min<span class="sub" id="minLbl'+id+'">'+servoMin[i]+'%</span></button>'+
      '<button type="button" class="btn-tool" data-setmax="'+id+'">Set Max<span class="sub" id="maxLbl'+id+'">'+servoMax[i]+'%</span></button>'+
      '<button type="button" class="btn-tool'+(servoSweeping[i]?' active':'')+'" data-sweep="'+id+'" id="sweep'+id+'">'+(servoSweeping[i]?'Sweeping…':'Sweep')+'</button>'+
      '</div>';
    root.appendChild(card);
  }
  root.querySelectorAll("input[type=range]").forEach(sl=>{
    sl.addEventListener("input",()=>{
      const id=+sl.dataset.id; const p=+sl.value;
      servoSweeping[id-1]=false;
      updateLimitLabels(id);
      applyServoLocal(id,p);
      queueServoPost(id,p);
    });
  });
  root.querySelectorAll("[data-reset]").forEach(btn=>{
    btn.addEventListener("click",()=>{
      const id=+btn.dataset.reset;
      servoSweeping[id-1]=false;
      updateLimitLabels(id);
      applyServoLocal(id,50);
      document.getElementById("sl"+id).value=50;
      queueServoReset(id);
    });
  });
  root.querySelectorAll("[data-setmin]").forEach(btn=>{
    btn.addEventListener("click",()=>setServoLimit(+btn.dataset.setmin,"min"));
  });
  root.querySelectorAll("[data-setmax]").forEach(btn=>{
    btn.addEventListener("click",()=>setServoLimit(+btn.dataset.setmax,"max"));
  });
  root.querySelectorAll("[data-sweep]").forEach(btn=>{
    btn.addEventListener("click",()=>toggleSweep(+btn.dataset.sweep));
  });
}

function applyServoLocal(id,p){
  p=clamp(p,0,100); servoPct[id-1]=p;
  const pctEl=document.getElementById("pct"+id);
  const usEl=document.getElementById("us"+id);
  const sl=document.getElementById("sl"+id);
  if(pctEl)pctEl.textContent=p+"%";
  if(usEl)usEl.textContent=pctToUs(p)+" μs";
  if(sl && document.activeElement!==sl)sl.value=p;
  saveLocal();
}
function queueServoPost(id,p){
  clearTimeout(servoTimers["s"+id]);
  servoTimers["s"+id]=setTimeout(()=>{
    postJson("/api/servo",{id:id,percent:p}).then(()=>setStatus("Servo "+id+" → "+p+"%","ok"))
      .catch(e=>setStatus("Servo error: "+e.message,"err"));
  },SERVO_DEBOUNCE_MS);
}
function queueServoReset(id){
  clearTimeout(servoTimers["s"+id]);
  servoTimers["s"+id]=setTimeout(()=>{
    postJson("/api/servo/reset",{id:id}).then(()=>{
      servoSweeping[id-1]=false; updateLimitLabels(id);
      setStatus("Servo "+id+" reset 50% / 1500 μs","ok");
    }).catch(e=>setStatus("Reset error: "+e.message,"err"));
  },SERVO_DEBOUNCE_MS);
}
async function setServoLimit(id,which){
  const body={id:id};
  if(which==="min")body.setMin=true; else body.setMax=true;
  try{
    const r=await postJson("/api/servo/limits",body);
    const j=await r.json();
    if(typeof j.minPercent==="number")servoMin[id-1]=clamp(j.minPercent,0,100);
    if(typeof j.maxPercent==="number")servoMax[id-1]=clamp(j.maxPercent,0,100);
    if(typeof j.sweeping==="boolean")servoSweeping[id-1]=j.sweeping;
    updateLimitLabels(id); saveLocal();
    setStatus("Servo "+id+" "+which+" → "+(which==="min"?servoMin[id-1]:servoMax[id-1])+"%","ok");
  }catch(e){setStatus("Limit error: "+e.message,"err");}
}
async function toggleSweep(id){
  try{
    const r=await postJson("/api/servo/sweep",{id:id});
    const j=await r.json();
    servoSweeping[id-1]=!!j.sweeping;
    if(typeof j.minPercent==="number")servoMin[id-1]=clamp(j.minPercent,0,100);
    if(typeof j.maxPercent==="number")servoMax[id-1]=clamp(j.maxPercent,0,100);
    if(typeof j.percent==="number")applyServoLocal(id,clamp(j.percent,0,100));
    updateLimitLabels(id); saveLocal();
    if(j.sweeping){
      setStatus("Servo "+id+" → "+(j.targetPercent!=null?j.targetPercent+"%":"end"),"ok");
    }else{
      setStatus("Servo "+id+" sweep settled/cancelled","ok");
    }
  }catch(e){setStatus("Sweep error: "+e.message,"err");}
}

function fillEffects(){
  const sel=document.getElementById("ledEffect");
  sel.innerHTML="";
  EFFECTS.forEach(e=>{
    const o=document.createElement("option"); o.value=e.v; o.textContent=e.l; sel.appendChild(o);
  });
}

function readLedFromDom(){
  const speed=clamp(document.getElementById("ledSpeed").value,0,100);
  document.getElementById("ledSpeedVal").textContent=speed+"%";
  ledState={
    count:clamp(document.getElementById("ledCount").value,1,300),
    brightness:clamp(document.getElementById("ledBright").value,0,100),
    speed:speed,
    invert:!!document.getElementById("ledInvert").checked,
    effect:document.getElementById("ledEffect").value,
    colorOrder:document.getElementById("ledOrder").value,
    color1:{
      r:clamp(document.getElementById("c1r").value,0,255),
      g:clamp(document.getElementById("c1g").value,0,255),
      b:clamp(document.getElementById("c1b").value,0,255)
    },
    color2:{
      r:clamp(document.getElementById("c2r").value,0,255),
      g:clamp(document.getElementById("c2g").value,0,255),
      b:clamp(document.getElementById("c2b").value,0,255)
    }
  };
  updateSwatches();
  saveLocal();
}
function writeLedToDom(){
  syncing=true;
  document.getElementById("ledCount").value=ledState.count;
  document.getElementById("ledBright").value=ledState.brightness;
  document.getElementById("ledSpeed").value=ledState.speed;
  document.getElementById("ledSpeedVal").textContent=ledState.speed+"%";
  document.getElementById("ledInvert").checked=!!ledState.invert;
  document.getElementById("ledEffect").value=ledState.effect;
  document.getElementById("ledOrder").value=ledState.colorOrder;
  document.getElementById("c1r").value=ledState.color1.r;
  document.getElementById("c1g").value=ledState.color1.g;
  document.getElementById("c1b").value=ledState.color1.b;
  document.getElementById("c2r").value=ledState.color2.r;
  document.getElementById("c2g").value=ledState.color2.g;
  document.getElementById("c2b").value=ledState.color2.b;
  updateSwatches();
  syncing=false;
}
function updateSwatches(){
  document.getElementById("sw1").style.background="rgb("+ledState.color1.r+","+ledState.color1.g+","+ledState.color1.b+")";
  document.getElementById("sw2").style.background="rgb("+ledState.color2.r+","+ledState.color2.g+","+ledState.color2.b+")";
}
function queueLedPost(){
  if(syncing)return;
  readLedFromDom();
  clearTimeout(ledTimer);
  ledTimer=setTimeout(()=>{
    postJson("/api/led",ledState).then(()=>setStatus("LED updated","ok"))
      .catch(e=>setStatus("LED error: "+e.message,"err"));
  },LED_DEBOUNCE_MS);
}

function buildStrand(){
  const n=Math.min(ledState.count,72);
  const root=document.getElementById("strand");
  if(root.childElementCount!==n){
    root.innerHTML="";
    for(let i=0;i<n;i++){const d=document.createElement("div");d.className="px";root.appendChild(d);}
  }
}
function lerp(a,b,t){return Math.round(a+(b-a)*t);}
function wheel(pos){
  pos=255-pos;
  if(pos<85)return[pos*3,255-pos*3,0];
  if(pos<170){pos-=85;return[255-pos*3,0,pos*3];}
  pos-=170;return[0,pos*3,255-pos*3];
}
/* Display-only perceptual gain for strand preview (API brightness stays linear %).
   gain = (pct/100)^0.4 → ~10%→40%, ~20%→53%, ~75%→89% (high end flatter). */
function vizBrightGain(pct){
  const x=Math.max(0,Math.min(100,+pct||0))/100;
  return Math.pow(x,0.4);
}
function paintViz(){
  buildStrand();
  const px=document.querySelectorAll("#strand .px");
  const n=px.length; if(!n)return;
  const spd=0.35+((ledState.speed||50)/100)*1.65;
  const t=((performance.now()-vizT0)/1000)*spd;
  const c1=ledState.color1,c2=ledState.color2;
  const br=vizBrightGain(ledState.brightness);
  const fx=ledState.effect;
  for(let i=0;i<n;i++){
    let r=0,g=0,b=0;
    const vi=ledState.invert?(n-1-i):i;
    const u=vi/(n-1||1);
    if(fx==="solid"){r=c1.r;g=c1.g;b=c1.b;}
    else if(fx==="rgb"){const s=Math.floor(t*1.5)%3; r=s===0?255:0;g=s===1?255:0;b=s===2?255:0;}
    else if(fx==="rainbow"||fx==="rainbow_chase"){const w=wheel(((vi*8)+t*80)&255);r=w[0];g=w[1];b=w[2];}
    else if(fx==="chase"||fx==="comet"||fx==="wipe"){
      const head=((t*12)%n)|0; const d=(vi-head+n)%n;
      if(d<Math.max(2,n/8)){const f=1-d/Math.max(2,n/8);r=lerp(c2.r,c1.r,f);g=lerp(c2.g,c1.g,f);b=lerp(c2.b,c1.b,f);}
      else{r=c2.r;g=c2.g;b=c2.b;}
    }
    else if(fx==="gradient"||fx==="pacifica"){const ph=(u+t*0.25)%1;r=lerp(c1.r,c2.r,ph);g=lerp(c1.g,c2.g,ph);b=lerp(c1.b,c2.b,ph);}
    else if(fx==="fade"||fx==="breathe"){const f=0.35+0.65*(0.5+0.5*Math.sin(t*2.2));r=c1.r*f;g=c1.g*f;b=c1.b*f;}
    else if(fx==="twinkle"){const on=((Math.sin(vi*1.7+t*6)+1)*0.5)>0.72;r=on?c1.r:0;g=on?c1.g:0;b=on?c1.b:0;}
    else if(fx==="fire"){const f=0.4+0.6*Math.random();r=255*f;g=80*f;b=10*f;}
    else if(fx==="dj"){const s=Math.floor(t*4+vi*0.1)%6; const cols=[[255,0,180],[0,220,255],[255,200,0],[120,0,255],[0,255,120],[255,40,40]]; const c=cols[s];r=c[0];g=c[1];b=c[2];}
    else if(fx==="flow"||fx==="theater"){const band=Math.sin((vi*0.45)-t*5)>0;r=band?c1.r:0;g=band?c1.g:0;b=band?c1.b:0;}
    else if(fx==="scanner"){const pos=((0.5+0.5*Math.sin(t*2.5))*(n-1)); const d=Math.abs(vi-pos); const f=Math.max(0,1-d/3);r=c1.r*f;g=c1.g*f;b=c1.b*f;}
    else if(fx==="police"){const left=vi<n/2; const on=((t*3)|0)%2===0; if(left){r=on?c1.r:0;g=on?c1.g:0;b=on?c1.b:0;}else{r=on?0:c2.r;g=on?0:c2.g;b=on?0:c2.b;}}
    else{r=c1.r;g=c1.g;b=c1.b;}
    px[i].style.background="rgb("+Math.round(r*br)+","+Math.round(g*br)+","+Math.round(b*br)+")";
  }
  requestAnimationFrame(paintViz);
}

async function syncFromDevice(){
  try{
    const r=await fetch("/api/state",{cache:"no-store"});
    if(!r.ok)throw new Error("HTTP "+r.status);
    const st=await r.json();
    let pinsChanged=false;
    if(Array.isArray(st.servos)){
      st.servos.forEach(s=>{
        const id=clamp(s.id,1,6); const p=clamp(s.percent,0,100);
        servoPct[id-1]=p;
        if(typeof s.minPercent==="number")servoMin[id-1]=clamp(s.minPercent,0,100);
        if(typeof s.maxPercent==="number")servoMax[id-1]=clamp(s.maxPercent,0,100);
        if(typeof s.sweeping==="boolean")servoSweeping[id-1]=s.sweeping;
        if(typeof s.gpio==="number" && s.gpio>0 && servoPins[id-1]!==s.gpio){
          servoPins[id-1]=s.gpio; pinsChanged=true;
        }
      });
    }
    if(pinsChanged){renderServos();}
    else if(Array.isArray(st.servos)){
      st.servos.forEach(s=>{
        const id=clamp(s.id,1,6); const p=clamp(s.percent,0,100);
        applyServoLocal(id,p);
        updateLimitLabels(id);
      });
    }
    if(st.led){
      if(typeof st.led.gpio==="number" && st.led.gpio>0){
        ledGpio=st.led.gpio;
        setLedPanelTitle();
      }
      ledState={
        count:clamp(st.led.count,1,300),
        brightness:clamp(st.led.brightness,0,100),
        speed:clamp(st.led.speed!=null?st.led.speed:50,0,100),
        invert:!!st.led.invert,
        effect:String(st.led.effect||"solid"),
        colorOrder:String(st.led.colorOrder||"GRB").toUpperCase(),
        color1:{r:clamp(st.led.color1&&st.led.color1.r,0,255),g:clamp(st.led.color1&&st.led.color1.g,0,255),b:clamp(st.led.color1&&st.led.color1.b,0,255)},
        color2:{r:clamp(st.led.color2&&st.led.color2.r,0,255),g:clamp(st.led.color2&&st.led.color2.g,0,255),b:clamp(st.led.color2&&st.led.color2.b,0,255)}
      };
      writeLedToDom();
    }
    saveLocal();
    setStatus("Synced from device","ok");
  }catch(e){setStatus("State sync failed: "+e.message,"err");}
}

async function softSyncServos(){
  if(!servoSweeping.some(Boolean))return;
  try{
    const r=await fetch("/api/state",{cache:"no-store"});
    if(!r.ok)return;
    const st=await r.json();
    if(!Array.isArray(st.servos))return;
    st.servos.forEach(s=>{
      const id=clamp(s.id,1,6);
      if(typeof s.percent==="number")applyServoLocal(id,clamp(s.percent,0,100));
      if(typeof s.sweeping==="boolean"){
        servoSweeping[id-1]=s.sweeping;
        updateLimitLabels(id);
      }
    });
  }catch(e){}
}

function bindLed(){
  ["ledCount","ledBright","ledSpeed","ledInvert","ledEffect","ledOrder","c1r","c1g","c1b","c2r","c2g","c2b"].forEach(id=>{
    const el=document.getElementById(id);
    el.addEventListener("input",queueLedPost);
    el.addEventListener("change",queueLedPost);
  });
}

(async function init(){
  fillEffects();
  loadLocal();
  renderServos();
  writeLedToDom();
  bindLed();
  buildStrand();
  requestAnimationFrame(paintViz);
  await syncFromDevice();
  // Push local-restored LED prefs after first sync so device matches UI if localStorage newer
  queueLedPost();
  for(let i=0;i<6;i++){
    queueServoPost(i+1,servoPct[i]);
    // Restore min/max from localStorage onto device
    postJson("/api/servo/limits",{id:i+1,minPercent:servoMin[i],maxPercent:servoMax[i]}).catch(()=>{});
  }
  setInterval(softSyncServos,180);
})();
</script>
</body>
</html>
)HTML";
