'use strict';
/* =====================================================================
   PIANO GENERATIVO — motore
   armonia per sezioni · voicing a due mani · arrangiamento a strati ·
   melodia per frasi · esportazione MIDI / ZIP
   ===================================================================== */

/* ---------------- Utilità ---------------- */
function rngFrom(seed){let a=seed>>>0;return()=>{a|=0;a=a+0x6D2B79F5|0;let t=Math.imul(a^a>>>15,1|a);t=t+Math.imul(t^t>>>7,61|t)^t;return((t^t>>>14)>>>0)/4294967296;};}
function hashStr(s){let h=2166136261;for(let i=0;i<s.length;i++){h^=s.charCodeAt(i);h=Math.imul(h,16777619);}return h>>>0;}
const subRng=(seed,tag)=>rngFrom(hashStr(seed+':'+tag));
const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
const pick=(r,a)=>a[Math.floor(r()*a.length)];
function pickW(r,arr){let s=0;for(const e of arr)s+=Math.max(0,e[1]);if(s<=0)return arr[Math.floor(r()*arr.length)][0];
  let x=r()*s;for(const e of arr){x-=Math.max(0,e[1]);if(x<=0)return e[0];}return arr[arr.length-1][0];}
function softPick(r,arr,T){const m=Math.min(...arr.map(e=>e[1]));return pickW(r,arr.map(([v,c])=>[v,Math.exp(-(c-m)/T)]));}
const mod12=x=>((x%12)+12)%12;
const nearestPc=(pc,ref)=>{let m=ref+mod12(pc-ref);if(m-ref>6)m-=12;return m;};

/* ---------------- Teoria ---------------- */
const MAJ=[0,2,4,5,7,9,11],ROMAN=['I','II','III','IV','V','VI','VII'];
const SHARP=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'],FLAT=['C','Db','D','Eb','E','F','Gb','G','Ab','A','Bb','B'];
const KEY_NAMES=['C','Db','D','Eb','E','F','F#','G','Ab','A','Bb','B'];
const SF=[0,-5,2,-3,4,-1,6,1,-4,3,-2,5];
const MODES={
  major:     {n:'Maggiore',          scale:[0,2,4,5,7,9,11],parent:0, minor:0},
  minor:     {n:'Minore naturale',   scale:[0,2,3,5,7,8,10],parent:3, minor:1},
  harmonic:  {n:'Minore armonico',   scale:[0,2,3,5,7,8,11],parent:3, minor:1},
  dorian:    {n:'Dorico',            scale:[0,2,3,5,7,9,10],parent:10,minor:1,modal:1,char:{r:5,tri:'maj'}},
  phrygian:  {n:'Frigio',            scale:[0,1,3,5,7,8,10],parent:8, minor:1,modal:1,char:{r:1,tri:'maj'}},
  lydian:    {n:'Lidio',             scale:[0,2,4,6,7,9,11],parent:7, minor:0,modal:1,char:{r:2,tri:'maj'}},
  mixolydian:{n:'Misolidio',         scale:[0,2,4,5,7,9,10],parent:5, minor:0,modal:1,char:{r:10,tri:'maj'}}};
const MODE_ORDER=['major','minor','harmonic','dorian','phrygian','lydian','mixolydian'];
const keySf=(key,mode)=>SF[mod12(key+MODES[mode].parent)];
const speller=(key,mode)=>{const n=keySf(key,mode)>0?SHARP:FLAT;return pc=>n[mod12(pc)];};

const QT={
  maj:{iv:[0,4,7],n:'',r:''},min:{iv:[0,3,7],n:'m',r:'',m:1},dim:{iv:[0,3,6],n:'dim',r:'°',m:1},aug:{iv:[0,4,8],n:'aug',r:'+'},
  sus2:{iv:[0,2,7],n:'sus2',r:'sus2'},sus4:{iv:[0,5,7],n:'sus4',r:'sus4'},
  add9:{iv:[0,4,7,14],n:'add9',r:'add9'},madd9:{iv:[0,3,7,14],n:'m(add9)',r:'(add9)',m:1},
  '6':{iv:[0,4,7,9],n:'6',r:'6'},m6:{iv:[0,3,7,9],n:'m6',r:'6',m:1},'69':{iv:[0,4,7,9,14],n:'6/9',r:'6/9'},
  maj7:{iv:[0,4,7,11],n:'maj7',r:'maj7'},m7:{iv:[0,3,7,10],n:'m7',r:'7',m:1},'7':{iv:[0,4,7,10],n:'7',r:'7'},
  m7b5:{iv:[0,3,6,10],n:'m7b5',r:'ø7',m:1},dim7:{iv:[0,3,6,9],n:'dim7',r:'°7',m:1},mmaj7:{iv:[0,3,7,11],n:'m(maj7)',r:'(maj7)',m:1},
  '7sus4':{iv:[0,5,7,10],n:'7sus4',r:'7sus4'},'9sus4':{iv:[0,5,7,10,14],n:'9sus4',r:'9sus4'},
  maj9:{iv:[0,4,7,11,14],n:'maj9',r:'maj9'},m9:{iv:[0,3,7,10,14],n:'m9',r:'9',m:1},'9':{iv:[0,4,7,10,14],n:'9',r:'9'},
  'maj7#11':{iv:[0,4,7,11,18],n:'maj7#11',r:'maj7#11'},m11:{iv:[0,3,7,10,14,17],n:'m11',r:'11',m:1},
  '13':{iv:[0,4,7,10,14,21],n:'13',r:'13'},'7b9':{iv:[0,4,7,10,13],n:'7b9',r:'7b9'},'7#9':{iv:[0,4,7,10,15],n:'7#9',r:'7#9'}};
const TRI_IV={maj:[0,4,7],min:[0,3,7],dim:[0,3,6],aug:[0,4,8]};

