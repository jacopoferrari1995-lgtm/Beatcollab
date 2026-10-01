'use strict';
/* =====================================================================
   INTERFACCIA — generatore, vista arrangiamento, trasporto, esportazione
   ===================================================================== */
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const LS={get(k,d){try{const v=localStorage.getItem(k);return v==null?d:JSON.parse(v);}catch(e){return d;}},
          set(k,v){try{localStorage.setItem(k,JSON.stringify(v));}catch(e){}}};
const rndSeed=()=>1+Math.floor(Math.random()*99998);
const fmtTime=s=>{s=Math.max(0,Math.round(s));return Math.floor(s/60)+':'+String(s%60).padStart(2,'0');};
function toast(msg){const t=$('#toast');t.textContent=msg;t.classList.add('show');clearTimeout(t._h);t._h=setTimeout(()=>t.classList.remove('show'),2000);}

const DEF={genre:'pop',mood:'rilassato',mode:'auto',key:'auto',structure:'vc',color:'auto',human:true};
const St={opts:Object.assign({},DEF,LS.get('pg_opts',{})),seeds:{h:rndSeed(),a:rndSeed(),m:rndSeed()},bpm:null,song:null,ev:[],
  mute:{},solo:null,sel:0,loop:false,muteInit:false,layout:null};
const VALID={genre:GENRES,mood:MOODS,mode:Object.assign({auto:1},MODES),structure:STRUCTS,color:{auto:1,triadi:1,colorati:1,settime:1,estesi:1}};
for(const k in VALID)if(!(St.opts[k] in VALID[k]))St.opts[k]=DEF[k];
if(St.opts.key!=='auto'&&!(+St.opts.key>=0&&+St.opts.key<12))St.opts.key='auto';

const curBpm=()=>St.bpm||St.song.bpm;
const audible=id=>St.solo?St.solo===id:!St.mute[id];
const instOf=l=>{const p=St.song.prog;
  if(l==='piano')return p.piano===4?'epiano':'piano';
  if(l==='arp')return p.arp===4?'epiano':(p.arp===24||p.arp===46)?'pluck':'piano';
  return{mel:'lead',bass:'bass',pad:'pad'}[l]||'piano';};

/* ---------------- Controlli ---------------- */
const fillSel=(id,entries)=>{$(id).innerHTML=entries.map(([v,t])=>`<option value="${v}">${t}</option>`).join('');};
fillSel('#genre',Object.entries(GENRES).map(([k,g])=>[k,g.n]));
fillSel('#mood',Object.entries(MOODS).map(([k,m])=>[k,m.n]));
fillSel('#mode',[['auto','Auto (dal mood)'],...MODE_ORDER.map(m=>[m,MODES[m].n])]);
fillSel('#key',[['auto','Auto'],...KEY_NAMES.map((n,i)=>[i,n])]);
fillSel('#structure',Object.entries(STRUCTS).map(([k,s])=>[k,s.n]));
fillSel('#color',[['auto','Auto (dal genere)'],['triadi','Triadi'],['colorati','Colorati (add9, sus, 6)'],['settime','Settime'],['estesi','Estesi (9, 11, 13)']]);
function syncControls(){['genre','mood','mode','key','structure','color'].forEach(k=>$('#'+k).value=St.opts[k]);$('#human').checked=St.opts.human;}
['genre','mood','mode','key','structure','color'].forEach(k=>$('#'+k).addEventListener('change',e=>{
  St.opts[k]=e.target.value;LS.set('pg_opts',St.opts);if(k==='genre'||k==='mood')St.bpm=null;
  if(k==='structure')St.sel=0;regen({resetMute:k==='genre'});}));
$('#human').addEventListener('change',e=>{St.opts.human=e.target.checked;LS.set('pg_opts',St.opts);regen();});
$('#btnGen').onclick=()=>{St.seeds={h:rndSeed(),a:rndSeed(),m:rndSeed()};St.bpm=null;regen({resetMute:true});};
$$('[data-re]').forEach(b=>b.onclick=()=>{St.seeds[b.dataset.re]=rndSeed();regen();toast({h:'Nuova armonia',a:'Nuovo arrangiamento',m:'Nuova melodia'}[b.dataset.re]);});
const codeOf=()=>[St.opts.genre,St.opts.mood,St.opts.mode,St.opts.key,St.opts.structure,St.opts.color,`${St.seeds.h}-${St.seeds.a}-${St.seeds.m}`].join('/');
$('#code').addEventListener('change',e=>{
  const v=e.target.value.trim(),parts=v.split('/'),sd=(parts[parts.length-1]||'').match(/^(\d+)-(\d+)-(\d+)$/);
  if(!sd){toast('Codice non valido');e.target.value=codeOf();return;}
  if(parts.length===7){const [g,m,mo,k,s,c]=parts;
    if(GENRES[g])St.opts.genre=g;if(MOODS[m])St.opts.mood=m;if(VALID.mode[mo])St.opts.mode=mo;
    if(k==='auto'||(+k>=0&&+k<12))St.opts.key=k;if(STRUCTS[s])St.opts.structure=s;if(VALID.color[c])St.opts.color=c;syncControls();}
  St.seeds={h:+sd[1],a:+sd[2],m:+sd[3]};St.bpm=null;regen({resetMute:true});});

