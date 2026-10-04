'use strict';
/* =====================================================================
   INTERFACCIA — generatore, arrangiamento, editor accordi, trasporto, esportazione
   ===================================================================== */
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const LS={get(k,d){try{const v=localStorage.getItem(k);return v==null?d:JSON.parse(v);}catch(e){return d;}},
          set(k,v){try{localStorage.setItem(k,JSON.stringify(v));}catch(e){}}};
const bootHadLocal=!!LS.get('pg_current',null);
const Cloud={ready:false,list:[],col:null,dl:null,_t:null,_last:null,
  save(id,body){if(!this.col)return;this.col.doc(id).set(body).catch(e=>{if(e&&e.code==='invalid_argument')toast('Progetto troppo grande per l\'archivio: fai anche un backup .json');});},
  remove(id){if(this.col)this.col.doc(id).delete().catch(()=>{});},
  // audio: a pezzi da ~180 KB in data/users/<id>/samples/<campione>/<n>
  smpCol(id){return this.col.doc('samples').collection(id);},
  async putSample(id,name,type,buf){if(!this.col)return false;if(buf.byteLength>12e6){toast(`“${name}” è troppo grande per l'archivio (oltre 12 MB): resta solo in questo browser`);return false;}
    const u8=new Uint8Array(buf),CH=180000,n=Math.ceil(u8.length/CH),c=this.smpCol(id);
    try{for(let k=0;k<n;k++){let s='';const part=u8.subarray(k*CH,(k+1)*CH);for(let i=0;i<part.length;i+=8192)s+=String.fromCharCode.apply(null,part.subarray(i,i+8192));
        await c.doc('p'+k).set({name,type,k,n,b64:btoa(s)});}return true;}catch(e){toast('Archivio pieno o non raggiungibile: il campione resta solo in questo browser');return false;}},
  async getSample(id){if(!this.col)return null;try{const snap=await this.smpCol(id).get();const ps=snap.docs.map(d=>d.data()).filter(Boolean).sort((x,y)=>x.k-y.k);
      if(!ps.length||ps.length!==ps[0].n)return null;const bins=ps.map(p=>{const s=atob(p.b64),u=new Uint8Array(s.length);for(let i=0;i<s.length;i++)u[i]=s.charCodeAt(i);return u;});
      const out=new Uint8Array(bins.reduce((q,b)=>q+b.length,0));let o=0;bins.forEach(b=>{out.set(b,o);o+=b.length;});return{name:ps[0].name,type:ps[0].type,data:out.buffer};}catch(e){return null;}},
  saveCurrent(json){if(!this.col||json===this._last)return;clearTimeout(this._t);this._t=setTimeout(()=>{this._last=json;this.col.doc('current').set({date:Date.now(),state:JSON.parse(json)}).catch(()=>{});},4000);}};
const rndSeed=()=>1+Math.floor(Math.random()*99998);
const fmtTime=s=>{s=Math.max(0,Math.round(s));return Math.floor(s/60)+':'+String(s%60).padStart(2,'0');};
function toast(msg){const t=$('#toast');t.textContent=msg;t.classList.add('show');clearTimeout(t._h);t._h=setTimeout(()=>t.classList.remove('show'),2000);}

const DEF={genre:'pop',mood:'rilassato',mode:'auto',key:'auto',structure:'vc',color:'auto',human:true,pad:false,pianoStyle:'live',modLast:false,sound:'samp',dens:'auto',spaceLvl:'auto',gtr:'auto',drumKit:'auto',arr:'sobria'};
const St={opts:Object.assign({},DEF,LS.get('pg_opts',{})),seeds:{h:rndSeed(),a:rndSeed(),m:rndSeed()},bpm:null,song:null,ev:[],
  mute:{},solo:null,oct:{},vol:{},padMute:{},padVol:{},padDens:{},drumGrid:{},feel:{},style:{},inst:{},lib:[],clips:[],padSmp:{},padSmpName:{},imports:[],secCfg:{},padSel:null,showPads:false,reseed:{L:{},S:{}},sel:0,selChord:-1,scope:'rep',loop:false,muteInit:false,layout:null,secs:null,typeOrder:null,edits:[]};
const VALID={genre:GENRES,mood:MOODS,mode:Object.assign({auto:1},MODES),structure:STRUCTS,color:{auto:1,triadi:1,colorati:1,settime:1,estesi:1},pianoStyle:{live:1,classic:1},sound:{samp:1,synth:1},gtr:{auto:1,on:1,off:1},drumKit:{auto:1,acoustic:1,synth:1},arr:{sobria:1,ricca:1}};
// densità e vuoto/pieno: 'auto' o 0–100 (i vecchi valori a parole vengono convertiti)
const numOrAuto=(v,leg)=>v==null||v==='auto'?'auto':leg[v]!=null?leg[v]:isFinite(+v)?clamp(Math.round(+v),0,100):'auto';
St.name='Senza titolo';St.projId=null;St.savedSnap=null;
(()=>{const cur=LS.get('pg_current',null),meta=LS.get('pg_meta',{});
  if(cur&&cur.opts&&cur.seeds){Object.assign(St,{opts:Object.assign({},DEF,cur.opts),seeds:cur.seeds,secs:cur.secs||null,typeOrder:cur.typeOrder||null,edits:cur.edits||[],
    oct:cur.oct||{},mute:cur.mute||{},solo:cur.solo||null,bpm:cur.bpm||null,name:cur.name||'Senza titolo',muteInit:!!cur.mute,reseed:cur.reseed||{L:{},S:{}},vol:cur.vol||{},padMute:cur.padMute||{},padVol:cur.padVol||{},padDens:cur.padDens||{},drumGrid:cur.drumGrid||{},feel:cur.feel||{},style:cur.style||{},inst:cur.inst||{},lib:cur.lib||[],clips:cur.clips||[],padSmp:cur.padSmp||{},padSmpName:cur.padSmpName||{},imports:cur.imports||[],secCfg:cur.secCfg||{}});}
  St.projId=meta.projId||null;St.savedSnap=meta.savedSnap||null;})();
for(const k in VALID)if(!(St.opts[k] in VALID[k]))St.opts[k]=DEF[k];
St.opts.dens=numOrAuto(St.opts.dens,DENS_LEGACY);St.opts.spaceLvl=numOrAuto(St.opts.spaceLvl,SPACE_LEGACY);
if(St.opts.key!=='auto'&&!(+St.opts.key>=0&&+St.opts.key<12))St.opts.key='auto';

