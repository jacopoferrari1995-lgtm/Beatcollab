'use strict';
/* =====================================================================
   INTERFACCIA — generatore, arrangiamento, editor accordi, trasporto, esportazione
   ===================================================================== */
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const LS={get(k,d){try{const v=localStorage.getItem(k);return v==null?d:JSON.parse(v);}catch(e){return d;}},
          set(k,v){try{localStorage.setItem(k,JSON.stringify(v));}catch(e){}}};
const rndSeed=()=>1+Math.floor(Math.random()*99998);
const fmtTime=s=>{s=Math.max(0,Math.round(s));return Math.floor(s/60)+':'+String(s%60).padStart(2,'0');};
function toast(msg){const t=$('#toast');t.textContent=msg;t.classList.add('show');clearTimeout(t._h);t._h=setTimeout(()=>t.classList.remove('show'),2000);}

const DEF={genre:'pop',mood:'rilassato',mode:'auto',key:'auto',structure:'vc',color:'auto',human:true,pad:false,pianoStyle:'live',modLast:false,sound:'samp'};
const St={opts:Object.assign({},DEF,LS.get('pg_opts',{})),seeds:{h:rndSeed(),a:rndSeed(),m:rndSeed()},bpm:null,song:null,ev:[],
  mute:{},solo:null,oct:{},vol:{},padMute:{},padVol:{},padDens:{},drumGrid:{},feel:{},padSel:null,showPads:false,reseed:{L:{},S:{}},sel:0,selChord:-1,scope:'rep',loop:false,muteInit:false,layout:null,secs:null,typeOrder:null,edits:[]};
const VALID={genre:GENRES,mood:MOODS,mode:Object.assign({auto:1},MODES),structure:STRUCTS,color:{auto:1,triadi:1,colorati:1,settime:1,estesi:1},pianoStyle:{live:1,classic:1},sound:{samp:1,synth:1}};
St.name='Senza titolo';St.projId=null;St.savedSnap=null;
(()=>{const cur=LS.get('pg_current',null),meta=LS.get('pg_meta',{});
  if(cur&&cur.opts&&cur.seeds){Object.assign(St,{opts:Object.assign({},DEF,cur.opts),seeds:cur.seeds,secs:cur.secs||null,typeOrder:cur.typeOrder||null,edits:cur.edits||[],
    oct:cur.oct||{},mute:cur.mute||{},solo:cur.solo||null,bpm:cur.bpm||null,name:cur.name||'Senza titolo',muteInit:!!cur.mute,reseed:cur.reseed||{L:{},S:{}},vol:cur.vol||{},padMute:cur.padMute||{},padVol:cur.padVol||{},padDens:cur.padDens||{},drumGrid:cur.drumGrid||{},feel:cur.feel||{}});}
  St.projId=meta.projId||null;St.savedSnap=meta.savedSnap||null;})();
for(const k in VALID)if(!(St.opts[k] in VALID[k]))St.opts[k]=DEF[k];
if(St.opts.key!=='auto'&&!(+St.opts.key>=0&&+St.opts.key<12))St.opts.key='auto';

const curBpm=()=>St.bpm||St.song.bpm;
const audible=id=>St.solo?St.solo===id:!St.mute[id];
const instOf=l=>{const s=St.song,p=s.prog;
  if(l==='piano')return p.piano===4?'epiano':'piano';
  if(l==='arp')return p.arp===4?'epiano':(p.arp===24||p.arp===46)?'pluck':'piano';
  if(l==='bass')return GENRES[s.genre].bass808?'b808':'bass';
  return{mel:'lead',pad:'pad'}[l]||'piano';};

/* ---------------- Controlli ---------------- */
const fillSel=(id,entries)=>{$(id).innerHTML=entries.map(([v,t])=>`<option value="${v}">${t}</option>`).join('');};
fillSel('#genre',Object.entries(GENRES).map(([k,g])=>[k,g.n]));
fillSel('#mood',Object.entries(MOODS).map(([k,m])=>[k,m.n]));
fillSel('#mode',[['auto','Auto (dal mood)'],...MODE_ORDER.map(m=>[m,MODES[m].n])]);
fillSel('#key',[['auto','Auto'],...KEY_NAMES.map((n,i)=>[i,n])]);
fillSel('#structure',Object.entries(STRUCTS).map(([k,s])=>[k,s.n]));
fillSel('#color',[['auto','Auto (dal genere)'],['triadi','Triadi'],['colorati','Colorati (add9, sus, 6)'],['settime','Settime'],['estesi','Estesi (9, 11, 13)']]);
fillSel('#addType',['intro','verse','pre','chorus','bridge','special','outro'].map(t=>[t,SEC_NAME[t]]));
function syncControls(){['genre','mood','mode','key','structure','color','sound'].forEach(k=>$('#'+k).value=St.opts[k]);$('#modLast').checked=!!St.opts.modLast;
  $('#pstyle').value=St.opts.pianoStyle;$('#human').checked=St.opts.human;$('#pad').checked=St.opts.pad;}
const saveOpts=()=>LS.set('pg_opts',St.opts);
function clearReseed(k){const R=St.reseed;Object.keys(R.S).forEach(x=>{if(x.endsWith('.'+k))delete R.S[x];});
  if(k==='a')['piano','arp','pad','bass','drums'].forEach(l=>delete R.L[l]);if(k==='m')delete R.L.mel;}
function dropEdits(){if(St.edits.length){St.edits=[];toast('Modifiche agli accordi azzerate (armonia nuova)');}}
['genre','mood','mode','key','structure','color'].forEach(k=>$('#'+k).addEventListener('change',e=>{
  St.opts[k]=e.target.value;saveOpts();
  if(k==='genre'||k==='mood')St.bpm=null;
  if(k==='structure'){St.sel=0;St.secs=null;St.typeOrder=null;}
  if(k!=='key')St.reseed={L:{},S:{}};
  if(k!=='key')dropEdits();
  St.selChord=-1;regen({resetMute:k==='genre'});}));
$('#pstyle').addEventListener('change',e=>{St.opts.pianoStyle=e.target.value;saveOpts();regen();toast(e.target.value==='classic'?'Pianoforte classico (v3)':'Pianoforte vivo');});
$('#human').addEventListener('change',e=>{St.opts.human=e.target.checked;saveOpts();regen();});
$('#modLast').addEventListener('change',e=>{St.opts.modLast=e.target.checked;saveOpts();regen();
  const n=St.song.sections.filter(x=>x.type==='chorus').length;toast(e.target.checked?(n>=2?'L\'ultimo ritornello sale di un tono':'Serve una struttura con almeno due ritornelli'):'Modulazione tolta');});
$('#sound').addEventListener('change',e=>{St.opts.sound=e.target.value;saveOpts();ensureSound(true);renderHead();});
let soundMsg='';
function ensureSound(verbose){
  if(St.opts.sound!=='samp'){Synth.useSamples(false);soundMsg='';return;}
  if(Synth.sampleState==='ready'){Synth.useSamples(true);return;}
  if(Synth.sampleState==='loading')return;
  soundMsg='caricamento 0%';renderHead();
  Synth.useSamples(true,p=>{soundMsg='caricamento '+Math.round(p*100)+'%';const c=$('#sndChip');if(c)c.textContent='piano: '+soundMsg;})
    .then(st=>{soundMsg='';renderHead();toast(st==='ready'?'Pianoforte campionato pronto':'Campioni non raggiungibili: uso il piano sintetico');});
}
$('#pad').addEventListener('change',e=>{St.opts.pad=e.target.checked;saveOpts();St.mute.pad=false;regen();});
$('#btnGen').onclick=()=>{St.seeds={h:rndSeed(),a:rndSeed(),m:rndSeed()};St.reseed={L:{},S:{}};St.bpm=null;St.edits=[];St.selChord=-1;regen({resetMute:true});};
$$('[data-re]').forEach(b=>b.onclick=()=>{const k=b.dataset.re;if(k==='h')dropEdits();St.seeds[k]=rndSeed();clearReseed(k);regen();
  toast({h:'Nuova armonia',a:'Nuovo arrangiamento',m:'Nuova melodia'}[k]);});
const TL={intro:'i',verse:'v',pre:'p',chorus:'c',bridge:'b',special:'x',outro:'o',loop:'l'},LT=Object.fromEntries(Object.entries(TL).map(([k,v])=>[v,k]));
const codeOf=()=>{const p=[St.opts.genre,St.opts.mood,St.opts.mode,St.opts.key,St.opts.structure,St.opts.color,`${St.seeds.h}-${St.seeds.a}-${St.seeds.m}`,(St.opts.pad?'p':'-')+(St.opts.pianoStyle==='classic'?'c':'v')];
  const rs=[...Object.entries(St.reseed.L).filter(x=>x[1]).map(([k,v])=>'L'+k+v),...Object.entries(St.reseed.S).filter(x=>x[1]).map(([k,v])=>'S'+k+v)].join(',');
  if(St.secs||rs)p.push(St.secs?St.secs.map(([t,b])=>TL[t]+b).join('.'):'',St.secs?(St.typeOrder||[]).map(t=>TL[t]).join(''):'');if(rs)p.push(rs);return p.join('/');};