/* ---------------- Generazione ---------------- */
function regen({resetMute=false}={}){
  const was=Player.playing,pos=was?Player.pos():0;
  if(was)Player.stop();
  const s=generateSong({...St.opts,bpm:St.bpm,seeds:St.seeds});
  St.song=s;
  if(resetMute||!St.muteInit){St.mute={};LAYERS.forEach(l=>St.mute[l.id]=!s.defaults[l.id]);St.solo=null;St.muteInit=true;}
  St.ev=[];LAYERS.forEach(l=>s.layers[l.id].forEach(e=>St.ev.push({t:e.t,d:e.d,n:e.n,v:e.v,l:l.id})));St.ev.sort((a,b)=>a.t-b.t);
  St.sel=Math.min(St.sel,s.sections.length-1);
  renderAll();
  if(was)Player.start(Math.min(pos,s.beats-.01));
}
function renderAll(){renderHead();renderLanes();drawArr();renderSection();renderHow();updateTransport(0);}

function renderHead(){
  const s=St.song,G=GENRES[s.genre],M=MOODS[s.mood],dur=s.beats*60/curBpm();
  $('#songTitle').textContent=`${G.n} · ${M.n}`;
  const lvl={auto:'auto',triadi:'triadi',colorati:'colorati',settime:'settime',estesi:'estesi'}[St.opts.color];
  const chips=[`<b>${s.keyName}</b> ${MODES[s.mode].n.toLowerCase()}`,`<b>${curBpm()}</b> BPM`,`<b>${s.bars}</b> battute · ${fmtTime(dur)}`,
    `<b>${s.sections.length}</b> ${s.sections.length===1?'sezione':'sezioni'}`,`accordi ${lvl}`];
  if(s.swing)chips.push(`swing ${Math.round(s.swing*100)}%`);
  $('#chips').innerHTML=chips.map(c=>`<span class="chip">${c}</span>`).join('');
  $('#code').value=codeOf();
}