function parseRN(s){
  const m=s.match(/^([b#]?)([ivIV]+)([°+]?)$/);if(!m)throw new Error('RN '+s);
  const up=m[2]===m[2].toUpperCase(),deg=ROMAN.indexOf(m[2].toUpperCase());
  return{r:mod12(MAJ[deg]+(m[1]==='b'?-1:m[1]==='#'?1:0)),tri:m[3]==='°'?'dim':m[3]==='+'?'aug':up?'maj':'min'};
}
function numeral(r,q){
  const t=QT[q];let i=MAJ.indexOf(r),acc='';
  if(i<0){if(/dim|m7b5/.test(q)){acc='#';i=MAJ.indexOf(mod12(r-1));}else{acc='b';i=MAJ.indexOf(mod12(r+1));}}
  let s=ROMAN[i];if(t.m)s=s.toLowerCase();return acc+s+t.r;
}
function degChord(mode,d){
  const s=MODES[mode].scale,r=s[d],t3=mod12(s[(d+2)%7]-r),t5=mod12(s[(d+4)%7]-r);
  return{r,tri:t3===4?(t5===7?'maj':'aug'):(t5===7?'min':'dim'),deg:d};
}
const inScale=(sc,r,tri)=>TRI_IV[tri].every(x=>sc.includes(mod12(r+x)));
function srcScale(mode,c){
  for(const m of [mode,'minor','major','harmonic','dorian','mixolydian','lydian','phrygian'])if(inScale(MODES[m].scale,c.r,c.tri))return MODES[m].scale;
  return MODES[mode].scale;
}
function role(c,minor){
  const r=c.r;
  if(r===0)return'T';
  if((r===7&&c.tri==='maj')||(r===11&&c.tri==='dim'))return'D';
  if(r===7)return'Dm';
  if(r===10)return minor?'D':'S';
  if(r===5||r===2||r===1||r===6)return'S';
  return'Tm';
}

/* ---------------- Mood ---------------- */
const MOODS={
  felice:     {n:'Felice',      modes:[['major',.65],['lydian',.12],['mixolydian',.23]],tempo:1.08,dens:.65,borrow:.04,lift:.3, tension:.15,color:-.1, reg:2, arp:0},
  malinconico:{n:'Malinconico', modes:[['minor',.6],['dorian',.22],['harmonic',.18]],  tempo:.88, dens:.42,borrow:.3, lift:0,  tension:.25,color:.08, reg:-2,arp:.1},
  nostalgico: {n:'Nostalgico',  modes:[['major',.55],['mixolydian',.2],['dorian',.25]],tempo:.9,  dens:.45,borrow:.35,lift:.15,tension:.25,color:.12, reg:0, arp:.15},
  epico:      {n:'Epico',       modes:[['minor',.55],['major',.25],['dorian',.2]],      tempo:1,   dens:.55,borrow:.35,lift:.1, tension:.2, color:-.15,reg:0, arp:.25,oct:1},
  rilassato:  {n:'Rilassato',   modes:[['major',.45],['dorian',.25],['lydian',.15],['mixolydian',.15]],tempo:.84,dens:.36,borrow:.08,lift:.1,tension:.2,color:.18,reg:0,arp:.05},
  teso:       {n:'Teso / Oscuro',modes:[['minor',.35],['phrygian',.35],['harmonic',.3]],tempo:1.02,dens:.6,borrow:.3, lift:0,  tension:.6, color:.05, reg:-3,arp:.15,tense:1},
  sognante:   {n:'Sognante',    modes:[['lydian',.4],['major',.3],['dorian',.2],['minor',.1]],tempo:.8,dens:.32,borrow:.1,lift:.2,tension:.15,color:.22,reg:3,arp:.35},
  romantico:  {n:'Romantico',   modes:[['major',.5],['minor',.38],['harmonic',.12]],   tempo:.86, dens:.42,borrow:.18,lift:.1, tension:.3, color:.1,  reg:0, arp:.2}};

/* ---------------- Generi ---------------- */
// Pattern pianoforte: 16 sedicesimi. l = mano sinistra (cifra=voicing intero, a=solo basso, b=solo parte alta), r = mano destra (cifra>=7 accordo pieno, <7 parte alta)
const P=(l,r)=>({l,r});
const GENRES={
 pop:{n:'Pop',desc:'Giri ciclici, anticipi in levare, colori add9/sus, basso che scende per rivolti.',
  tempo:[88,124],swing:0,unit:8,color:.3,harmV:.35,secdom:.12,tritone:0,split:.1,pass:0,push:.5,inv:.3,susRes:.15,
  lh:'octave5',rh:'close',top:[68,79],leg:.92,arps:['pulse8','updown8','broken8'],arpOn:.3,walk:0,
  prog:{piano:0,arp:0,pad:89,bass:33,mel:0},mel:{dens:.55,sync:.55,s16:1,leg:.86},
  piano:[P('9.......7.......','8.......7.......'),P('9.....6.8.......','8..6..7.6..6..6.'),P('9..6..7.9..6..7.','8.67.6.78.67.6.7')],
  bass:['R.......5.......','R..R..R.R.....n.','R.R.R.R.R.R.R.n.'],drums:'pop',
  tm:{maj:['I V vi IV','vi IV I V','I vi IV V','I IV vi V','IV I V vi','I V IV V','I iii vi IV','IV V iii vi','vi V IV V','I IV I V'],
      min:['i bVI bIII bVII','i bVII bVI bVII','i iv bVII bIII','bVI bVII i i','i bVI iv bVII','iv bVI bVII i','i bIII bVII iv','i bVI bVII v']}},
 rock:{n:'Rock',desc:'Accordi pieni e ottave, prestiti da bVII e bVI, ottavi dritti.',
  tempo:[84,140],swing:0,unit:8,color:.12,harmV:.1,secdom:0,tritone:0,split:0,pass:0,push:.4,inv:.12,susRes:.12,
  lh:'octave',rh:'close',top:[67,78],leg:.85,arps:['pulse8','up8'],arpOn:.15,walk:0,
  prog:{piano:0,arp:0,pad:16,bass:34,mel:0},mel:{dens:.5,sync:.45,s16:0,leg:.8},
  piano:[P('9.......9.......','9.......8.......'),P('9...7...9...7...','9.7.8.7.9.7.8.7.'),P('9.7.9.7.9.7.9.7.','9.8.9.8.9.8.9.8.')],
  bass:['R.......R.......','R.R.R.R.R.R.R.R.','R.R.R.R.R.R.5.8.'],drums:'rock',
  tm:{maj:['I bVII IV I','I IV V IV','I V vi IV','I IV I V','vi IV I V','I bIII IV I','I bVII bVI bVII','IV I V V'],
      min:['i bVII bVI bVII','i bVI bIII bVII','i bIII bVII iv','i bVII iv i','i iv bVI bVII','i bIII iv bVI']}},
 jazz:{n:'Jazz',desc:'Cicli ii–V–I, voicing senza fondamentale, dominanti secondarie, sostituzioni di tritono, walking bass.',
  tempo:[96,176],swing:.62,unit:8,color:.82,harmV:1,secdom:.4,tritone:.3,split:.5,pass:.15,push:.25,inv:0,susRes:0,
  lh:'shell',rh:'rootless',top:[68,79],leg:.8,arps:['broken8','updown8'],arpOn:.1,walk:1,
  prog:{piano:0,arp:0,pad:48,bass:32,mel:0},mel:{dens:.6,sync:.6,s16:0,leg:.8},
  piano:[P('8.....7.........','8.....7.........'),P('8.....6.........','8.....7.....6...'),P('8.....7...6.....','8..6..7...6...7.')],
  bass:['R.......5.......','W','W'],drums:'jazz',
  tm:{maj:['I vi ii V','iii VI ii V','ii V I vi','ii V I IV','I IV iii VI','I VI ii V','IV iv iii VI','ii V iii VI'],
      min:['ii° V i iv','i iv ii° V','i bVI ii° V','iv bVII bIII bVI','i i ii° V','i bVII bVI V']}},
 blues:{n:'Blues',desc:'Blues a 12 battute (forma AAB), accordi di settima dominante, shuffle e boogie.',
  tempo:[70,128],swing:.7,unit:8,color:.75,harmV:1,secdom:0,tritone:.1,split:0,pass:0,push:.2,inv:0,susRes:0,blues:1,
  lh:'octave5',rh:'close',top:[67,78],leg:.85,arps:['pulse8','up8'],arpOn:.1,walk:0,
  prog:{piano:0,arp:0,pad:16,bass:33,mel:0},mel:{dens:.55,sync:.5,s16:0,leg:.82},
  piano:[P('9.......7.......','8.....6.8.....6.'),P('9...7...9...7...','8.6.7.6.8.6.7.6.'),P('a.b.a.b.a.b.a.b.','8.6.8.6.8.6.8.6.')],
  bass:['R.......5.......','R...3...5...6...','R.3.5.6.8.6.5.3.'],drums:'blues',
  modeW:{major:.5,mixolydian:3,minor:1,dorian:1.5,lydian:0,phrygian:0,harmonic:.2},tm:{maj:[],min:[]}},
 soul:{n:'Soul / R&B',desc:'Settime e none, voicing a grappolo, groove in sedicesimi e anticipi.',
  tempo:[64,100],swing:.18,unit:16,color:.62,harmV:.6,secdom:.25,tritone:.05,split:.25,pass:.1,push:.55,inv:.25,susRes:.2,
  lh:'octave5',rh:'cluster',top:[67,78],leg:.88,arps:['lazy','broken8'],arpOn:.15,walk:0,
  prog:{piano:4,arp:4,pad:89,bass:33,mel:0},mel:{dens:.55,sync:.7,s16:1,leg:.82},
  piano:[P('9.......7.......','8.....6...7.....'),P('9.....7.8.......','8..6..7.8..6.7..'),P('9..7..7.9..7..7.','8.6..7.68.6..7.6')],
  bass:['R.......5.......','R..R....R..5..a.','R.R..R.8R..5.Ra.'],drums:'soul',
  tm:{maj:['I iii vi IV','IV iii ii I','ii iii IV V','IV V iii vi','I vi ii V','IV iii vi I','ii V I vi','I IV iii IV'],
      min:['i iv bVII bIII','i bVII bVI V','iv bVII bIII bVI','i iv v bVI','i bIII iv v']}},
 lofi:{n:'Lo‑fi',desc:'maj9 e m9, swing in sedicesimi, accordi lunghi e pigri.',
  tempo:[68,88],swing:.38,unit:16,color:.82,harmV:.5,secdom:.15,tritone:.1,split:.12,pass:0,push:.3,inv:.1,susRes:0,
  lh:'shell',rh:'cluster',top:[66,77],leg:.95,arps:['lazy','updown8'],arpOn:.2,walk:0,
  prog:{piano:4,arp:4,pad:89,bass:33,mel:0},mel:{dens:.4,sync:.6,s16:1,leg:.75},
  piano:[P('8...............','7...............'),P('8.......6.......','7......6..6.....'),P('8.....6.7.......','7..6....7..6..5.')],
  bass:['R...............','R.....R.....5...','R.....R...R...a.'],drums:'lofi',
  tm:{maj:['ii V I vi','IV iii ii I','IV V iii vi','vi ii V I','I vi ii V','IV iii vi V'],
      min:['i iv bVII bIII','i bVI iv V','iv bVII bIII bVI','i v iv bVI','ii° V i bVI']}},
 classical:{n:'Classico',desc:'Funzioni tonali, V7 e dominanti secondarie, ritardi 4–3, rivolti, basso albertino.',
  tempo:[64,116],swing:0,unit:8,color:.12,harmV:1,secdom:.25,tritone:0,split:.2,pass:.15,push:0,inv:.4,susRes:.35,v7:.7,plainColors:1,
  lh:'classical',rh:'close',top:[70,81],leg:.96,arps:['alberti16','broken8','updown16'],arpOn:.55,walk:0,
  prog:{piano:0,arp:0,pad:48,bass:42,mel:0},mel:{dens:.5,sync:.1,s16:1,leg:.95},
  piano:[P('9.......7.......','8.......7.......'),P('a...b...a...b...','8...6...7...6...'),P('a.b.b.b.a.b.b.b.','8.......7...6...')],
  bass:['R...............','R.......5.......','R...5...8...5...'],drums:null,
  modeW:{major:1.4,minor:1.4,harmonic:.9,dorian:.15,lydian:.05,mixolydian:.05,phrygian:.05},
  tm:{maj:['I IV V I','I vi ii V','I V vi iii','I ii V I','I IV ii V','vi ii V I','IV V I vi'],
      min:['i iv V i','i bVI ii° V','i iv bVII bIII','i ii° V i','i bVI iv V']}},
 cinematic:{n:'Cinematico',desc:'Prestiti modali (bVI, bVII, iv), voicing aperti, ottave nel basso, arpeggi ampi.',
  tempo:[60,96],swing:0,unit:8,color:.3,harmV:.5,secdom:.05,tritone:0,split:0,pass:0,push:0,inv:.15,susRes:.2,
  lh:'open',rh:'spread',top:[72,84],leg:.97,arps:['rise16','cascade16','ballad8'],arpOn:.75,walk:0,
  prog:{piano:0,arp:46,pad:48,bass:43,mel:0},mel:{dens:.35,sync:.2,s16:0,leg:.95},
  piano:[P('9...............','8...............'),P('9.......8.......','8.......7.......'),P('9...8...9...8...','8...7...8...7...')],
  bass:['R...............','R.......R.......','R...R...R...R...'],drums:'cinematic',
  tm:{maj:['vi IV I V','I bVI bVII I','I iii IV iv','I V vi iii','IV vi V V','I bVII IV IV'],
      min:['i bVI bIII bVII','i bVI bVII i','i bVII bVI V','i iv bVI V','bVI bVII i i','i bIII bVII bVI']}},
 gospel:{n:'Gospel',desc:'Cadenze plagali (IV–iv–I), diminuiti di passaggio, sospensioni, voicing densi.',
  tempo:[62,98],swing:.3,unit:16,color:.7,harmV:.8,secdom:.4,tritone:0,split:.3,pass:.2,push:.5,inv:.4,susRes:.3,
  lh:'octave5',rh:'cluster',top:[68,79],leg:.9,arps:['ballad8','updown8'],arpOn:.15,walk:0,
  prog:{piano:0,arp:0,pad:16,bass:33,mel:0},mel:{dens:.5,sync:.6,s16:1,leg:.85},
  piano:[P('9.......7.......','8.....6.7.......'),P('9..7..7.9.......','8.6.7...8.6.7..6'),P('9.7..79.9.7..79.','8.6.7.68.6.7.67.')],
  bass:['R.......5.......','R..R....R..5..a.','R.R..R.R..R.5.a.'],drums:'gospel',
  tm:{maj:['I IV I V','IV iv I vi','ii iii IV V','I vi ii V','IV V iii vi','IV #iv° I V'],
      min:['i iv bVII bIII','iv V i i','i bVI iv V','iv bVII i bVI']}},
 bossa:{n:'Bossa Nova',desc:'Settime morbide, ii–V a catena, rootless, ritmo sincopato in sedicesimi.',
  tempo:[104,144],swing:0,unit:16,color:.8,harmV:1,secdom:.35,tritone:.25,split:.4,pass:0,push:.4,inv:.1,susRes:0,
  lh:'shell',rh:'rootless',top:[67,77],leg:.8,arps:['lazy','broken8'],arpOn:.15,walk:0,
  prog:{piano:0,arp:24,pad:48,bass:32,mel:0},mel:{dens:.55,sync:.75,s16:1,leg:.75},
  piano:[P('8.......7.......','7..6..7...6.....'),P('a.....b.a.....b.','7..6..7...6..7..'),P('a.....b.a.....b.','7..6..7...6..7.6')],
  bass:['R.......5.......','R.....5.5.....5.','R.....5.5.....5.'],drums:'bossa',
  modeW:{major:1.5,minor:1.2,dorian:.6,lydian:.4,mixolydian:.2,phrygian:0,harmonic:.4},
  tm:{maj:['I II ii V','I vi ii V','iii VI ii V','ii V I VI','I IV iii VI'],
      min:['i iv ii° V','i bVI ii° V','i iv bVII bIII','iv bVII bIII bVI']}}};

const MODAL_TM={
  dorian:['i IV i IV','i bIII IV i','i IV bVII i','i ii bIII IV','i bVII IV i','i v bVII IV'],
  phrygian:['i bII i bII','i bII bIII bII','i bVI bII i','i bvii bVI bII','i bII bvii i','i iv bII i'],
  lydian:['I II I II','I II iii II','I II vi I','I II V I','I vii II I','I II I vi'],
  mixolydian:['I bVII IV I','I bVII I bVII','I v bVII IV','I IV bVII IV','I bVII v IV','I ii bVII I'],
  harmonic:['i iv V i','i bVI V V','i iv bVI V','i ii° V i','i bVI iv V','i V i iv']};

/* ---------------- Struttura ---------------- */
const SEC_NAME={intro:'Intro',verse:'Strofa',pre:'Pre-ritornello',chorus:'Ritornello',bridge:'Bridge',outro:'Outro',loop:'Loop'};
const STRUCTS={
  loop4:{n:'Loop · 4 battute',secs:[['loop',4]]},
  loop8:{n:'Loop · 8 battute',secs:[['loop',8]]},
  vc:{n:'Strofa + Ritornello',secs:[['verse',8],['chorus',8]]},
  song:{n:'Canzone completa',secs:[['intro',4],['verse',8],['pre',4],['chorus',8],['verse',8],['pre',4],['chorus',8],['bridge',8],['chorus',8],['outro',4]]}};
const SPEC={
  loop:  {start:{T:1.2,Tm:.6,S:.4},end:{D:.8,S:.6,T:-.5,Dm:.3},energy:.62},
  verse: {start:{T:1.5,Tm:.8,S:.3},end:{D:.8,S:.7,T:-.3,Dm:.3},energy:.45},
  pre:   {start:{S:1.2,Tm:.8,T:-.5},end:{D:2,S:.4,T:-1},energy:.64},
  chorus:{start:{S:1.1,T:1,Tm:1},end:{D:.8,S:.6,T:.2},energy:.88},
  bridge:{start:{Tm:1,S:.9,D:-.5,T:-1},end:{D:2,T:-.8},energy:.55},
  intro:{energy:.3},outro:{energy:.32}};

/* ---------------- Armonia ---------------- */
const W=[[0,2,1,3,3,3,.3],[.5,0,.3,1,5,.5,1],[.3,1,0,3,.5,3,.3],[2,1.5,.5,0,3,1,.5],[5,.3,.5,1.5,0,3,.3],[.5,3,.5,3,2.5,0,.5],[3,.3,1,.5,.3,1,0]];
const cellKey=c=>c.map(x=>x.r+x.tri).join(',');

function markovCell(ctx,r){
  const md=MODES[ctx.mode],allowed=d=>{const c=degChord(ctx.mode,d);return ctx.allowDim||(c.tri!=='dim'&&c.tri!=='aug');};
  const charDeg=md.char?md.scale.indexOf(md.char.r):-1;
  const nxt=prev=>pickW(r,[0,1,2,3,4,5,6].map(d=>{let w=W[prev][d]||0;if(d===(prev+3)%7)w*=ctx.G.n==='Jazz'||ctx.G.n==='Bossa Nova'?2.4:1.2;
    if(d===charDeg)w=w*2.5+.8;if(md.minor&&d===6)w*=2;if(!allowed(d))w*=.03;return[d,w];}));
  const a=[pickW(r,[[0,.6],[5,.2],[3,.12],[1,.08]].filter(([d])=>allowed(d)))];
  while(a.length<4)a.push(nxt(a[a.length-1]));
  return a.map(d=>degChord(ctx.mode,d));
}
function templatesFor(ctx){
  const md=MODES[ctx.mode],G=ctx.G,out=[];
  if(md.modal||ctx.mode==='harmonic')MODAL_TM[ctx.mode].forEach(s=>out.push([s,1.2]));
  if(!md.modal)(md.minor?G.tm.min:G.tm.maj).forEach(s=>out.push([s,1]));
  return out;
}
function chooseCell(type,ctx,r,others){
  const spec=SPEC[type]||SPEC.loop,cands=[];
  for(const [s,b] of templatesFor(ctx)){let c=s.split(' ').map(parseRN);
    cands.push([c,b]);if(r()<.4){const k=1+Math.floor(r()*3);cands.push([c.slice(k).concat(c.slice(0,k)),b*.8]);}}
  for(let i=0;i<10;i++)cands.push([markovCell(ctx,r),0]);
  const md=MODES[ctx.mode];
  const scored=cands.map(([c,bonus])=>{
    let s=bonus;
    s+=spec.start[role(c[0],md.minor)]||0;s+=spec.end[role(c[3],md.minor)]||0;
    const dist=new Set(c.map(x=>x.r+x.tri)).size;if(dist<=2)s-=1.1;if(dist===4)s+=.3;
    for(let i=0;i<3;i++)if(c[i].r===c[i+1].r&&c[i].tri===c[i+1].tri)s-=1.4;
    for(const o of others){if(cellKey(o)===cellKey(c))s-=3;else if(o[0].r===c[0].r)s-=.35;}
    if(md.char&&c.some(x=>x.r===md.char.r&&x.tri===md.char.tri))s+=1.2;
    c.forEach(x=>{if((x.tri==='dim'||x.tri==='aug')&&!ctx.allowDim)s-=1.5;if(!inScale(md.scale,x.r,x.tri))s-=.15;});
    return[c,-s];});
  return softPick(r,scored,.55).map(x=>({...x}));
}
function cadenceVariant(cell,ctx,r,type){
  const md=MODES[ctx.mode],c=cell.map(x=>({...x}));const x=r();
  const pairs=md.minor?[['iv','V'],['bVI','bVII'],['iv','bVII'],['bVI','V']]:[['ii','V'],['IV','V'],['IV','iv'],['vi','V'],['bVI','bVII']];
  if(type==='bridge'||type==='pre'){c[3]=md.modal?{...c[3]}:parseRN(md.minor&&ctx.mode!=='harmonic'&&r()<.3?'bVII':'V');return c;}
  if(x<.4)return c;
  if(md.modal){const ch=[1,2,3,4,5,6].map(d=>degChord(ctx.mode,d)).filter(d=>d.tri!=='dim'&&d.tri!=='aug'&&!(d.r===c[2].r&&d.tri===c[2].tri));
    c[3]=pickW(r,ch.map(d=>[d,md.char&&d.r===md.char.r?2.5:1]));return c;}
  if(x<.75){const p=pick(r,pairs).map(parseRN);c[2]=p[0];c[3]=p[1];}
  else{const opts=(md.minor?['V','bVII','iv']:['V','IV','ii']).map(parseRN).filter(x=>!(x.r===c[2].r&&x.tri===c[2].tri));c[3]=pick(r,opts);}
  return c;
}
function bluesBars(ctx,n,type,r){
  const minor=MODES[ctx.mode].minor,I={r:0,tri:minor?'min':'maj'},IV={r:5,tri:minor?'min':'maj'},V={r:7,tri:'maj'};
  const quick=r()<.5,jazzy=ctx.color>.8&&!minor;
  let b12=[I,quick?IV:I,I,I,IV,IV,I,I,V,IV,I,V];
  if(jazzy){b12[8]={r:2,tri:'min'};b12[9]=V;}
  if(minor&&r()<.5){b12[8]={r:8,tri:'maj',dom:1};}
  const T={loop:n===12?b12:n===8?[I,IV,I,I,IV,IV,I,V]:[I,IV,I,V],verse:b12,chorus:b12,bridge:[IV,IV,I,I,IV,IV,V,V],
    intro:[V,IV,I,V],outro:[I,IV,I,I],pre:[IV,IV,V,V]};
  return(T[type]||b12).map(x=>({...x}));
}
function transformBars(bars,type,ctx,r,info){
  const G=ctx.G,M=ctx.M,md=MODES[ctx.mode],plain=ctx.color<.05;
  // prestiti modali e dominante armonica
  bars.forEach((c,i)=>{
    if(G.blues)return;
    if(!md.minor&&c.r===5&&c.tri==='maj'&&r()<M.borrow*.6&&i>0){c.tri='min';info.add('prestito modale (iv minore)');}
    else if(!md.minor&&c.r===9&&c.tri==='min'&&r()<M.borrow*.3){c.r=8;c.tri='maj';info.add('prestito modale (bVI)');}
    else if(md.minor&&c.r===5&&c.tri==='min'&&r()<M.lift*.5&&ctx.mode!=='phrygian'){c.tri='maj';info.add('IV maggiore (colore dorico)');}
    if(md.minor&&c.r===7&&c.tri==='min'&&(ctx.mode==='minor'||ctx.mode==='harmonic')&&r()<G.harmV){c.tri='maj';info.add('dominante maggiore (V)');}
  });
  // espansione in segmenti (accordo per battuta, eventualmente spezzato)
  const out=[],budget={},cap=(G.n==='Jazz'||G.n==='Bossa Nova')?2:1;
  const can=i=>(budget[i>>2]||0)<cap,use=i=>{budget[i>>2]=(budget[i>>2]||0)+1;};
  bars.forEach((c,i)=>{
    const nx=bars[i+1]||null,last=i===bars.length-1;
    c.beats=4;
    if(G.blues||!can(i)){out.push(c);return;}
    const tgtOk=nx&&(nx.tri==='maj'||nx.tri==='min')&&!(nx.r===c.r&&nx.tri===c.tri);
    const isD=role(c,md.minor)==='D';
    // dominante secondaria al posto dell'accordo o a metà battuta
    if(!plain&&tgtOk&&i>0&&!isD&&mod12(nx.r+7)!==c.r&&r()<G.secdom*(.5+M.tension)){
      const sec={r:mod12(nx.r+7),tri:'maj',sec:1,target:nx};
      if(r()<G.split+.15){c.beats=2;sec.beats=2;out.push(c,sec);}
      else if((G.n==='Jazz'||G.n==='Bossa Nova')&&r()<.6){const ii={r:mod12(nx.r+2),tri:nx.tri==='min'?'dim':'min',beats:2,rel:1};sec.beats=2;out.push(ii,sec);}
      else{sec.beats=4;out.push(sec);}
      info.add('dominanti secondarie');use(i);return;
    }
    // diminuito di passaggio
    if(!plain&&tgtOk&&G.pass&&r()<G.pass&&mod12(nx.r-1)!==c.r){c.beats=2;out.push(c,{r:mod12(nx.r-1),tri:'dim',beats:2,pass:1});info.add('diminuiti di passaggio');use(i);return;}
    // ritardo 4–3 sulla dominante
    if(isD&&c.beats===4&&r()<G.susRes&&!plain){c.beats=2;c.afterSus=1;out.push({r:c.r,tri:'maj',beats:2,susp:1},c);info.add('ritardo 4–3');use(i);return;}
    // turnaround a metà battuta
    if(!plain&&(last||tgtOk)&&r()<G.split*.6&&nx){const sec={r:mod12(nx.r+7),tri:'maj',sec:1,target:nx,beats:2};if(sec.r!==c.r){c.beats=2;out.push(c,sec);info.add('turnaround');use(i);return;}}
    out.push(c);
  });
  // sostituzione di tritono
  out.forEach((c,i)=>{const nx=out[i+1];if(nx&&(c.sec||role(c,md.minor)==='D')&&c.tri==='maj'&&mod12(c.r+5)===nx.r&&r()<G.tritone&&!plain){c.r=mod12(c.r+6);c.tt=1;c.ttTarget=nx;info.add('sostituzione di tritono');}});
  return out;
}
function colorize(c,nx,ctx,r){
  const G=ctx.G,sc=srcScale(ctx.mode,c),fit=iv=>sc.includes(mod12(c.r+iv));
  if(c.pass)return'dim7';
  if(c.susp)return ctx.color>.6?'9sus4':'7sus4';
  const minorTarget=c.target?c.target.tri==='min':(nx&&nx.tri==='min');
  const isDom=c.tri==='maj'&&(c.sec||c.tt||c.dom||(c.r===7&&!(MODES[ctx.mode].modal&&ctx.mode!=='mixolydian'))||G.blues||(nx&&mod12(c.r+5)===nx.r&&!inScale(MODES[ctx.mode].scale,c.r,c.tri)));
  let lvl=clamp(ctx.color+(r()-.5)*.28,0,1);const B=lvl<.16?0:lvl<.4?1:lvl<.66?2:lvl<.86?3:4;
  if(isDom){
    if(c.afterSus)return B>=3?'9':'7';
    if(G.blues)return B>=3?(r()<.5?'9':'13'):'7';
    if(B===0)return r()<(G.v7||.3)+(c.sec?.4:0)?'7':'maj';
    if(B===1)return r()<.3&&!ctx.tense?'7sus4':'7';
    if(B===2)return'7';
    if(B===3)return minorTarget?'7b9':(r()<.2?'9sus4':'9');
    return minorTarget?(r()<.5?'7b9':'7#9'):(r()<.6?'13':'9');
  }
  if(c.tri==='maj'){
    if(B===0)return'maj';
    const sev=fit(11)?'maj7':'7';
    if(B===1&&G.plainColors)return'maj';
    if(B===1){const o=[];if(fit(2))o.push('add9','add9','sus2');if(fit(9))o.push('6');if(fit(5)&&r()<.3)o.push('sus4');return o.length?pick(r,o):'maj';}
    if(B===2)return sev==='maj7'?(fit(9)&&r()<.2?'6':'maj7'):'7';
    if(sev==='7')return fit(2)?(B===4&&fit(9)?'13':'9'):'7';
    if(B===4&&fit(6))return'maj7#11';
    return fit(2)?(fit(9)&&r()<.3?'69':'maj9'):'maj7';
  }
  if(c.tri==='min'){
    if(B===0)return'min';
    const sev=fit(10)?'m7':'mmaj7';
    if(sev==='mmaj7'&&!ctx.tense)return fit(2)?'madd9':'min';
    if(B===1&&G.plainColors)return'min';
    if(B===1)return fit(2)&&r()<.6?'madd9':(fit(9)&&r()<.5?'m6':(fit(10)?'m7':'min'));
    if(B===2)return sev;
    if(B===3)return sev==='m7'&&fit(2)?'m9':sev;
    return sev==='m7'&&fit(2)&&fit(5)?'m11':(sev==='m7'&&fit(2)?'m9':sev);
  }
  if(c.tri==='dim')return B===0?'dim':(fit(10)?'m7b5':'dim7');
  return'aug';
}
function chooseInversions(chs,ctx,r){
  let prev=null;
  chs.forEach((c,i)=>{
    c.bass=c.pc;
    const iv=QT[c.q].iv;
    if(prev!=null&&ctx.G.inv>0&&!/sus|dim|aug|b5/.test(c.q)&&iv[1]&&iv[1]<=4){
      const third=mod12(c.pc+iv[1]),dRoot=Math.min(mod12(c.pc-prev),mod12(prev-c.pc)),dThird=Math.min(mod12(third-prev),mod12(prev-third));
      if(dRoot>=3&&dThird<=2&&dThird>0&&r()<ctx.G.inv*1.6)c.bass=third;
    }
    prev=c.bass;
  });
}

/* ---------------- Voicing ---------------- */
function rhPcs(c,style){
  const iv=QT[c.q].iv;
  if(iv.length<=3)return style==='spread'?[iv[0],iv[1],iv[2],iv[0]+12]:[...iv,iv[0]+12];
  let s=iv.filter(x=>x!==0);
  if(style==='close'&&iv.length===4)s=iv.slice();
  if(s.length>4)s=s.filter(x=>x!==7);
  if(s.length>4)s=s.slice(0,4);
  return s;
}
function stackVoicings(pcs,lo,hi){
  // tutte le rotazioni impilate in modo ascendente, trasposte per ottava nella finestra della nota alta
  const out=[],u=pcs.map(mod12);
  for(let k=0;k<u.length;k++){
    const a=[];let p=-1;
    for(let j=0;j<u.length;j++){const pc=u[(k+j)%u.length];let m=p<0?48+pc:p+1+mod12(pc-(p+1));a.push(m);p=m;}
    for(let o=-36;o<=36;o+=12){const t=a[a.length-1]+o;if(t>=lo&&t<=hi)out.push(a.map(x=>x+o));}
  }
  return out;
}
function voiceRH(c,prev,ctx,top){
  const style=ctx.G.rh,pcs=rhPcs(c,style).map(x=>c.pc+x);
  let cands=stackVoicings(pcs,top[0],top[1]);
  if((style==='spread'||style==='rootless'||style==='cluster')&&pcs.length>=4)
    cands=cands.concat(cands.map(v=>{const d=v.slice();d[d.length-2]-=12;return d.sort((a,b)=>a-b);}));
  let best=null,bs=1e9;
  for(const v of cands){
    let s=0;
    if(prev)s+=v.reduce((x,m)=>x+Math.min(...prev.map(p=>Math.abs(m-p))),0)+1.5*Math.abs(v[v.length-1]-prev[prev.length-1]);
    else s+=Math.abs(v[v.length-1]-(top[0]+top[1])/2);
    if(v[0]<55)s+=(55-v[0])*1.2;
    const span=v[v.length-1]-v[0];if(style==='close'&&span>14)s+=span-14;if(style==='spread'&&span<14)s+=(14-span)*.6;
    if(s<bs){bs=s;best=v;}
  }
  return best||pcs.map(x=>60+mod12(x));
}
function voiceLH(c,prevB,ctx,energy){
  const style=ctx.G.lh,root=c.pc,iv=QT[c.q].iv;
  const b=nearestPc(c.bass,prevB==null?43:prevB);const bb=clamp(b,36,50)===b?b:(b<36?b+12:b-12);
  const r0=nearestPc(root,bb);
  const sev=iv.find(x=>x===10||x===11||x===9&&c.q==='dim7');
  switch(style){
    case'octave':return[bb,bb+12];
    case'octave5':return c.bass!==root?[bb,bb+12]:(energy>.55?[bb,bb+7,bb+12]:[bb,bb+7]);
    case'shell':{const lo=clamp(r0,36,47)===r0?r0:(r0<36?r0+12:r0-12);return sev?[lo,lo+sev]:[lo,lo+7];}
    case'open':return c.bass!==root?[bb,bb+12]:[bb,bb+7,bb+12+(energy>.7?7:0)].filter((x,i,a)=>a.indexOf(x)===i);
    case'classical':return c.bass!==root?[bb,bb+12]:[bb,bb+7,bb+12];
    default:return[bb,bb+12];
  }
}
function arpPool(c,prevLow){
  const low=clamp(nearestPc(c.bass,prevLow==null?48:prevLow),43,55);
  const pcs=new Set(QT[c.q].iv.map(x=>mod12(c.pc+x)));pcs.add(mod12(c.pc));
  const out=[low];
  for(let m=low+1;m<=low+31&&out.length<9;m++){if(!pcs.has(mod12(m)))continue;const gap=m-out[out.length-1];if(m<60&&gap<5)continue;if(gap<2)continue;out.push(m);}
  return out;
}

/* ---------------- Pattern ---------------- */
const SHAPES={
  up8:n=>at8(i=>i%Math.min(n,8)),
  updown8:n=>{const m=Math.min(n,5),s=[...Array(m).keys(),...[...Array(m).keys()].slice(1,-1).reverse()];return at8(i=>s[i%s.length]);},
  broken8:n=>at8(i=>[0,2,1,3,2,4,3,5][i]),
  pulse8:n=>at8(i=>i%2===0?0:[2,3,4,3][(i>>1)%4]),
  ballad8:n=>at8(i=>[0,2,3,4,5,4,3,2][i]),
  lazy:n=>{const a=Array(16).fill(null);[[0,0],[3,2],[6,3],[10,4],[12,3]].forEach(([s,v])=>a[s]=v);return a;},
  alberti16:n=>at16(i=>i===0?0:[1,3,2,3][i%4]),
  rise16:n=>at16(i=>(i>>2)+(i&3)),
  cascade16:n=>at16(i=>n-1-((i>>2)+(i&3))),
  updown16:n=>{const m=Math.min(n,7),s=[...Array(m).keys(),...[...Array(m).keys()].slice(1,-1).reverse()];return at16(i=>s[i%s.length]);}};
function at8(f){const a=Array(16).fill(null);for(let i=0;i<8;i++)a[i*2]=f(i);return a;}
function at16(f){const a=[];for(let i=0;i<16;i++)a.push(f(i));return a;}

const DRUMS={
  pop:[{k:'9.......9.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9.....7.9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9.7.....',s:'....9.......9...',h:'7575757575757575',o:'..............7.'}],
  rock:[{k:'9.......9.......',h:'7.6.7.6.7.6.7.6.'},{k:'9.......9.9.....',s:'....9.......9...',h:'7.6.7.6.7.6.7.6.'},{k:'9.9.....9.9.....',s:'....9.......9...',o:'8.7.8.7.8.7.8.7.'}],
  jazz:[{r:'8...7.6.8...7.6.',p:'....7.......7...'},{r:'8...7.6.8...7.6.',p:'....7.......7...',k:'3...3...3...3...',g:1},{r:'9...8.7.9...8.7.',p:'....7.......7...',k:'4...4...4...4...',g:2}],
  blues:[{k:'9.......9.......',x:'....7.......7...',h:'7.5.7.5.7.5.7.5.'},{k:'9.......9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9.......',s:'....9.......9...',r:'8.6.8.6.8.6.8.6.'}],
  soul:[{k:'9.......7.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9..7....9.7.....',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9..7..7.9.7.....',s:'....9..4....9...',h:'7575757575757575',o:'......7.........'}],
  lofi:[{k:'9.......9.......',h:'6.4.6.4.6.4.6.4.'},{k:'9......79.......',s:'....9.......9...',h:'6.4.6.4.6.4.6.4.'},{k:'9......79.7.....',s:'....9.......9..4',h:'6.4.6.4.6.4.6.4.'}],
  cinematic:[{k:'9...............'},{k:'9.......8.......',t:'............7.7.'},{k:'9...8...9...8...',s:'........9.......',t:'..........7.7.77'}],
  gospel:[{k:'9.......9.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9.....7.9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9..7....',s:'....9.......9...',c:'....8.......8...',h:'7575757575757575'}],
  bossa:[{x:'9..9..9...9..9..',k:'9.....79.....79.',h:'5353535353535353'},{x:'9..9..9...9..9..',k:'9.....79.....79.',h:'6464646464646464'},{x:'9..9..9...9..9..',k:'9.....79.....79.',r:'7.5.7.5.7.5.7.5.'}]};
const DRUM_NOTE={k:36,s:38,x:37,c:39,h:42,o:46,p:44,r:51,t:45,y:49};

/* ---------------- Melodia ---------------- */
const POSW_S=[1,.04,.3,.04,.8,.04,.5,.04,.9,.04,.35,.04,.75,.04,.45,.04];
const POSW_Y=[.75,.08,.4,.25,.5,.08,.8,.12,.6,.08,.65,.25,.45,.08,.75,.18];
const ENDINGS=[[0],[0,4],[0,6],[0,2,4],[0,3,6],[2,4],[0,8],[0,4,8]];
const CADENCES=[[0],[0,4],[0,8],[2,4],[0,2]];
const FORMS={verse:[['ABAC',.45],['ABAB',.3],['AABC',.25]],chorus:[['AAAC',.3],['ABAC',.3],['AABC',.2],['ABAB',.2]],
  bridge:[['ABAB',.4],['AABC',.3],['ABAC',.3]],pre:[['AABC',.5],['ABAC',.5]],loop:[['ABAC',.35],['ABAB',.3],['AABC',.2],['AAAC',.15]]};
const CONTOUR={arch:t=>Math.sin(Math.PI*t)-.35,rise:t=>t-.45,fall:t=>.5-t,wave:t=>.6*Math.sin(2*Math.PI*t)};

function rhythmBar(r,dens,sync,s16){
  const w=POSW_S.map((x,i)=>{const v=x*(1-sync)+POSW_Y[i]*sync;return(!s16&&i%2)?0:v;});
  const n=clamp(Math.round(1.6+dens*5.2+(r()-.5)*1.6),2,s16?8:7),set=new Set();let g=0;
  while(set.size<n&&g++<200)set.add(pickW(r,w.map((v,i)=>[i,set.has(i)?0:v])));
  return[...set].sort((a,b)=>a-b);
}
function genMelody(sec,ctx,r){
  const G=ctx.G,M=ctx.M,prof=G.mel,type=sec.type,notes=[];
  const center=({verse:69,pre:71,chorus:74,bridge:72,loop:72}[type]||72)+M.reg,lo=center-10,hi=center+10;
  const sc=MODES[ctx.mode].scale,list=[];
  for(let m=lo-3;m<=hi+3;m++)if(sc.includes(mod12(m-ctx.key)))list.push(m);
  const nearIdx=p=>{let b=0;list.forEach((m,i)=>{if(Math.abs(m-p)<Math.abs(list[b]-p))b=i;});return b;};
  const amp=({verse:6,chorus:8,bridge:7,pre:7,loop:7}[type]||7);
  const dens=clamp(prof.dens+(M.dens-.5)*.4+(type==='chorus'?.05:type==='verse'?-.05:0)+(r()-.5)*.15,.12,.95);
  const form=pickW(r,FORMS[type]||FORMS.loop).split('');
  const RH={A:rhythmBar(r,dens,prof.sync,prof.s16),B:rhythmBar(r,dens*.85,prof.sync,prof.s16),E:pick(r,ENDINGS),C:pick(r,CADENCES)};
  const pickup=form[1]==='E'&&!RH.E.some(x=>x>=10)&&r()<.5?(r()<.5?[14]:[12,14]):null;
  const nPh=Math.max(1,Math.round(sec.bars/4));
  const chordAt=s=>{const t=sec.startBeat+s/4;let c=sec.chords[0];for(const x of sec.chords)if(x.start<=t+1e-6)c=x;return c;};
  const tonic=mod12(ctx.key),third=mod12(ctx.key+sc[2]),deg2=mod12(ctx.key+sc[1]),deg5=mod12(ctx.key+sc[4]),deg7=mod12(ctx.key+sc[6]);
  let st={idx:nearIdx(center),pd:0,rep:0,nct:false,p2:-1,ls:-1};
  const mem={},barMem=[];

  const choose=(o,target,cad)=>{
    const ch=chordAt(o.s),p=st.idx,cands=[];
    for(let i=Math.max(0,p-7);i<=Math.min(list.length-1,p+7);i++){
      const m=list[i],pc=mod12(m),d=i-p,ad=Math.abs(d);
      let c=[.9,0,.35,.9,1.3,1.7,2.3,2.8][ad];
      if(ad===0&&st.rep>=1)c+=1.2+st.rep;
      if(Math.abs(st.pd)>=3){if(Math.sign(d)===-Math.sign(st.pd)&&ad<=2)c-=.45;else if(Math.sign(d)===Math.sign(st.pd)&&ad>=2)c+=1.1;}
      if(st.nct&&ad>2)c+=1.4;
      if(i===st.p2&&ad>0&&ad<=2)c+=.75;
      if(o.strong&&i===st.ls)c+=.6;
      if(!o.strong&&ad>0&&ad<=2&&Math.sign(d)===Math.sign(st.pd))c-=.25;
      const ct=ch.pcs.has(pc),core=ch.core.has(pc),clash=!ct&&[...ch.core].some(x=>mod12(pc-x)===1);
      if(o.strong){c+=ct?(core?-.7:-.35):1.7;if(clash)c+=2;}else c+=ct?-.15:(ad>2?1.3:.1);
      c+=Math.abs(m-target)/2.6;
      if(m<lo||m>hi)c+=3;
      if(Math.abs(m-list[p])===6)c+=1.2;
      if(cad){c+=cad.has(pc)?-2.4:2;if(ad>2)c+=.8;}
      cands.push([i,c]);
    }
    return softPick(r,cands,.32);
  };
  const commit=(o,idx)=>{const d=idx-st.idx;st.rep=d===0?st.rep+1:0;st.pd=d;st.p2=st.idx;st.idx=idx;if(o.strong)st.ls=idx;
    const ch=chordAt(o.s);st.nct=!ch.pcs.has(mod12(list[idx]));notes.push({s:o.s,len:o.len,idx,strong:o.strong});};
  const snap=(idx,o)=>{const ch=chordAt(o.s);for(let k=0;k<4;k++)for(const j of[idx+k,idx-k])if(j>=0&&j<list.length&&ch.pcs.has(mod12(list[j])))return j;return clamp(idx,0,list.length-1);};
  const copyBar=(src,ons)=>{
    // trasposizione del motivo: sceglie lo spostamento che fa cadere più tempi forti su note dell'accordo
    let best=0,bs=-1e9;
    for(const sh of [0,-1,1,-2,2,-3,3]){let s=-Math.abs(sh)*.35;
      src.forEach((n,k)=>{const o=ons[k];if(!o)return;const j=n.idx+sh;if(j<0||j>=list.length){s-=3;return;}
        if(o.strong&&chordAt(o.s).pcs.has(mod12(list[j])))s+=1;});
      if(s>bs){bs=s;best=sh;}}
    return ons.map((o,k)=>{const n=src[k%src.length];let j=clamp(n.idx+best,0,list.length-1);if(o.strong)j=snap(j,o);return j;});
  };

  let firstPh=null;
  for(let p=0;p<nPh;p++){
    const base=p*64,lastPh=p===nPh-1,ons=[];
    form.forEach((k,bi)=>{RH[k].forEach(s=>ons.push({s:base+bi*16+s,key:k,bi,st:s}));
      if(bi===1&&pickup)pickup.forEach(s=>ons.push({s:base+16+s,key:'P',bi,st:s}));});
    ons.sort((a,b)=>a.s-b.s);
    ons.forEach((o,i)=>{
      const nx=i+1<ons.length?ons[i+1].s:base+64;let len=nx-o.s,cap=8;
      const lastInBar=i+1>=ons.length||ons[i+1].bi!==o.bi;
      if(o.key==='E'&&lastInBar)len=Math.min(len,Math.max(4,base+o.bi*16+12-o.s));
      if(o.key==='C'&&i===ons.length-1){cap=12;len=Math.min(len,Math.max(4,base+48+14-o.s));}
      o.len=Math.max(1,Math.min(len,cap));
      o.strong=o.st%8===0||o.len>=6||(o.st%4===0&&o.len>=3);
    });
    const shape=CONTOUR[nPh===1?pick(r,['arch','wave','arch']):p===0?pick(r,['arch','rise']):lastPh?pick(r,['arch','fall']):pick(r,['wave','arch'])];
    const copyHalf=p>0&&firstPh&&r()<(sec.bars===12&&p===1?.9:.65);
    const cadSet=()=>{const ch=chordAt(base+60);let s;
      if(lastPh){s=[tonic,third].filter(x=>ch.pcs.has(x));if(!s.length)s=[...ch.core].slice(0,3);}
      else{s=[deg2,deg5,deg7].filter(x=>ch.pcs.has(x));if(!s.length)s=[...ch.core];}
      return new Set(s);};
    for(let bi=0;bi<4;bi++){
      const bo=ons.filter(o=>o.bi===bi);if(!bo.length){barMem[bi]=barMem[bi]||[];continue;}
      const key=form[bi];
      let idxs=null;
      if(copyHalf&&bi<2&&firstPh[bi]&&firstPh[bi].length===bo.length)idxs=copyBar(firstPh[bi],bo);
      else if((key==='A'||key==='B')&&mem[key]&&mem[key].length===bo.filter(o=>o.key===key).length&&bo.every(o=>o.key===key)){
        idxs=copyBar(mem[key],bo);
        if(key==='A'&&form.join('')==='AAAC'&&bi===2&&r()<.6)idxs=idxs.map(j=>clamp(j+(r()<.5?1:-1),0,list.length-1)).map((j,k)=>bo[k].strong?snap(j,bo[k]):j);
      }
      const made=[];
      bo.forEach((o,k)=>{
        const t01=(o.s-base)/64,target=center+shape(t01)*amp+(type==='chorus'?1:0);
        const isLast=bi===3&&k===bo.length-1;
        let idx;
        if(idxs&&!isLast)idx=idxs[k];else idx=choose(o,target,isLast?cadSet():null);
        commit(o,idx);made.push(notes[notes.length-1]);
      });
      if((key==='A'||key==='B')&&!mem[key]&&bo.every(o=>o.key===key))mem[key]=made.map(n=>({idx:n.idx}));
      if(p===0)barMem[bi]=made.map(n=>({idx:n.idx}));
    }
    if(p===0)firstPh=barMem.slice();
  }
  // in eventi
  const peak=Math.max(...notes.map(n=>list[n.idx]));
  return notes.map(n=>({s:n.s,len:n.len,n:list[n.idx],strong:n.strong,peak:list[n.idx]===peak}));
}

/* ---------------- Generazione canzone ---------------- */
function swingDelay(G,step){
  if(!G.swing)return 0;
  if(G.unit===8)return(step%4>=2)?G.swing*.1667:0;
  return(step%2===1)?G.swing*.0833:0;
}
const COLOR_LEVELS={auto:null,triadi:0,colorati:.32,settime:.58,estesi:.92};

function generateSong(o){
  const G=GENRES[o.genre],M=MOODS[o.mood],seeds=o.seeds;
  const hr=subRng(seeds.h,'harm');
  const key=o.key==='auto'||o.key==null?Math.floor(hr()*12):+o.key;
  let mode=o.mode&&o.mode!=='auto'?o.mode:null;
  const modeR=hr();
  if(!mode){
    let mw=M.modes.map(([m,w])=>[m,w*(G.modeW&&G.modeW[m]!=null?G.modeW[m]:1)]);
    if(mw.every(x=>x[1]<=0))mw=Object.entries(G.modeW||{major:1});
    mode=pickW(()=>modeR,mw);
  }
  const color=COLOR_LEVELS[o.color]!=null?COLOR_LEVELS[o.color]:clamp(G.color+M.color,0,1);
  const autoBpm=Math.round(clamp((G.tempo[0]+(G.tempo[1]-G.tempo[0])*hr())*M.tempo,52,180));
  const ctx={G,M,mode,key,color,tense:!!M.tense,allowDim:!!(M.tense||G.n==='Jazz'||G.n==='Gospel'||G.n==='Classico')};
  const info=new Set();
  const spell=speller(key,mode);
  // struttura
  let secs=STRUCTS[o.structure||'loop4'].secs.map(x=>x.slice());
  if(G.blues&&o.structure!=='loop4'&&o.structure!=='loop8')secs=secs.filter(s=>s[0]!=='pre').map(([t,b])=>[t,(t==='verse'||t==='chorus')?12:b]);
  if(o.structure==='loop12'||(G.blues&&o.structure==='loop8'&&false))secs=[['loop',12]];
  // materiale armonico per tipo di sezione
  const types=[...new Set(secs.map(s=>s[0]))],mat={},cells={},others=[];
  const barsOf=t=>secs.find(s=>s[0]===t)[1];
  const order=['chorus','verse','loop','pre','bridge'].filter(t=>types.includes(t));
  for(const t of order){
    const r=subRng(seeds.h,'sec-'+t);
    if(G.blues){mat[t]=bluesBars(ctx,barsOf(t),t,r);continue;}
    const cell=chooseCell(t,ctx,r,others);others.push(cell);cells[t]=cell;
    const n=barsOf(t);let bars=[];
    for(let i=0;i<n;i+=4){const last=i+4>=n;bars=bars.concat(last&&n>4||(t==='pre'||t==='bridge')&&last?cadenceVariant(cell,ctx,r,t):cell.map(x=>({...x})));}
    mat[t]=bars;
  }
  if(types.includes('intro')){const r=subRng(seeds.h,'sec-intro');
    mat.intro=G.blues?bluesBars(ctx,4,'intro',r):(cells[r()<.5&&cells.chorus?'chorus':'verse']||cells.chorus||cells.verse).map(x=>({...x}));}
  if(types.includes('outro')){const r=subRng(seeds.h,'sec-outro');
    if(G.blues)mat.outro=bluesBars(ctx,4,'outro',r);
    else{const c=(cells.chorus||cells.verse).map(x=>({...x}));c[3]={r:0,tri:MODES[mode].minor?'min':'maj'};
      if(c[2].r===0)c[2]=parseRN(MODES[mode].minor?(r()<.5?'iv':'bVII'):(r()<.5?'IV':'V'));mat.outro=c;}}
  // trasformazioni (dominanti secondarie, passaggi, colori) per tipo
  const segs={};
  for(const t of types){
    const r=subRng(seeds.h,'tr-'+t);
    const s=transformBars(mat[t].map(x=>({...x})),t,ctx,r,info);
    s.forEach((c,i)=>{c.q=colorize(c,s[i+1]||s[0],ctx,r);});
    if(t==='outro'){const l=s[s.length-1];if(l.r===0)l.q=l.tri==='min'?(color>.5?'m9':'min'):(color>.5?'maj9':(color>.2?'add9':'maj'));}
    segs[t]=s;
  }
  // istanze di sezione
  const sections=[],chords=[];let bar=0;const occ={};
  secs.forEach(([t,n])=>{
    occ[t]=(occ[t]||0)+1;
    const sec={type:t,name:SEC_NAME[t]+(secs.filter(x=>x[0]===t).length>1?' '+occ[t]:''),bars:n,startBar:bar,startBeat:bar*4,energy:SPEC[t].energy+(t==='chorus'&&occ[t]>=3?.06:0),occ:occ[t],chords:[]};
    let b=sec.startBeat;
    segs[t].forEach(c=>{const x={...c,start:b,si:sections.length};b+=c.beats;sec.chords.push(x);chords.push(x);});
    sections.push(sec);bar+=n;
  });
  // dettagli accordi
  chords.forEach((c,i)=>{
    c.pc=mod12(key+c.r);
    const iv=QT[c.q].iv;c.pcs=new Set(iv.map(x=>mod12(c.pc+x)));c.core=new Set(iv.filter(x=>x<12).map(x=>mod12(c.pc+x)));
  });
  chooseInversions(chords,ctx,subRng(seeds.h,'inv'));
  chords.forEach(c=>{
    c.name=spell(c.pc)+QT[c.q].n+(c.bass!==c.pc?'/'+spell(c.bass):'');
    const tn=x=>numeral(x.r,x.tri==='min'?'min':x.tri==='dim'?'dim':'maj');
    c.rn=c.tt&&c.ttTarget?'subV'+(QT[c.q].r||'')+'/'+tn(c.ttTarget):c.sec&&c.target&&c.target.r!==0?'V'+(QT[c.q].r||'')+'/'+tn(c.target):c.sec&&c.target?'V'+(QT[c.q].r||''):numeral(c.r,c.q);
    if(c.bass!==c.pc)c.inv=1;
  });
  // voicing
  let pr=null,pl=null,pa=null;
  chords.forEach(c=>{const e=sections[c.si].energy,top=[G.top[0]+(e>.7?1:0),G.top[1]+(e>.7?2:0)];
    c.rh=voiceRH(c,pr,ctx,top);pr=c.rh;c.lh=voiceLH(c,pl,ctx,e);pl=c.lh[0];c.pool=arpPool(c,pa);pa=c.pool[0];
    c.pad=[nearestPc(c.pc,50),...stackVoicings(rhPcs(c,'rootless').map(x=>c.pc+x),70,78)[0]||c.rh];});
  // strati
  const L={mel:[],piano:[],arp:[],pad:[],bass:[],drums:[]};
  const human=o.human!==false,arng=t=>subRng(seeds.a,t),jit=(r,a)=>human?(r()-.5)*a:0;
  const arpHeavy=G.arpOn+M.arp>=.45;
  const melCache={};
  sections.forEach((sec,si)=>{
    const t=sec.type,e=sec.energy,next=sections[si+1],tierOf=x=>x<.4?0:x<.7?1:2,tier=tierOf(e);
    const chordAt=beat=>{let c=sec.chords[0];for(const x of sec.chords)if(x.start<=beat+1e-6)c=x;return c;};
    const end=sec.startBeat+sec.bars*4;
    // ---- pianoforte
    {const r=arng('piano-'+t),push=r()<G.push,hitsR=[],hitsL=[];
      for(let b=0;b<sec.bars;b++){
        const fill=b===sec.bars-1&&next&&r()<.5,p=G.piano[Math.min(2,tier+(fill?1:0))];
        const bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
        for(let s=0;s<16;s++){
          if(p.r[s]!=='.')hitsR.push({s:b*16+s,v:+p.r[s]});else if(starts.includes(s))hitsR.push({s:b*16+s,v:7});
          if(p.l[s]!=='.')hitsL.push({s:b*16+s,k:p.l[s]});else if(starts.includes(s))hitsL.push({s:b*16+s,k:'8'});
        }
      }
      // anticipi: il colpo sull'ultimo sedicesimo pari prima del cambio d'accordo suona già l'accordo successivo
      const ant=new Set();
      if(push)hitsR.forEach(h=>{if(h.s%16===14){const nb=sec.startBeat+(h.s+2)/4;const nc=sec.chords.find(c=>Math.abs(c.start-nb)<1e-6);
        if(nc){h.chord=nc;ant.add(h.s+2);}}});
      const R=hitsR.filter(h=>!ant.has(h.s));
      R.forEach((h,i)=>{
        const beat=sec.startBeat+h.s/4,c=h.chord||chordAt(beat),cEnd=c.start+c.beats;
        const nx=i+1<R.length?sec.startBeat+R[i+1].s/4:end;
        const d=Math.max(.2,(Math.min(nx,cEnd,end)-beat)*G.leg);
        const full=h.v>=7||h.s%16===0||h.chord,notes=full?c.rh:c.rh.slice(-Math.max(2,c.rh.length-2));
        const base=50+e*36,tt=beat+swingDelay(G,h.s%16)+jit(r,.03);
        notes.forEach((n,j)=>L.piano.push({t:tt+(human?j*.008:0),d,n,v:clamp(Math.round(base*(.55+h.v*.06)+(j===notes.length-1?6:0)+jit(r,10)),24,120)}));
        if(h.chord)info.add('anticipi in levare');
      });
      hitsL.forEach((h,i)=>{
        const beat=sec.startBeat+h.s/4,c=chordAt(beat),cEnd=c.start+c.beats,nx=i+1<hitsL.length?sec.startBeat+hitsL[i+1].s/4:end;
        const d=Math.max(.2,(Math.min(nx,cEnd,end)-beat)*G.leg);
        const notes=h.k==='a'?[c.lh[0]]:h.k==='b'?c.lh.slice(1):c.lh;const vv=h.k==='a'||h.k==='b'?7:+h.k;
        const tt=beat+swingDelay(G,h.s%16)+jit(r,.03);
        notes.forEach(n=>L.piano.push({t:tt,d,n,v:clamp(Math.round((46+e*34)*(.55+vv*.06)+jit(r,8)),22,115)}));
      });
    }
    // ---- arpeggio
    if(t!=='verse'||arpHeavy||t==='loop'){
      const r=arng('arp-'+t),shape=pick(r,G.arps),rate16=e>=.5;
      for(let b=0;b<sec.bars;b++)for(let s=0;s<16;s++){
        const beat=sec.startBeat+b*4+s/4,c=chordAt(beat),pat=SHAPES[shape](c.pool.length);
        if(!rate16&&s%2===1)continue;let ix=pat[s];if(ix==null)continue;
        ix=clamp(ix,0,c.pool.length-1);
        const cEnd=c.start+c.beats,d=Math.max(.15,Math.min(cEnd-beat+.05,(rate16?.6:1.2)*(shape==='lazy'?1.5:1)));
        L.arp.push({t:beat+swingDelay(G,s)+jit(r,.02),d,n:c.pool[ix],v:clamp(Math.round(44+e*30+(s%4===0?8:0)+(ix>=c.pool.length-2?4:0)+jit(r,8)),24,110)});
      }
      info.add('arpeggio: '+shape);
    }
    // ---- pad
    sec.chords.forEach(c=>L.pad.push({t:c.start,d:c.beats-.04,n:0,v:0,chord:c}));
    // ---- basso
    if(t!=='intro'){
      const r=arng('bass-'+t);let prev=L.bass.length?L.bass[L.bass.length-1].n:38;
      const near=pc=>{let m=nearestPc(pc,prev);while(m<31)m+=12;while(m>52)m-=12;return m;};
      const walking=G.walk&&tier>=1;
      if(walking){
        sec.chords.forEach((c,ci)=>{
          const nxc=sec.chords[ci+1]||(next&&next.chords[0])||sec.chords[0],n=Math.round(c.beats),line=[];
          let cur=near(c.bass);line.push(cur);
          const tgt=nearestPc(nxc.bass,cur);
          const scale=MODES[mode].scale.map(x=>mod12(key+x));
          for(let k=1;k<n-1;k++){const dir=Math.sign(tgt-cur)||(r()<.5?1:-1);
            const opts=[];for(let m=cur-5;m<=cur+5;m++){if(m===cur||m<31||m>52)continue;const pc=mod12(m);
              let w=c.pcs.has(pc)?2:scale.includes(pc)?1:0;if(Math.sign(m-cur)===dir)w*=2;if(Math.abs(m-cur)>4)w*=.4;opts.push([m,w]);}
            cur=pickW(r,opts);line.push(cur);}
          if(n>=2){let a=tgt+(r()<.65?(tgt>cur?-1:1):(r()<.5?7:-5));while(a<31)a+=12;while(a>52)a-=12;line.push(a);}
          line.forEach((m,k)=>{const beat=c.start+k;L.bass.push({t:beat+jit(r,.02),d:.92,n:m,v:clamp(Math.round(70+e*22+(k===0?8:0)+jit(r,8)),30,120)});prev=m;});
        });
        info.add('walking bass');
      }else{
        for(let b=0;b<sec.bars;b++){
          const pat=G.bass[tier],hits=[];
          const bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
          for(let s=0;s<16;s++){if(pat[s]!=='.')hits.push({s,k:pat[s]});else if(starts.includes(s))hits.push({s,k:'R'});}
          hits.forEach((h,i)=>{
            const beat=bs+h.s/4,c=chordAt(beat),ci=sec.chords.indexOf(c),nxc=sec.chords[ci+1]||(next&&next.chords[0])||sec.chords[0];
            const root=near(c.bass);let m=root;
            switch(h.k){case'5':m=root+7>52?root-5:root+7;break;case'8':m=root+12<=55?root+12:root;break;case'3':m=root+QT[c.q].iv[1];break;
              case'6':m=root+9;break;case'a':{const nb=nearestPc(nxc.bass,root);m=nb+(nb>root?-1:1);break;}
              case'n':m=s16Next(h.s)?near(nxc.bass):root;break;}
            function s16Next(s){return Math.abs(nxc.start-(bs+4))<1e-6&&s>=14;}
            const nxB=i+1<hits.length?bs+hits[i+1].s/4:bs+4,d=Math.max(.15,(Math.min(nxB,c.start+c.beats+(h.k==='n'?1:0))-beat)*.9);
            L.bass.push({t:beat+swingDelay(G,h.s)+jit(r,.02),d,n:m,v:clamp(Math.round(68+e*24+(h.k==='R'?8:0)+jit(r,8)),30,120)});
            if(M.oct&&h.k==='R'&&m-12>=28)L.bass.push({t:beat+swingDelay(G,h.s),d,n:m-12,v:clamp(Math.round(60+e*20),30,110)});
            prev=m;
          });
        }
      }
    }
    // ---- batteria
    if(G.drums&&t!=='intro'){
      const r=arng('drums-'+t),set=DRUMS[G.drums];
      for(let b=0;b<sec.bars;b++){
        const bs=sec.startBeat+b*4,p=set[tier];
        const fill=b===sec.bars-1&&next&&e>=.4&&r()<.85;
        for(const ins in p){if(ins==='g')continue;const line=p[ins];
          for(let s=0;s<16;s++){if(line[s]==='.')continue;if(fill&&s>=12&&(ins==='s'||ins==='k'||ins==='t'))continue;
            L.drums.push({t:bs+s/4+swingDelay(G,s)+jit(r,.012),d:.12,n:DRUM_NOTE[ins],v:clamp(Math.round(34+(+line[s])*9*(.8+e*.3)+jit(r,10)),15,127)});}}
        if(p.g)for(let s=2;s<16;s+=4)if(r()<.25*p.g)L.drums.push({t:bs+s/4+swingDelay(G,s),d:.1,n:38,v:Math.round(28+r()*25)});
        if(fill){const fl=pick(r,[[38,38,38,38],[50,48,47,45],[38,38,47,45],[38,0,38,38]]);
          fl.forEach((n,k)=>{if(n)L.drums.push({t:bs+3+k/4,d:.12,n,v:clamp(Math.round(70+k*8+e*20),30,127)});});}
        if(b===0&&(e>=.6||(si>0&&sections[si-1].type==='intro')))L.drums.push({t:bs,d:1,n:49,v:clamp(Math.round(80+e*30),30,127)});
      }
    }
    // ---- melodia
    if(t!=='intro'&&t!=='outro'){
      if(!melCache[t])melCache[t]=genMelody({...sec,startBeat:sec.startBeat},ctx,subRng(seeds.m,'mel-'+t));
      const r=arng('melh-'+t);
      melCache[t].forEach(x=>{const tt=sec.startBeat+x.s/4+swingDelay(G,x.s%16)+jit(r,.02)+(human?.01:0);
        L.mel.push({t:tt,d:Math.max(.2,x.len/4*G.mel.leg),n:x.n,v:clamp(Math.round(68+e*22+(x.strong?8:0)+(x.peak?6:0)+jit(r,8)),30,122)});});
    }
  });
  // il melodia memorizzata va riallineata agli accordi delle sezioni ripetute (stessa armonia → stessi pitch)
  // pad: note reali
  L.pad=L.pad.flatMap(p=>p.chord.pad.map((n,j)=>({t:p.t,d:p.d,n,v:clamp(Math.round(40+sections[p.chord.si].energy*28-(j===0?0:4)),20,100)})));
  // pulizia: note uguali sovrapposte
  const total=bar*4;
  for(const k in L){
    const a=L[k];a.forEach(e=>{e.t=Math.max(0,e.t);if(e.t+e.d>total+.5)e.d=Math.max(.06,total+.5-e.t);});
    if(k!=='drums'){const by={};a.forEach(e=>{(by[e.n]=by[e.n]||[]).push(e);});
      Object.values(by).forEach(x=>{x.sort((p,q)=>p.t-q.t);for(let i=0;i<x.length-1;i++)x[i].d=Math.min(x[i].d,x[i+1].t-x[i].t-.01);});}
    L[k]=a.filter(e=>e.d>.03).sort((p,q)=>p.t-q.t);
  }
  const bpm=o.bpm||autoBpm;
  return{opts:o,genre:o.genre,mood:o.mood,key,mode,bpm,autoBpm,color,spell,keyName:spell(key),bars:bar,beats:total,sections,chords,layers:L,
    info:[...info],swing:G.swing,prog:G.prog,defaults:{mel:1,piano:1,arp:arpHeavy?1:0,pad:(G.n==='Cinematico'||M.n==='Sognante')?1:0,bass:1,drums:0}};
}

/* ---------------- MIDI ---------------- */
const LAYERS=[
  {id:'mel',n:'Melodia',ch:0},{id:'piano',n:'Pianoforte',ch:1},{id:'arp',n:'Arpeggio',ch:2},
  {id:'pad',n:'Pad',ch:3},{id:'bass',n:'Basso',ch:4},{id:'drums',n:'Batteria',ch:9}];
function vlq(n){const b=[n&127];while(n>>=7)b.unshift((n&127)|128);return b;}
const txt=s=>Array.from(unescape(encodeURIComponent(s))).map(c=>c.charCodeAt(0));
function toMidi(song,{layers,from=0,to=song.beats,bpm=song.bpm,title='Piano Generativo'}={}){
  const PPQ=480,meta=(t,d)=>[0xFF,t,...vlq(d.length),...d];
  const chunk=(id,d)=>[...id].map(c=>c.charCodeAt(0)).concat([d.length>>>24&255,d.length>>>16&255,d.length>>>8&255,d.length&255],d);
  const body=list=>{list.sort((a,b)=>a.tick-b.tick||a.o-b.o);let p=0;const o=[];list.forEach(e=>{o.push(...vlq(e.tick-p),...e.b);p=e.tick;});o.push(0,0xFF,0x2F,0);return o;};
  const mpq=Math.round(60000000/bpm),sf=keySf(song.key,song.mode),tk=b=>Math.max(0,Math.round((b-from)*PPQ));
  const cond=[{tick:0,o:0,b:meta(3,txt(title))},{tick:0,o:0,b:meta(0x51,[mpq>>16&255,mpq>>8&255,mpq&255])},
    {tick:0,o:0,b:meta(0x58,[4,2,24,8])},{tick:0,o:0,b:meta(0x59,[sf&255,0])}];
  song.sections.forEach(s=>{if(s.startBeat>=from&&s.startBeat<to)cond.push({tick:tk(s.startBeat),o:1,b:meta(6,txt(s.name))});});
  song.chords.forEach(c=>{if(c.start>=from&&c.start<to)cond.push({tick:tk(c.start),o:2,b:meta(1,txt(c.name))});});
  const tracks=[chunk('MTrk',body(cond))];
  const progs={mel:song.prog.mel,piano:song.prog.piano,arp:song.prog.arp,pad:song.prog.pad,bass:song.prog.bass};
  LAYERS.filter(l=>layers.includes(l.id)).forEach(l=>{
    const ev=song.layers[l.id].filter(e=>e.t>=from-1e-6&&e.t<to);if(!ev.length)return;
    const list=[{tick:0,o:0,b:meta(3,txt(l.n))}];
    if(l.ch!==9)list.push({tick:0,o:0,b:[0xC0|l.ch,progs[l.id]||0]});
    ev.forEach(e=>{const a=tk(e.t),z=Math.max(a+1,tk(Math.min(e.t+e.d,to)));
      list.push({tick:a,o:1,b:[0x90|l.ch,e.n,clamp(e.v,1,127)]},{tick:z,o:0,b:[0x80|l.ch,e.n,0]});});
    tracks.push(chunk('MTrk',body(list)));
  });
  return new Uint8Array([...chunk('MThd',[0,1,0,tracks.length,PPQ>>8,PPQ&255]),...tracks.flat()]);
}
function chordChart(song){
  const L=[`${GENRES[song.genre].n} · ${MOODS[song.mood].n}`,`Tonalità: ${song.keyName} ${MODES[song.mode].n} · ${song.bpm} BPM · ${song.bars} battute`,''];
  const seen=new Set();
  song.sections.forEach(s=>{
    L.push(`[${s.name}] battute ${s.startBar+1}–${s.startBar+s.bars}`);
    if(seen.has(s.type)){L.push('  (come sopra)','');return;}seen.add(s.type);
    const rows=[];for(let b=0;b<s.bars;b++){const bs=s.startBeat+b*4,cs=s.chords.filter(c=>c.start>=bs&&c.start<bs+4);
      rows.push(cs.map(c=>c.name).join(' ')||'%');}
    for(let i=0;i<rows.length;i+=4)L.push('  | '+rows.slice(i,i+4).map(x=>x.padEnd(14)).join('| ')+'|');
    L.push('  '+s.chords.map(c=>c.rn).join(' – '),'');
  });
  return L.join('\n');
}
/* ---------------- ZIP (store) ---------------- */
const CRC=(()=>{const t=new Uint32Array(256);for(let n=0;n<256;n++){let c=n;for(let k=0;k<8;k++)c=c&1?0xEDB88320^(c>>>1):c>>>1;t[n]=c>>>0;}return t;})();
function crc32(d){let c=0xFFFFFFFF;for(let i=0;i<d.length;i++)c=CRC[(c^d[i])&255]^(c>>>8);return(c^0xFFFFFFFF)>>>0;}
function makeZip(files){
  const parts=[],central=[];let off=0;
  const u16=n=>[n&255,n>>8&255],u32=n=>[n&255,n>>>8&255,n>>>16&255,n>>>24&255];
  files.forEach(f=>{
    const name=new Uint8Array(txt(f.name)),data=f.data instanceof Uint8Array?f.data:new Uint8Array(txt(f.data)),crc=crc32(data);
    const head=[0x50,0x4b,3,4,...u16(20),...u16(0x800),...u16(0),...u16(0),...u16(0x21),...u32(crc),...u32(data.length),...u32(data.length),...u16(name.length),...u16(0)];
    parts.push(new Uint8Array(head),name,data);
    central.push(new Uint8Array([0x50,0x4b,1,2,...u16(20),...u16(20),...u16(0x800),...u16(0),...u16(0),...u16(0x21),...u32(crc),...u32(data.length),...u32(data.length),...u16(name.length),...u16(0),...u16(0),...u16(0),...u16(0),...u32(0),...u32(off)]),name);
    off+=head.length+name.length+data.length;
  });
  const csize=central.reduce((a,p)=>a+p.length,0);
  const end=new Uint8Array([0x50,0x4b,5,6,...u16(0),...u16(0),...u16(files.length),...u16(files.length),...u32(csize),...u32(off),...u16(0)]);
  const all=[...parts,...central,end],out=new Uint8Array(all.reduce((a,p)=>a+p.length,0));let p=0;all.forEach(x=>{out.set(x,p);p+=x.length;});
  return out;
}
if(typeof module!=='undefined'&&module.exports)module.exports={GENRES,MOODS,MODES,MODE_ORDER,STRUCTS,QT,KEY_NAMES,LAYERS,SEC_NAME,generateSong,toMidi,chordChart,makeZip,crc32,
  degChord,numeral,parseRN,speller,rngFrom,mod12,stackVoicings,SHAPES,DRUMS};