$('#code').addEventListener('change',e=>{
  const parts=e.target.value.trim().split('/'),sd=(parts[6]||parts[parts.length-1]||'').match(/^(\d+)-(\d+)-(\d+)$/);
  if(!sd){toast('Codice non valido');e.target.value=codeOf();return;}
  if(parts.length>=7){const [g,m,mo,k,s,c]=parts;
    if(GENRES[g])St.opts.genre=g;if(MOODS[m])St.opts.mood=m;if(VALID.mode[mo])St.opts.mode=mo;
    if(k==='auto'||(+k>=0&&+k<12))St.opts.key=k;if(STRUCTS[s])St.opts.structure=s;if(VALID.color[c])St.opts.color=c;
    if(parts[7]){St.opts.pad=parts[7][0]==='p';St.opts.pianoStyle=parts[7][1]==='c'?'classic':'live';}syncControls();saveOpts();}
  St.secs=null;St.typeOrder=null;
  if(parts[8]){const secs=parts[8].split('.').map(x=>[LT[x[0]],+x.slice(1)]).filter(([t,b])=>t&&b>0&&b<=64);if(secs.length){St.secs=secs;St.typeOrder=(parts[9]||'').split('').map(c=>LT[c]).filter(Boolean);}}
  St.reseed={L:{},S:{}};(parts[10]||'').split(',').forEach(x=>{const m=x.match(/^([LS])([a-z.]+?)(\d+)$/);if(m)St.reseed[m[1]][m[2]]=+m[3];});
  St.seeds={h:+sd[1],a:+sd[2],m:+sd[3]};St.bpm=null;St.edits=[];St.sel=0;St.selChord=-1;regen({resetMute:true});});

/* ---------------- Generazione ---------------- */
function buildEv(){
  const s=St.song;St.ev=[];
  LAYERS.forEach(l=>{const sh=l.id==='drums'?0:12*(St.oct[l.id]||0),sf=((St.feel[l.id]||{}).shift||0)*curBpm()/60000;
    s.layers[l.id].forEach(e=>St.ev.push({t:Math.max(0,e.t+sf),d:e.d,n:clamp(e.n+sh,0,127),v:l.id==='drums'?clamp(Math.round(e.v*(St.padVol[e.pad]!=null?St.padVol[e.pad]:1)),1,127):e.v,l:l.id,gl:e.gl!=null?e.gl+sh:null,pad:e.pad}));});
  St.ev.sort((a,b)=>a.t-b.t);
}
function regen({resetMute=false}={}){
  const was=Player.playing,pos=was?Player.pos():0;
  if(was)Player.stop(true);
  const s=generateSong({...St.opts,bpm:St.bpm,seeds:St.seeds,secs:St.secs,typeOrder:St.typeOrder,edits:St.edits,reseed:St.reseed,feel:St.feel,padDens:St.padDens,drumGrid:St.drumGrid});
  St.song=s;
  if(resetMute||!St.muteInit){St.mute={};LAYERS.forEach(l=>St.mute[l.id]=!s.defaults[l.id]);St.solo=null;St.muteInit=true;}
  buildEv();
  St.sel=Math.min(St.sel,s.sections.length-1);
  if(St.selChord>=s.sections[St.sel].chords.length)St.selChord=s.sections[St.sel].chords.length-1;
  renderAll();
  if(was)Player.start(Math.min(pos,s.beats-.01));
  track();
}
function renderAll(){applyVols();renderHead();renderLanes();drawArr();renderPads();renderSection();renderHow();renderProg();updateTransport(Player.playing?Player.pos():0);}

function renderHead(){
  const s=St.song,G=GENRES[s.genre],M=MOODS[s.mood],dur=s.beats*60/curBpm();
  $('#songTitle').textContent=`${G.n} · ${M.n}`;
  const chips=[`<b>${s.keyName}</b> ${MODES[s.mode].n.toLowerCase()}`,`<b>${curBpm()}</b> BPM`,`<b>${s.bars}</b> battute · ${fmtTime(dur)}`,
    `<b>${s.sections.length}</b> ${s.sections.length===1?'sezione':'sezioni'}`,`accordi ${St.opts.color}`,`pianoforte ${St.opts.pianoStyle==='classic'?'classico v3':'vivo'}`];
  if(s.swing)chips.push(`swing ${Math.round(s.swing*100)}%`);
  const ss=typeof Synth!=='undefined'?Synth.sampleState:'off';
  chips.push(`<span id="sndChip">piano: ${St.opts.sound==='synth'?'sintetico':soundMsg?soundMsg:ss==='ready'?'campionato':ss==='failed'?'sintetico (campioni non disponibili)':'campionato (si carica al play)'}</span>`);
  $('#chips').innerHTML=chips.map(c=>`<span class="chip">${c}</span>`).join('')+(St.edits.length?`<span class="chip hot"><b>${St.edits.length}</b> modifiche agli accordi</span>`:'');
  $('#code').value=codeOf();
}

/* ---------------- Vista arrangiamento ---------------- */
const LANE_NAME={mel:'Melodia',piano:'Piano',arp:'Arpeggio',pad:'Pad',bass:'Basso',drums:'Batteria'};
const LANE_H={mel:62,piano:84,arp:62,pad:46,bass:58,drums:84},RULER=28,CHROW=22;
const lanes=()=>LAYERS.filter(l=>St.song.layers[l.id].length);
const feelShift=()=>Object.fromEntries(Object.entries(St.feel).map(([k,v])=>[k,v.shift||0]));
const volOf=id=>St.vol[id]!=null?St.vol[id]:1;
function applyVols(){LAYERS.forEach(l=>Synth.setLayerGain(l.id,volOf(l.id)));}
function renderLanes(){
  const h=$('#laneHead');
  h.innerHTML=`<div class="label" style="height:${RULER+CHROW}px;display:flex;align-items:center;padding:0 12px">Tracce</div>`+lanes().map(l=>{
    const o=St.oct[l.id]||0,v=Math.round(volOf(l.id)*100);
    return`<div class="lane-h ${audible(l.id)?'':'off'} ${LANE_H[l.id]<50?'short':''}" style="height:${LANE_H[l.id]}px">
      <span class="dot" style="background:var(--c-${l.id})"></span><span class="nm" title="${l.n}">${LANE_NAME[l.id]}</span>
      <div class="btns"><button class="ib rg" data-rl="${l.id}" title="Rigenera solo ${l.n.toLowerCase()}">🎲</button>
        <button class="ib ${St.mute[l.id]?'on':''}" data-mute="${l.id}" title="Silenzia">M</button>
        <button class="ib ${St.solo===l.id?'on':''}" data-solo="${l.id}" title="Ascolta solo questa traccia">S</button>
        <button class="ib ${St.feel[l.id]&&Object.keys(St.feel[l.id]).length?'on':''}" data-feel="${l.id}" title="Feel: umanizzazione, swing, anticipo/ritardo">≈</button>
        <button class="ib dl" data-dl="${l.id}" title="Scarica solo questa traccia (.mid)">⬇</button></div>
      <div class="r2"><label class="vol" style="--vc:var(--c-${l.id})" title="Volume ${l.n}"><input type="range" min="0" max="150" value="${v}" data-vol="${l.id}"><span>${v}%</span></label>
        ${l.id==='drums'?`<button class="btn sm ${St.showPads?'primary-soft':''}" data-pads style="padding:3px 8px">Sequencer</button>`:`<span class="oct ${o?'on':''}" title="Ottava"><button data-oct="${l.id}" data-d="-1">−</button><span>${o>0?'+'+o:o}</span><button data-oct="${l.id}" data-d="1">+</button></span>`}</div></div>`;}).join('');
  h.querySelectorAll('[data-rl]').forEach(b=>b.onclick=()=>{const id=b.dataset.rl;St.reseed.L[id]=(St.reseed.L[id]||0)+1;
    if(St.mute[id])St.mute[id]=false;regen();toast(`${LAYERS.find(l=>l.id===id).n}: nuova versione`);});
  h.querySelectorAll('[data-mute]').forEach(b=>b.onclick=()=>{const id=b.dataset.mute;St.mute[id]=!St.mute[id];renderLanes();drawArr();track();});
  h.querySelectorAll('[data-solo]').forEach(b=>b.onclick=()=>{const id=b.dataset.solo;St.solo=St.solo===id?null:id;renderLanes();drawArr();track();});
  h.querySelectorAll('[data-oct]').forEach(b=>b.onclick=()=>{const id=b.dataset.oct;St.oct[id]=clamp((St.oct[id]||0)+(+b.dataset.d),-2,2);buildEv();renderLanes();track();
    toast(`${LAYERS.find(l=>l.id===id).n}: ottava ${St.oct[id]>0?'+':''}${St.oct[id]}`);});
  h.querySelectorAll('[data-vol]').forEach(el=>{el.oninput=()=>{const id=el.dataset.vol;St.vol[id]=+el.value/100;el.nextElementSibling.textContent=el.value+'%';Synth.setLayerGain(id,St.vol[id]);};
    el.onchange=()=>track();el.ondblclick=()=>{St.vol[el.dataset.vol]=1;applyVols();renderLanes();track();};});
  h.querySelectorAll('[data-feel]').forEach(b=>b.onclick=e=>{e.stopPropagation();openFeel(b.dataset.feel,b);});
  const pb=h.querySelector('[data-pads]');if(pb)pb.onclick=()=>{St.showPads=!St.showPads;renderLanes();renderPads();};
  h.querySelectorAll('[data-dl]').forEach(b=>b.onclick=()=>{const l=LAYERS.find(x=>x.id===b.dataset.dl);
    download(`${baseName()}-${slug(l.n)}.mid`,toMidi(St.song,{layers:[l.id],bpm:curBpm(),title:l.n,oct:St.oct,vol:St.vol,padMute:St.padMute}),'audio/midi');});
}
/* ---------------- Feel per traccia ---------------- */
let feelFor=null;
function openFeel(id,anchor){
  const pop=$('#feelPop'),F=St.feel[id]||{},G=GENRES[St.song.genre],hum=F.hum!=null?F.hum:1,sw=F.swing,sh=F.shift||0;feelFor=id;
  $('#feelBody').innerHTML=`<div class="phead"><div><div class="big" style="font-size:17px">Feel · ${LAYERS.find(l=>l.id===id).n}</div><div class="sub">Come suona questa traccia, non cosa suona</div></div><span class="spacer"></span><button class="ib" id="fClose">✕</button></div>
    <div class="pbody">
      <div class="frow"><span>Umanizzazione</span><input type="range" id="fHum" min="0" max="200" value="${Math.round(hum*100)}"><span id="fHumV">${Math.round(hum*100)}%</span></div>
      <div class="frow"><span>Swing</span><input type="range" id="fSw" min="-1" max="100" value="${sw==null?-1:Math.round(sw*100)}"><span id="fSwV">${sw==null?'genere ('+Math.round(G.swing*100)+'%)':Math.round(sw*100)+'%'}</span></div>
      <div class="frow"><span>Anticipo / ritardo</span><input type="range" id="fSh" min="-40" max="40" value="${sh}"><span id="fShV">${sh>0?'+':''}${sh} ms</span></div>
      <p class="dim" style="font-size:11.5px">Swing tutto a sinistra = quello del genere. Ritardo positivo = la traccia suona "dietro" il tempo (laid back), negativo = in anticipo (spinge).</p>
    </div><div class="pfoot"><button class="btn sm ghost" id="fReset">Ripristina</button></div>`;
  pop.hidden=false;
  const r=anchor.getBoundingClientRect(),w=pop.offsetWidth,left=clamp(r.left+r.width/2-w/2,12,innerWidth-w-12);
  pop.classList.remove('up');pop.style.left=left+scrollX+'px';pop.style.top=r.bottom+10+scrollY+'px';pop.querySelector('.parrow').style.left=clamp(r.left+r.width/2-left-7,16,w-30)+'px';
  const set=(k,v,re)=>{St.feel[id]=Object.assign({},St.feel[id],{[k]:v});if(v==null)delete St.feel[id][k];if(!Object.keys(St.feel[id]).length)delete St.feel[id];
    if(re)regen();else{buildEv();renderLanes();track();}};
  $('#fHum').oninput=e=>$('#fHumV').textContent=e.target.value+'%';$('#fHum').onchange=e=>set('hum',+e.target.value/100,true);
  $('#fSw').oninput=e=>$('#fSwV').textContent=+e.target.value<0?'genere':e.target.value+'%';$('#fSw').onchange=e=>set('swing',+e.target.value<0?null:+e.target.value/100,true);
  $('#fSh').oninput=e=>$('#fShV').textContent=(+e.target.value>0?'+':'')+e.target.value+' ms';$('#fSh').onchange=e=>set('shift',+e.target.value||null,false);
  $('#fReset').onclick=()=>{delete St.feel[id];regen();openFeel(id,$(`[data-feel="${id}"]`)||anchor);};
  $('#fClose').onclick=()=>{pop.hidden=true;feelFor=null;};
}
document.addEventListener('pointerdown',e=>{const p=$('#feelPop');if(!p.hidden&&!p.contains(e.target)&&!e.target.closest('[data-feel]')){p.hidden=true;feelFor=null;}});