/* ---------------- Vista arrangiamento ---------------- */
const LANE_H={mel:62,piano:84,arp:62,pad:44,bass:54,drums:50},RULER=28,CHROW=22;
const lanes=()=>LAYERS.filter(l=>St.song.layers[l.id].length);
function renderLanes(){
  const h=$('#laneHead');
  h.innerHTML=`<div class="label" style="height:${RULER+CHROW}px;display:flex;align-items:center;padding:0 10px">Tracce</div>`+lanes().map(l=>`
    <div class="lane-h ${audible(l.id)?'':'off'}" style="height:${LANE_H[l.id]}px">
      <span class="dot" style="background:var(--c-${l.id})"></span><span class="nm">${l.n}</span>
      <button class="ib ${St.mute[l.id]?'on':''}" data-mute="${l.id}" title="Silenzia">M</button>
      <button class="ib ${St.solo===l.id?'on':''}" data-solo="${l.id}" title="Ascolta solo questa traccia">S</button>
      <button class="ib dl" data-dl="${l.id}" title="Scarica solo questa traccia (.mid)">⬇</button></div>`).join('');
  h.querySelectorAll('[data-mute]').forEach(b=>b.onclick=()=>{const id=b.dataset.mute;St.mute[id]=!St.mute[id];renderLanes();drawArr();});
  h.querySelectorAll('[data-solo]').forEach(b=>b.onclick=()=>{const id=b.dataset.solo;St.solo=St.solo===id?null:id;renderLanes();drawArr();});
  h.querySelectorAll('[data-dl]').forEach(b=>b.onclick=()=>{const l=LAYERS.find(x=>x.id===b.dataset.dl);
    download(`${baseName()}-${slug(l.n)}.mid`,toMidi(St.song,{layers:[l.id],bpm:curBpm(),title:l.n}),'audio/midi');});
}
const cache=document.createElement('canvas');
function cssVar(k){return getComputedStyle(document.documentElement).getPropertyValue(k).trim();}
function drawArr(){
  const s=St.song,sc=$('#arrScroll'),W0=Math.max(200,sc.clientWidth),barW=clamp(W0/s.bars,24,120),W=Math.max(W0,Math.round(s.bars*barW));
  const L=lanes(),H=RULER+CHROW+L.reduce((a,l)=>a+LANE_H[l.id],0),dpr=window.devicePixelRatio||1,cv=$('#arr');
  cv.style.width=W+'px';cv.style.height=H+'px';cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);
  cache.width=cv.width;cache.height=cv.height;
  const x=cache.getContext('2d');x.setTransform(dpr,0,0,dpr,0,0);x.clearRect(0,0,W,H);
  const px=b=>b/4*barW;
  x.font='600 11.5px Inter,system-ui,sans-serif';x.textBaseline='middle';
  // sezioni
  s.sections.forEach((sec,i)=>{const x0=px(sec.startBeat)+1.5,w=sec.bars*barW-3;
    x.fillStyle=cssVar('--s-'+sec.type);x.globalAlpha=i===St.sel?1:.42;rr(x,x0,4,w,RULER-8,6);x.fill();x.globalAlpha=1;
    x.fillStyle=i===St.sel?'#fff':cssVar('--tx');x.fillText(fit0(x,sec.name,w-12),x0+8,RULER/2);});
  // accordi
  x.font='500 11px Inter,system-ui,sans-serif';x.fillStyle=cssVar('--tx2');
  const fit=(t,w)=>{if(x.measureText(t).width<=w)return t;while(t.length>1&&x.measureText(t+'…').width>w)t=t.slice(0,-1);return t+'…';};
  s.chords.forEach(c=>{const x0=px(c.start),w=c.beats/4*barW;if(w<22)return;x.fillText(fit(c.name,w-7),x0+4,RULER+CHROW/2);});
  // corsie
  let y=RULER+CHROW;
  const line=cssVar('--line');
  L.forEach((l,li)=>{
    const h=LANE_H[l.id],ev=s.layers[l.id],on=audible(l.id);
    x.fillStyle=li%2?cssVar('--panel'):cssVar('--panel2');x.fillRect(0,y,W,h);
    x.fillStyle=line;x.fillRect(0,y,W,1);
    x.fillStyle=cssVar('--c-'+l.id);
    if(l.id==='drums'){
      const row=n=>n===36?3:(n===38||n===37||n===39)?2:(n===45||n===47||n===48||n===50)?1.5:0;
      ev.forEach(e=>{x.globalAlpha=(on?.35:.1)+(on?e.v/250:0);x.fillRect(px(e.t),y+6+row(e.n)*(h-14)/3,Math.max(1.5,barW/18),(h-14)/4);});
    }else{
      let lo=127,hi=0;ev.forEach(e=>{if(e.n<lo)lo=e.n;if(e.n>hi)hi=e.n;});lo-=1;hi+=1;
      const nh=clamp((h-10)/(hi-lo+1),2,5);
      ev.forEach(e=>{x.globalAlpha=on?.3+e.v/200:.12;x.fillRect(px(e.t),y+5+(hi-e.n)/(hi-lo)*(h-10-nh),Math.max(1.5,px(e.d)-.6),nh);});
    }
    x.globalAlpha=1;y+=h;
  });
  // griglia
  for(let b=0;b<=s.bars;b++){const isSec=s.sections.some(z=>z.startBar===b);x.fillStyle=line;x.globalAlpha=isSec?1:.55;x.fillRect(Math.round(b*barW),RULER+CHROW,1,H);}
  x.globalAlpha=1;
  St.layout={W,H,barW,dpr};
  paintArr(Player.playing?Player.pos():-1);
}
function fit0(x,t,w){if(x.measureText(t).width<=w)return t;while(t.length>1&&x.measureText(t+'…').width>w)t=t.slice(0,-1);return t.length>1?t+'…':'';}
function rr(x,a,b,w,h,r){x.beginPath();x.moveTo(a+r,b);x.arcTo(a+w,b,a+w,b+h,r);x.arcTo(a+w,b+h,a,b+h,r);x.arcTo(a,b+h,a,b,r);x.arcTo(a,b,a+w,b,r);x.closePath();}
function paintArr(pos){
  const cv=$('#arr'),x=cv.getContext('2d');x.setTransform(1,0,0,1,0,0);x.clearRect(0,0,cv.width,cv.height);x.drawImage(cache,0,0);
  if(pos>=0&&St.layout){const{barW,dpr,H}=St.layout,X=pos/4*barW*dpr;x.fillStyle=cssVar('--acc');x.fillRect(X-dpr,0,2*dpr,H*dpr);
    x.globalAlpha=.08;x.fillRect(0,0,X,H*dpr);x.globalAlpha=1;}
}
$('#arr').addEventListener('click',e=>{
  if(!St.layout)return;const beat=e.offsetX/St.layout.barW*4;
  const i=St.song.sections.findIndex(s=>beat>=s.startBeat&&beat<s.startBeat+s.bars*4);if(i<0)return;
  St.sel=i;drawArr();renderSection();if(Player.playing)Player.start(St.song.sections[i].startBeat);});