const curBpm=()=>St.bpm||St.song.bpm;
const audible=id=>St.solo?St.solo===id:!St.mute[id];
const instOf=l=>{const s=St.song,p=s.prog,ch=St.inst&&St.inst[l];if(ch&&ch.b)return ch.b;
  if(l==='piano')return p.piano===4?'epiano':'piano';
  if(l==='arp')return p.arp===4?'epiano':(p.arp===24||p.arp===46)?'pluck':'piano';
  if(l==='bass')return s.bass808?'b808':'bass';
  if(l==='cm')return'epiano';
  if(l==='gtr')return'gtr';
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
function syncControls(){['genre','mood','mode','key','structure','color','sound','gtr','drumKit'].forEach(k=>$('#'+k).value=St.opts[k]);$('#arrMode').value=St.opts.arr;syncRanges();$('#modLast').checked=!!St.opts.modLast;
  $('#pstyle').value=St.opts.pianoStyle;$('#human').checked=St.opts.human;$('#pad').checked=St.opts.pad;}
const saveOpts=()=>LS.set('pg_opts',St.opts);
function clearReseed(k){const R=St.reseed;Object.keys(R.S).forEach(x=>{if(x.endsWith('.'+k))delete R.S[x];});
  if(k==='a')['piano','arp','pad','bass','gtr','drums'].forEach(l=>delete R.L[l]);if(k==='m'){delete R.L.mel;delete R.L.cm;}}
function dropEdits(){if(St.edits.length){St.edits=[];toast('Modifiche agli accordi azzerate (armonia nuova)');}}
['genre','mood','mode','key','structure','color'].forEach(k=>$('#'+k).addEventListener('change',e=>{
  St.opts[k]=e.target.value;saveOpts();
  if(k==='genre'||k==='mood')St.bpm=null;
  if(k==='structure'){St.sel=0;St.secs=null;St.typeOrder=null;}
  if(k==='genre'){St.style={};setTimeout(ensureDrums,0);}
  if(k!=='key')St.reseed={L:{},S:{}};
  if(k!=='key')dropEdits();
  St.selChord=-1;regen({resetMute:k==='genre'});}));
$('#drumKit').addEventListener('change',e=>{St.opts.drumKit=e.target.value;saveOpts();ensureDrums();toast('Batteria: '+e.target.selectedOptions[0].text.split(' (')[0].toLowerCase());});
$('#gtr').addEventListener('change',e=>{St.opts.gtr=e.target.value;saveOpts();St.mute.gtr=false;regen();toast('Chitarra: '+e.target.selectedOptions[0].text.toLowerCase());});
$('#arrMode').addEventListener('change',e=>{St.opts.arr=e.target.value;saveOpts();regen({resetMute:true});toast(e.target.value==='sobria'?'Composizione sobria: solo pianoforte, arpeggio e basso':'Composizione ricca: tutti gli strati');});
$('#pstyle').addEventListener('change',e=>{St.opts.pianoStyle=e.target.value;saveOpts();regen();toast(e.target.value==='classic'?'Pianoforte classico (v3)':'Pianoforte vivo');});
$('#human').addEventListener('change',e=>{St.opts.human=e.target.checked;saveOpts();regen();});
// barre 0–100: densità (50 = come decide il genere) e vuoto/pieno (auto = valore del genere e del mood)
function syncRanges(){const d=St.opts.dens,s=St.opts.spaceLvl,as=St.song?Math.round(St.song.autoSpace*100):30;
  $('#densR').value=d==='auto'?50:d;$('#densV').textContent=d==='auto'?'auto':d;$('#densR').classList.toggle('auto',d==='auto');
  $('#spaceR').value=s==='auto'?as:s;$('#spaceV').textContent=s==='auto'?'auto ('+as+')':s;$('#spaceR').classList.toggle('auto',s==='auto');}
[['dens','densR','densAuto','Densità'],['spaceLvl','spaceR','spaceAuto','Vuoto/pieno']].forEach(([k,r,au,lab])=>{
  $('#'+r).addEventListener('input',e=>{$('#'+(k==='dens'?'densV':'spaceV')).textContent=e.target.value;e.target.classList.remove('auto');});
  $('#'+r).addEventListener('change',e=>{St.opts[k]=+e.target.value;saveOpts();regen();toast(`${lab}: ${e.target.value}`);});
  $('#'+au).onclick=()=>{St.opts[k]='auto';saveOpts();regen();toast(`${lab}: auto`);};});
$('#modLast').addEventListener('change',e=>{St.opts.modLast=e.target.checked;saveOpts();regen();
  const n=St.song.sections.filter(x=>x.type==='chorus').length;toast(e.target.checked?(n>=2?'L\'ultimo ritornello sale di un tono':'Serve una struttura con almeno due ritornelli'):'Modulazione tolta');});
$('#sound').addEventListener('change',e=>{St.opts.sound=e.target.value;saveOpts();ensureSound(true);renderHead();});
let soundMsg='';
// batteria acustica campionata nei generi "suonati", elettronica negli altri
const ACOUSTIC_DR=new Set(['pop','rock','soul','gospel','jazz','blues','bossa','reggae','lofi','boombap','triphop','cinematic','classical']);
function ensureDrums(){const o=St.opts.drumKit,on=o==='acoustic'||(o==='auto'&&ACOUSTIC_DR.has(St.opts.genre));
  Synth.useDrumSamples(on).then(st=>{if(on&&st==='error'&&o==='acoustic')toast('Kit acustico non raggiungibile: uso la batteria elettronica');});}
function ensureSound(verbose){
  ensureDrums();
  if(St.opts.sound!=='samp'){Synth.useSamples(false);soundMsg='';return;}
  if(Synth.sampleState==='ready'){Synth.useSamples(true);return;}
  if(Synth.sampleState==='loading')return;
  soundMsg='caricamento 0%';renderHead();
  Synth.useSamples(true,p=>{soundMsg='caricamento '+Math.round(p*100)+'%';const c=$('#sndChip');if(c)c.textContent='piano: '+soundMsg;})
    .then(st=>{soundMsg='';renderHead();toast(st==='ready'?'Pianoforte campionato pronto':'Campioni non raggiungibili: uso il piano sintetico');});
}
$('#pad').addEventListener('change',e=>{St.opts.pad=e.target.checked;saveOpts();St.mute.pad=false;regen();});
// tutto casuale: genere, mood, struttura e un brano nuovo (modo, tonalità e carattere tornano su auto)
$('#btnRandom').onclick=()=>{const pk=a=>a[Math.floor(Math.random()*a.length)];
  Object.assign(St.opts,{genre:pk(Object.keys(GENRES)),mood:pk(Object.keys(MOODS)),structure:pk(['vc','song','song','vb','bc','loop8']),mode:'auto',key:'auto',color:'auto',dens:'auto',spaceLvl:'auto',gtr:'auto'});
  St.seeds={h:rndSeed(),a:rndSeed(),m:rndSeed()};St.reseed={L:{},S:{}};St.style={};St.secCfg={};St.secs=null;St.typeOrder=null;St.edits=[];St.bpm=null;St.sel=0;St.selChord=-1;
  syncControls();saveOpts();ensureDrums();if($('#tab-gen').hidden)document.querySelector('.tabs [data-tab="gen"]').click();regen({resetMute:true});
  toast(`${GENRES[St.opts.genre].n} · ${MOODS[St.opts.mood].n} · ${St.song.keyName} ${MODES[St.song.mode].n.toLowerCase()} · ${St.song.bpm} BPM`);};
$('#btnGen').onclick=()=>{St.seeds={h:rndSeed(),a:rndSeed(),m:rndSeed()};St.reseed={L:{},S:{}};St.style={};St.secCfg={};St.bpm=null;St.edits=[];St.selChord=-1;regen({resetMute:true});};
$$('[data-re]').forEach(b=>b.onclick=()=>{const k=b.dataset.re;if(k==='h')dropEdits();St.seeds[k]=rndSeed();clearReseed(k);regen();
  toast({h:'Nuova armonia',a:'Nuovo arrangiamento',m:'Nuova melodia'}[k]);});
const DENS_K=['auto','scarna','leggera','media','piena','moltopiena'],SPACE_K=['auto','continuo','leggero','marcato','estremo'];
const TL={intro:'i',verse:'v',pre:'p',chorus:'c',bridge:'b',special:'x',outro:'o',loop:'l'},LT=Object.fromEntries(Object.entries(TL).map(([k,v])=>[v,k]));
const codeOf=()=>{const p=[St.opts.genre,St.opts.mood,St.opts.mode,St.opts.key,St.opts.structure,St.opts.color,`${St.seeds.h}-${St.seeds.a}-${St.seeds.m}`,(St.opts.pad?'p':'-')+(St.opts.pianoStyle==='classic'?'c':'v')+(St.opts.dens==='auto'?'a':St.opts.dens)+'.'+(St.opts.spaceLvl==='auto'?'a':St.opts.spaceLvl)+(St.opts.gtr!=='auto'?'.'+St.opts.gtr[1]:'')];
  const rs=[...Object.entries(St.reseed.L).filter(x=>x[1]).map(([k,v])=>'L'+k+v),...Object.entries(St.reseed.S).filter(x=>x[1]).map(([k,v])=>'S'+k+v)].join(',');
  const sy=Object.entries(St.style||{}).map(([k,v])=>k+':'+v).join(',');
  if(St.secs||rs||sy)p.push(St.secs?St.secs.map(([t,b])=>TL[t]+b).join('.'):'',St.secs?(St.typeOrder||[]).map(t=>TL[t]).join(''):'');if(rs||sy)p.push(rs);if(sy)p.push(sy);return p.join('/');};
$('#code').addEventListener('change',e=>{
  const parts=e.target.value.trim().split('/'),sd=(parts[6]||parts[parts.length-1]||'').match(/^(\d+)-(\d+)-(\d+)$/);
  if(!sd){toast('Codice non valido');e.target.value=codeOf();return;}
  if(parts.length>=7){const [g,m,mo,k,s,c]=parts;
    if(GENRES[g])St.opts.genre=g;if(MOODS[m])St.opts.mood=m;if(VALID.mode[mo])St.opts.mode=mo;
    if(k==='auto'||(+k>=0&&+k<12))St.opts.key=k;if(STRUCTS[s])St.opts.structure=s;if(VALID.color[c])St.opts.color=c;
    if(parts[7]){St.opts.pad=parts[7][0]==='p';St.opts.pianoStyle=parts[7][1]==='c'?'classic':'live';if(parts[7].includes('.')){const [dd,ss,gg]=parts[7].slice(2).split('.');St.opts.gtr=gg==='n'?'on':gg==='f'?'off':'auto';St.opts.dens=dd==='a'?'auto':numOrAuto(dd,{});St.opts.spaceLvl=ss==='a'?'auto':numOrAuto(ss,{});}
      else{St.opts.dens=numOrAuto(DENS_K[+parts[7][2]]||'auto',DENS_LEGACY);St.opts.spaceLvl=numOrAuto(SPACE_K[+parts[7][3]]||'auto',SPACE_LEGACY);}}syncControls();saveOpts();}
  St.secs=null;St.typeOrder=null;
  if(parts[8]){const secs=parts[8].split('.').map(x=>[LT[x[0]],+x.slice(1)]).filter(([t,b])=>t&&b>0&&b<=64);if(secs.length){St.secs=secs;St.typeOrder=(parts[9]||'').split('').map(c=>LT[c]).filter(Boolean);}}
  St.reseed={L:{},S:{}};(parts[10]||'').split(',').forEach(x=>{const m=x.match(/^([LS])([a-z.]+?)(\d+)$/);if(m)St.reseed[m[1]][m[2]]=+m[3];});
  St.style={};(parts[11]||'').split(',').forEach(x=>{const m=x.match(/^(bass|drums|piano|arp|mel|gtr):([a-zA-Z0-9]+)$/);if(m)St.style[m[1]]=m[2];});
  St.seeds={h:+sd[1],a:+sd[2],m:+sd[3]};St.bpm=null;St.edits=[];St.sel=0;St.selChord=-1;regen({resetMute:true});});

/* ---------------- Generazione ---------------- */
function buildEv(){
  const s=St.song;St.ev=[];
  LAYERS.forEach(l=>{const sh=l.id==='drums'?0:12*(St.oct[l.id]||0),sf=((St.feel[l.id]||{}).shift||0)*curBpm()/60000;
    s.layers[l.id].forEach(e=>St.ev.push({t:Math.max(0,e.t+sf),d:e.d,n:clamp(e.n+sh,0,127),v:l.id==='drums'?clamp(Math.round(e.v*(St.padVol[e.pad]!=null?St.padVol[e.pad]:1)),1,127):e.v,l:l.id,gl:e.gl!=null?e.gl+sh:null,pad:e.pad,art:e.art}));});
  if(St.clips&&St.clips.length)St.ev.push(...clipEvents());
  St.ev.sort((a,b)=>a.t-b.t);
}
function regen({resetMute=false}={}){
  const was=Player.playing,pos=was?Player.pos():0;
  if(was)Player.stop(true);
  const s=generateSong({...St.opts,dens:null,densV:St.opts.dens==='auto'?null:+St.opts.dens,spaceV:St.opts.spaceLvl==='auto'?null:+St.opts.spaceLvl,secCfg:St.secCfg,bpm:St.bpm,seeds:St.seeds,secs:St.secs,typeOrder:St.typeOrder,edits:St.edits,reseed:St.reseed,feel:St.feel,style:St.style,padDens:St.padDens,drumGrid:St.drumGrid});
  applyImports(s);St.song=s;if(s.gtrTone)Synth.setGuitarTone(s.gtrTone);
  if(resetMute||!St.muteInit){St.mute={};LAYERS.forEach(l=>St.mute[l.id]=!s.defaults[l.id]);St.solo=null;St.muteInit=true;}
  buildEv();
  St.sel=Math.min(St.sel,s.sections.length-1);
  if(St.selChord>=s.sections[St.sel].chords.length)St.selChord=s.sections[St.sel].chords.length-1;
  renderAll();
  if(was)Player.start(Math.min(pos,s.beats-.01));
  track();
}
function renderAll(){applyVols();syncRanges();renderSamples();renderHead();renderLanes();drawArr();renderPads();renderSection();renderHow();renderProg();updateTransport(Player.playing?Player.pos():0);}

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
const LANE_NAME={smp:'Campioni',mel:'Melodia',cm:'Controcanto',gtr:'Chitarra',piano:'Piano',arp:'Arpeggio',pad:'Pad',bass:'Basso',drums:'Batteria'};
const LANE_H={smp:40,mel:62,cm:50,gtr:72,piano:84,arp:62,pad:46,bass:58,drums:84},RULER=28,CHROW=22;
const lanes=()=>[...LAYERS.filter(l=>St.song.layers[l.id].length),...(St.clips&&St.clips.length?[{id:'smp',n:'Campioni'}]:[])];
const feelShift=()=>Object.fromEntries(Object.entries(St.feel).map(([k,v])=>[k,v.shift||0]));
const volOf=id=>St.vol[id]!=null?St.vol[id]:1;
function applyVols(){LAYERS.forEach(l=>Synth.setLayerGain(l.id,volOf(l.id)));Synth.setLayerGain('smp',volOf('smp'));}
function renderLanes(){
  const h=$('#laneHead');
  h.innerHTML=`<div class="label" style="height:${RULER+CHROW}px;display:flex;align-items:center;padding:0 12px">Tracce</div>`+lanes().map(l=>{
    const o=St.oct[l.id]||0,v=Math.round(volOf(l.id)*100);
    return`<div class="lane-h ${audible(l.id)?'':'off'} ${LANE_H[l.id]<50?'short':''}" style="height:${LANE_H[l.id]}px">
      <span class="dot" style="background:var(--c-${l.id})"></span><span class="nm" title="${l.n}">${LANE_NAME[l.id]}</span>
      <div class="btns">${l.id==='smp'?'':`<button class="ib rg" data-rl="${l.id}" title="Rigenera ${l.n.toLowerCase()}: casuale o con uno stile scelto">🎲${St.style[l.id]?'<i class="lk">•</i>':''}</button>`}
        ${l.id==='smp'?'':`<button class="ib ${St.inst[l.id]||(l.id==='drums'&&Object.keys(St.padSmp).length)?'on':''}" data-snd="${l.id}" title="Suono della traccia: strumenti e i tuoi campioni">♫</button>`}
        <button class="ib ${St.mute[l.id]?'on':''}" data-mute="${l.id}" title="Silenzia">M</button>
        <button class="ib ${St.solo===l.id?'on':''}" data-solo="${l.id}" title="Ascolta solo questa traccia">S</button>
        ${l.id==='smp'?`<input type="range" min="0" max="150" value="${v}" data-vol="smp" title="Volume campioni ${v}%" style="width:70px">`:`<button class="ib ${St.feel[l.id]&&Object.keys(St.feel[l.id]).length?'on':''}" data-feel="${l.id}" title="Feel: umanizzazione, swing, anticipo/ritardo">≈</button>
        <button class="ib dl" data-dl="${l.id}" title="Scarica solo questa traccia (.mid)">⬇</button>`}</div>
      ${l.id==='smp'?'':`<div class="r2"><label class="vol" style="--vc:var(--c-${l.id})" title="Volume ${l.n}"><input type="range" min="0" max="150" value="${v}" data-vol="${l.id}"><span>${v}%</span></label>
        ${l.id==='drums'?`<button class="btn sm ${St.showPads?'primary-soft':''}" data-pads style="padding:3px 8px">Sequencer</button>`:`<span class="oct ${o?'on':''}" title="Ottava"><button data-oct="${l.id}" data-d="-1">−</button><span>${o>0?'+'+o:o}</span><button data-oct="${l.id}" data-d="1">+</button></span>`}</div>`}</div>`;}).join('');
  h.querySelectorAll('[data-rl]').forEach(b=>b.onclick=e=>{e.stopPropagation();const id=b.dataset.rl;
    if(!styleOpts(id).length){reseedLayer(id);return;}openStyle(id,b);});
  h.querySelectorAll('[data-snd]').forEach(b=>b.onclick=e=>{e.stopPropagation();openSound(b.dataset.snd,b);});
  h.querySelectorAll('[data-mute]').forEach(b=>b.onclick=()=>{const id=b.dataset.mute;St.mute[id]=!St.mute[id];renderLanes();drawArr();track();});
  h.querySelectorAll('[data-solo]').forEach(b=>b.onclick=()=>{const id=b.dataset.solo;St.solo=St.solo===id?null:id;renderLanes();drawArr();track();});
  h.querySelectorAll('[data-oct]').forEach(b=>b.onclick=()=>{const id=b.dataset.oct;St.oct[id]=clamp((St.oct[id]||0)+(+b.dataset.d),-2,2);buildEv();renderLanes();track();
    toast(`${LAYERS.find(l=>l.id===id).n}: ottava ${St.oct[id]>0?'+':''}${St.oct[id]}`);});
  h.querySelectorAll('[data-vol]').forEach(el=>{el.oninput=()=>{const id=el.dataset.vol;St.vol[id]=+el.value/100;if(el.nextElementSibling)el.nextElementSibling.textContent=el.value+'%';Synth.setLayerGain(id,St.vol[id]);};
    el.onchange=()=>track();el.ondblclick=()=>{St.vol[el.dataset.vol]=1;applyVols();renderLanes();track();};});
  h.querySelectorAll('[data-feel]').forEach(b=>b.onclick=e=>{e.stopPropagation();openFeel(b.dataset.feel,b);});
  const pb=h.querySelector('[data-pads]');if(pb)pb.onclick=()=>{St.showPads=!St.showPads;renderLanes();renderPads();};
  h.querySelectorAll('[data-dl]').forEach(b=>b.onclick=()=>{const l=LAYERS.find(x=>x.id===b.dataset.dl);
    download(`${baseName()}-${slug(l.n)}.mid`,toMidi(St.song,{layers:[l.id],bpm:curBpm(),title:l.n,oct:St.oct,vol:St.vol,padMute:St.padMute}),'audio/midi');});
}
/* ---------------- Rigenera: casuale o secondo le direttive ---------------- */
// popover delle tracce: sotto il pulsante, o sopra se in basso non c'è spazio (barra di trasporto)
function placeLanePop(pop,anchor){const r=anchor.getBoundingClientRect(),w=pop.offsetWidth,h=pop.offsetHeight,left=clamp(r.left+r.width/2-w/2,12,innerWidth-w-12);
  const up=r.bottom+10+h>innerHeight-86&&r.top-10-h>60;pop.classList.toggle('up',up);
  pop.style.left=left+scrollX+'px';pop.style.top=(up?r.top-10-h:r.bottom+10)+scrollY+'px';pop.querySelector('.parrow').style.left=clamp(r.left+r.width/2-left-7,16,w-30)+'px';}
const ARP_NAME={up8:'Salita (ottavi)',updown8:'Su e giù (ottavi)',broken8:'Spezzato',pulse8:'Pulsazione',ballad8:'Ballad',lazy:'Pigro (sparso)',alberti16:'Alberti (sedicesimi)',rise16:'Cascata in salita',cascade16:'Cascata in discesa',updown16:'Su e giù (sedicesimi)'};
const MEL_NAME={straight:'Dritta',sync:'Sincopata',flow16:'Flow (sedicesimi)',swing:'Swing',offbeat:'In levare',ballad:'Lunga, cantabile'};
function styleOpts(id){const G=GENRES[St.song.genre];
  if(id==='bass')return Object.entries(BASS_STY);
  if(id==='drums')return G.drums?drumStyles(G).map(s=>[s.id,s.n]):[];
  if(id==='piano')return St.opts.pianoStyle==='classic'?[]:Object.entries(TEX_NAME).map(([k,v])=>[k,v.charAt(0).toUpperCase()+v.slice(1)]);
  if(id==='arp')return Object.entries(ARP_NAME);
  if(id==='mel')return Object.entries(MEL_NAME);
  if(id==='gtr')return Object.entries(GTR_STY).map(([k,v])=>[k,v.n]);
  return[];}
function reseedLayer(id){St.reseed.L[id]=(St.reseed.L[id]||0)+1;if(St.mute[id])St.mute[id]=false;regen();toast(`${LAYERS.find(l=>l.id===id).n}: nuova versione`);}
function openStyle(id,anchor){
  const pop=$('#feelPop'),cur=St.style[id],now=(St.song.styles||{})[id],nm=LAYERS.find(l=>l.id===id).n;feelFor=id;
  $('#feelBody').innerHTML=`<div class="phead"><div><div class="big" style="font-size:17px">Rigenera · ${nm}</div><div class="sub">${cur?'Stile bloccato: le nuove versioni restano in questo stile':'Stile libero: scelto dal genere e dal mood'}</div></div><span class="spacer"></span><button class="ib" id="sClose">✕</button></div>
    <div class="pbody"><button class="btn primary" id="sRand" style="width:100%">🎲 ${cur?'Nuova versione in questo stile':'Casuale'}</button>
      <div class="label" style="margin:12px 0 6px">Oppure rigenera come…</div>
      <div class="stylechips">${styleOpts(id).map(([k,n])=>`<button class="chip ${k===cur?'lock':''} ${k===now&&!cur?'cur':''}" data-sty="${k}">${k===cur?'🔒 ':''}${n}${k===now&&!cur?' <small>· ora</small>':''}</button>`).join('')}</div></div>
    <div class="pfoot">${cur?'<button class="btn sm ghost" id="sFree">Sblocca (stile libero)</button>':'<span class="dim" style="font-size:11.5px">Scegliendo uno stile lo blocchi per questa traccia</span>'}</div>`;
  pop.hidden=false;
  placeLanePop(pop,anchor);
  const close=()=>{pop.hidden=true;feelFor=null;};
  $('#sClose').onclick=close;
  $('#sRand').onclick=()=>{close();reseedLayer(id);};
  $$('#feelBody [data-sty]').forEach(b=>b.onclick=()=>{const k=b.dataset.sty;St.style={...St.style,[id]:k};close();St.reseed.L[id]=(St.reseed.L[id]||0)+1;if(St.mute[id])St.mute[id]=false;
    regen();toast(`${nm}: ${b.textContent.replace('🔒','').replace('· ora','').trim()}`);});
  const f=$('#sFree');if(f)f.onclick=()=>{const s={...St.style};delete s[id];St.style=s;close();regen();toast(`${nm}: stile libero`);};
}
/* ---------------- Suono di ogni traccia: strumenti interni o i tuoi campioni ---------------- */
const BUILTIN=[['piano','Pianoforte'],['epiano','Rhodes'],['pluck','Pizzicato'],['pad','Pad'],['gtr','Chitarra'],['bass','Basso'],['b808','808']];
// libreria: i campioni del progetto più quelli caricati in passato (browser e archivio)
function libAll(){const m=new Map();[...LS.get('pg_lib',[]),...(Cloud.lib||[]),...(St.lib||[])].forEach(x=>{if(x&&x.sid&&!m.has(x.sid))m.set(x.sid,x);});return[...m.values()];}
function libAdd(x){St.lib=[...(St.lib||[]).filter(y=>y.sid!==x.sid),x];const g=LS.get('pg_lib',[]).filter(y=>y.sid!==x.sid);g.push(x);LS.set('pg_lib',g.slice(-200));
  if(Cloud.ready){Cloud.lib=[...(Cloud.lib||[]).filter(y=>y.sid!==x.sid),x];Cloud.col.doc('library').set({items:Cloud.lib.slice(-200)}).catch(()=>{});}}
const NOTE_OPTS=Array.from({length:61},(_,i)=>i+24);
let sndFile=null;
function openSound(id,anchor){
  const pop=$('#feelPop'),nm=LAYERS.find(l=>l.id===id).n,lib=libAll(),cur=St.inst[id]||{};feelFor=id;
  const libChips=lib.length?lib.map(x=>`<button class="chip ${cur.sid===x.sid?'lock':''}" data-ls="${x.sid}" title="${Synth.hasSample(x.sid)?'':'non ancora caricato'}">${cur.sid===x.sid?'♪ ':''}${esc(x.name)}</button>`).join(''):'<span class="dim" style="font-size:12px">Nessun campione ancora: caricane uno qui sotto.</span>';
  let body;
  if(id==='drums'){const kit=drumKit(GENRES[St.song.genre],St.song.drumStyle);
    body=`<div class="padsnd">${PADS.map((p,i)=>`<div class="psr"><span class="pn">${kit.names[i]}</span>
      <select data-pl="${i}"><option value="">${St.padSmp[i]?'— suono interno —':'suono interno'}</option>${lib.map(x=>`<option value="${x.sid}" ${St.padSmp[i]===x.sid?'selected':''}>${esc(x.name)}</option>`).join('')}</select>
      <button class="ib" data-pu="${i}" title="Carica un file audio per ${kit.names[i]}">📁</button><button class="ib" data-pp="${i}" title="Ascolta">▶</button></div>`).join('')}</div>`;}
  else body=`<div class="label" style="margin:2px 0 6px">Strumenti</div><div class="stylechips"><button class="chip ${!St.inst[id]?'lock':''}" data-bi="">Auto (del genere)</button>${BUILTIN.map(([k,n])=>`<button class="chip ${cur.b===k?'lock':''}" data-bi="${k}">${n}</button>`).join('')}</div>
    <div class="label" style="margin:12px 0 6px">I tuoi campioni</div><div class="stylechips">${libChips}</div>
    ${cur.sid?`<div class="frow" style="margin-top:10px"><span title="L’altezza a cui il campione è stato registrato: da lì si calcolano tutte le altre note">Nota del campione</span><select id="sRoot">${NOTE_OPTS.map(n=>`<option value="${n}" ${n===(cur.root||60)?'selected':''}>${NOTE_NM(n)}</option>`).join('')}</select></div>`:''}`;
  $('#feelBody').innerHTML=`<div class="phead"><div><div class="big" style="font-size:17px">Suono · ${nm}</div><div class="sub">${id==='drums'?'Un campione per ogni pad, o il suono interno':'Scegli lo strumento o un tuo campione: suona a tutte le altezze'}</div></div><span class="spacer"></span><button class="ib" id="sClose">✕</button></div>
    <div class="pbody">${body}</div>
    <div class="pfoot">${id==='drums'?'':'<button class="btn sm primary" id="sUp">＋ Carica campione audio</button><button class="btn sm" id="sTry">▶ Prova</button>'}<span class="spacer"></span><span class="dim" style="font-size:11px">${Cloud.ready?'☁ salvati nel tuo archivio':'wav, mp3, ogg…'}</span></div>`;
  pop.hidden=false;placeLanePop(pop,anchor);
  const again=()=>openSound(id,$(`[data-snd="${id}"]`)||anchor),apply=(msg)=>{renderLanes();track();if(msg)toast(msg);again();};
  $('#sClose').onclick=()=>{pop.hidden=true;feelFor=null;};
  $$('#feelBody [data-bi]').forEach(b=>b.onclick=()=>{const k=b.dataset.bi,x={...St.inst};if(k)x[id]={b:k};else delete x[id];St.inst=x;apply(`${nm}: ${b.textContent}`);});
  $$('#feelBody [data-ls]').forEach(b=>b.onclick=async()=>{const x=lib.find(y=>y.sid===b.dataset.ls);St.inst={...St.inst,[id]:{sid:x.sid,root:x.root||60,name:x.name}};await ensureSamples();apply(`${nm}: “${x.name}”`);});
  const rs=$('#sRoot');if(rs)rs.onchange=()=>{St.inst={...St.inst,[id]:{...St.inst[id],root:+rs.value}};const L2=libAll().find(y=>y.sid===St.inst[id].sid);if(L2)libAdd({...L2,root:+rs.value});track();};
  const up=$('#sUp');if(up)up.onclick=()=>{sndFile={layer:id};$('#sndF').click();};
  const tr=$('#sTry');if(tr)tr.onclick=()=>{Synth.init();const t0=Synth.now()+.05,si=St.inst[id];[60,64,67,72].forEach((n,i)=>{const at=t0+i*.28;
    if(si&&si.sid&&Synth.hasSample(si.sid))Synth.playSample(si.sid,at,{dur:.5,rate:Math.pow(2,(n-(si.root||60))/12),gain:.8,layer:id});else Synth.note(instOf(id),n-(id==='bass'?24:0),at,.5,85,id);});};
  $$('#feelBody [data-pl]').forEach(sel=>sel.onchange=async()=>{const i=+sel.dataset.pl,v=sel.value,p={...St.padSmp},q={...St.padSmpName};
    if(v){const x=lib.find(y=>y.sid===v);p[i]=v;q[i]=x?x.name:'';}else{delete p[i];delete q[i];}St.padSmp=p;St.padSmpName=q;await ensureSamples();renderPads();apply();});
  $$('#feelBody [data-pu]').forEach(b=>b.onclick=()=>{sndFile={pad:+b.dataset.pu};$('#sndF').click();});
  $$('#feelBody [data-pp]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pp,kit=drumKit(GENRES[St.song.genre],St.song.drumStyle);Synth.init();hitPad(i,kit.notes[i],Synth.now()+.02,105,i===9?2:.2);});
}
$('#sndF').onchange=async e=>{const f=e.target.files[0];e.target.value='';if(!f||!sndFile)return;Synth.init();
  try{const s=await addSampleFile(f),root=sndFile.pad!=null?null:(Synth.detectRoot(s.id)||60),x={sid:s.id,name:s.name,root:root||60};libAdd(x);
    if(sndFile.pad!=null){St.padSmp={...St.padSmp,[sndFile.pad]:s.id};St.padSmpName={...St.padSmpName,[sndFile.pad]:s.name};renderPads();openSound('drums',$('[data-snd="drums"]'));}
    else{const id=sndFile.layer;St.inst={...St.inst,[id]:{sid:s.id,root:x.root,name:s.name}};openSound(id,$(`[data-snd="${id}"]`));toast(`${LAYERS.find(l=>l.id===id).n}: “${s.name}” (nota ${NOTE_NM(x.root)})`);}
    renderLanes();track();}catch(err){toast('File audio non leggibile');}};
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
  placeLanePop(pop,anchor);
  const set=(k,v,re)=>{St.feel[id]=Object.assign({},St.feel[id],{[k]:v});if(v==null)delete St.feel[id][k];if(!Object.keys(St.feel[id]).length)delete St.feel[id];
    if(re)regen();else{buildEv();renderLanes();track();}};
  $('#fHum').oninput=e=>$('#fHumV').textContent=e.target.value+'%';$('#fHum').onchange=e=>set('hum',+e.target.value/100,true);
  $('#fSw').oninput=e=>$('#fSwV').textContent=+e.target.value<0?'genere':e.target.value+'%';$('#fSw').onchange=e=>set('swing',+e.target.value<0?null:+e.target.value/100,true);
  $('#fSh').oninput=e=>$('#fShV').textContent=(+e.target.value>0?'+':'')+e.target.value+' ms';$('#fSh').onchange=e=>set('shift',+e.target.value||null,false);
  $('#fReset').onclick=()=>{delete St.feel[id];regen();openFeel(id,$(`[data-feel="${id}"]`)||anchor);};
  $('#fClose').onclick=()=>{pop.hidden=true;feelFor=null;};
}
document.addEventListener('pointerdown',e=>{const p=$('#feelPop');if(!p.hidden&&!p.contains(e.target)&&!e.target.closest('[data-feel]')&&!e.target.closest('[data-rl]')&&!e.target.closest('[data-snd]')){p.hidden=true;feelFor=null;}});

/* ---------------- Pad batteria ---------------- */
const PAD_COL=['#ef4444','#f59e0b','#eab308','#22c55e','#14b8a6','#06b6d4','#3b82f6','#6366f1','#a855f7','#ec4899'];
const NOTE_NM=n=>KEY_NAMES[n%12]+(Math.floor(n/12)-1);
// sequencer: 10 pad × 32 passi (le due battute del groove della sezione selezionata)
function seqData(){
  const s=St.song,sec=s.sections[St.sel],t=sec.type,man=St.drumGrid[t]||{};
  const R=St.seqRes||16,N=2*R;
  const gen=Array.from({length:10},()=>Array(N).fill(0));
  // prima metà = battuta A (groove), seconda = prima battuta B (risposta), come le suona il motore
  let bB=0;for(let b=0;b<sec.bars;b++)if(drumKind(b,sec.bars)==='B'){bB=b;break;}
  // più colpi nello stesso sedicesimo = roll (2) / terzina (3) / raffica (4): codificati come tipo*1000+velocity
  const cnt=Array.from({length:10},()=>Array(N).fill(0)),eps=1/R;
  const half=(bar,off)=>s.layers.drums.forEach(e=>{const rel=e.t-sec.startBeat-bar*4;if(rel<-eps||rel>=4-eps)return;const st=clamp(Math.floor((rel+eps)*R/4),0,R-1)+off;gen[e.pad][st]=Math.max(gen[e.pad][st],e.v);cnt[e.pad][st]++;});
  half(0,0);half(bB,R);
  gen.forEach((g,p)=>g.forEach((v,k)=>{if(cnt[p][k]>1)g[k]=(Math.min(cnt[p][k],4)-1)*1000+v;}));
  // righe scritte a mano: si portano alla risoluzione scelta
  const fit=m=>{const L=m.length>16?m.length:m.length*2,src=m.length>16?m:m.concat(m),r0=L/2;if(r0===R)return src;const out=Array(N).fill(0);
    src.forEach((v,i)=>{if(!(v>0))return;const bar=Math.floor(i/r0),pos=(i%r0)/r0,j=bar*R+Math.min(R-1,Math.round(pos*R));out[j]=Math.max(out[j],v);});return out;};
  return{sec,t,R,rows:gen.map((g,p)=>{const m=man[p];return m?{v:fit(m),man:true}:{v:g,man:false};})};
}
function renderPads(){
  const s=St.song,panel=$('#padPanel'),has=s.layers.drums.length>0;
  panel.hidden=!(St.showPads&&has);if(panel.hidden)return;
  $('#seqRes').value=String(St.seqRes||16);const kit=drumKit(GENRES[s.genre],s.drumStyle),D=seqData(),nM=Object.keys(St.drumGrid[D.t]||{}).length;
  $('#padInfo').textContent=`${D.sec.name} · 2 battute di groove (si ripetono per tutta la sezione)${nM?` · ${nM} pad scritti a mano`:''}`;
  // etichette: ogni casella dice dove cade (1 e & a per i sedicesimi)
  const R=D.R,per=R/4,SUB={4:['','e','&','a'],6:['','·','·','&','·','·'],8:['','·','e','·','&','·','a','·']}[per];
  const head=`<div class="sq-h"></div>${Array.from({length:2*R},(_,k)=>{const kb=k%R,beat=Math.floor(kb/per),sub=kb%per;
    return`<div class="sq-n ${sub===0?'b':''}">${sub===0?(kb===0?'B'+(Math.floor(k/R)+1)+'·1':beat+1):SUB[sub]}</div>`;}).join('')}`;
  $('#padGrid').style.gridTemplateColumns=innerWidth<=560?`150px repeat(${2*R},22px)`:`236px repeat(${2*R},minmax(${R>16?12:18}px,1fr))`;
  $('#padGrid').innerHTML=head+PADS.map((p,i)=>{const R=D.rows[i],vol=St.padVol[i]!=null?St.padVol[i]:1;
    return`<div class="sq-h ${St.padMute[i]?'off':''}" data-row="${i}" style="--pc:${PAD_COL[i]}">
        <button class="sq-name" data-aud="${i}" title="Ascolta · nota ${kit.notes[i]} (${NOTE_NM(kit.notes[i])})"><i></i>${kit.names[i]}</button>
        <button class="ib ${St.padMute[i]?'on':''}" data-pm="${i}" title="Silenzia il pad">M</button>
        <button class="ib ${St.padSmp[i]?'on':''}" data-ps="${i}" title="${St.padSmp[i]?'Campione: '+esc(St.padSmpName[i]||'')+' — clic per tornare al suono sintetico':'Carica un campione audio per questo pad'}">${St.padSmp[i]?'♪':'📁'}</button>
        <button class="ib rg" data-pr="${i}" title="Rigenera solo questo pad">🎲</button>
        <input type="range" min="0" max="150" value="${Math.round(vol*100)}" data-pv="${i}" title="Volume ${Math.round(vol*100)}%">
        ${R.man?`<button class="ib on" data-pg="${i}" title="Scritto a mano: torna al generato">✎</button>`:'<span class="ib ghost" title="Generato">·</span>'}</div>`+
      R.v.map((v,k)=>{const ty=Math.floor(v/1000),vv=v%1000;return`<div class="sq-c ${v>0?'on':''} ${R.man?'man':''} ${k%per===0?'b':''} ${k===D.R?'bar':''}" data-p="${i}" data-s="${k}" style="--pc:${PAD_COL[i]};--o:${v>0?(.35+vv/127*.65).toFixed(2):0}">${ty?`<i class="rl">${ty+1}</i>`:''}</div>`;}).join('');}).join('');
  const g=$('#padGrid');
  g.querySelectorAll('[data-aud]').forEach(b=>b.onclick=()=>{const i=+b.dataset.aud;Synth.init();hitPad(i,kit.notes[i],Synth.now()+.01,100*(St.padVol[i]!=null?St.padVol[i]:1),i===9?2:.2);flashPad(i);});
  g.querySelectorAll('[data-ps]').forEach(b=>b.onclick=()=>{const i=+b.dataset.ps;if(St.padSmp[i]){const p={...St.padSmp},q={...St.padSmpName};delete p[i];delete q[i];St.padSmp=p;St.padSmpName=q;renderPads();track();toast('Pad: torna al suono sintetico');return;}padFor=i;$('#padSmpF').click();});
  g.querySelectorAll('[data-pm]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pm;St.padMute[i]=!St.padMute[i];if(!St.padMute[i])delete St.padMute[i];renderPads();drawArr();track();});
  g.querySelectorAll('[data-pr]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pr,k='drums.'+i;St.reseed.L[k]=(St.reseed.L[k]||0)+1;
    if(St.drumGrid[D.t]&&St.drumGrid[D.t][i]){delete St.drumGrid[D.t][i];}regen();toast(`${kit.names[i]}: nuova figura`);});
  g.querySelectorAll('[data-pv]').forEach(el=>{el.onchange=()=>{const i=+el.dataset.pv,v=+el.value/100;if(v===1)delete St.padVol[i];else St.padVol[i]=v;buildEv();track();};});
  g.querySelectorAll('[data-pg]').forEach(b=>b.onclick=()=>{const i=+b.dataset.pg;setGrid(D.t,i,null);});
  g.querySelectorAll('.sq-c').forEach(el=>el.onclick=()=>{const i=+el.dataset.p,k=+el.dataset.s,arr=D.rows[i].v.map(x=>Math.round(x)),v=arr[k];
    // ciclo: vuota → forte → ghost → roll ×2 → terzina ×3 → raffica ×4 → vuota
    const ty=Math.floor(v/1000),vv=v%1000;arr[k]=v===0?110:ty===0&&vv>=85?60:ty===0?1100:ty<3?(ty+1)*1000+100:0;setGrid(D.t,i,arr);
    if(arr[k]){Synth.init();const n2=Math.floor(arr[k]/1000)+1,sp=60/curBpm()*(4/D.R)/n2;for(let q=0;q<n2;q++)hitPad(i,kit.notes[i],Synth.now()+.01+q*sp,arr[k]%1000*(n2>1?.6+.4*q/(n2-1):1),.2);}});
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
$('#seqRes').onchange=e=>{St.seqRes=+e.target.value;LS.set('pg_seqRes',St.seqRes);renderPads();toast('Griglia: '+e.target.selectedOptions[0].text);};
St.seqRes=LS.get('pg_seqRes',16);
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
    if(l.id==='smp'){
      St.ev.forEach(e=>{if(e.l!=='smp')return;const ok=Synth.hasSample(e.sid);x.globalAlpha=on?(ok?.55:.2):.15;rr(x,px(e.t)+1,y+6,Math.max(4,px(e.d)-2),h-12,5);x.fill();
        x.globalAlpha=on?.95:.4;x.fillStyle=cssVar('--tx');x.font='600 10.5px Inter,system-ui,sans-serif';const c=St.clips[e.ci];if(c&&px(e.d)>40)x.fillText(fitTxt(x,c.name,px(e.d)-10),px(e.t)+6,y+h/2);x.fillStyle=cssVar('--c-smp');});
    }else if(l.id==='drums'){
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
  renderChar(sec);
  $('#regenSec').innerHTML=`<span class="label">🎲 Rigenera ${SEC_NAME[sec.type].toLowerCase()} <span class="hint" style="text-transform:none;letter-spacing:0;font-weight:500">· con il carattere impostato sopra</span></span><div class="seg">
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
// carattere della sezione: energia, densità, vuoto/pieno, strati (vale per tutte le ripetizioni di quel tipo)
const LAY_LAB=[['piano','Piano'],['arp','Arpeggio'],['gtr','Chitarra'],['pad','Pad'],['bass','Basso'],['drums','Batteria'],['mel','Melodia'],['cm','Controcanto']];
function renderChar(sec){
  const t=sec.type,C=St.secCfg[t]||{},LY=C.layers||{},p=sec.plan||{},G=GENRES[St.song.genre];
  const active={piano:p.piano!==false,arp:!!p.arp,gtr:!!p.gtr,pad:!!p.pad,bass:!!p.bass,drums:p.drums!=null,mel:!!p.mel,cm:!!p.mel&&p.cm!==false};
  const segBtns=(key,opts,cur)=>opts.map(([v,l])=>`<button data-ck="${key}" data-cv="${v}" class="${String(cur)===String(v)?'on':''}">${l}</button>`).join('');
  const has=Object.keys(C).length>0;
  // valori effettivi della sezione (propri o ereditati dal brano)
  const dv=sec.densV!=null?sec.densV:50,sv=Math.round((p.space||0)*100);
  const rangeCtl=(k,lab,v,own)=>`<div class="grp"><span class="dim" style="font-size:11.5px">${lab}</span><div class="rng sm"><input type="range" min="0" max="100" value="${v}" data-cr="${k}" class="${own?'':'auto'}"><b class="rv">${v}</b>${own?`<button class="btn sm ghost" data-crx="${k}" title="Usa il valore del brano">↺</button>`:'<small class="dim">brano</small>'}</div></div>`;
  $('#secChar').innerHTML=`<span class="label">Carattere · ${SEC_NAME[t].toLowerCase()}</span>
    <div class="grp"><span class="dim" style="font-size:11.5px">Lunghezza</span><div class="seg">${[2,4,8,12,16].map(n=>`<button data-len="${n}" class="${sec.bars===n?'on':''}" title="${n} battute (vale per tutte le sezioni ${SEC_NAME[t].toLowerCase()})">${n}</button>`).join('')}</div></div>
    <div class="grp"><span class="dim" style="font-size:11.5px">Energia</span><div class="seg">${segBtns('energy',[[-.3,'Calma'],['','Auto'],[.25,'Carica']],C.energy==null?'':C.energy)}</div></div>
    ${rangeCtl('densV','Densità',dv,C.densV!=null||C.dens!=null)}${rangeCtl('spaceV','Vuoto/pieno',sv,C.spaceV!=null||C.space!=null)}
    <div class="grp"><span class="dim" style="font-size:11.5px">Cosa suona</span>${LAY_LAB.filter(([k])=>k!=='drums'||G.drums).filter(([k])=>k!=='pad'||St.opts.pad).map(([k,l])=>{const f=LY[k];
      return`<button class="lyc ${active[k]?'act':''} ${f===true?'force-on':f===false?'force-off':''}" data-ly="${k}" style="--lc:var(--c-${k})" title="Clic: auto → sempre → mai">
        <i></i>${l}<small>${f===true?'sempre':f===false?'mai':'auto'}</small></button>`;}).join('')}</div>
    ${has?'<button class="btn sm ghost" id="charReset">Ripristina</button>':''}`;
  const set=(k,v)=>{const c=Object.assign({},St.secCfg[t]);if(v===''||v==null)delete c[k];else c[k]=v;if(Object.keys(c).length)St.secCfg[t]=c;else delete St.secCfg[t];regen();};
  $$('#secChar [data-len]').forEach(b=>b.onclick=()=>{const n=+b.dataset.len,s=St.song;if(n===sec.bars)return;
    St.secs=s.secs.map(([x,m])=>[x,x===t?n:m]);St.typeOrder=s.typeOrder.slice();const k=St.edits.length;St.edits=St.edits.filter(e=>e.type!==t);
    regen();toast(`${SEC_NAME[t]}: ${n} battute`+(St.edits.length<k?' · modifiche agli accordi azzerate':''));});
  $$('#secChar [data-cr]').forEach(el=>{el.oninput=()=>{el.nextElementSibling.textContent=el.value;el.classList.remove('auto');};
    el.onchange=()=>{const c=Object.assign({},St.secCfg[t]);delete c[el.dataset.cr==='densV'?'dens':'space'];c[el.dataset.cr]=+el.value;St.secCfg[t]=c;regen();};});
  $$('#secChar [data-crx]').forEach(b=>b.onclick=()=>{const c=Object.assign({},St.secCfg[t]);delete c[b.dataset.crx];delete c[b.dataset.crx==='densV'?'dens':'space'];
    if(Object.keys(c).length)St.secCfg[t]=c;else delete St.secCfg[t];regen();});
  $$('#secChar [data-ck]').forEach(b=>b.onclick=()=>set(b.dataset.ck,b.dataset.cv===''?null:+b.dataset.cv));
  $$('#secChar [data-ly]').forEach(b=>b.onclick=()=>{const k=b.dataset.ly,cur=LY[k],nx=cur==null?true:cur===true?false:null;
    const ly=Object.assign({},LY);if(nx==null)delete ly[k];else ly[k]=nx;set('layers',Object.keys(ly).length?ly:null);});
  const rr=$('#charReset');if(rr)rr.onclick=()=>{delete St.secCfg[t];regen();};
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
  const want=+$('#addLen').value||0,had=secs.find(x=>x[0]===t);
  const bars=want||(had?had[1]:(GENRES[s.genre].blues&&(t==='verse'||t==='chorus')?12:SEC_BARS[t]));
  // le sezioni dello stesso tipo condividono gli accordi: stessa lunghezza per tutte
  if(want&&had)secs.forEach(x=>{if(x[0]===t)x[1]=want;});
  secs.splice(St.sel+1,0,[t,bars]);if(TYPE_ORDER.includes(t)&&!order.includes(t))order.push(t);
  St.secs=secs;St.typeOrder=order;St.sel=St.sel+1;St.selChord=-1;regen();toast(`${SEC_NAME[t]} (${bars} battute) inserito dopo la sezione selezionata`);};
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
    // campioni già iniziati prima del punto di partenza: partono da metà
    if(audible('smp'))St.ev.forEach(e=>{if(e.l==='smp'&&e.t<b-1e-3&&e.t+e.d>b+.05)playClipEv(e,this.base+b*60/curBpm(),b-e.t);});
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
      if(at>=now-.04&&audible(e.l)){if(e.l==='smp')playClipEv(e,at,0);else if(e.l==='drums'){if(!St.padMute[e.pad]){hitPad(e.pad,e.n,at,e.v,e.d*spb);if(St.showPads){const pd=e.pad;setTimeout(()=>flashPad(pd),Math.max(0,(at-now)*1000));}}}
        else{const si=St.inst[e.l];if(si&&si.sid&&Synth.hasSample(si.sid))Synth.playSample(si.sid,at,{dur:Math.max(.08,e.d*spb),rate:Math.pow(2,(e.n-(si.root||60))/12),gain:Math.pow(e.v/127,1.2)*1.1,layer:e.l});
          else Synth.note(instOf(e.l),e.n,at,e.d*spb,e.v,e.l,e.gl,e.art);}}
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
  if(Cloud.dl){const ext=name.split('.').pop().toLowerCase();let fn=name,d=data;
    if(!['zip','json','txt','csv','md','html','pdf'].includes(ext)){d=makeZip([{name,data:typeof data==='string'?new TextEncoder().encode(data):data}]);fn=name+'.zip';}
    Cloud.dl.save({filename:fn,data:d}).then(()=>toast('Salvato: '+fn)).catch(e=>{if(e&&e.code!=='declined')toast('Download non riuscito: '+((e&&e.message)||'riprova'));});return;}
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
const snapState=()=>JSON.stringify({opts:St.opts,seeds:St.seeds,secs:St.secs,typeOrder:St.typeOrder,edits:St.edits,oct:St.oct,mute:St.mute,solo:St.solo,bpm:St.bpm,name:St.name,reseed:St.reseed,vol:St.vol,padMute:St.padMute,padVol:St.padVol,padDens:St.padDens,drumGrid:St.drumGrid,feel:St.feel,style:St.style,inst:St.inst,lib:St.lib,clips:St.clips,padSmp:St.padSmp,padSmpName:St.padSmpName,imports:St.imports,secCfg:St.secCfg});
function track(){
  const cur=snapState();
  if(H.last&&cur!==H.last&&!H.restoring){H.undo.push(H.last);if(H.undo.length>200)H.undo.shift();H.redo=[];}
  H.last=cur;LS.set('pg_current',JSON.parse(cur));updHist();Cloud.saveCurrent(cur);
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
  Object.assign(St,{opts:Object.assign({},DEF,{arr:'ricca'},o.opts),seeds:o.seeds,secs:o.secs||null,typeOrder:o.typeOrder||null,edits:o.edits||[],oct:o.oct||{},
    mute:o.mute||{},solo:o.solo||null,bpm:o.bpm||null,name:o.name||St.name,muteInit:true,reseed:JSON.parse(JSON.stringify(o.reseed||{L:{},S:{}})),vol:{...(o.vol||{})},padMute:{...(o.padMute||{})},padVol:{...(o.padVol||{})},padDens:{...(o.padDens||{})},drumGrid:JSON.parse(JSON.stringify(o.drumGrid||{})),feel:JSON.parse(JSON.stringify(o.feel||{})),style:{...(o.style||{})},inst:JSON.parse(JSON.stringify(o.inst||{})),lib:JSON.parse(JSON.stringify(o.lib||[])),clips:JSON.parse(JSON.stringify(o.clips||[])),padSmp:{...(o.padSmp||{})},padSmpName:{...(o.padSmpName||{})},imports:JSON.parse(JSON.stringify(o.imports||[])),secCfg:JSON.parse(JSON.stringify(o.secCfg||{}))});
  syncControls();$('#projName').value=St.name;closePop();
  if(St.sel>=0)St.sel=Math.min(St.sel,99);regen();ensureSamples();H.restoring=false;H.last=json;LS.set('pg_current',o);updHist();
}
function undo(){if(!H.undo.length)return;H.redo.push(H.last);restore(H.undo.pop());toast('Indietro');}
function redo(){if(!H.redo.length)return;H.undo.push(H.last);restore(H.redo.pop());toast('Avanti');}
$('#undo').onclick=undo;$('#redo').onclick=redo;

/* ---------------- Progetti ---------------- */
const projects=()=>Cloud.ready?Cloud.list.slice():LS.get('pg_projects',[]);
// scrive la lista: nel browser e, se disponibile, nell'archivio permanente del tuo account
function setProjects(list,{put=[],del=[]}={}){LS.set('pg_projects',list);if(!Cloud.ready)return;Cloud.list=list.slice();
  put.forEach(id=>{const p=list.find(x=>x.id===id);if(p)Cloud.save(p.id,{name:p.name,date:p.date,state:p.state});});del.forEach(id=>Cloud.remove(id));}
const saveMeta=()=>LS.set('pg_meta',{projId:St.projId,savedSnap:St.savedSnap});
function writeProject(asNew,name){
  const list=projects(),now=Date.now();if(name)St.name=name;$('#projName').value=St.name;track();const state=JSON.parse(H.last);
  if(asNew||!St.projId||!list.some(p=>p.id===St.projId)){St.projId='p'+now.toString(36);list.unshift({id:St.projId,name:St.name,date:now,state});}
  else{const p=list.find(p=>p.id===St.projId);Object.assign(p,{name:St.name,date:now,state});}
  setProjects(list,{put:[St.projId]});St.savedSnap=H.last;saveMeta();updHist();toast(`Progetto “${St.name}” salvato`+(Cloud.ready?' nel tuo archivio':''));
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
  // con un progetto aperto chiede sempre: sovrascrivere o salvare come nuovo progetto (anche senza modifiche)
  const dirty=isDirty();
  ask('Dove salvo?',`<p class="muted" style="font-size:13.5px">${dirty?`Hai modifiche rispetto a <b>“${esc(cur.name)}”</b> (salvato ${fmtDate(cur.date)}).`:`<b>“${esc(cur.name)}”</b> è già salvato così. Puoi salvarne una copia come nuovo progetto.`}</p>
    <div class="mrow"><input type="text" id="askName" value="${esc(St.name===cur.name?St.name+' (2)':St.name)}" placeholder="Nome del nuovo progetto"></div>`,
    [{label:'Annulla',cls:'ghost'},{label:'Salva come nuovo',fn:()=>{writeProject(true,($('#askName').value||'').trim()||St.name+' (2)');then&&then();}},
     ...(dirty?[{label:`Sovrascrivi “${esc(cur.name)}”`,cls:'primary',fn:()=>{writeProject(false,cur.name===St.name?null:St.name);then&&then();}}]:[])]);
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
    <div class="mrow"><button class="btn primary" id="mSave">Salva il brano attuale…</button>${St.projId?'<button class="btn" id="mSaveNew">Salva come nuovo…</button>':''}<span class="dim" style="font-size:12px">${St.projId?'progetto: “'+esc(St.name)+'”'+(isDirty()?' · modifiche non salvate':' · salvato'):'non ancora salvato'}</span></div>
    <div class="plist">${list.length?list.map(p=>`<div class="pitem ${p.id===St.projId?'cur':''}"><div class="pi"><b>${p.name.replace(/</g,'&lt;')}</b>
      <small>${(GENRES[p.state.opts.genre]||{}).n||''} · ${(MOODS[p.state.opts.mood]||{}).n||''} · ${fmtDate(p.date)}${p.state.edits&&p.state.edits.length?` · ${p.state.edits.length} modifiche`:''}</small></div>
      <button class="btn sm" data-open="${p.id}">Apri</button><button class="btn sm ghost danger" data-del="${p.id}">Elimina</button></div>`).join(''):'<div class="empty">Nessun progetto salvato. Dai un nome al brano e premi Salva.</div>'}</div>
    <div class="mrow"><button class="btn" id="mNew">+ Nuovo progetto</button><span class="spacer"></span>
      <button class="btn ghost" id="mExp" title="Solo il brano aperto">⬇ Brano .json</button><button class="btn" id="mBak" ${list.length?'':'disabled'} title="Tutti i progetti in un file">⬇ Backup di tutti</button><button class="btn ghost" id="mImp" title="Un brano o un backup completo">⬆ Importa</button><input type="file" id="mFile" accept=".json,application/json" hidden></div>
    ${Cloud.ready?'<p class="okmsg">☁ I progetti sono salvati nel tuo archivio personale: restano anche se chiudi, ricarichi o aggiorni il sito.</p>':'<p class="warnmsg">⚠ Qui i progetti stanno solo nella memoria del browser, che alcuni visori cancellano dopo pochi minuti. Apri il sito dal suo link fisso per salvarli in modo permanente, oppure fai spesso “Backup di tutti”.</p>'}
    <p class="dim" style="font-size:12px">I progetti vivono nella memoria del browser <b>legata all'indirizzo da cui apri il file</b>: se apri una nuova versione da un altro percorso (o in un'altra app/browser) la lista riparte vuota. Prima di passare a una versione nuova fai <b>Backup di tutti</b>, poi nella nuova versione usa <b>Importa</b>.</p>`;
  $('#mSave').onclick=()=>saveProject();
  const sn=$('#mSaveNew');if(sn)sn.onclick=()=>ask('Salva come nuovo progetto',`<div class="mrow"><input type="text" id="askName" value="${esc(St.name+' (2)')}" placeholder="Nome del nuovo progetto"></div>`,
    [{label:'Annulla',cls:'ghost'},{label:'Salva come nuovo',cls:'primary',fn:()=>writeProject(true,($('#askName').value||'').trim()||St.name+' (2)')}]);
  $$('#mBody [data-open]').forEach(b=>b.onclick=()=>openProject(b.dataset.open));
  $$('#mBody [data-del]').forEach(b=>b.onclick=()=>{if(b.dataset.sure){setProjects(projects().filter(p=>p.id!==b.dataset.del),{del:[b.dataset.del]});
      if(St.projId===b.dataset.del){St.projId=null;St.savedSnap=null;saveMeta();updHist();}renderProjects();}else{b.dataset.sure=1;b.textContent='Sicuro?';}});
  $('#mNew').onclick=()=>newProject();
  $('#mExp').onclick=()=>download(slug(St.name||'progetto')+'.pianogen.json',JSON.stringify({app:'piano-generativo',v:1,name:St.name,state:JSON.parse(H.last)},null,1),'application/json');
  $('#mBak').onclick=()=>{const d=new Date(),ts=d.toISOString().slice(0,10);download(`piano-generativo-backup-${ts}.json`,JSON.stringify({app:'piano-generativo',v:1,backup:true,date:Date.now(),projects:projects()},null,1),'application/json');LS.set('pg_lastBackup',Date.now());toast(`Backup di ${list.length} progetti scaricato`);};
  $('#mImp').onclick=()=>$('#mFile').click();
  $('#mFile').onchange=e=>{const f=e.target.files[0];if(!f)return;if(isDirty()&&!e.target._ok){guard(()=>{e.target._ok=1;e.target.onchange(e);});return;}e.target._ok=0;const rd=new FileReader();rd.onload=()=>{try{const d=JSON.parse(rd.result);
      // backup completo: unisce i progetti (quelli già presenti con lo stesso id restano, quelli nuovi si aggiungono)
      if(d.backup&&Array.isArray(d.projects)){const cur=projects(),ids=new Set(cur.map(x=>x.id)),add=d.projects.filter(x=>x&&x.id&&x.state&&x.state.opts&&!ids.has(x.id));
        setProjects(cur.concat(add).sort((x,y)=>(y.date||0)-(x.date||0)),{put:add.map(x=>x.id)});renderProjects();toast(add.length?`${add.length} progetti ripristinati`:'Nessun progetto nuovo nel backup');return;}
      const st=d.state||d;
      if(!st.opts||!st.seeds)throw 0;H.undo.push(H.last);H.redo=[];St.projId=null;St.savedSnap=null;restore(JSON.stringify({...st,name:d.name||st.name||'Importato'}));closeModal();toast('Progetto importato');}
    catch(err){toast('File non valido');}};rd.readAsText(f);};
}
$('#btnProjects').onclick=()=>{renderProjects();$('#modal').hidden=false;};
$('#btnSave').onclick=()=>saveProject();
$('#mClose').onclick=closeModal;
$('#modal').addEventListener('pointerdown',e=>{if(e.target.id==='modal')closeModal();});
$('#projName').addEventListener('change',e=>{St.name=e.target.value.trim()||'Senza titolo';e.target.value=St.name;track();});

/* ---------------- Tema ---------------- */
function setTheme(t){if(t==='auto')delete document.documentElement.dataset.skin;else document.documentElement.dataset.skin=t;LS.set('pg_theme',t);$('#theme').value=t;
  if(St.song){drawArr();renderPads();}}
$('#theme').onchange=e=>setTheme(e.target.value);
setTheme(LS.get('pg_theme','crema'));
matchMedia('(prefers-color-scheme: dark)').addEventListener('change',()=>{if(LS.get('pg_theme','crema')==='auto'&&St.song)drawArr();});

/* ---------------- Campioni audio e sequenze MIDI ----------------
   - i file audio restano in questo browser (IndexedDB); progetto e codice tengono solo il riferimento
   - clip: un campione che parte da una battuta, lungo N battute, ripetuto; "adatta al tempo" lo stira sulle battute
   - pad: un campione al posto del suono sintetico di un pad della batteria
   - sequenze MIDI: le note entrano in una traccia (sostituiscono il generato in quel tratto) e finiscono nell'export */
const IDB={db:null,
  open(){return this.db?Promise.resolve(this.db):new Promise((ok,ko)=>{try{const r=indexedDB.open('pg_samples',1);r.onupgradeneeded=()=>r.result.createObjectStore('s');
    r.onsuccess=()=>{this.db=r.result;ok(this.db);};r.onerror=()=>ko(r.error);}catch(e){ko(e);}});},
  async put(id,v){const db=await this.open();return new Promise((ok,ko)=>{const tx=db.transaction('s','readwrite');tx.objectStore('s').put(v,id);tx.oncomplete=ok;tx.onerror=()=>ko(tx.error);});},
  async get(id){const db=await this.open();return new Promise((ok,ko)=>{const q=db.transaction('s').objectStore('s').get(id);q.onsuccess=()=>ok(q.result);q.onerror=()=>ko(q.error);});}};
const smpLoading=new Set();
// carica nel motore audio i campioni usati dal progetto (se ci sono in questo browser)
async function ensureSamples(){
  const ids=new Set([...(St.clips||[]).map(c=>c.sid),...Object.values(St.padSmp||{}),...Object.values(St.inst||{}).map(x=>x&&x.sid).filter(Boolean),...libAll().map(x=>x.sid)]);
  for(const id of ids){if(Synth.hasSample(id)||smpLoading.has(id))continue;smpLoading.add(id);
    try{let rec=await IDB.get(id).catch(()=>null);
      // non è nel browser: si prende dall'archivio
      if(!(rec&&rec.data)&&Cloud.ready){rec=await Cloud.getSample(id);if(rec)IDB.put(id,{name:rec.name,type:rec.type,data:rec.data}).catch(()=>{});}
      if(rec&&rec.data){await Synth.loadSample(id,rec.data);}
      // era solo nel browser: lo copia nell'archivio
      if(rec&&rec.data&&Cloud.ready&&!rec._fromCloud){Cloud.smpCol(id).doc('p0').get().then(s=>{if(!s.exists)Cloud.putSample(id,rec.name||id,rec.type||'',rec.data.slice(0));}).catch(()=>{});}
    }catch(e){}smpLoading.delete(id);}
  renderSamples();drawArr();
}
async function addSampleFile(f){
  const data=await f.arrayBuffer(),id='s'+Date.now().toString(36)+Math.floor(Math.random()*1e4).toString(36);
  await Synth.loadSample(id,data);await IDB.put(id,{name:f.name,type:f.type,data}).catch(()=>{});
  if(Cloud.ready)Cloud.putSample(id,f.name,f.type,data.slice(0)).then(ok=>{if(ok)toast(`“${f.name}” salvato nel tuo archivio`);});
  return{id,name:f.name.replace(/\.[^.]+$/,'')};
}
const curSec=()=>St.song.sections[St.sel];
function clipEvents(){const out=[];(St.clips||[]).forEach((c,ci)=>{for(let k=0;k<(c.reps||1);k++){const t=c.start+k*c.bars*4;if(t>=St.song.beats)break;
  out.push({t,d:Math.min(c.bars*4,St.song.beats-t),l:'smp',sid:c.sid,fit:c.fit!==false,bars:c.bars,v:Math.round((c.vol!=null?c.vol:1)*100),ci});}});return out;}
function playClipEv(e,at,offBeats){const spb=60/curBpm(),dur=Synth.sampleDur(e.sid);if(!dur)return;
  const rate=e.fit?dur/(e.bars*4*spb):1;Synth.playSample(e.sid,at,{dur:(e.d-(offBeats||0))*spb,rate,gain:e.v/100,offset:(offBeats||0)*spb*rate});}
function hitPad(i,n,at,v,d){const sid=(St.padSmp||{})[i];if(sid&&Synth.hasSample(sid)){Synth.playSample(sid,at,{gain:Math.pow(v/127,1.2)*1.1,layer:'drums'});return;}Synth.hit(n,at,v,d);}
async function onAudioFiles(files){
  if(!files.length)return;Synth.init();let n=0;const sec=curSec();
  for(const f of files){try{const s=await addSampleFile(f),spb=60/curBpm(),bars=clamp(Math.round(Synth.sampleDur(s.id)/(4*spb))||1,1,16);
      libAdd({sid:s.id,name:s.name,root:60});St.clips.push({sid:s.id,name:s.name,start:sec.startBeat,bars,reps:Math.max(1,Math.floor(sec.bars/bars)),fit:true,vol:1});n++;}catch(e){toast(`“${f.name}” non è un file audio leggibile`);}}
  if(n){St.mute.smp=false;buildEv();renderAll();track();toast(`${n} campion${n>1?'i':'e'} su ${sec.name}`);}
}
/* ---- MIDI: lettura di un file .mid ---- */
function parseMidi(buf){
  const d=new DataView(buf);let p=0;const str=n=>{let s='';for(let i=0;i<n;i++)s+=String.fromCharCode(d.getUint8(p+i));p+=n;return s;};
  const u32=()=>{const v=d.getUint32(p);p+=4;return v;},u16=()=>{const v=d.getUint16(p);p+=2;return v;},vlq2=()=>{let v=0,b;do{b=d.getUint8(p++);v=(v<<7)|(b&127);}while(b&128);return v;};
  if(str(4)!=='MThd')throw new Error('non è un file MIDI');const hl=u32(),fmt=u16(),nt=u16(),div=u16();p+=hl-6;if(div&0x8000)throw new Error('formato SMPTE non supportato');
  const tracks=[];
  for(let k=0;k<nt&&p<buf.byteLength;k++){if(str(4)!=='MTrk'){break;}const len=u32(),end=p+len;let tick=0,run=0,name='',on={};const notes=[];
    while(p<end){tick+=vlq2();let st=d.getUint8(p);if(st&0x80){p++;run=st;}else st=run;
      const ty=st&0xF0,ch=st&15;
      if(st===0xFF){const mt=d.getUint8(p++),l=vlq2();if(mt===3)name=str(l);else p+=l;}
      else if(st===0xF0||st===0xF7){p+=vlq2();}
      else if(ty===0x90||ty===0x80){const n=d.getUint8(p++),v=d.getUint8(p++),key=ch+'_'+n;
        if(ty===0x90&&v>0){(on[key]=on[key]||[]).push({t:tick,v});}else{const o=on[key]&&on[key].shift();if(o)notes.push({t:o.t/div,d:Math.max(.05,(tick-o.t)/div),n,v:o.v,ch});}}
      else if(ty===0xC0||ty===0xD0)p+=1;else p+=2;}
    p=end;if(notes.length)tracks.push({name:name||('Traccia '+(tracks.length+1)),notes:notes.sort((a,b)=>a.t-b.t),drums:notes.every(x=>x.ch===9)});}
  return tracks;
}
const GM2PAD=n=>n<=36?0:n===38||n===40||n===39?1:n===37?2:n===42||n===44?3:n===46?4:n===69||n===70||n===82?5:[49,52,55,57].includes(n)?8:[41,43,45,60,62,63,65].includes(n)?6:[47,48,50,61,64,66].includes(n)?7:9;
async function onMidiFile(f){
  let tracks;try{tracks=parseMidi(await f.arrayBuffer());}catch(e){toast('MIDI non leggibile: '+e.message);return;}
  if(!tracks.length){toast('Nessuna nota nel file');return;}
  const sec=curSec(),dest=LAYERS.filter(l=>l.id!=='pad');
  ask('Importa sequenza MIDI',`<p class="muted" style="font-size:13px">“${esc(f.name)}” · ${tracks.length} tracc${tracks.length>1?'e':'ia'} con note.</p>
    <div class="mrow"><label class="field" style="flex:1"><span class="label">Traccia del file</span><select id="impT">${tracks.map((t,i)=>`<option value="${i}">${esc(t.name)} · ${t.notes.length} note${t.drums?' (batteria)':''}</option>`).join('')}</select></label></div>
    <div class="mrow"><label class="field" style="flex:1"><span class="label">Nella traccia</span><select id="impL">${dest.map(l=>`<option value="${l.id}">${l.n}</option>`).join('')}</select></label>
      <label class="field" style="flex:1"><span class="label">Da battuta</span><select id="impB">${Array.from({length:St.song.bars},(_,b)=>`<option value="${b}" ${b===sec.startBar?'selected':''}>${b+1}${St.song.sections.find(z=>z.startBar===b)?' · '+St.song.sections.find(z=>z.startBar===b).name:''}</option>`).join('')}</select></label></div>
    <label class="toggle" style="margin-top:6px"><input type="checkbox" id="impR" checked>Sostituisci il generato in quel tratto</label>`,
    [{label:'Annulla',cls:'ghost'},{label:'Importa',cls:'primary',fn:()=>{
      const T=tracks[+$('#impT').value],layer=$('#impL').value,start=+$('#impB').value*4,t0=Math.floor(T.notes[0].t/4)*4;
      const notes=T.notes.map(x=>({t:+(x.t-t0).toFixed(4),d:+x.d.toFixed(4),n:x.n,v:x.v}));const span=Math.ceil((Math.max(...notes.map(x=>x.t+x.d))-.01)/4)*4||4;
      St.imports.push({name:f.name.replace(/\.[^.]+$/,'')+(tracks.length>1?' · '+T.name:''),layer,start,len:span,replace:$('#impR').checked,notes});
      regen();toast(`${notes.length} note importate in ${LAYERS.find(l=>l.id===layer).n}`);}}]);
  const tSel=$('#impT');const pickL=()=>{const T=tracks[+tSel.value];const avg=T.notes.reduce((q,x)=>q+x.n,0)/T.notes.length,poly=T.notes.filter((x,i)=>i&&Math.abs(x.t-T.notes[i-1].t)<.03).length/T.notes.length;
    $('#impL').value=T.drums?'drums':avg<52?'bass':poly>.4?'piano':'mel';};tSel.onchange=pickL;pickL();
}
// le sequenze importate entrano nella canzone generata
function applyImports(s){
  (St.imports||[]).forEach(im=>{const L=s.layers[im.layer];if(!L)return;const a=im.start,b=im.start+im.len;
    if(im.replace)s.layers[im.layer]=L.filter(e=>e.t<a-.01||e.t>=b-.01);
    const kit=drumKit(GENRES[s.genre]);
    im.notes.forEach(x=>{const t=a+x.t;if(t>=s.beats)return;const e={t,d:Math.min(x.d,s.beats-t),n:x.n,v:clamp(x.v,1,127)};
      if(im.layer==='drums'){e.pad=GM2PAD(x.n);e.d=.12;}s.layers[im.layer].push(e);});
    s.layers[im.layer].sort((p,q)=>p.t-q.t);});
}
function renderSamples(){
  const el=$('#smpList');if(!el)return;const s=St.song,bars=s.bars;
  const barOpts=cur=>Array.from({length:bars},(_,b)=>`<option value="${b}" ${b*4===cur?'selected':''}>batt. ${b+1}</option>`).join('');
  const clips=(St.clips||[]).map((c,i)=>{const ok=Synth.hasSample(c.sid);return`<div class="smprow ${ok?'':'miss'}">
      <span class="sn" title="${esc(c.name)}">🔊 ${esc(c.name)}${ok?'':' <small>(non in questo browser)</small>'}</span>
      <select data-cs="${i}" data-k="start">${barOpts(c.start)}</select>
      <label>lungo <select data-cs="${i}" data-k="bars">${[1,2,4,8,16].map(n=>`<option ${n===c.bars?'selected':''}>${n}</option>`).join('')}</select> batt.</label>
      <label>× <select data-cs="${i}" data-k="reps">${[1,2,3,4,6,8,12,16,24,32].map(n=>`<option ${n===c.reps?'selected':''}>${n}</option>`).join('')}</select></label>
      <label class="toggle sm" title="Stira il campione sulla lunghezza in battute (cambia anche l'intonazione)"><input type="checkbox" data-cs="${i}" data-k="fit" ${c.fit!==false?'checked':''}>adatta al tempo</label>
      <input type="range" min="0" max="150" value="${Math.round((c.vol!=null?c.vol:1)*100)}" data-cs="${i}" data-k="vol" title="Volume">
      <button class="ib" data-cp="${i}" title="Ascolta">▶</button><button class="ib" data-cx="${i}" title="Rimuovi">✕</button></div>`;}).join('');
  const imps=(St.imports||[]).map((m,i)=>`<div class="smprow"><span class="sn">🎼 ${esc(m.name)}</span><span class="dim">${LAYERS.find(l=>l.id===m.layer).n} · da batt. ${m.start/4+1} · ${m.notes.length} note · ${m.len/4} batt.${m.replace?' · sostituisce':' · si aggiunge'}</span><span class="spacer"></span><button class="ib" data-ix="${i}" title="Rimuovi">✕</button></div>`).join('');
  el.innerHTML=(clips||imps)?clips+imps:'<div class="empty" style="padding:10px 0">Nessun campione o sequenza. Carica uno o più file audio (wav, mp3, ogg…) o un file MIDI: entrano dalla sezione selezionata.</div>';
  el.querySelectorAll('[data-cs]').forEach(x=>x.onchange=()=>{const c=St.clips[+x.dataset.cs],k=x.dataset.k;
    c[k]=k==='fit'?x.checked:k==='vol'?+x.value/100:k==='start'?+x.value*4:+x.value;buildEv();drawArr();track();});
  el.querySelectorAll('[data-cp]').forEach(b=>b.onclick=()=>{Synth.init();const c=St.clips[+b.dataset.cp];playClipEv({sid:c.sid,fit:c.fit!==false,bars:c.bars,d:c.bars*4,v:Math.round((c.vol??1)*100)},Synth.now()+.05,0);});
  el.querySelectorAll('[data-cx]').forEach(b=>b.onclick=()=>{St.clips.splice(+b.dataset.cx,1);buildEv();renderAll();track();});
  el.querySelectorAll('[data-ix]').forEach(b=>b.onclick=()=>{St.imports.splice(+b.dataset.ix,1);regen();});
}
$('#smpAudio').onclick=()=>$('#smpAudioF').click();
$('#smpAudioF').onchange=e=>{onAudioFiles([...e.target.files]);e.target.value='';};
$('#smpMidi').onclick=()=>$('#smpMidiF').click();
$('#smpMidiF').onchange=e=>{const f=e.target.files[0];if(f)onMidiFile(f);e.target.value='';};
// campione su un pad della batteria
let padFor=null;
$('#padSmpF').onchange=async e=>{const f=e.target.files[0];e.target.value='';if(!f||padFor==null)return;Synth.init();
  try{const s=await addSampleFile(f);libAdd({sid:s.id,name:s.name,root:60});St.padSmp={...St.padSmp,[padFor]:s.id};St.padSmpName={...St.padSmpName,[padFor]:s.name};renderPads();track();
    hitPad(padFor,0,Synth.now()+.02,110);toast(`Pad ${padFor+1}: “${s.name}”`);}catch(err){toast('File audio non leggibile');}};

/* ---------------- Archivio permanente ----------------
   Aperto dal link fisso del sito, i progetti e il lavoro in corso vanno nel database del tuo account
   (privato, sopravvive a ricariche, aggiornamenti e dispositivi). Altrove resta la memoria del browser, che alcuni visori cancellano. */
(async()=>{try{
  if(!window.claude||typeof window.claude.use!=='function')return;
  const [db,user,dl]=await Promise.all([claude.use('db'),claude.use('user'),claude.use('downloads')]);
  Cloud.dl=dl;if(!db||!user)return;const uid=await user.id();if(!uid)return;
  Cloud.col=db.collection('data/users/'+uid);
  const snap=await Cloud.col.get();const list=[];let cur=null;
  snap.docs.forEach(d=>{const x=d.data();if(!x)return;if(d.id==='library'){Cloud.lib=x.items||[];return;}if(d.id==='current')cur=x;else if(x.state&&x.state.opts)list.push({id:d.id,name:x.name,date:x.date,state:x.state});});
  // i progetti che c'erano solo nel browser entrano nell'archivio
  const ids=new Set(list.map(p=>p.id)),local=LS.get('pg_projects',[]).filter(p=>p&&p.id&&p.state&&!ids.has(p.id));
  local.forEach(p=>{list.push(p);Cloud.save(p.id,{name:p.name,date:p.date,state:p.state});});
  list.sort((x,y)=>(y.date||0)-(x.date||0));Cloud.list=list;Cloud.ready=true;LS.set('pg_projects',list);
  // se il browser ha perso il lavoro in corso, si riprende quello dell'archivio
  if(!bootHadLocal&&cur&&cur.state&&cur.state.opts){restore(JSON.stringify(cur.state));toast('Ripreso il lavoro in corso dal tuo archivio');}
  else toast(list.length?`Archivio collegato: ${list.length} progetti`:'Archivio collegato: i progetti si salvano nel tuo account');
  updHist();ensureSamples();if(!$('#modal').hidden&&$('#mTitle').textContent==='Progetti')renderProjects();
}catch(e){}})();

/* ---------------- Avvio ---------------- */
$('#projName').value=St.name;
// una volta sola: con la composizione sobria la melodia parte spenta anche nel lavoro in corso
if(St.opts.arr==='sobria'&&!LS.get('pg_sob2',null)){['mel','cm','gtr','pad','drums'].forEach(k=>St.mute[k]=true);St.mute.piano=St.mute.arp=St.mute.bass=false;LS.set('pg_sob2',1);}
syncControls();regen({resetMute:!St.muteInit});ensureSamples();
window.__pg={St,regen,Player,selectSection,parseMidi,onAudioFiles,applyImports};