/* ---------------- Pad batteria ---------------- */
const PAD_COL=['#ef4444','#f59e0b','#eab308','#22c55e','#14b8a6','#06b6d4','#3b82f6','#6366f1','#a855f7','#ec4899'];
const NOTE_NM=n=>KEY_NAMES[n%12]+(Math.floor(n/12)-1);
// sequencer: 10 pad × 32 passi (le due battute del groove della sezione selezionata)
function seqData(){
  const s=St.song,sec=s.sections[St.sel],t=sec.type,man=St.drumGrid[t]||{};
  const gen=Array.from({length:10},()=>Array(32).fill(0));
  s.layers.drums.forEach(e=>{const rel=e.t-sec.startBeat;if(rel<-.05||rel>=7.95)return;const st=clamp(Math.round(rel*4),0,31);gen[e.pad][st]=Math.max(gen[e.pad][st],e.v);});
  return{sec,t,rows:gen.map((g,p)=>{const m=man[p];return m?{v:m.length>16?m:m.concat(m),man:true}:{v:g,man:false};})};
}
function renderPads(){
  const s=St.song,panel=$('#padPanel'),has=s.layers.drums.length>0;
  panel.hidden=!(St.showPads&&has);if(panel.hidden)return;
  const kit=drumKit(GENRES[s.genre]),D=seqData(),nM=Object.keys(St.drumGrid[D.t]||{}).length;
  $('#padInfo').textContent=`${D.sec.name} · 2 battute di groove (si ripetono per tutta la sezione)${nM?` · ${nM} pad scritti a mano`:''}`;
  const head=`<div class="sq-h"></div>${Array.from({length:32},(_,k)=>`<div class="sq-n ${k%4===0?'b':''}">${k%16===0?'batt. '+(k/16+1):k%4===0?(k%16)/4+1:''}</div>`).join('')}`;
  $('#padGrid').innerHTML=head+PADS.map((p,i)=>{const R=D.rows[i],vol=St.padVol[i]!=null?St.padVol[i]:1;
    return`<div class="sq-h ${St.padMute[i]?'off':''}" data-row="${i}" style="--pc:${PAD_COL[i]}">
        <button class="sq-name" data-aud="${i}" title="Ascolta · nota ${kit.notes[i]} (${NOTE_NM(kit.notes[i])})"><i></i>${kit.names[i]}</button>
        <button class="ib ${St.padMute[i]?'on':''}" data-pm="${i}" title="Silenzia il pad">M</button>
        <button class="ib rg" data-pr="${i}" title="Rigenera solo questo pad">🎲</button>
        <input type="range" min="0" max="150" value="${Math.round(vol*100)}" data-pv="${i}" title="Volume ${Math.round(vol*100)}%">
        ${R.man?`<button class="ib on" data-pg="${i}" title="Scritto a mano: torna al generato">✎</button>`:'<span class="ib ghost" title="Generato">·</span>'}</div>`+
      R.v.map((v,k)=>`<div class="sq-c ${v>0?'on':''} ${R.man?'man':''} ${k%4===0?'b':''} ${k===16?'bar':''}" data-p="${i}" data-s="${k}" style="--pc:${PAD_COL[i]};--o:${v>0?(.35+v/127*.65).toFixed(2):0}"></div>`).join('');}).join('');
  const g=$('#padGrid');
  g.querySelectorAll('[data-aud]').forEach(b=>b.onclick=()=>{const i=+b.dataset.aud;Synth.init();Synth.hit(kit.notes[i],Synth.now()+.01,100*(St.padVol[i]!=null?St.padVol[i]:1),i===9?2:.2);flashPad(i);});
  g.querySelectorAll('[data-pm]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pm;St.padMute[i]=!St.padMute[i];if(!St.padMute[i])delete St.padMute[i];renderPads();drawArr();track();});
  g.querySelectorAll('[data-pr]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pr,k='drums.'+i;St.reseed.L[k]=(St.reseed.L[k]||0)+1;
    if(St.drumGrid[D.t]&&St.drumGrid[D.t][i]){delete St.drumGrid[D.t][i];}regen();toast(`${kit.names[i]}: nuova figura`);});
  g.querySelectorAll('[data-pv]').forEach(el=>{el.onchange=()=>{const i=+el.dataset.pv,v=+el.value/100;if(v===1)delete St.padVol[i];else St.padVol[i]=v;buildEv();track();};});
  g.querySelectorAll('[data-pg]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pg;setGrid(D.t,i,null);});
  g.querySelectorAll('.sq-c').forEach(el=>el.onclick=()=>{const i=+el.dataset.p,k=+el.dataset.s,arr=D.rows[i].v.map(x=>Math.round(x)),v=arr[k];
    arr[k]=v===0?110:v>=85?60:0;setGrid(D.t,i,arr);if(arr[k]){Synth.init();Synth.hit(kit.notes[i],Synth.now()+.01,arr[k],.2);}});
}
function setGrid(t,i,arr){St.drumGrid[t]=Object.assign({},St.drumGrid[t]);if(arr)St.drumGrid[t][i]=arr;else delete St.drumGrid[t][i];if(!Object.keys(St.drumGrid[t]).length)delete St.drumGrid[t];regen();}
function flashPad(i){const el=$(`#padGrid .sq-h[data-row="${i}"]`);if(!el)return;el.classList.add('flash');clearTimeout(el._t);el._t=setTimeout(()=>el.classList.remove('flash'),110);}
let lastSeqStep=-1;
function seqPlayhead(p){
  if(!St.showPads||$('#padPanel').hidden)return;const sec=St.song.sections[St.sel];
  const k=p>=sec.startBeat&&p<sec.startBeat+sec.bars*4?Math.floor(((p-sec.startBeat)%8)*4):-1;
  if(k===lastSeqStep)return;$$('#padGrid .sq-c.ph').forEach(e=>e.classList.remove('ph'));
  if(k>=0)$$(`#padGrid .sq-c[data-s="${k}"]`).forEach(e=>e.classList.add('ph'));lastSeqStep=k;
}
$('#seqNew').onclick=()=>{St.reseed.L.drums=(St.reseed.L.drums||0)+1;Object.keys(St.reseed.L).forEach(k=>{if(k.startsWith('drums.'))delete St.reseed.L[k];});regen();toast('Nuovo groove');};
$('#padClose').onclick=()=>{St.showPads=false;renderLanes();renderPads();};
const cache=document.createElement('canvas');
function cssVar(k){return getComputedStyle(document.documentElement).getPropertyValue(k).trim();}
function fitTxt(x,t,w){if(x.measureText(t).width<=w)return t;while(t.length>1&&x.measureText(t+'…').width>w)t=t.slice(0,-1);return x.measureText(t+'…').width<=w+2?t+'…':(x.measureText(t).width<=w+2?t:'');}
function drawArr(){
  const s=St.song,sc=$('#arrScroll'),W0=Math.max(200,sc.clientWidth),barW=clamp(W0/s.bars,24,120),W=Math.max(W0,Math.round(s.bars*barW));
  const L=lanes(),H=RULER+CHROW+L.reduce((a,l)=>a+LANE_H[l.id],0),dpr=window.devicePixelRatio||1,cv=$('#arr');
  cv.style.width=W+'px';cv.style.height=H+'px';cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);
  cache.width=cv.width;cache.height=cv.height;
  const x=cache.getContext('2d');x.setTransform(dpr,0,0,dpr,0,0);x.clearRect(0,0,W,H);
  const px=b=>b/4*barW;
  x.font='600 11.5px Inter,system-ui,sans-serif';x.textBaseline='middle';
  s.sections.forEach((sec,i)=>{const x0=px(sec.startBeat)+1.5,w=sec.bars*barW-3;
    x.fillStyle=cssVar('--s-'+sec.type);x.globalAlpha=i===St.sel?1:.42;rr(x,x0,4,w,RULER-8,6);x.fill();x.globalAlpha=1;
    x.fillStyle=i===St.sel?'#fff':cssVar('--tx');x.fillText(fitTxt(x,sec.name,w-12),x0+8,RULER/2);});
  x.font='500 11px Inter,system-ui,sans-serif';x.fillStyle=cssVar('--tx2');
  s.chords.forEach(c=>{const x0=px(c.start),w=c.beats/4*barW;if(w<14)return;x.fillText(fitTxt(x,c.name.split('/')[0],w-5),x0+3,RULER+CHROW/2);});
  let y=RULER+CHROW;const line=cssVar('--line');
  L.forEach((l,li)=>{
    const h=LANE_H[l.id],ev=s.layers[l.id],on=audible(l.id);
    x.fillStyle=li%2?cssVar('--panel'):cssVar('--panel2');x.fillRect(0,y,W,h);x.fillStyle=line;x.fillRect(0,y,W,1);
    x.fillStyle=cssVar('--c-'+l.id);
    if(l.id==='drums'){
      const rh=(h-10)/10;
      ev.forEach(e=>{const off=St.padMute[e.pad];x.fillStyle=PAD_COL[e.pad];x.globalAlpha=on&&!off?.3+e.v/220:.1;
        x.fillRect(px(e.t),y+5+(9-e.pad)*rh,e.pad===9&&e.d>1?px(e.d):Math.max(1.5,barW/18),Math.max(1.5,rh-1));});
    }else{
      let lo=127,hi=0;ev.forEach(e=>{if(e.n<lo)lo=e.n;if(e.n>hi)hi=e.n;});lo-=1;hi+=1;
      const nh=clamp((h-10)/(hi-lo+1),2,5);
      ev.forEach(e=>{x.globalAlpha=on?.3+e.v/200:.12;x.fillRect(px(e.t),y+5+(hi-e.n)/(hi-lo)*(h-10-nh),Math.max(1.5,px(e.d)-.6),nh);});
    }
    x.globalAlpha=1;y+=h;
  });
  for(let b=0;b<=s.bars;b++){const isSec=s.sections.some(z=>z.startBar===b);x.fillStyle=line;x.globalAlpha=isSec?1:.55;x.fillRect(Math.round(b*barW),RULER+CHROW,1,H);}
  x.globalAlpha=1;St.layout={W,H,barW,dpr};
  paintArr(Player.playing?Player.pos():-1);
}
function rr(x,a,b,w,h,r){x.beginPath();x.moveTo(a+r,b);x.arcTo(a+w,b,a+w,b+h,r);x.arcTo(a+w,b+h,a,b+h,r);x.arcTo(a,b+h,a,b,r);x.arcTo(a,b,a+w,b,r);x.closePath();}
function paintArr(pos){
  const cv=$('#arr'),x=cv.getContext('2d');x.setTransform(1,0,0,1,0,0);x.clearRect(0,0,cv.width,cv.height);x.drawImage(cache,0,0);
  if(pos>=0&&St.layout){const{barW,dpr,H}=St.layout,X=pos/4*barW*dpr;x.fillStyle=cssVar('--acc');x.fillRect(X-dpr,0,2*dpr,H*dpr);
    x.globalAlpha=.08;x.fillRect(0,0,X,H*dpr);x.globalAlpha=1;}
}
function selectSection(i,seek){St.sel=i;St.selChord=-1;drawArr();renderSection();renderPads();if(seek&&Player.playing)Player.start(St.song.sections[i].startBeat);}
$('#arr').addEventListener('click',e=>{
  if(!St.layout)return;const beat=e.offsetX/St.layout.barW*4;
  const i=St.song.sections.findIndex(s=>beat>=s.startBeat&&beat<s.startBeat+s.bars*4);if(i>=0)selectSection(i,true);});