let rzT;addEventListener('resize',()=>{clearTimeout(rzT);rzT=setTimeout(()=>{if(St.song&&!$('#tab-gen').hidden)drawArr();},120);});

/* ---------------- Sezione selezionata ---------------- */
function renderSection(){
  const s=St.song,sec=s.sections[St.sel];
  $('#secTitle').textContent=sec.name;
  $('#secInfo').textContent=`battute ${sec.startBar+1}–${sec.startBar+sec.bars} · ${sec.chords.length} accordi · clicca un accordo per ascoltarlo`;
  $('#chords').innerHTML=sec.chords.map(c=>{const ci=s.chords.indexOf(c),bar=Math.floor(c.start/4)+1,half=c.start%4?'½':'';
    return`<div class="cc" data-ci="${ci}"><div class="bn"><span>batt. ${bar}${half?'½':''}</span>${c.beats<4?'<span class="half">2 tempi</span>':''}</div><div class="nm">${c.name}</div><div class="rn">${c.rn}</div></div>`;}).join('');
  $$('#chords .cc').forEach(el=>el.onclick=()=>{const c=s.chords[+el.dataset.ci];Synth.init();const t=Synth.now()+.03,inst=instOf('piano');
    c.lh.forEach(n=>Synth.note(inst,n,t,1.9,70,'piano'));c.rh.forEach((n,j)=>Synth.note(inst,n,t+.014*j,1.9,80,'piano'));});
}
const MODE_DESC={major:'luminoso, cadenze V–I',minor:'malinconico, con bVI e bVII',harmonic:'minore con V maggiore e sensibile',dorian:'minore con 6ª maggiore: il IV è maggiore',
  phrygian:'minore scuro con 2ª minore: il bII',lydian:'maggiore sospeso con #4: il II è maggiore',mixolydian:'maggiore con 7ª minore: il bVII'};
function renderHow(){
  const s=St.song,G=GENRES[s.genre];
  $('#genreDesc').textContent=G.desc;
  const items=[`Modo <b>${MODES[s.mode].n}</b>: ${MODE_DESC[s.mode]}.`,
    `Progressioni scelte confrontando i giri tipici del genere con catene di Markov sulle funzioni armoniche (T–S–D), con inizio e fine adatti a ogni sezione.`,
    `Voicing a due mani con condotta delle parti: ${({close:'accordi stretti',rootless:'voicing senza fondamentale',cluster:'grappoli con none',spread:'voicing aperti'})[G.rh]} a destra, ${({octave:'ottave',octave5:'quinte e ottave',shell:'shell (fondamentale + 7ª)',open:'quinte aperte',classical:'basso e accordo alternati'})[G.lh]} a sinistra.`,
    `Melodia a frasi domanda–risposta con motivi ripetuti, note dell'accordo sui tempi forti e cadenze sulla tonica.`];
  s.info.filter(x=>!/^arpeggio/.test(x)).forEach(x=>items.push(x[0].toUpperCase()+x.slice(1)+'.'));
  $('#howList').innerHTML=items.map(x=>`<li>${x}</li>`).join('');
}

/* ---------------- Trasporto ---------------- */
const Player={playing:false,base:0,p:0,timer:null,raf:0,
  range(){const s=St.song;if(St.loop){const sec=s.sections[St.sel];return[sec.startBeat,sec.startBeat+sec.bars*4];}return[0,s.beats];},
  first(b){const ev=St.ev;let lo=0,hi=ev.length;while(lo<hi){const m=(lo+hi)>>1;if(ev[m].t<b-1e-6)lo=m+1;else hi=m;}return lo;},
  pos(){const spb=60/curBpm(),[L0,L1]=this.range();let p=(Synth.now()-this.base)/spb;if(p<L0)p+=L1-L0;return clamp(p,0,St.song.beats);},
  start(beat){
    Synth.init();this.stop(true);const[L0,L1]=this.range();const b=clamp(beat==null?L0:beat,L0,L1-.01);
    this.playing=true;this.base=Synth.now()+.08-b*60/curBpm();this.p=this.first(b);
    this.timer=setInterval(()=>this.sched(),25);this.sched();this.loop();
    $('#playIc').innerHTML='<path fill="currentColor" d="M4 3h3.6v12H4zM10.4 3H14v12h-3.6z"/>';
  },
  stop(silent){this.playing=false;clearInterval(this.timer);cancelAnimationFrame(this.raf);
    $('#playIc').innerHTML='<path fill="currentColor" d="M4 2.5v13l11-6.5z"/>';
    if(!silent){paintArr(-1);$$('.cc.on').forEach(e=>e.classList.remove('on'));updateTransport(0);}},
  sched(){
    const now=Synth.now(),spb=60/curBpm(),[L0,L1]=this.range(),ev=St.ev;
    for(let guard=0;guard<5000;guard++){
      const e=ev[this.p];
      if(!e||e.t>=L1){if(now+.2<this.base+L1*spb)break;this.base+=(L1-L0)*spb;this.p=this.first(L0);continue;}
      const at=this.base+e.t*spb;if(at>now+.2)break;
      if(at>=now-.04&&audible(e.l)){if(e.l==='drums')Synth.hit(e.n,at,e.v);else Synth.note(instOf(e.l),e.n,at,e.d*spb,e.v,e.l);}
      this.p++;
    }
  },
  loop(){if(!this.playing)return;const p=this.pos();paintArr(p);updateTransport(p);
    const sc=$('#arrScroll'),X=p/4*St.layout.barW;if(X<sc.scrollLeft+20||X>sc.scrollLeft+sc.clientWidth-40)sc.scrollLeft=Math.max(0,X-60);
    this.raf=requestAnimationFrame(()=>this.loop());}};
let lastCi=-1;
function updateTransport(p){
  const s=St.song,spb=60/curBpm();
  const sec=s.sections.find(z=>p>=z.startBeat&&p<z.startBeat+z.bars*4)||s.sections[0];
  $('#posSec').textContent=`${sec.name} · batt. ${Math.floor(p/4)+1}`;
  $('#posTime').textContent=`${fmtTime(p*spb)} / ${fmtTime(s.beats*spb)}`;
  $('#bpm').value=curBpm();$('#bpmv').textContent=curBpm()+' BPM';
  let ci=-1;if(Player.playing)s.chords.forEach((c,i)=>{if(c.start<=p)ci=i;});
  if(ci!==lastCi){$$('.cc.on').forEach(e=>e.classList.remove('on'));const el=$(`.cc[data-ci="${ci}"]`);if(el)el.classList.add('on');lastCi=ci;}
}
$('#play').onclick=()=>Player.playing?Player.stop():Player.start(St.loop?null:0);
$('#loopBtn').onclick=()=>{St.loop=!St.loop;$('#loopBtn').classList.toggle('on',St.loop);$('#loopBtn').style.color=St.loop?'var(--acc)':'';
  if(Player.playing)Player.start(St.song.sections[St.sel].startBeat);toast(St.loop?'Loop sulla sezione selezionata':'Riproduzione di tutto il brano');};
$('#bpm').oninput=e=>{const old=60/curBpm(),p=Player.playing?Player.pos():0;St.bpm=+e.target.value;
  if(Player.playing)Player.base=Synth.now()-p*60/curBpm();$('#bpmv').textContent=curBpm()+' BPM';renderHead();};
$('#bpmAuto').onclick=()=>{const p=Player.playing?Player.pos():0;St.bpm=null;if(Player.playing)Player.base=Synth.now()-p*60/curBpm();renderHead();updateTransport(p);};
$('#vol').oninput=e=>Synth.setVolume(+e.target.value/100);
document.addEventListener('keydown',e=>{
  if($('#tab-gen').hidden||/INPUT|SELECT|TEXTAREA/.test(e.target.tagName)||e.metaKey||e.ctrlKey)return;
  if(e.code==='Space'){e.preventDefault();$('#play').click();}
  else if(e.key==='l'||e.key==='L')$('#loopBtn').click();});