let rzT;addEventListener('resize',()=>{clearTimeout(rzT);rzT=setTimeout(()=>{if(St.song&&!$('#tab-gen').hidden){drawArr();renderSection();}},120);});

/* ---------------- Sezione: timeline ed editor accordi ---------------- */
const QUAL=['maj','min','7','maj7','m7','9','maj9','m9','add9','6','sus2','sus4','7sus4','dim','dim7','m7b5','aug','13','m11','7b9','69','maj7#11'];
const qLab=q=>q==='maj'?'M':q==='min'?'m':QT[q].n;
const lenLab=b=>{const bars=Math.floor(b/4+1e-9),rest=b-bars*4;let s=bars?bars+(bars>1?' batt.':' batt.'):'';
  if(rest>1e-6){const sx=Math.round(rest*4);s+=(s?' + ':'')+(sx%4===0?(sx/4)+(sx/4>1?' tempi':' tempo'):sx+'/16');}return s;};
function renderSection(){
  const s=St.song,sec=s.sections[St.sel],B=sec.bars*4;
  $('#secTitle').textContent=sec.name;
  $('#secInfo').textContent=`battute ${sec.startBar+1}–${sec.startBar+sec.bars} · ${sec.chords.length} accordi`+(sec.texture?` · pianoforte: ${TEX_NAME[sec.texture]}`:'');
  $('#delSec').disabled=s.sections.length<2;
  $('#regenSec').innerHTML=`<span class="label">🎲 Rigenera ${SEC_NAME[sec.type].toLowerCase()}</span><div class="seg">
    <button data-rs="h" title="Nuovi accordi per questa sezione e le sue ripetizioni">Accordi</button><button data-rs="a" title="Nuovo arrangiamento (piano, arpeggio, basso, batteria)">Arrangiamento</button>
    <button data-rs="m" title="Nuova melodia">Melodia</button><button data-rs="all">Tutto</button></div>`;
  $$('#regenSec [data-rs]').forEach(b=>b.onclick=()=>{const k=b.dataset.rs,t=sec.type,R=St.reseed.S,ks=k==='all'?['h','a','m']:[k];
    ks.forEach(x=>R[t+'.'+x]=(R[t+'.'+x]||0)+1);
    if(ks.includes('h')){const n=St.edits.length;St.edits=St.edits.filter(e=>e.type!==t);if(St.edits.length<n)toast('Modifiche agli accordi di questa sezione azzerate');}
    closePop();regen();toast(`${SEC_NAME[t]}: ${{h:'nuovi accordi',a:'nuovo arrangiamento',m:'nuova melodia',all:'tutto nuovo'}[k]}`);});
  $('#tlBars').innerHTML=Array.from({length:sec.bars},(_,b)=>`<span style="left:${b*4/B*100}%">${sec.startBar+b+1}</span>`).join('');
  const tl=$('#tl');tl.style.setProperty('--b',B);const mw=sec.bars*64+'px';tl.style.minWidth=mw;$('#tlBars').style.minWidth=mw;
  tl.innerHTML=sec.chords.map((c,i)=>{const ci=s.chords.indexOf(c),l=(c.start-sec.startBeat)/B*100,w=c.beats/B*100;
    return`<div class="blk ${fnClass(c)} ${i===St.selChord?'sel':''} ${hasEdit(sec,i)?'ed':''}" data-i="${i}" data-ci="${ci}" style="left:calc(${l}% + 2px);width:calc(${w}% - 4px)">
      <div class="nm">${c.name}</div><div class="rn">${c.rn}</div><div class="ln">${lenLab(c.beats)}</div></div>`;}).join('')+
    sec.chords.slice(0,-1).map((c,i)=>`<div class="hd" data-h="${i}" style="left:${(c.start+c.beats-sec.startBeat)/B*100}%"></div>`).join('');
  tl.querySelectorAll('.blk').forEach(el=>el.onclick=()=>{const i=+el.dataset.i;if(i===St.selChord&&!$('#pop').hidden){closePop();return;}
    St.selChord=i;tl.querySelectorAll('.blk').forEach(b=>b.classList.toggle('sel',b===el));renderPop();audition(sec.chords[i]);});
  tl.querySelectorAll('.hd').forEach(el=>el.addEventListener('pointerdown',e=>startDrag(e,+el.dataset.h)));
  renderPop();lastCi=-1;
}
const hasEdit=(sec,i)=>St.edits.some(e=>e.type===sec.type&&e.ci===i&&(e.sel==='rep'||e.occ===sec.occ));
function audition(c){if(!c)return;Synth.init();ensureSound();const t=Synth.now()+.03,inst=instOf('piano'),notes=[...c.lh,...c.rh];
  notes.forEach((n,j)=>Synth.note(inst,n,t+(c.strum&&c.strum.dir?(c.strum.dir==='down'?notes.length-1-j:j)*c.strum.spd*60/curBpm():.012*j),1.9,clamp(76+(c.vel||0),20,124),'piano'));}
function startDrag(e,i){
  e.preventDefault();const sec=St.song.sections[St.sel],B=sec.bars*4,tl=$('#tl'),rect=tl.getBoundingClientRect();
  const beats=sec.chords.map(c=>c.beats),a=sec.chords.slice(0,i).reduce((x,c)=>x+c.beats,0),b=a+beats[i]+beats[i+1];
  const hd=tl.querySelector(`[data-h="${i}"]`),tip=document.createElement('div');tip.className='tip';tl.append(tip);hd.classList.add('drag');
  let cut=a+beats[i];
  const move=ev=>{const x=clamp((ev.clientX-rect.left)/rect.width,0,1)*B;cut=clamp(Math.round(x*4)/4,a+.25,b-.25);
    const bl=tl.querySelectorAll('.blk');bl[i].style.width=`calc(${(cut-a)/B*100}% - 4px)`;
    bl[i+1].style.left=`calc(${cut/B*100}% + 2px)`;bl[i+1].style.width=`calc(${(b-cut)/B*100}% - 4px)`;
    hd.style.left=cut/B*100+'%';tip.style.left=cut/B*100+'%';
    const bar=Math.floor(cut/4),beat=cut-bar*4;tip.textContent=`batt. ${sec.startBar+bar+1} · ${Math.floor(beat)+1}${beat%1?'.'+Math.round((beat%1)*4+1)+'/4':''}`;};
  const up=()=>{removeEventListener('pointermove',move);removeEventListener('pointerup',up);tip.remove();hd.classList.remove('drag');
    if(Math.abs(cut-(a+beats[i]))<1e-6){renderSection();return;}
    const nb=beats.slice();nb[i]=cut-a;nb[i+1]=b-cut;pushEdit({op:'len',beats:nb},St.scope==='one'?'one':'rep');};
  addEventListener('pointermove',move);addEventListener('pointerup',up);move(e);
}
function pushEdit(e,sel){
  const sec=St.song.sections[St.sel],c=sec.chords[St.selChord]||sec.chords[0];
  St.edits.push({...e,sel:sel||St.scope,type:sec.type,occ:sec.occ,ci:St.selChord<0?0:St.selChord,match:{r:c.r,q:c.q}});
  if(e.op==='split'&&St.selChord>=0){}else if(e.op==='merge')St.selChord=Math.max(0,St.selChord-1);
  regen();
}
function fnClass(c){
  const md=MODES[St.song.mode];if(c.sec||c.tt||c.pass||!TRI_IV[c.tri])return'fB';
  if(!inScale(md.scale,c.r,c.tri))return'fB';const r=role(c,md.minor);return r==='T'||r==='Tm'?'fT':r==='S'?'fS':'fD';}
const QGROUPS=[['Triadi',['maj','min','dim','aug','sus2','sus4']],['Settime',['7','maj7','m7','m7b5','dim7','7sus4']],['Estese',['add9','6','69','9','maj9','m9','m11','13','7b9','maj7#11']]];
function closePop(){St.selChord=-1;$('#pop').hidden=true;$$('#tl .blk.sel').forEach(e=>e.classList.remove('sel'));}
function placePop(){
  const pop=$('#pop');if(pop.hidden)return;const el=$(`#tl .blk[data-i="${St.selChord}"]`);if(!el){pop.hidden=true;return;}
  if(innerWidth<=560){pop.style.left='';pop.style.top='';return;}
  const r=el.getBoundingClientRect(),w=pop.offsetWidth,h=pop.offsetHeight;
  const left=clamp(r.left+r.width/2-w/2,12,innerWidth-w-12);
  const below=r.bottom+14+h<innerHeight-78||r.top-14-h<70;
  pop.classList.toggle('up',!below);
  pop.style.left=left+scrollX+'px';pop.style.top=(below?r.bottom+12:r.top-12-h)+scrollY+'px';
  $('#parrow').style.left=clamp(r.left+r.width/2-left-7,18,w-32)+'px';
}
function renderPop(){
  const pop=$('#pop'),s=St.song,sec=s.sections[St.sel],c=sec.chords[St.selChord];
  if(!c){pop.hidden=true;return;}
  const sp=s.spell,A=chordAdvice(s,St.sel,St.selChord),iv=QT[c.q].iv;
  const bassIv=c.bassIv!=null?c.bassIv:(c.inv?iv.findIndex(x=>mod12(c.pc+x)===c.bass):0);
  const sd=c.strum&&c.strum.dir?c.strum.dir:'',spd=c.strum?c.strum.spd:.04,base=c.name.split('/')[0];
  const isCur=x=>mod12(c.r)===mod12(x.r)&&c.q===x.q;
  const chip=(x,cls,extra)=>`<button class="qc ${cls} ${isCur(x)?'on':''}" data-sr="${x.r}" data-sq="${x.q}" title="${(x.why||'').replace(/"/g,'')}">${x.lab}<small>${x.name}</small>${extra||''}</button>`;
  const md=MODES[s.mode],rootOpts=Array.from({length:12},(_,pc)=>{const r=mod12(pc-s.key),d=md.scale.indexOf(r);
    return`<option value="${pc}" ${mod12(c.pc)===pc?'selected':''}>${sp(pc)}${d>=0?' · '+ROMAN[d]:' · fuori scala'}</option>`;}).join('');
  const qIn=QGROUPS.flatMap(g=>g[1]).filter(q=>A.inQ.has(q)),qOut=QGROUPS.flatMap(g=>g[1]).filter(q=>!A.inQ.has(q));
  $('#popBody').innerHTML=`
    <div class="phead"><div><div class="big">${c.name}</div><div class="sub">${c.rn} · ${A.curRole} · ${lenLab(c.beats)} · ${sec.name}, batt. ${Math.floor(c.start/4)+1}</div>
      <div style="margin-top:6px"><span class="fitlab ${A.inScale?'in':'out'}">${A.inScale?'✓ nella scala di '+s.keyName+' '+md.n.toLowerCase():'◐ fuori scala (prestito o cromatico)'}</span></div></div>
      <span class="spacer"></span><button class="btn sm" data-a="play" title="Ascolta">🔊</button><button class="ib" data-a="close" title="Chiudi (Esc)">✕</button></div>
    <div class="pbody">
      <div class="psec"><span class="label">Applica la modifica a</span><div class="seg full">
        <button data-sc="one" class="${St.scope==='one'?'on':''}">Solo questo</button>
        <button data-sc="rep" class="${St.scope==='rep'?'on':''}">Ogni ${SEC_NAME[sec.type]}</button>
        <button data-sc="same" class="${St.scope==='same'?'on':''}">Ogni ${base}</button></div></div>
      <div class="psec"><span class="label">★ Consigliati qui <span class="hint">· nella scala, scelti in base a ${A.rec.length&&St.selChord>=0?'accordo prima e dopo':'contesto'}</span></span>
        <div class="rec">${A.rec.map(x=>`<button class="reci" data-sr="${x.r}" data-sq="${x.q}"><span class="star">★</span><b>${x.name}</b><span class="rnl">${x.lab}</span><small>${x.why}</small></button>`).join('')}</div></div>
      <div class="psec"><span class="label">Tutti gli accordi della scala <span class="hint">· ${md.n.toLowerCase()}</span></span>
        <div class="prow">${A.scale.map(x=>chip(x,'in')).join('')}</div></div>
      <div class="psec"><span class="label">Colore di ${base.replace(QT[c.q].n,'')||base} <span class="hint">· verde = sta nella scala</span></span>
        <div class="prow">${qIn.map(q=>`<button class="qc in ${c.q===q?'on':''}" data-q="${q}">${qLab(q)}</button>`).join('')}</div>
        <details class="more"><summary>Fuori scala (${qOut.length})</summary><div class="prow" style="margin-top:6px">${qOut.map(q=>`<button class="qc out ${c.q===q?'on':''}" data-q="${q}">${qLab(q)}</button>`).join('')}</div></details>
        <div class="prow" style="margin-top:2px"><span class="label">Radice</span><select id="tbRoot">${rootOpts}</select></div></div>
      <div class="psec"><span class="label">Interscambio modale <span class="hint">· fuori scala, presi dai modi paralleli: colore più scuro o luminoso</span></span>
        <div class="prow">${A.bor.map(x=>chip(x,'bor',`<span class="src">${x.src.toLowerCase()}</span>`)).join('')}</div></div>
      ${A.chrom.length?`<div class="psec"><span class="label">Cromatici verso l'accordo dopo <span class="hint">· tensione che risolve</span></span>
        <div class="prow">${A.chrom.map(x=>chip(x,'chrom')).join('')}</div></div>`:''}
      <div class="psec"><span class="label">Nota al basso</span><div class="seg">${['Fondamentale','3ª','5ª','7ª'].map((t,i)=>i<iv.length?`<button data-bass="${i}" class="${bassIv===i?'on':''}">${t}</button>`:'').join('')}</div></div>
      <div class="psec"><span class="label">Strum</span><div class="prow"><div class="seg">
        <button data-st="" class="${!sd?'on':''}">No</button><button data-st="up" class="${sd==='up'?'on':''}">↑ dal basso</button><button data-st="down" class="${sd==='down'?'on':''}">↓ dall'alto</button></div>
        <input type="range" id="tbSpd" min="1" max="12" value="${Math.round(spd*100)}" title="Velocità dello strum" ${sd?'':'disabled'}></div></div>
      <div class="psec"><span class="label">Velocity <b id="tbVelV" style="color:var(--tx)">${(c.vel||0)>0?'+':''}${c.vel||0}</b></span><input type="range" id="tbVel" min="-40" max="40" value="${c.vel||0}" style="width:100%"></div>
    </div>
    <div class="pfoot"><button class="btn sm" data-a="split" ${c.beats<.5?'disabled':''}>✂ Dividi</button>
      <button class="btn sm" data-a="merge" ${St.selChord<1?'disabled':''}>⇤ Unisci al precedente</button><span class="spacer"></span>
      <button class="btn sm ghost" data-a="reset" ${St.edits.length?'':'disabled'}>Ripristina accordi</button></div>`;
  pop.hidden=false;
  const scopeFor=op=>St.scope==='same'&&['split','merge','len'].includes(op)?'rep':St.scope;
  pop.querySelectorAll('[data-sc]').forEach(b=>b.onclick=()=>{St.scope=b.dataset.sc;renderPop();});
  $('#tbRoot').onchange=e=>{const r=mod12(+e.target.value-s.key),fit=fitQ(md.scale,r),q=fit.includes(c.q)?c.q:(fit.find(x=>qTri(x)===qTri(c.q))||c.q);pushEdit({op:'chord',r,q});};
  pop.querySelectorAll('[data-q]').forEach(b=>b.onclick=()=>pushEdit({op:'chord',r:c.r,q:b.dataset.q}));
  pop.querySelectorAll('[data-sr]').forEach(b=>b.onclick=()=>pushEdit({op:'chord',r:+b.dataset.sr,q:b.dataset.sq}));
  pop.querySelectorAll('[data-bass]').forEach(b=>b.onclick=()=>pushEdit({op:'bass',iv:+b.dataset.bass}));
  pop.querySelectorAll('[data-st]').forEach(b=>b.onclick=()=>pushEdit({op:'strum',dir:b.dataset.st,spd:+$('#tbSpd').value/100}));
  $('#tbSpd').onchange=e=>{if(sd)pushEdit({op:'strum',dir:sd,spd:+e.target.value/100});};
  $('#tbVel').oninput=e=>{$('#tbVelV').textContent=(+e.target.value>0?'+':'')+e.target.value;};
  $('#tbVel').onchange=e=>pushEdit({op:'vel',v:+e.target.value});
  pop.querySelectorAll('[data-a]').forEach(b=>b.onclick=()=>{const a=b.dataset.a;
    if(a==='play')audition(c);else if(a==='close')closePop();
    else if(a==='split')pushEdit({op:'split'},scopeFor('split'));else if(a==='merge')pushEdit({op:'merge'},scopeFor('merge'));
    else if(a==='reset'){St.edits=[];regen();toast('Accordi originali ripristinati');}});
  requestAnimationFrame(placePop);
}
document.addEventListener('pointerdown',e=>{const pop=$('#pop');if(pop.hidden)return;
  if(pop.contains(e.target)||e.target.closest('#tl .blk')||e.target.closest('.hd'))return;closePop();});