/* ---------------- Esportazione ---------------- */
const slug=s=>s.normalize('NFD').replace(/[̀-ͯ]/g,'').replace(/#/g,'s').replace(/[^A-Za-z0-9]+/g,'-').replace(/^-|-$/g,'');
const baseName=()=>{const s=St.song;return slug(`${GENRES[s.genre].n} ${MOODS[s.mood].n} ${s.keyName} ${MODES[s.mode].n} ${curBpm()}bpm`);};
function download(name,data,type){
  const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([data],{type}));a.download=name;
  document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(a.href),5000);toast('Scaricato: '+name);}
const allLayers=()=>lanes().map(l=>l.id);
$('#expMidi').onclick=()=>{const ls=allLayers().filter(audible);if(!ls.length){toast('Nessuna traccia attiva');return;}
  download(baseName()+'.mid',toMidi(St.song,{layers:ls,bpm:curBpm(),title:`${GENRES[St.song.genre].n} · ${MOODS[St.song.mood].n}`}),'audio/midi');};
$('#expSec').onclick=()=>{const sec=St.song.sections[St.sel];
  download(`${baseName()}-${slug(sec.name)}.mid`,toMidi(St.song,{layers:allLayers(),from:sec.startBeat,to:sec.startBeat+sec.bars*4,bpm:curBpm(),title:sec.name}),'audio/midi');};
$('#expZip').onclick=()=>{
  const s=St.song,b=baseName(),bpm=curBpm(),files=[];
  files.push({name:`${b}/${b} - completo.mid`,data:toMidi(s,{layers:allLayers(),bpm,title:b})});
  lanes().forEach((l,i)=>files.push({name:`${b}/tracce/${i+1} ${l.n}.mid`,data:toMidi(s,{layers:[l.id],bpm,title:l.n})}));
  const seen=new Set();let k=1;
  s.sections.forEach(sec=>{if(seen.has(sec.type))return;seen.add(sec.type);
    files.push({name:`${b}/sezioni/${k++} ${SEC_NAME[sec.type]}.mid`,data:toMidi(s,{layers:allLayers(),from:sec.startBeat,to:sec.startBeat+sec.bars*4,bpm,title:SEC_NAME[sec.type]})});});
  files.push({name:`${b}/accordi.txt`,data:chordChart({...s,bpm})});
  files.push({name:`${b}/LEGGIMI.txt`,data:[
    'PIANO GENERATIVO — pacchetto MIDI','',`Brano: ${GENRES[s.genre].n} · ${MOODS[s.mood].n}`,`Tonalità: ${s.keyName} ${MODES[s.mode].n} · ${bpm} BPM · 4/4 · ${s.bars} battute`,
    `Codice per rigenerarlo: ${codeOf()}`,'',
    'completo.mid  — tutte le tracce, con marcatori di sezione e nomi degli accordi',
    'tracce/       — ogni strato da solo (si allineano dalla battuta 1)',
    'sezioni/      — ogni sezione (strofa, ritornello…) da sola, tutte le tracce',
    'accordi.txt   — schema accordi con gradi',
    '','Canali MIDI: 1 Melodia · 2 Pianoforte · 3 Arpeggio · 4 Pad · 5 Basso · 10 Batteria (GM)'].join('\n')});
  download(b+'.zip',makeZip(files),'application/zip');};
$('#expCopy').onclick=()=>{const t=chordChart({...St.song,bpm:curBpm()});
  const fb=()=>{const ta=document.createElement('textarea');ta.value=t;document.body.append(ta);ta.select();try{document.execCommand('copy');toast('Accordi copiati');}catch(e){toast('Copia non riuscita');}ta.remove();};
  if(navigator.clipboard&&navigator.clipboard.writeText)navigator.clipboard.writeText(t).then(()=>toast('Accordi copiati'),fb);else fb();};

/* ---------------- Schede ---------------- */
$$('.tabs button').forEach(b=>b.onclick=()=>{
  $$('.tabs button').forEach(x=>x.classList.toggle('on',x===b));
  const gen=b.dataset.tab==='gen';$('#tab-gen').hidden=!gen;$('#tab-ex').hidden=gen;$('#transport').hidden=!gen;
  document.body.style.paddingBottom=gen?'':'24px';
  if(!gen&&Player.playing)Player.stop();
  if(gen)drawArr();else if(typeof Ex!=='undefined')Ex.show();});

/* ---------------- Avvio ---------------- */
syncControls();regen({resetMute:true});
window.__pg={St,regen,Player};