addEventListener('scroll',()=>placePop(),{passive:true});
$('.tlwrap').addEventListener('scroll',()=>placePop(),{passive:true});
$('#addSec').onclick=()=>{
  const t=$('#addType').value,s=St.song,secs=s.secs.map(x=>x.slice()),order=s.typeOrder.slice();
  const bars=secs.find(x=>x[0]===t)?secs.find(x=>x[0]===t)[1]:(GENRES[s.genre].blues&&(t==='verse'||t==='chorus')?12:SEC_BARS[t]);
  secs.splice(St.sel+1,0,[t,bars]);if(TYPE_ORDER.includes(t)&&!order.includes(t))order.push(t);
  St.secs=secs;St.typeOrder=order;St.sel=St.sel+1;St.selChord=-1;regen();toast(`${SEC_NAME[t]} inserito dopo la sezione selezionata`);};
$('#delSec').onclick=()=>{const s=St.song;if(s.sections.length<2)return;const secs=s.secs.map(x=>x.slice());const nm=s.sections[St.sel].name;
  secs.splice(St.sel,1);St.secs=secs;St.typeOrder=s.typeOrder.slice();St.sel=Math.max(0,St.sel-1);St.selChord=-1;regen();toast(`${nm} rimosso`);};

const MODE_DESC={major:'luminoso, cadenze V–I',minor:'malinconico, con bVI e bVII',harmonic:'minore con V maggiore e sensibile',dorian:'minore con 6ª maggiore: il IV è maggiore',
  phrygian:'minore scuro con 2ª minore: il bII',lydian:'maggiore sospeso con #4: il II è maggiore',mixolydian:'maggiore con 7ª minore: il bVII'};
function renderHow(){
  const s=St.song,G=GENRES[s.genre];
  $('#genreDesc').textContent=G.desc;
  const items=[`Modo <b>${MODES[s.mode].n}</b>: ${MODE_DESC[s.mode]}.`,
    `Progressioni scelte confrontando i giri tipici del genere con catene di Markov sulle funzioni armoniche (T–S–D), con inizio e fine adatti a ogni sezione.`];
  if(St.opts.pianoStyle==='live'){const seen=new Set(),tx=[];s.sections.forEach(z=>{if(!seen.has(z.type)&&z.texture){seen.add(z.type);tx.push(`${SEC_NAME[z.type].toLowerCase()}: ${TEX_NAME[z.texture]}`);}});
    items.push(`Pianoforte vivo — ${tx.join('; ')}. Densità, vuoti e pieni seguono genere, mood e sezione.`);}
  else items.push('Pianoforte classico (v3): accordi ritmici con anticipi in tutte le sezioni.');
  const lay=s.sections.map(z=>{const p=z.plan,on=['piano',p.arp&&'arpeggio',p.pad&&'pad',p.bass&&'basso',p.drums!=null&&'batteria',p.mel&&'melodia'].filter(Boolean);return`${z.name}: ${on.join(', ')}`;});
  if(s.sections.length>1)items.push('Arrangiamento — '+lay.join(' · ')+'.');
  items.push(`Melodia a frasi domanda–risposta, adattata agli accordi (anche quelli modificati a mano).`);
  s.info.filter(x=>!/^arpeggio/.test(x)).forEach(x=>items.push(x[0].toUpperCase()+x.slice(1)+'.'));
  $('#howList').innerHTML=items.map(x=>`<li>${x}</li>`).join('');
}

/* ---------------- Trasporto ---------------- */
const Player={playing:false,base:0,p:0,timer:null,raf:0,
  range(){const s=St.song;if(St.loop){const sec=s.sections[St.sel];return[sec.startBeat,sec.startBeat+sec.bars*4];}return[0,s.beats];},
  first(b){const ev=St.ev;let lo=0,hi=ev.length;while(lo<hi){const m=(lo+hi)>>1;if(ev[m].t<b-1e-6)lo=m+1;else hi=m;}return lo;},
  pos(){const spb=60/curBpm(),[L0,L1]=this.range();let p=(Synth.now()-this.base)/spb;if(p<L0)p+=L1-L0;return clamp(p,0,St.song.beats);},
  start(beat){
    Synth.init();ensureSound();this.stop(true);const[L0,L1]=this.range();const b=clamp(beat==null?L0:beat,L0,L1-.01);
    this.playing=true;this.base=Synth.now()+.08-b*60/curBpm();this.p=this.first(b);
    this.timer=setInterval(()=>this.sched(),25);this.sched();this.loop();
    $('#playIc').innerHTML='<path fill="currentColor" d="M4 3h3.6v12H4zM10.4 3H14v12h-3.6z"/>';
  },
  stop(silent){this.playing=false;clearInterval(this.timer);cancelAnimationFrame(this.raf);
    $('#playIc').innerHTML='<path fill="currentColor" d="M4 2.5v13l11-6.5z"/>';
    if(!silent){paintArr(-1);$$('.blk.on').forEach(e=>e.classList.remove('on'));lastCi=-1;updateTransport(0);}},
  sched(){
    const now=Synth.now(),spb=60/curBpm(),[L0,L1]=this.range(),ev=St.ev;
    for(let guard=0;guard<6000;guard++){
      const e=ev[this.p];
      if(!e||e.t>=L1){if(now+.2<this.base+L1*spb)break;this.base+=(L1-L0)*spb;this.p=this.first(L0);continue;}
      const at=this.base+e.t*spb;if(at>now+.2)break;
      if(at>=now-.04&&audible(e.l)){if(e.l==='drums'){if(!St.padMute[e.pad]){Synth.hit(e.n,at,e.v,e.d*spb);if(St.showPads){const pd=e.pad;setTimeout(()=>flashPad(pd),Math.max(0,(at-now)*1000));}}}
        else Synth.note(instOf(e.l),e.n,at,e.d*spb,e.v,e.l,e.gl);}
      this.p++;
    }
  },
  loop(){if(!this.playing)return;const p=this.pos();paintArr(p);updateTransport(p);seqPlayhead(p);
    const sc=$('#arrScroll'),X=p/4*St.layout.barW;if(X<sc.scrollLeft+20||X>sc.scrollLeft+sc.clientWidth-40)sc.scrollLeft=Math.max(0,X-60);
    this.raf=requestAnimationFrame(()=>this.loop());}};
function renderProg(){
  const s=St.song;$('#tprog').innerHTML=s.sections.map((z,i)=>`<div class="seg2" data-s="${i}" title="${z.name}" style="width:${z.bars*4/s.beats*100}%;background:var(--s-${z.type})"></div>`).join('')+'<div class="ph" id="tph"></div>';}
$('#tprog').addEventListener('click',e=>{const r=e.currentTarget.getBoundingClientRect(),b=clamp((e.clientX-r.left)/r.width,0,.999)*St.song.beats;
  const i=St.song.sections.findIndex(z=>b>=z.startBeat&&b<z.startBeat+z.bars*4);if(i>=0&&i!==St.sel)selectSection(i,false);
  if(St.loop&&!Player.playing){Player.start(St.song.sections[St.sel].startBeat);return;}Player.start(St.loop?St.song.sections[St.sel].startBeat:b);});
let lastCi=-1;
function updateTransport(p){
  const s=St.song,spb=60/curBpm();
  const sec=s.sections.find(z=>p>=z.startBeat&&p<z.startBeat+z.bars*4)||s.sections[0];
  $('#posSec').textContent=`${sec.name} · batt. ${Math.floor(p/4)+1}`;
  $('#posTime').textContent=`${fmtTime(p*spb)} / ${fmtTime(s.beats*spb)}`;
  $('#bpm').value=curBpm();$('#bpmv').textContent=curBpm()+' BPM';
  const ph=$('#tph');if(ph)ph.style.left=p/s.beats*100+'%';
  $$('#tprog .seg2').forEach((el,i)=>el.classList.toggle('cur',s.sections[i]===sec));
  let ci=-1;if(Player.playing)s.chords.forEach((c,i)=>{if(c.start<=p)ci=i;});
  if(ci!==lastCi){$$('.blk.on').forEach(e=>e.classList.remove('on'));const el=$(`.blk[data-ci="${ci}"]`);if(el)el.classList.add('on');lastCi=ci;}
}
$('#play').onclick=()=>Player.playing?Player.stop():Player.start(St.loop?null:0);
$('#loopBtn').onclick=()=>{St.loop=!St.loop;$('#loopBtn').classList.toggle('on',St.loop);
  if(Player.playing)Player.start(St.song.sections[St.sel].startBeat);toast(St.loop?'Loop sulla sezione selezionata':'Riproduzione di tutto il brano');};
$('#bpm').onchange=()=>{buildEv();track();};
$('#bpm').oninput=e=>{const p=Player.playing?Player.pos():0;St.bpm=+e.target.value;
  if(Player.playing)Player.base=Synth.now()-p*60/curBpm();$('#bpmv').textContent=curBpm()+' BPM';renderHead();};
$('#bpmAuto').onclick=()=>{const p=Player.playing?Player.pos():0;St.bpm=null;if(Player.playing)Player.base=Synth.now()-p*60/curBpm();renderHead();updateTransport(p);track();};
$('#vol').oninput=e=>Synth.setVolume(+e.target.value/100);
document.addEventListener('keydown',e=>{
  if($('#tab-gen').hidden||/INPUT|SELECT|TEXTAREA/.test(e.target.tagName))return;
  if(e.key==='Escape'){if(!$('#modal').hidden)closeModal();else closePop();return;}
  if(e.metaKey||e.ctrlKey){const k=e.key.toLowerCase();if(k==='z'){e.preventDefault();e.shiftKey?redo():undo();}else if(k==='y'){e.preventDefault();redo();}else if(k==='s'){e.preventDefault();saveProject();}return;}
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
  download(baseName()+'.mid',toMidi(St.song,{layers:ls,bpm:curBpm(),oct:St.oct,vol:St.vol,padMute:St.padMute,shift:feelShift(),padVol:St.padVol,title:`${GENRES[St.song.genre].n} · ${MOODS[St.song.mood].n}`}),'audio/midi');};
$('#expSec').onclick=()=>{const sec=St.song.sections[St.sel];
  download(`${baseName()}-${slug(sec.name)}.mid`,toMidi(St.song,{layers:allLayers(),from:sec.startBeat,to:sec.startBeat+sec.bars*4,bpm:curBpm(),oct:St.oct,vol:St.vol,padMute:St.padMute,shift:feelShift(),padVol:St.padVol,title:sec.name}),'audio/midi');};
$('#expZip').onclick=()=>{
  const s=St.song,b=baseName(),bpm=curBpm(),files=[],oct=St.oct,vol=St.vol,padMute=St.padMute,shift=feelShift(),padVol=St.padVol;
  files.push({name:`${b}/${b} - completo.mid`,data:toMidi(s,{layers:allLayers(),bpm,oct,vol,padMute,shift,padVol,title:b})});
  lanes().forEach((l,i)=>files.push({name:`${b}/tracce/${i+1} ${l.n}.mid`,data:toMidi(s,{layers:[l.id],bpm,oct,vol,padMute,shift,padVol,title:l.n})}));
  const seen=new Set();let k=1;
  s.sections.forEach(sec=>{if(seen.has(sec.type))return;seen.add(sec.type);
    files.push({name:`${b}/sezioni/${k++} ${SEC_NAME[sec.type]}.mid`,data:toMidi(s,{layers:allLayers(),from:sec.startBeat,to:sec.startBeat+sec.bars*4,bpm,oct,vol,padMute,shift,padVol,title:SEC_NAME[sec.type]})});});
  files.push({name:`${b}/accordi.txt`,data:chordChart({...s,bpm})});
  files.push({name:`${b}/LEGGIMI.txt`,data:[
    'PIANO GENERATIVO — pacchetto MIDI','',`Brano: ${GENRES[s.genre].n} · ${MOODS[s.mood].n}`,`Tonalità: ${s.keyName} ${MODES[s.mode].n} · ${bpm} BPM · 4/4 · ${s.bars} battute`,
    `Struttura: ${s.sections.map(z=>z.name).join(' → ')}`,`Codice: ${codeOf()}${St.edits.length?` (+ ${St.edits.length} modifiche manuali agli accordi, già incluse nei file)`:''}`,'',
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
  if(gen){drawArr();renderSection();}else{closePop();if(typeof Ex!=='undefined')Ex.show();}});

/* ---------------- Storia: indietro / avanti ---------------- */
const H={undo:[],redo:[],last:null,restoring:false};
const snapState=()=>JSON.stringify({opts:St.opts,seeds:St.seeds,secs:St.secs,typeOrder:St.typeOrder,edits:St.edits,oct:St.oct,mute:St.mute,solo:St.solo,bpm:St.bpm,name:St.name,reseed:St.reseed,vol:St.vol,padMute:St.padMute,padVol:St.padVol,padDens:St.padDens,drumGrid:St.drumGrid,feel:St.feel});
function track(){
  const cur=snapState();
  if(H.last&&cur!==H.last&&!H.restoring){H.undo.push(H.last);if(H.undo.length>200)H.undo.shift();H.redo=[];}
  H.last=cur;LS.set('pg_current',JSON.parse(cur));updHist();
}
function updHist(){
  $('#undo').disabled=!H.undo.length;$('#redo').disabled=!H.redo.length;
  const st=$('#saveState');
  if(!St.projId){st.className='sstate';st.textContent='non salvato';}
  else if(St.savedSnap===H.last){st.className='sstate saved';st.textContent='salvato';}
  else{st.className='sstate dirty';st.textContent='modifiche non salvate';}
}
function restore(json){
  const o=JSON.parse(json);H.restoring=true;
  Object.assign(St,{opts:Object.assign({},DEF,o.opts),seeds:o.seeds,secs:o.secs||null,typeOrder:o.typeOrder||null,edits:o.edits||[],oct:o.oct||{},
    mute:o.mute||{},solo:o.solo||null,bpm:o.bpm||null,name:o.name||St.name,muteInit:true,reseed:JSON.parse(JSON.stringify(o.reseed||{L:{},S:{}})),vol:{...(o.vol||{})},padMute:{...(o.padMute||{})},padVol:{...(o.padVol||{})},padDens:{...(o.padDens||{})},drumGrid:JSON.parse(JSON.stringify(o.drumGrid||{})),feel:JSON.parse(JSON.stringify(o.feel||{}))});
  syncControls();$('#projName').value=St.name;closePop();
  if(St.sel>=0)St.sel=Math.min(St.sel,99);regen();H.restoring=false;H.last=json;LS.set('pg_current',o);updHist();
}
function undo(){if(!H.undo.length)return;H.redo.push(H.last);restore(H.undo.pop());toast('Indietro');}
function redo(){if(!H.redo.length)return;H.undo.push(H.last);restore(H.redo.pop());toast('Avanti');}
$('#undo').onclick=undo;$('#redo').onclick=redo;

/* ---------------- Progetti ---------------- */
const projects=()=>LS.get('pg_projects',[]);
const saveMeta=()=>LS.set('pg_meta',{projId:St.projId,savedSnap:St.savedSnap});
function writeProject(asNew,name){
  const list=projects(),now=Date.now();if(name)St.name=name;$('#projName').value=St.name;track();const state=JSON.parse(H.last);
  if(asNew||!St.projId||!list.some(p=>p.id===St.projId)){St.projId='p'+now.toString(36);list.unshift({id:St.projId,name:St.name,date:now,state});}
  else{const p=list.find(p=>p.id===St.projId);Object.assign(p,{name:St.name,date:now,state});}
  LS.set('pg_projects',list);St.savedSnap=H.last;saveMeta();updHist();toast(`Progetto “${St.name}” salvato`);
}
const esc=t=>String(t).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;');
function ask(title,html,buttons){
  $('#mTitle').textContent=title;
  $('#mBody').innerHTML=html+`<div class="mrow" style="justify-content:flex-end">${buttons.map((b,i)=>`<button class="btn ${b.cls||''}" data-ab="${i}">${b.label}</button>`).join('')}</div>`;
  $$('#mBody [data-ab]').forEach(el=>el.onclick=()=>{const b=buttons[+el.dataset.ab];if(b.close!==false)closeModal();b.fn&&b.fn();});
  $('#modal').hidden=false;const inp=$('#mBody input');if(inp){inp.focus();inp.select();}
}
const isDirty=()=>St.projId?St.savedSnap!==H.last:!!H.undo.length;
// Salva: la prima volta chiede il nome; poi chiede sempre se sovrascrivere o salvare come nuovo
function saveProject(then){
  const list=projects(),cur=St.projId&&list.find(p=>p.id===St.projId);
  if(!cur){ask('Salva progetto',`<div class="mrow"><input type="text" id="askName" value="${esc(St.name==='Senza titolo'?'':St.name)}" placeholder="Nome del brano"></div>`,
    [{label:'Annulla',cls:'ghost'},{label:'Salva',cls:'primary',fn:()=>{writeProject(true,($('#askName')?$('#askName').value:'').trim()||St.name);then&&then();},close:true}]);
    const inp=$('#askName');if(inp){const v=inp.value;inp.onkeydown=e=>{if(e.key==='Enter')$$('#mBody [data-ab]')[1].click();};}return;}
  if(!isDirty()){toast(`“${cur.name}” è già salvato`);then&&then();return;}
  ask('Dove salvo?',`<p class="muted" style="font-size:13.5px">Hai modifiche rispetto a <b>“${esc(cur.name)}”</b> (salvato ${fmtDate(cur.date)}).</p>
    <div class="mrow"><input type="text" id="askName" value="${esc(St.name===cur.name?St.name+' (2)':St.name)}" placeholder="Nome del nuovo progetto"></div>`,
    [{label:'Annulla',cls:'ghost'},{label:'Salva come nuovo',fn:()=>{writeProject(true,($('#askName').value||'').trim()||St.name+' (2)');then&&then();}},
     {label:`Sovrascrivi “${esc(cur.name)}”`,cls:'primary',fn:()=>{writeProject(false,cur.name===St.name?null:St.name);then&&then();}}]);
}
// prima di lasciare il brano attuale, se ci sono modifiche non salvate
function guard(next){
  if(!isDirty()){next();return;}
  ask('Modifiche non salvate',`<p class="muted" style="font-size:13.5px">${St.projId?`Le ultime modifiche a <b>“${esc(St.name)}”</b> non sono salvate.`:'Il brano attuale non è salvato in nessun progetto.'}</p>`,
    [{label:'Annulla',cls:'ghost'},{label:'Continua senza salvare',fn:next},{label:'Salva e continua',cls:'primary',fn:()=>saveProject(next)}]);
}
function openProject(id,ok){const p=projects().find(x=>x.id===id);if(!p)return;if(!ok){guard(()=>openProject(id,true));return;}
  St.projId=id;St.sel=0;St.selChord=-1;const json=JSON.stringify({...p.state,name:p.name});H.undo.push(H.last);H.redo=[];restore(json);St.savedSnap=H.last;saveMeta();updHist();closeModal();toast(`Aperto “${p.name}”`);}
function newProject(ok){if(!ok){guard(()=>newProject(true));return;}H.undo.push(H.last);H.redo=[];St.projId=null;St.savedSnap=null;saveMeta();
  restore(JSON.stringify({opts:{...St.opts},seeds:{h:rndSeed(),a:rndSeed(),m:rndSeed()},secs:null,typeOrder:null,edits:[],oct:{},mute:null,name:'Senza titolo',reseed:{L:{},S:{}}}));
  St.muteInit=false;regen({resetMute:true});closeModal();toast('Nuovo progetto');}
const fmtDate=t=>new Date(t).toLocaleString('it-IT',{day:'2-digit',month:'short',hour:'2-digit',minute:'2-digit'});
function closeModal(){$('#modal').hidden=true;}
function renderProjects(){
  const list=projects();
  $('#mTitle').textContent='Progetti';
  $('#mBody').innerHTML=`
    <div class="mrow"><button class="btn primary" id="mSave">Salva il brano attuale…</button><span class="dim" style="font-size:12px">${St.projId?'progetto: “'+esc(St.name)+'”'+(isDirty()?' · modifiche non salvate':' · salvato'):'non ancora salvato'}</span></div>
    <div class="plist">${list.length?list.map(p=>`<div class="pitem ${p.id===St.projId?'cur':''}"><div class="pi"><b>${p.name.replace(/</g,'&lt;')}</b>
      <small>${(GENRES[p.state.opts.genre]||{}).n||''} · ${(MOODS[p.state.opts.mood]||{}).n||''} · ${fmtDate(p.date)}${p.state.edits&&p.state.edits.length?` · ${p.state.edits.length} modifiche`:''}</small></div>
      <button class="btn sm" data-open="${p.id}">Apri</button><button class="btn sm ghost danger" data-del="${p.id}">Elimina</button></div>`).join(''):'<div class="empty">Nessun progetto salvato. Dai un nome al brano e premi Salva.</div>'}</div>
    <div class="mrow"><button class="btn" id="mNew">+ Nuovo progetto</button><span class="spacer"></span>
      <button class="btn ghost" id="mExp">⬇ Esporta .json</button><button class="btn ghost" id="mImp">⬆ Importa .json</button><input type="file" id="mFile" accept=".json,application/json" hidden></div>
    <p class="dim" style="font-size:12px">I progetti sono salvati in questo browser. Il lavoro in corso viene comunque ricordato automaticamente. Con l'esportazione .json puoi spostarli su un altro dispositivo.</p>`;
  $('#mSave').onclick=()=>saveProject();
  $$('#mBody [data-open]').forEach(b=>b.onclick=()=>openProject(b.dataset.open));
  $$('#mBody [data-del]').forEach(b=>b.onclick=()=>{if(b.dataset.sure){LS.set('pg_projects',projects().filter(p=>p.id!==b.dataset.del));
      if(St.projId===b.dataset.del){St.projId=null;St.savedSnap=null;saveMeta();updHist();}renderProjects();}else{b.dataset.sure=1;b.textContent='Sicuro?';}});
  $('#mNew').onclick=()=>newProject();
  $('#mExp').onclick=()=>download(slug(St.name||'progetto')+'.pianogen.json',JSON.stringify({app:'piano-generativo',v:1,name:St.name,state:JSON.parse(H.last)},null,1),'application/json');
  $('#mImp').onclick=()=>$('#mFile').click();
  $('#mFile').onchange=e=>{const f=e.target.files[0];if(!f)return;if(isDirty()&&!e.target._ok){guard(()=>{e.target._ok=1;e.target.onchange(e);});return;}e.target._ok=0;const rd=new FileReader();rd.onload=()=>{try{const d=JSON.parse(rd.result),st=d.state||d;
      if(!st.opts||!st.seeds)throw 0;H.undo.push(H.last);H.redo=[];St.projId=null;St.savedSnap=null;restore(JSON.stringify({...st,name:d.name||st.name||'Importato'}));closeModal();toast('Progetto importato');}
    catch(err){toast('File non valido');}};rd.readAsText(f);};
}
$('#btnProjects').onclick=()=>{renderProjects();$('#modal').hidden=false;};
$('#btnSave').onclick=()=>saveProject();
$('#mClose').onclick=closeModal;
$('#modal').addEventListener('pointerdown',e=>{if(e.target.id==='modal')closeModal();});
$('#projName').addEventListener('change',e=>{St.name=e.target.value.trim()||'Senza titolo';e.target.value=St.name;track();});

/* ---------------- Tema ---------------- */
function setTheme(t){if(t==='auto')delete document.documentElement.dataset.theme;else document.documentElement.dataset.theme=t;LS.set('pg_theme',t);$('#theme').value=t;
  if(St.song){drawArr();renderPads();}}
$('#theme').onchange=e=>setTheme(e.target.value);
setTheme(LS.get('pg_theme','crema'));
matchMedia('(prefers-color-scheme: dark)').addEventListener('change',()=>{if(LS.get('pg_theme','crema')==='auto'&&St.song)drawArr();});

/* ---------------- Avvio ---------------- */
$('#projName').value=St.name;
syncControls();regen({resetMute:!St.muteInit});
window.__pg={St,regen,Player};
