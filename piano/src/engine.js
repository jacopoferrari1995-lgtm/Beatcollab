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

/* ---------------- Mood ----------------
   dens: densità ritmica · space: vuoto/pieno · stasis: armonia ferma · ostinato: ripetizione di figure · drive: spinta ribattuta */
const MOODS={
  felice:     {n:'Felice',      modes:[['major',.65],['lydian',.12],['mixolydian',.23]],tempo:1.08,dens:.65,space:.2, borrow:.04,lift:.3, tension:.15,color:-.1, reg:2, arp:0,  stasis:.1, ostinato:.15,drive:.45},
  malinconico:{n:'Malinconico', modes:[['minor',.6],['dorian',.22],['harmonic',.18]],  tempo:.88, dens:.42,space:.4, borrow:.3, lift:0,  tension:.25,color:.08, reg:-2,arp:.1, stasis:.15,ostinato:.25,drive:.15},
  nostalgico: {n:'Nostalgico',  modes:[['major',.55],['mixolydian',.2],['dorian',.25]],tempo:.9,  dens:.45,space:.35,borrow:.35,lift:.15,tension:.25,color:.12, reg:0, arp:.15,stasis:.1, ostinato:.2, drive:.2},
  epico:      {n:'Epico',       modes:[['minor',.55],['major',.25],['dorian',.2]],      tempo:1,   dens:.55,space:.2, borrow:.35,lift:.1, tension:.2, color:-.15,reg:0, arp:.25,stasis:.1, ostinato:.3, drive:.55,oct:1},
  rilassato:  {n:'Rilassato',   modes:[['major',.45],['dorian',.25],['lydian',.15],['mixolydian',.15]],tempo:.84,dens:.36,space:.45,borrow:.08,lift:.1,tension:.2,color:.18,reg:0,arp:.05,stasis:.2,ostinato:.2,drive:.1},
  teso:       {n:'Teso',        modes:[['minor',.35],['phrygian',.35],['harmonic',.3]],tempo:1.02,dens:.6, space:.35,borrow:.3, lift:0,  tension:.6, color:.05, reg:-3,arp:.15,stasis:.3, ostinato:.4, drive:.5,tense:1},
  sognante:   {n:'Sognante',    modes:[['lydian',.4],['major',.3],['dorian',.2],['minor',.1]],tempo:.8,dens:.32,space:.5,borrow:.1,lift:.2,tension:.15,color:.22,reg:3,arp:.35,stasis:.3,ostinato:.45,drive:.05},
  romantico:  {n:'Romantico',   modes:[['major',.5],['minor',.38],['harmonic',.12]],   tempo:.86, dens:.42,space:.35,borrow:.18,lift:.1, tension:.3, color:.1,  reg:0, arp:.2, stasis:.05,ostinato:.15,drive:.15},
  ipnotico:   {n:'Ipnotico',    modes:[['minor',.35],['dorian',.35],['phrygian',.15],['lydian',.15]],tempo:.95,dens:.45,space:.4,borrow:.05,lift:.1,tension:.1,color:.1,reg:0,arp:.4,stasis:.75,ostinato:.85,drive:.3},
  martellante:{n:'Martellante', modes:[['minor',.5],['phrygian',.25],['major',.25]],   tempo:1.1, dens:.85,space:.1, borrow:.15,lift:0,  tension:.3, color:-.2, reg:-1,arp:.1, stasis:.45,ostinato:.4, drive:.95},
  cupo:       {n:'Cupo',        modes:[['minor',.45],['phrygian',.3],['harmonic',.25]],tempo:.85, dens:.35,space:.55,borrow:.3, lift:0,  tension:.4, color:.05, reg:-5,arp:.2, stasis:.3, ostinato:.3, drive:.2},
  euforico:   {n:'Euforico',    modes:[['major',.5],['lydian',.25],['mixolydian',.25]],tempo:1.12,dens:.8, space:.15,borrow:.05,lift:.3, tension:.2, color:-.05,reg:4, arp:.35,stasis:.2, ostinato:.3, drive:.6},
  sensuale:   {n:'Sensuale',    modes:[['dorian',.45],['minor',.35],['major',.2]],     tempo:.9,  dens:.45,space:.45,borrow:.15,lift:.1, tension:.3, color:.3,  reg:0, arp:.15,stasis:.2, ostinato:.2, drive:.2},
  misterioso: {n:'Misterioso',  modes:[['phrygian',.3],['harmonic',.3],['lydian',.2],['dorian',.2]],tempo:.88,dens:.35,space:.6,borrow:.2,lift:0,tension:.5,color:.15,reg:0,arp:.3,stasis:.35,ostinato:.5,drive:.1,tense:1},
  aggressivo: {n:'Aggressivo',  modes:[['phrygian',.4],['minor',.4],['harmonic',.2]],  tempo:1.08,dens:.8, space:.2, borrow:.2, lift:0,  tension:.45,color:-.25,reg:-3,arp:.1, stasis:.35,ostinato:.3, drive:.85,tense:1},
  trionfale:  {n:'Trionfale',   modes:[['major',.45],['mixolydian',.3],['minor',.25]], tempo:1,   dens:.65,space:.2, borrow:.35,lift:.2, tension:.2, color:-.1, reg:2, arp:.3, stasis:.15,ostinato:.2, drive:.5,oct:1}};

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

/* ---------------- Generi urban, afro e caraibici ---------------- */
Object.assign(GENRES,{
 trap:{n:'Trap',desc:'Half-time, 808 lunghi con glide, hi-hat a raffiche, giri minori/frigi di pochi accordi, tanto vuoto.',
  tempo:[130,160],swing:.08,unit:16,color:.35,harmV:.5,secdom:0,tritone:0,split:0,pass:0,push:.2,inv:0,susRes:0,
  lh:'octave',rh:'close',top:[70,82],leg:.9,arps:['pulse8','lazy','up8'],arpOn:.3,walk:0,bass808:1,groove:1,stasis:.45,space:.6,dens:-.15,
  prog:{piano:0,arp:8,pad:89,bass:38,mel:0},mel:{dens:.45,sync:.6,s16:1,leg:.8},
  piano:[P('9...............','8...............'),P('9.......7.......','8.....6...7.....'),P('9......7..9.....','8......7..8.....')],
  bass:['R...............','R......R..R.....','R......R..R...R.'],drums:'trap',
  tex:{ostinato:3,sustain:2,broken:1.5,stabs:1,block:.4},
  modeW:{minor:2,phrygian:1.5,harmonic:1.2,dorian:.5,major:.3,lydian:.2,mixolydian:.1},
  tm:{maj:['vi IV I V','I iii vi IV','vi V IV IV','IV V vi vi'],min:['i bVI i bVI','i iv bVI V','i bII i bVII','i bVI bIII bVII','i v bVI iv','i i bVI bVII']}},
 boombap:{n:'Hip hop boom bap',desc:'Groove swingato, accordi jazz "campionati" e tagliati, basso caldo, cassa e rullante pesanti.',
  tempo:[84,96],swing:.45,unit:16,color:.7,harmV:.5,secdom:.1,tritone:.05,split:.1,pass:0,push:.3,inv:.1,susRes:0,
  lh:'shell',rh:'cluster',top:[66,77],leg:.85,arps:['lazy'],arpOn:.1,walk:0,groove:1,stasis:.3,space:.4,dens:0,
  prog:{piano:0,arp:4,pad:89,bass:33,mel:0},mel:{dens:.5,sync:.55,s16:1,leg:.8},
  piano:[P('9.......7.......','8.....6.........'),P('9......79.......','8.....7.8.......'),P('9......79.9.....','8..6..7.8..6..7.')],
  bass:['R.......R.......','R.....R...R.....','R..R..R...R..5..'],drums:'boombap',
  tex:{chops:3,block:1.5,stabs:1.5,sustain:1},
  modeW:{minor:1.5,dorian:1.5,major:.8,harmonic:.5,phrygian:.4,lydian:.2,mixolydian:.4},
  tm:{maj:['ii V I vi','IV iii ii I','I vi ii V','IV IV iii vi'],min:['i iv i iv','i bVI iv V','ii° V i i','i bVII bVI V','iv bVII i i']}},
 triphop:{n:'Trip hop',desc:'Lento e scuro: batteria spezzata, accordi tenuti, ostinati malinconici e molto spazio.',
  tempo:[70,90],swing:.25,unit:16,color:.6,harmV:.3,secdom:.05,tritone:0,split:0,pass:0,push:.1,inv:.15,susRes:0,
  lh:'open',rh:'cluster',top:[66,78],leg:.97,arps:['lazy','ballad8'],arpOn:.3,walk:0,groove:1,stasis:.35,space:.55,dens:-.15,
  prog:{piano:4,arp:0,pad:95,bass:33,mel:0},mel:{dens:.35,sync:.4,s16:0,leg:.95},
  piano:[P('9...............','7...............'),P('9.......7.......','7.......6.......'),P('9.....7.9.......','7.....6.7.......')],
  bass:['R...............','R.......R.......','R.....R.R.......'],drums:'triphop',
  tex:{sustain:3,ostinato:2,broken:1.5,block:.8},
  modeW:{minor:2,dorian:1.2,phrygian:1,harmonic:1,major:.3,lydian:.3,mixolydian:.2},
  tm:{maj:['vi IV vi IV','I iii IV iv'],min:['i bVI i bVI','i iv i iv','i bIII bVII iv','i bVII bVI bVII','i v iv i']}},
 afrobeat:{n:'Afrobeat',desc:'100–115 BPM: accordi brillanti a stacchi sincopati, percussioni 3‑3‑2, basso che dialoga.',
  tempo:[100,115],swing:.15,unit:16,color:.5,harmV:.3,secdom:0,tritone:0,split:0,pass:0,push:.5,inv:.1,susRes:0,
  lh:'octave5',rh:'close',top:[68,80],leg:.8,arps:['pulse8','broken8'],arpOn:.2,walk:0,groove:1,stasis:.3,space:.35,dens:.15,
  prog:{piano:0,arp:0,pad:89,bass:33,mel:0},mel:{dens:.6,sync:.7,s16:1,leg:.75},
  piano:[P('9.......7.......','..7..7....7..7..'),P('9.......9.......','..8..7..8.7..7..'),P('9..7....9..7....','8.78.7.78.78.7.7')],
  bass:['R.....R...R.....','R..R..R...R..5..','R..R..R.5.R..8..'],drums:'afrobeat',
  tex:{stabs:3,ostinato:2,block:1.2,broken:1},
  modeW:{major:1.5,mixolydian:1,dorian:1,minor:1,lydian:.4,phrygian:.2,harmonic:.2},
  tm:{maj:['I vi IV V','IV V iii vi','ii V I I','I IV vi V','vi IV V I'],min:['i iv bVII bIII','i bVII bVI bVII','i iv v iv','iv bVII i i']}},
 afrorage:{n:'Afro rage / phonk',desc:'135–150 BPM: batteria movimentata con terzine e poliritmi, conga e tom, 808, stacchi scuri.',
  tempo:[135,150],swing:0,unit:16,color:.3,harmV:.4,secdom:0,tritone:0,split:0,pass:0,push:.3,inv:0,susRes:0,
  lh:'octave',rh:'close',top:[70,82],leg:.75,arps:['pulse8','rise16'],arpOn:.35,walk:0,bass808:1,groove:1,stasis:.45,space:.3,dens:.2,
  prog:{piano:0,arp:0,pad:89,bass:38,mel:0},mel:{dens:.55,sync:.65,s16:1,leg:.7},
  piano:[P('9...............','8...............'),P('9.......9.......','8..7..8...7..7..'),P('9..7..7.9..7..7.','8.78.78.8.78.78.')],
  bass:['R...............','R.....R...R.....','R..R..R...R..R..'],drums:'afrorage',
  tex:{stabs:2.5,ostinato:2.5,sustain:1,block:.8},
  modeW:{minor:2,phrygian:1.5,harmonic:1,dorian:1,major:.3,lydian:.1,mixolydian:.2},
  tm:{maj:['vi IV I V','vi V IV V'],min:['i bVI bVII i','i iv bVI bVII','i bII i bVI','i bVI iv V']}},
 dancehall:{n:'Dancehall',desc:'Ritmo 3‑3‑2, stacchi in levare, basso protagonista, accordi ripetuti.',
  tempo:[90,105],swing:.1,unit:16,color:.3,harmV:.3,secdom:0,tritone:0,split:0,pass:0,push:.3,inv:.05,susRes:0,
  lh:'octave',rh:'close',top:[68,79],leg:.5,arps:['pulse8'],arpOn:.15,walk:0,groove:1,stasis:.35,space:.35,dens:.15,
  prog:{piano:0,arp:0,pad:89,bass:38,mel:0},mel:{dens:.6,sync:.7,s16:1,leg:.7},
  piano:[P('9.......9.......','...8..8.....8...'),P('9.......9.......','...8..8....8..8.'),P('9..7....9..7....','.8.8.88..8.8.88.')],
  bass:['R..R..R.........','R..R..R...R..R..','R..R..R.R..R..R.'],drums:'dancehall',
  tex:{stabs:3,skank:2,ostinato:1.5,block:.4},
  modeW:{minor:1.6,dorian:1.2,major:1,mixolydian:.8,phrygian:.4,harmonic:.3,lydian:.2},
  tm:{maj:['I IV V IV','vi IV V V','I V vi IV'],min:['i bVII bVI bVII','i iv i iv','i bVI bVII i','iv v i i']}},
 reggae:{n:'Reggae',desc:'One drop: cassa e rullante sul 3, skank del piano in levare, basso melodico in primo piano.',
  tempo:[70,90],swing:.2,unit:8,color:.25,harmV:.3,secdom:0,tritone:0,split:0,pass:0,push:0,inv:.05,susRes:0,
  lh:'octave',rh:'close',top:[66,77],leg:.4,arps:['pulse8'],arpOn:.05,walk:0,groove:1,stasis:.25,space:.3,dens:.1,
  prog:{piano:0,arp:0,pad:16,bass:33,mel:0},mel:{dens:.5,sync:.6,s16:0,leg:.8},
  piano:[P('................','....8.......8...'),P('................','..8...8...8...8.'),P('9.......9.......','..8...8...8...8.')],
  bass:['R.......5.......','R...5...8...5...','R.R.5...8.R.5...'],drums:'reggae',
  tex:{skank:5,block:.4},
  modeW:{major:1.3,minor:1.2,dorian:1,mixolydian:.8,lydian:.2,phrygian:.2,harmonic:.2},
  tm:{maj:['I IV I V','I V vi IV','IV V I I','I ii IV V'],min:['i iv i iv','i bVII bVI bVII','i bVII i bVII','i iv bVII bIII']}}});
// texture del pianoforte "vivo" per i generi esistenti
const GENRE_EXTRA={
  pop:{tex:{block:3,broken:2,ostinato:1,stabs:1,sustain:.6,octaves:.5},space:.25},
  rock:{tex:{block:4,stabs:1,octaves:1.5,broken:.5},space:.15},
  jazz:{tex:{block:4,broken:1,stabs:1},space:.3},
  blues:{tex:{block:3,stabs:1,broken:1},space:.25},
  soul:{tex:{block:3,stabs:1.5,broken:1,ostinato:.5},space:.3},
  lofi:{tex:{block:2,sustain:2,broken:1.5,ostinato:1},space:.45,stasis:.2},
  classical:{tex:{broken:3,block:2,octaves:1},space:.2},
  cinematic:{tex:{broken:2,sustain:2,ostinato:2,octaves:2},space:.3,stasis:.15},
  gospel:{tex:{block:3,stabs:1.5,broken:1},space:.2},
  bossa:{tex:{block:3,broken:1,stabs:1},space:.3}};
Object.entries(GENRE_EXTRA).forEach(([k,v])=>Object.assign(GENRES[k],v));

const MODAL_TM={
  dorian:['i IV i IV','i bIII IV i','i IV bVII i','i ii bIII IV','i bVII IV i','i v bVII IV'],
  phrygian:['i bII i bII','i bII bIII bII','i bVI bII i','i bvii bVI bII','i bII bvii i','i iv bII i'],
  lydian:['I II I II','I II iii II','I II vi I','I II V I','I vii II I','I II I vi'],
  mixolydian:['I bVII IV I','I bVII I bVII','I v bVII IV','I IV bVII IV','I bVII v IV','I ii bVII I'],
  harmonic:['i iv V i','i bVI V V','i iv bVI V','i ii° V i','i bVI iv V','i V i iv']};

/* ---------------- Struttura ---------------- */
const SEC_NAME={intro:'Intro',verse:'Strofa',pre:'Pre-ritornello',chorus:'Ritornello',bridge:'Bridge',special:'Special',outro:'Outro',loop:'Loop'};
const SEC_BARS={intro:4,verse:8,pre:4,chorus:8,bridge:8,special:8,outro:4,loop:8};
const TYPE_ORDER=['chorus','verse','loop','pre','bridge','special'];
const STRUCTS={
  loop4:{n:'Loop · 4 battute',secs:[['loop',4]]},
  loop8:{n:'Loop · 8 battute',secs:[['loop',8]]},
  vc:{n:'Strofa + Ritornello',secs:[['verse',8],['chorus',8]]},
  vb:{n:'Strofa + Bridge',secs:[['verse',8],['bridge',8]]},
  bc:{n:'Bridge + Ritornello',secs:[['bridge',8],['chorus',8]]},
  song:{n:'Canzone completa',secs:[['intro',4],['verse',8],['pre',4],['chorus',8],['verse',8],['pre',4],['chorus',8],['bridge',8],['chorus',8],['outro',4]]},
  intro:{n:'Solo intro',secs:[['intro',8]]},
  special:{n:'Solo special',secs:[['special',8]]},
  outro:{n:'Solo outro',secs:[['outro',8]]}};
const SPEC={
  loop:  {start:{T:1.2,Tm:.6,S:.4},end:{D:.8,S:.6,T:-.5,Dm:.3},energy:.62},
  verse: {start:{T:1.5,Tm:.8,S:.3},end:{D:.8,S:.7,T:-.3,Dm:.3},energy:.45},
  pre:   {start:{S:1.2,Tm:.8,T:-.5},end:{D:2,S:.4,T:-1},energy:.64},
  chorus:{start:{S:1.1,T:1,Tm:1},end:{D:.8,S:.6,T:.2},energy:.88},
  bridge:{start:{Tm:1,S:.9,D:-.5,T:-1},end:{D:2,T:-.8},energy:.55},
  special:{start:{Tm:1,S:1,T:-.6},end:{D:1.2,S:.6,T:-.4},energy:.5},
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

/* ---------------- Batteria ----------------
   ogni riga è una griglia: 16 caratteri = sedicesimi, 12 = terzine di ottavo, 24 = terzine di sedicesimo.
   cifra = colpo (forza), r = doppio trentaduesimo, z = rullata in terzina */
const DRUMS={
  pop:[{k:'9.......9.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9.....7.9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9.7.....',s:'....9.......9...',h:'7575757575757575',o:'..............7.'}],
  rock:[{k:'9.......9.......',h:'7.6.7.6.7.6.7.6.'},{k:'9.......9.9.....',s:'....9.......9...',h:'7.6.7.6.7.6.7.6.'},{k:'9.9.....9.9.....',s:'....9.......9...',o:'8.7.8.7.8.7.8.7.'}],
  jazz:[{r:'8...7.6.8...7.6.',p:'....7.......7...'},{r:'8...7.6.8...7.6.',p:'....7.......7...',k:'3...3...3...3...',g:1},{r:'9...8.7.9...8.7.',p:'....7.......7...',k:'4...4...4...4...',g:2}],
  blues:[{k:'9.......9.......',x:'....7.......7...',h:'7.5.7.5.7.5.7.5.'},{k:'9.......9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9.......',s:'....9.......9...',r:'8.6.8.6.8.6.8.6.'}],
  soul:[{k:'9.......7.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9..7....9.7.....',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9..7..7.9.7.....',s:'....9..4....9...',h:'7575757575757575',o:'......7.........'}],
  lofi:[{k:'9.......9.......',h:'6.4.6.4.6.4.6.4.'},{k:'9......79.......',s:'....9.......9...',h:'6.4.6.4.6.4.6.4.'},{k:'9......79.7.....',s:'....9.......9..4',h:'6.4.6.4.6.4.6.4.'}],
  cinematic:[{k:'9...............'},{k:'9.......8.......',t:'............7.7.'},{k:'9...8...9...8...',s:'........9.......',t:'..........7.7.77'}],
  gospel:[{k:'9.......9.......',x:'....7.......7...',h:'6.4.6.4.6.4.6.4.'},{k:'9.....7.9.......',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9.....7.9..7....',s:'....9.......9...',c:'....8.......8...',h:'7575757575757575'}],
  bossa:[{x:'9..9..9...9..9..',k:'9.....79.....79.',h:'5353535353535353'},{x:'9..9..9...9..9..',k:'9.....79.....79.',h:'6464646464646464'},{x:'9..9..9...9..9..',k:'9.....79.....79.',r:'7.5.7.5.7.5.7.5.'}],
  trap:[{k:'9.........9.....',h:'7.6.7.6.7.6.7.6.'},{k:'9......9..9.....',c:'........9.......',h:'7.6.7.6.7.6.7r6.'},{k:'9......9..9..9..',c:'........9.......',h:'7r6.7.6.z.6.7r7.',o:'..............6.'}],
  boombap:[{k:'9.......9.......',s:'....9.......9...',h:'6.4.6.4.6.4.6.4.'},{k:'9......79.9.....',s:'....9.......9...',h:'7.5.7.5.7.5.7.5.'},{k:'9......79.9..7..',s:'....9..4....9.4.',h:'7.5.7.5.7.5.7.5.',o:'..............6.'}],
  triphop:[{k:'9.........9.....',h:'5.4.5.4.5.4.5.4.'},{k:'9.....9...9.....',s:'....9.......9...',h:'6.4.6.4.6.4.6.4.'},{k:'9.....9...9.....',s:'....9..5....9...',h:'6.4.6.4.6.4.6.4.',o:'..............6.'}],
  afrobeat:[{k:'9.......9.......',x:'...7..7....7..7.',h:'6.4.6.4.6.4.6.4.'},{k:'9..6....9..6....',x:'...7..7....7..7.',h:'6464646464646464',q:'..6...6...6.6...'},
    {k:'9..6....9..6....',x:'...7..7....7..7.',c:'....8.......8...',h:'6464646464646464',q:'..6...6...6.6...',u:'......7.......7.',w:'5555555555555555'}],
  afrorage:[{k:'9.......9.......',w:'6.4.6.4.6.4.6.4.'},{k:'9.....9...9.....',c:'....9.......9...',q:'7..6..7..6..',u:'..7.....7...'},
    {k:'9.....9.9.9.....',c:'....9.......9...',q:'7.67.67.67.6',u:'..7..7..7..7',t:'.........7.7',h:'7r7.7.7z..7.7r7.'}],
  dancehall:[{k:'9.......9.......',x:'...7..7....7..7.',h:'6.4.6.4.6.4.6.4.'},{k:'9.......9.......',s:'...8..8....8..8.',h:'6.4.6.4.6.4.6.4.'},{k:'9...9...9...9...',s:'...8..8....8..8.',h:'6464646464646464',v:'..6...6...6...6.'}],
  reggae:[{k:'........9.......',x:'........8.......',h:'..6...6...6...6.'},{k:'........9.......',s:'........8.......',h:'6.5.6.5.6.5.6.5.'},{k:'9...9...9...9...',x:'........8.......',h:'6.5.6.5.6.5.6.5.',o:'..6...6...6...6.'}]};
const DRUM_NOTE={k:36,s:38,x:37,c:39,h:42,o:46,p:44,r:51,t:45,m:47,y:49,q:63,u:64,b:60,w:70,v:56};
// 10 pad: la mappa della batteria. Il suono di alcuni pad dipende dal genere (snare/clap, hat/ride, tom/conga)
const PADS=[{id:'kick',n:'Kick'},{id:'snare',n:'Snare'},{id:'rim',n:'Rimshot'},{id:'hat',n:'Hi-hat'},{id:'open',n:'Hat aperto'},
  {id:'shaker',n:'Shaker'},{id:'perc1',n:'Tom 1'},{id:'perc2',n:'Tom 2'},{id:'crash',n:'Crash'},{id:'fx',n:'FX'}];
const LINE_PAD={k:0,s:1,c:1,x:2,h:3,p:3,r:3,o:4,w:5,v:5,q:6,b:6,m:6,u:7,t:7,y:8};
function drumKit(G){
  const clap=['trap','afrobeat','afrorage','dancehall'].includes(G.drums),ride=G.drums==='jazz',conga=['afrobeat','afrorage','bossa','dancehall'].includes(G.drums);
  return{notes:[36,clap?39:38,37,ride?51:42,46,70,conga?63:48,conga?64:45,49,55],
    names:['Kick',clap?'Clap':'Snare','Rimshot',ride?'Ride':'Hi-hat','Hat aperto','Shaker',conga?'Conga alta':'Tom 1',conga?'Conga bassa':'Tom 2','Crash','FX']};
}

/* ---------------- Melodia ---------------- */
const POSW_S=[1,.04,.3,.04,.8,.04,.5,.04,.9,.04,.35,.04,.75,.04,.45,.04];
const POSW_Y=[.75,.08,.4,.25,.5,.08,.8,.12,.6,.08,.65,.25,.45,.08,.75,.18];
const ENDINGS=[[0],[0,4],[0,6],[0,2,4],[0,3,6],[2,4],[0,8],[0,4,8]];
const CADENCES=[[0],[0,4],[0,8],[2,4],[0,2]];
const FORMS={verse:[['ABAC',.45],['ABAB',.3],['AABC',.25]],chorus:[['AAAC',.3],['ABAC',.3],['AABC',.2],['ABAB',.2]],
  bridge:[['ABAB',.4],['AABC',.3],['ABAC',.3]],special:[['ABAB',.4],['AABC',.3],['ABAC',.3]],pre:[['AABC',.5],['ABAC',.5]],loop:[['ABAC',.35],['ABAB',.3],['AABC',.2],['AAAC',.15]]};
const CONTOUR={arch:t=>Math.sin(Math.PI*t)-.35,rise:t=>t-.45,fall:t=>.5-t,wave:t=>.6*Math.sin(2*Math.PI*t)};

function rhythmBar(r,dens,sync,s16){
  const w=POSW_S.map((x,i)=>{const v=x*(1-sync)+POSW_Y[i]*sync;return(!s16&&i%2)?0:v;});
  const n=clamp(Math.round(1.6+dens*5.2+(r()-.5)*1.6),2,s16?8:7),set=new Set();let g=0;
  while(set.size<n&&g++<200)set.add(pickW(r,w.map((v,i)=>[i,set.has(i)?0:v])));
  return[...set].sort((a,b)=>a-b);
}
/* ---------------- Melodia (v2: motivo e sviluppo) ----------------
   1) nasce un motivo di una battuta: un ritmo tipico del genere + un profilo melodico
   2) il motivo viene ripetuto, portato in progressione sugli accordi, variato
   3) frase = proposta · ripresa · sviluppo · chiusa (cadenza), con respiro alla fine
   4) nel ritornello: hook corto e ripetuto, registro più alto, culmine nella seconda frase */
const MEL_RH={
  straight:[[[0,4],[4,4],[8,8]],[[0,2],[2,2],[4,4],[8,4]],[[0,6],[6,2],[8,4],[12,4]],[[0,4],[4,2],[6,2],[8,8]],[[2,2],[4,4],[8,2],[10,6]],[[0,3],[3,3],[6,2],[8,8]],[[0,4],[4,4],[8,4],[12,4]]],
  sync:[[[0,3],[3,3],[6,4],[10,6]],[[2,2],[4,2],[6,4],[10,2],[12,4]],[[0,2],[3,3],[6,2],[8,2],[10,6]],[[0,3],[3,5],[10,2],[12,4]],[[1,2],[3,3],[6,2],[8,3],[11,5]],[[0,3],[3,3],[6,2],[8,3],[11,3],[14,2]]],
  flow16:[[[0,2],[2,2],[4,1],[5,1],[6,2],[8,2],[10,2],[12,4]],[[0,1],[1,1],[2,2],[4,2],[6,1],[7,3],[10,2],[12,2],[14,2]],[[2,2],[4,2],[6,2],[8,1],[9,1],[10,2],[12,4]],[[0,3],[3,1],[4,2],[6,2],[8,3],[11,1],[12,4]]],
  swing:[[[0,4],[6,2],[8,4],[14,2]],[[2,2],[4,2],[6,2],[8,6],[14,2]],[[0,6],[6,2],[8,2],[10,6]],[[0,2],[2,2],[4,2],[6,2],[8,8]]],
  offbeat:[[[2,2],[6,2],[10,2],[12,4]],[[2,4],[6,2],[8,6]],[[0,2],[2,2],[6,4],[10,2],[12,4]]],
  ballad:[[[0,8],[8,8]],[[0,6],[6,2],[8,8]],[[0,12],[12,4]],[[0,4],[4,12]],[[0,4],[4,4],[8,8]]]};
const MEL_HOOK=[[[0,2],[2,2],[4,4]],[[0,3],[3,3],[6,2]],[[2,2],[4,2],[6,2]],[[0,2],[3,3],[6,4]],[[0,4],[6,2],[8,4]],[[0,2],[2,2],[4,2],[6,4]]];
const MEL_CAD=[[[0,10]],[[0,4],[4,8]],[[0,2],[2,2],[4,8]],[[2,2],[4,8]],[[0,3],[3,9]]];
const MEL_CONT=[[1,1,1,-2],[1,1,-1,-1],[-1,-1,-1,2],[3,-1,-1,-1],[1,-1,0,2],[0,0,1,-1],[2,2,-1,-2],[1,2,-1,1],[-2,1,1,1],[0,1,1,-3],[2,-1,-1,1],[-1,-1,2,-1]];
const HOOK_CONT=[[0,1,-1],[0,1,1],[0,-1,-1],[2,-1,0],[0,-1,2],[1,-1,1],[0,2,-1],[-1,0,2]];
const MEL_FAM={pop:{straight:1,sync:1.2},rock:{straight:1.5,sync:.6},jazz:{swing:2,sync:.5},blues:{swing:1.5,straight:.5},soul:{sync:2,straight:.5},
  lofi:{sync:1,ballad:.8},classical:{straight:2,ballad:.6},cinematic:{ballad:2,straight:1},gospel:{sync:1,straight:1},bossa:{sync:1.5,swing:.3},
  trap:{flow16:2,sync:.6},boombap:{flow16:1.5,sync:1},triphop:{ballad:1.5,sync:.8},afrobeat:{sync:2,offbeat:.5},afrorage:{flow16:1.2,sync:1.2},
  dancehall:{sync:1.5,offbeat:1},reggae:{offbeat:2,sync:.6}};

function genMelody(sec,ctx,r,shared){
  const G=ctx.G,M=ctx.M,type=sec.type,gid=Object.keys(GENRES).find(k=>GENRES[k]===G);
  const center=({verse:68,pre:70,chorus:73,bridge:71,special:70,loop:71}[type]||71)+M.reg,lo=center-10,hi=center+11;
  const sc=MODES[ctx.mode].scale,list=[];for(let m=lo-3;m<=hi+3;m++)if(sc.includes(mod12(m-ctx.key)))list.push(m);
  const L=list.length,pcOf=i=>mod12(list[clamp(i,0,L-1)]);
  const chordAt=s=>{const t=sec.startBeat+s/4;let c=sec.chords[0];for(const x of sec.chords)if(x.start<=t+1e-6)c=x;return c;};
  const nearIdx=p=>{let b=0;list.forEach((m,i)=>{if(Math.abs(m-p)<Math.abs(list[b]-p))b=i;});return b;};
  const ctIdx=(p,ch)=>{let b=-1;list.forEach((m,i)=>{if(ch.pcs.has(mod12(m))&&(b<0||Math.abs(m-p)<Math.abs(list[b]-p)))b=i;});return b<0?nearIdx(p):b;};
  const snapDir=(i,ch,dir)=>{for(let k=0;k<5;k++)for(const j of [i+dir*k,i-dir*k])if(j>=0&&j<L&&ch.pcs.has(pcOf(j)))return j;return clamp(i,0,L-1);};
  // ritmo: famiglia del genere, densità dal mood
  const fam=Object.entries(MEL_FAM[gid]||{straight:1}).map(([k,w])=>[k,w*(k==='ballad'?1+M.space*1.5:1)*(k==='flow16'||k==='sync'?1+M.drive*.5:1)]);
  const famK=pickW(r,fam),pool=MEL_RH[famK];
  const want=clamp(2+(G.mel.dens+(M.dens-.5)*.5+(type==='chorus'?.05:type==='verse'?-.05:0))*6,2,8);
  const byCount=()=>pickW(r,pool.map(p=>[p,Math.exp(-Math.abs(p.length-want)/1.2)]));
  const dna=shared&&shared[0]&&shared[0].length>1?shared[0].map((st,k,a)=>[st,Math.max(1,Math.min(6,(a[k+1]!=null?a[k+1]:16)-st))]):null;
  const R1=byCount(),R2=dna&&r()<.6?dna:byCount(),H=pick(r,MEL_HOOK);
  const C1=pick(r,MEL_CONT),C2=C1.map(x=>-x).reverse(),HC=pick(r,HOOK_CONT);
  const hook=type==='chorus'||(type==='loop'&&r()<.5);
  const tonic=mod12(ctx.key),third=mod12(ctx.key+sc[2]),deg2=mod12(ctx.key+sc[1]),deg5=mod12(ctx.key+sc[4]),deg7=mod12(ctx.key+sc[6]);
  const notes=[];let prev=nearIdx(center),prevDir=1;
  // realizza un ritmo con un profilo, partendo da una nota dell'accordo vicina al bersaglio
  function realize(rh,cont,bar,target,cad){
    const base=bar*16;let i=ctIdx(target,chordAt(base+rh[0][0]));
    const out=[];
    rh.forEach(([st,len],k)=>{
      const s=base+st,ch=chordAt(s),strong=st%8===0||len>=6||(st%4===0&&len>=3);
      if(k>0)i+=cont[(k-1)%cont.length];
      i=clamp(i,0,L-1);
      const d=i-prev;if(d)prevDir=Math.sign(d);
      if(strong&&!ch.pcs.has(pcOf(i)))i=snapDir(i,ch,prevDir);
      else if(!strong&&!ch.pcs.has(pcOf(i))&&Math.abs(i-prev)>1)i=snapDir(i,ch,prevDir);
      if(list[i]>hi)i=snapDir(i-2,ch,-1);if(list[i]<lo)i=snapDir(i+2,ch,1);
      out.push({s,len,idx:i,strong});prev=i;
    });
    if(cad){const lst=out[out.length-1],c=chordAt(lst.s),pp=out.length>1?out[out.length-2].idx:prev;let best=lst.idx,bs=1e9;
      for(let j=Math.max(0,pp-4);j<=Math.min(L-1,pp+4);j++){const pc=pcOf(j);if(!cad.has(pc)||!c.pcs.has(pc))continue;const sc2=Math.abs(j-pp)+(j===pp?1.5:0);if(sc2<bs){bs=sc2;best=j;}}
      lst.idx=best;prev=best;}
    return out;
  }
  const nPh=Math.max(1,Math.round(sec.bars/4)),first=[];let startT=center+(hook?2:-1);
  for(let p=0;p<nPh;p++){
    const last=p===nPh-1,b0=p*4,climax=hook&&p===1;
    const cadSet=()=>{const c=chordAt((b0+3)*16+4);let s=last?[tonic,third]:[deg2,deg5,deg7];s=s.filter(x=>c.pcs.has(x));return new Set(s.length?s:[...c.core].slice(0,last?2:3));};
    let bars;
    if(p%2===1&&first.length&&!(sec.bars===12&&p===2)){
      // seconda frase: riprende l'inizio della prima, poi cambia strada
      bars=[realize(first[0].rh,first[0].c,b0,first[0].t,null),realize(first[1].rh,first[1].c,b0+1,first[1].t,null)];
    }else if(p===1&&sec.bars===12){bars=[realize(first[0].rh,first[0].c,b0,first[0].t+2,null),realize(first[1].rh,first[1].c,b0+1,first[1].t+2,null)];}
    else{
      const rA=hook?H:R1,cA=hook?HC:C1,seq=hook?0:pick(r,[1,-1,1,2]);
      const t0=startT,t1=startT+seq*2;
      bars=[realize(rA,cA,b0,t0,null),realize(rA,r()<.3&&!hook?cA.map((x,k)=>k===cA.length-1?-x:x):cA,b0+1,t1,null)];
      if(p===0)first.push({rh:rA,c:cA,t:t0},{rh:rA,c:cA,t:t1});
    }
    // sviluppo
    const devT=list[prev]+(climax?5:hook?2:pick(r,[-2,2,3]));
    bars.push(realize(hook&&r()<.6?H:R2,hook?HC.map(x=>x+(r()<.5?1:0)):C2,b0+2,devT,null));
    // chiusa con cadenza e respiro
    const cr=pick(r,MEL_CAD);bars.push(realize(cr,[-1,-1,1],b0+3,list[prev]-1,cadSet()));
    // levare verso la frase successiva
    if(!last&&r()<.4){const nx=Math.min(L-1,prev+1);bars.push([{s:(b0+3)*16+14,len:2,idx:nx,strong:false}]);prev=nx;}
    bars.forEach(b=>b.forEach(n=>notes.push(n)));
    startT=list[bars[bars.length-1][0].idx]+(hook?0:1);
  }
  const peak=Math.max(...notes.map(n=>list[n.idx]));
  return notes.filter(n=>n.s<sec.bars*16).map(n=>({s:n.s,len:n.len,n:list[n.idx],strong:n.strong,peak:list[n.idx]===peak}));
}

/* ---------------- Generazione canzone ---------------- */
function swingDelay(G,step){
  if(!G.swing)return 0;
  if(G.unit===8)return(step%4>=2)?G.swing*.1667:0;
  return(step%2===1)?G.swing*.0833:0;
}
const COLOR_LEVELS={auto:null,triadi:0,colorati:.32,settime:.58,estesi:.92};
const TEX_NAME={block:'accordi ritmici',broken:'accordi spezzati (arpeggia il piano)',ostinato:'ostinato',stabs:'stacchi sincopati',
  sustain:'accordi tenuti con respiro',octaves:'ottave melodiche',skank:'skank in levare',chops:'accordi "campionati" tagliati'};
const qTri=q=>/dim|b5/.test(q)?'dim':q==='aug'?'aug':QT[q].m?'min':'maj';

function applyEdits(sections,edits){
  const op=(s,ci,e)=>{
    const c=s.chords[ci];
    switch(e.op){
      case'chord':if(c&&QT[e.q])Object.assign(c,{r:mod12(e.r),q:e.q,tri:qTri(e.q),sec:null,target:null,tt:null,ttTarget:null,user:1});break;
      case'bass':if(c)c.bassIv=e.iv;break;
      case'strum':if(c)c.strum=e.dir?{dir:e.dir,spd:e.spd||.04}:null;break;
      case'vel':if(c)c.vel=e.v;break;
      case'len':{const tot=s.chords.reduce((a,x)=>a+x.beats,0);
        if(e.beats.length===s.chords.length&&Math.abs(e.beats.reduce((a,x)=>a+x,0)-tot)<1e-6&&e.beats.every(b=>b>=.25))e.beats.forEach((b,i)=>s.chords[i].beats=b);break;}
      case'split':if(c&&c.beats>=.5){const h=Math.round(c.beats/2*4)/4;s.chords.splice(ci+1,0,{...c,beats:c.beats-h});c.beats=h;}break;
      case'merge':if(c&&ci>0){s.chords[ci-1].beats+=c.beats;s.chords.splice(ci,1);}break;
    }
  };
  for(const e of edits||[]){
    if(e.sel==='same'){sections.forEach(s=>s.chords.forEach((c,ci)=>{if(c.r===e.match.r&&c.q===e.match.q)op(s,ci,e);}));}
    else if(e.sel==='rep'){sections.forEach(s=>{if(s.type===e.type)op(s,e.ci,e);});}
    else{const s=sections.find(x=>x.type===e.type&&x.occ===e.occ);if(s)op(s,e.ci,e);}
  }
  sections.forEach(s=>{let b=s.startBeat;s.chords.forEach(c=>{c.start=b;b+=c.beats;});});
}

const BUSY={sustain:0,broken:1,ostinato:1.4,chops:1.5,octaves:2,block:2,skank:2,stabs:2.4};
const OST_RH=[['9.......7.......','9.....7.....7...'],['9..7..7.9..7..7.','9.7.9...9.7.9...','9..7..9...7..7..'],['9.7.9.7.9.7.9.7.','97.797.797.797.7']];
const STAB_RH=[['..9..9....9.....','...9..9....9..9.'],['..9..9..9.9..9..','9..9..9...9..9..'],['9.99.9.99.99.9.9','9.9.99.9.9.99.9.']];
// scelte comuni a tutto il brano: le sezioni variano la stessa idea invece di inventarne una nuova
function songStyle(G,M,rp,ra){
  const w=Object.assign({},G.tex||{block:1});const mul=(k,x)=>{if(w[k]!=null)w[k]*=x;};
  w.ostinato=(w.ostinato||0)*(1+3*M.ostinato)+M.ostinato*1.2;
  mul('stabs',1+1.5*M.drive);mul('block',1+.5*M.drive);
  w.sustain=((w.sustain||0)*(1+2*M.space)+.5*M.space)*(1-M.drive*.8);
  mul('broken',1+2*M.arp);if(w.octaves)w.octaves*=M.oct?2:.8;
  const P=pickW(rp,Object.entries(w));
  const w2=Object.entries(w).filter(([k])=>k!==P).map(([k,v])=>[k,v*(Math.abs(BUSY[k]-BUSY[P])>=.5?1.6:.6)]);
  const S=w2.length&&w2.some(x=>x[1]>0)?pickW(rp,w2):P;
  const calm=BUSY[P]<=BUSY[S]?P:S,busy=calm===P?S:P;
  return{piano:{calm,busy,ost:OST_RH.map(a=>pick(rp,a)),contour:pick(rp,[[0,1,2,1],[0,2,1,3],[2,1,0,1],[0,1,0,2],[3,2,1,0],[0,0,2,1],[0,2,1]]),
      dyad:rp()<.5,stab:STAB_RH.map(a=>pick(rp,a)),chopsA:pick(rp,['9......7..9.....','9.....7...9..7..']),chopsB:pick(rp,['9.....7...7..9..','9......7..9...7.']),
      broken:pick(rp,[...G.arps.filter(a=>SHAPES[a]),'updown8','broken8']),push:rp()<G.push},
    arp:pick(ra,G.arps.filter(a=>SHAPES[a]))};
}
function planSection(sec,ctx,r,o){
  // estrazioni a indice fisso: cambiare una traccia non sposta le scelte delle altre
  const G=ctx.G,M=ctx.M,t=sec.type,e=sec.energy,sp=ctx.sty.piano,R=Array.from({length:10},()=>r());
  const busyP={chorus:.8,pre:.6,loop:.55,bridge:.3,special:.25,verse:.25,intro:.15,outro:.15}[t];
  const tex=R[0]<(busyP==null?.4:busyP)?sp.busy:sp.calm;
  const space=clamp((G.space||.3)*.6+M.space*.6,0,1);
  const dens=clamp(.22+e*.55+(M.dens-.5)*.45+(G.dens||0)-space*.25,.08,1);
  const shape=t==='pre'?[.6,.75,.9,1]:R[1]<space?[1,.45,1,.55]:R[2]<.3?[1,.85,1,.4]:[1,1,1,1];
  const p={tex,dens,shape,space};
  p.mel=t!=='intro'&&t!=='outro'&&!(t==='special'&&R[3]<.5);
  const ap=clamp(G.arpOn+M.arp+({intro:.1,verse:-.2,pre:.25,chorus:.3,bridge:.2,special:.2,outro:.05,loop:.15}[t]||0),0,.95);
  p.arp=R[4]<ap&&(o.pianoStyle==='classic'||!['broken','ostinato'].includes(tex));
  if(p.arp&&o.pianoStyle!=='classic')p.dens*=.85;
  p.bass=t==='intro'?R[5]<(G.groove?.35:.2):t==='special'?R[5]<.5:true;
  const tier=e<.4?0:e<.7?1:2;
  p.drums=!G.drums?null:t==='intro'?(G.groove&&R[6]<.35?0:null):t==='outro'?(R[6]<.4?0:null):t==='bridge'?(R[6]<.35?null:1):
    t==='special'?(R[6]<.55?null:0):t==='verse'?Math.min(tier,1):tier;
  p.pad=!!o.pad&&R[7]<({intro:.8,verse:.15,pre:.9,chorus:.85,bridge:.9,special:.95,outro:.9,loop:.6}[t]||.5);
  return p;
}

/* ---- strumenti di base per gli strati ---- */
function secDyn(sec){
  const nx=sec.next?sec.next.energy:sec.energy,ramp=clamp(nx-sec.energy,-.4,.4);
  return t=>{const rel=(t-sec.startBeat)/4,b=Math.floor(rel),inBar=rel-b;
    let m=[.95,.98,1.04,1][((b%4)+4)%4]*(.96+.07*clamp(rel/sec.bars,0,1));
    if(b>=sec.bars-1&&Math.abs(ramp)>.12)m*=1+ramp*.45*inBar;
    return m;};
}
function env(sec,ctx,L,layer){
  const F=(ctx.feel&&ctx.feel[layer])||{},G=F.swing!=null?{...ctx.G,swing:F.swing,unit:ctx.G.unit}:ctx.G,hum=F.hum!=null?F.hum:1;
  const human=ctx.human&&hum>0,end=sec.startBeat+sec.bars*4;
  const dyn=secDyn(sec);
  return{G,M:ctx.M,e:sec.energy,human,end,dyn,
    at:b=>{let c=sec.chords[0];for(const x of sec.chords)if(x.start<=b+1e-6)c=x;return c;},
    add:(t,d,n,v)=>{if(t>=end-1e-6)return;L[layer].push({t,d:Math.max(.06,Math.min(d,end+.5-t)),n,v:clamp(Math.round(v*dyn(t)),16,124)});},
    jit:(r,a)=>human?(r()+r()-1)*a*.7*hum:0,
    vv:(v,j,n)=>v*(n>1?.86+.14*j/(n-1):1),
    strum:(c,j,n,def)=>c.strum&&c.strum.dir?(c.strum.dir==='down'?n-1-j:j)*c.strum.spd:(human?j*(def==null?.008:def):0),
    vel:(c,v)=>v+(c.vel||0)};
}
const barOf=(sec,b)=>Math.floor((b-sec.startBeat)/4+1e-6);
// densità di battuta: la forma della frase (vuoto/pieno) e la melodia (il piano si dirada dove la melodia è fitta e riempie dove tace)
function dBar(sec,plan,b){
  const bs=sec.startBeat+b*4,n=(sec.mel||[]).filter(x=>x.t>=bs&&x.t<bs+4).length,act=clamp(n/6,0,1);
  let d=plan.dens*plan.shape[b%4]*(1-.35*act);
  if(sec.mel&&sec.mel.length&&act<.15)d+=.12*(1-plan.space);
  return clamp(d,.04,1);}
const thinP=dB=>Math.pow(1-dB,1.4)*.8;

// accordi ritmici (anche il pianoforte "classico" v3)
function texBlock(sec,ctx,r,L,{tierFor,thin,push}){
  const E=env(sec,ctx,L,'piano'),G=E.G,e=E.e,hitsR=[],hitsL=[];
  for(let b=0;b<sec.bars;b++){
    const fill=b===sec.bars-1&&sec.next&&r()<.5,p=G.piano[Math.min(2,tierFor(b)+(fill?1:0))];
    const bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs-1e-6&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
    const th=thin?thin(b):0;
    for(let s=0;s<16;s++){
      if(p.r[s]!=='.'){if(!(th&&+p.r[s]<7&&s!==0&&!starts.includes(s)&&r()<th))hitsR.push({s:b*16+s,v:+p.r[s]});}
      else if(starts.includes(s))hitsR.push({s:b*16+s,v:7});
      if(p.l[s]!=='.'){if(!(th&&s!==0&&!starts.includes(s)&&r()<th*.6))hitsL.push({s:b*16+s,k:p.l[s]});}
      else if(starts.includes(s))hitsL.push({s:b*16+s,k:'8'});
    }
  }
  const ant=new Set();
  if(push)hitsR.forEach(h=>{if(h.s%16===14){const nb=sec.startBeat+(h.s+2)/4,nc=sec.chords.find(c=>Math.abs(c.start-nb)<1e-6);if(nc){h.chord=nc;ant.add(h.s+2);}}});
  const R=hitsR.filter(h=>!ant.has(h.s));
  R.forEach((h,i)=>{
    const beat=sec.startBeat+h.s/4,c=h.chord||E.at(beat),cEnd=c.start+c.beats,nx=i+1<R.length?sec.startBeat+R[i+1].s/4:E.end;
    const d=Math.max(.2,(Math.min(nx,cEnd,E.end)-beat)*G.leg),full=h.v>=7||h.s%16===0||h.chord,notes=full?c.rh:c.rh.slice(-Math.max(2,c.rh.length-2));
    const tt=beat+swingDelay(G,h.s%16)+E.jit(r,.03);
    notes.forEach((n,j)=>E.add(tt+E.strum(c,j,notes.length),d,n,E.vv(E.vel(c,(50+e*36)*(.55+h.v*.06)+(j===notes.length-1?6:0)+E.jit(r,10)),j,notes.length)));
  });
  hitsL.forEach((h,i)=>{
    const beat=sec.startBeat+h.s/4,c=E.at(beat),cEnd=c.start+c.beats,nx=i+1<hitsL.length?sec.startBeat+hitsL[i+1].s/4:E.end;
    const d=Math.max(.2,(Math.min(nx,cEnd,E.end)-beat)*G.leg),notes=h.k==='a'?[c.lh[0]]:h.k==='b'?c.lh.slice(1):c.lh,vv=h.k==='a'||h.k==='b'?7:+h.k;
    notes.forEach(n=>E.add(beat+swingDelay(G,h.s%16)+E.jit(r,.03),d,n,E.vel(c,(46+e*34)*(.55+vv*.06)+E.jit(r,8))));
  });
  return ant.size>0;
}
function pianoClassic(sec,ctx,r,L){
  const tier=sec.energy<.4?0:sec.energy<.7?1:2;
  return texBlock(sec,ctx,r,L,{tierFor:()=>tier,thin:null,push:r()<ctx.G.push});
}
// il pianoforte arpeggia: basso alla sinistra, accordo spezzato alla destra
function texBroken(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),G=E.G,shp=ctx.sty.piano.broken;
  sec.chords.forEach(c=>{
    const s0=c.start,s1=c.start+c.beats,b=barOf(sec,s0),dB=dBar(sec,plan,b);
    E.add(s0+E.jit(r,.02),c.beats*.98,c.lh[0],E.vel(c,56+E.e*26));
    if(dB>.55&&c.lh.length>1)E.add(s0+.02,c.beats*.95,c.lh[c.lh.length-1],E.vel(c,46+E.e*20));
    let pool=c.pool.filter(n=>n>c.lh[0]+4);if(pool.length<3)pool=c.pool.slice(1);
    const rate16=dB>.65;
    for(let t=Math.ceil((s0-sec.startBeat)*4-1e-6)/4+sec.startBeat;t<s1-1e-6;t+=.25){
      const st=Math.round((t-sec.startBeat)*4)%16;if(!rate16&&st%2)continue;
      const pat=SHAPES[shp](pool.length);let ix=pat[st];if(ix==null)continue;
      if(st%4!==0&&r()<thinP(dB)*.7)continue;
      ix=clamp(ix,0,pool.length-1);
      E.add(t+swingDelay(G,st)+E.jit(r,.02),Math.min(s1-t+.05,rate16?.75:1.5),pool[ix],E.vel(c,46+E.e*28+(st%4===0?7:0)+E.jit(r,8)));
    }
  });
}
// ostinato: una figura che resta, ancorata alle note comuni tra un accordo e l'altro
function texOstinato(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),G=E.G,M=E.M;
  const sp=ctx.sty.piano,tier=plan.dens<.38?0:plan.dens<.72?1:2,rh=sp.ost[tier],contour=sp.contour;
  const dyad=plan.dens>.55&&sp.dyad,stacc=M.drive>.5;
  let anchor=Math.round((G.top[0]+G.top[1])/2)-3,k=0,lastStart=null;
  const hits=[];
  for(let b=0;b<sec.bars;b++)for(let s=0;s<16;s++)if(rh[s]!=='.')hits.push({t:sec.startBeat+b*4+s/4,s,v:+rh[s],b});
  hits.forEach((h,i)=>{
    const c=E.at(h.t);
    if(c.start!==lastStart){
      lastStart=c.start;
      if(!(plan.shape[barOf(sec,c.start)%4]<.6&&r()<.5))c.lh.forEach(n=>E.add(c.start+E.jit(r,.02),c.beats*.97,n,E.vel(c,42+E.e*22)));
    }
    const ladder=[];for(let m=anchor-8;m<=anchor+10;m++)if(c.pcs.has(mod12(m)))ladder.push(m);
    if(!ladder.length)return;
    let bi=0;ladder.forEach((m,j)=>{if(Math.abs(m-anchor)<Math.abs(ladder[bi]-anchor))bi=j;});
    if(h.s===0&&h.b%2===0)anchor=ladder[bi];
    if(h.v<8&&r()<thinP(dBar(sec,plan,h.b))*.6){k++;return;}
    const ix=clamp(bi+contour[k%contour.length],0,ladder.length-1);k++;
    const nx=i+1<hits.length?hits[i+1].t:E.end,d=Math.max(.12,(nx-h.t)*(stacc?.5:.95));
    const tt=h.t+swingDelay(G,h.s)+E.jit(r,.02);
    E.add(tt,d,ladder[ix],E.vel(c,52+E.e*28+(h.v-7)*5+E.jit(r,8)));
    if(dyad&&ix>=2)E.add(tt,d,ladder[ix-2],E.vel(c,44+E.e*22));
  });
}
// stacchi sincopati brevi
function texStabs(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),G=E.G;
  const choice=ctx.sty.piano.stab,dur=G.leg<.6?.16:.28;
  for(let b=0;b<sec.bars;b++){
    const dB=dBar(sec,plan,b),p=choice[dB<.38?0:dB<.72?1:2],bs=sec.startBeat+b*4;
    for(let s=0;s<16;s++){if(p[s]==='.')continue;const t=bs+s/4,c=E.at(t),v=+p[s];
      if(v<9&&r()<thinP(dB)*.6)continue;
      const notes=v>=9?c.rh:c.rh.slice(-3),tt=t+swingDelay(G,s)+E.jit(r,.02);
      notes.forEach((n,j)=>E.add(tt+E.strum(c,j,notes.length,.004),dur,n,E.vv(E.vel(c,54+E.e*32+(s%4===0?6:0)+E.jit(r,8)),j,notes.length)));}
  }
  sec.chords.forEach(c=>{E.add(c.start,Math.min(c.beats,1)*.9,c.lh[0],E.vel(c,56+E.e*24));if(E.e>.6)E.add(c.start,Math.min(c.beats,1)*.9,c.lh[0]+12,E.vel(c,48+E.e*20));});
}
// accordi tenuti con spazio, e piccole note in alto come respiro
function texSustain(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano');
  sec.chords.forEach(c=>{
    const b=barOf(sec,c.start),low=plan.shape[b%4]<.6,notes=[...c.lh,...c.rh];
    if(!(low&&c.start>sec.startBeat&&r()<.4))
      notes.forEach((n,j)=>E.add(c.start+(c.strum&&c.strum.dir?E.strum(c,j,notes.length):(E.human?j*.03:0)),c.beats-.04,n,E.vel(c,(j<c.lh.length?44:50)+E.e*24+E.jit(r,6))));
    for(let k=0;k<c.beats-1;k+=4){
      const bb=barOf(sec,c.start+k);if(dBar(sec,plan,bb)<.25||r()>.7)continue;
      const top=c.rh[c.rh.length-1],cand=[];for(let m=top+2;m<=top+10;m++)if(c.pcs.has(mod12(m)))cand.push(m);
      if(!cand.length)continue;const pos=c.start+k+pick(r,[1.5,2.5,3.5]);if(pos>=c.start+c.beats)continue;
      E.add(pos+E.jit(r,.03),1,pick(r,cand),E.vel(c,44+E.e*22));
    }
  });
}
// ottave melodiche alla destra sopra un basso in ottava
function texOctaves(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),sc=ctx.scalePcs;
  sec.chords.forEach((c,ci)=>{
    E.add(c.start,c.beats*.97,c.lh[0],E.vel(c,58+E.e*24));E.add(c.start,c.beats*.97,c.lh[0]+12,E.vel(c,50+E.e*20));
    const top=c.rh[c.rh.length-1],nxt=(sec.chords[ci+1]||c).rh.slice(-1)[0];
    const lad=[];for(let m=top-12;m<=top+12;m++)if(sc.includes(mod12(m))||c.pcs.has(mod12(m)))lad.push(m);
    const i0=lad.indexOf(top)<0?0:lad.indexOf(top);let i1=0;lad.forEach((m,j)=>{if(Math.abs(m-nxt)<Math.abs(lad[i1]-nxt))i1=j;});
    const step=dBar(sec,plan,barOf(sec,c.start))>.6?.5:1,cnt=Math.max(1,Math.round(c.beats/step));
    for(let k=0;k<cnt;k++){const t=c.start+k*step;let ix=i0+Math.sign(i1-i0)*Math.min(k,Math.abs(i1-i0));
      if((t-sec.startBeat)%1===0&&!c.pcs.has(mod12(lad[ix])))ix=clamp(ix+(r()<.5?1:-1),0,lad.length-1);
      const n=lad[ix],v=E.vel(c,56+E.e*30+((t-sec.startBeat)%2===0?6:0));E.add(t+E.jit(r,.02),step*.95,n,v);E.add(t+E.jit(r,.02),step*.95,n-12,v-8);}
  });
}
// skank: accordi corti in levare
function texSkank(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),G=E.G,PT=['....9.......9...','..9...9...9...9.','..99..9...99..9.'];
  for(let b=0;b<sec.bars;b++){const dB=dBar(sec,plan,b),p=PT[dB<.35?0:dB<.75?1:2],bs=sec.startBeat+b*4;
    for(let s=0;s<16;s++){if(p[s]==='.')continue;const t=bs+s/4,c=E.at(t),notes=c.rh.slice(-3),tt=t+swingDelay(G,s)+E.jit(r,.015);
      notes.forEach((n,j)=>E.add(tt+j*.004,.16,n,E.vel(c,52+E.e*30+E.jit(r,8))));}}
}
// boom bap: accordi "campionati", tagliati su una figura di due battute
function texChops(sec,plan,ctx,r,L){
  const E=env(sec,ctx,L,'piano'),G=E.G,A=ctx.sty.piano.chopsA,B=ctx.sty.piano.chopsB;
  const hits=[];for(let b=0;b<sec.bars;b++){const p=b%2?B:A,bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs-1e-6&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
    for(let s=0;s<16;s++){if(p[s]!=='.'||starts.includes(s)){if(p[s]!=='.'&&s&&!starts.includes(s)&&r()<thinP(dBar(sec,plan,b))*.6)continue;hits.push({t:bs+s/4,s,v:p[s]==='.'?8:+p[s]});}}}
  hits.forEach((h,i)=>{const c=E.at(h.t),nx=i+1<hits.length?hits[i+1].t:E.end,d=(Math.min(nx,c.start+c.beats,E.end)-h.t)*.95,notes=[...c.lh,...c.rh];
    const tt=h.t+swingDelay(G,h.s)+E.jit(r,.02);
    notes.forEach((n,j)=>E.add(tt+(c.strum&&c.strum.dir?E.strum(c,j,notes.length):(notes.length-1-j)*.012),d,n,E.vv(E.vel(c,(48+E.e*30)*(.6+h.v*.05)+E.jit(r,8)),j,notes.length)));});
}
// il pianoforte risponde alla melodia nelle sue pause
function answers(sec,plan,ctx,r,L){
  const mel=sec.mel||[];if(!mel.length)return 0;
  const E=env(sec,ctx,L,'piano'),p=clamp(.2+E.M.space*.5+(E.G.space||0)*.3,0,.8),sc=ctx.scalePcs;let n=0;
  const wins=[];for(let i=0;i<mel.length;i++){const a=mel[i].t+mel[i].d,b=i+1<mel.length?mel[i+1].t:E.end;if(b-a>=1.25)wins.push([a,b,mel[i].n,i>0?Math.sign(mel[i].n-mel[i-1].n):1]);}
  wins.forEach(([a,b,last,dir])=>{
    if(r()>p)return;const st=Math.ceil(a*2)/2+.0,len=Math.min(b-st-.25,2);if(len<.75)return;
    const step=len>=1.5&&E.e>.5?.25:.5,cnt=clamp(Math.floor(len/step),2,5),c=E.at(st);
    const lad=[];for(let m=60;m<=88;m++)if(sc.includes(mod12(m))||c.pcs.has(mod12(m)))lad.push(m);
    let ix=0;lad.forEach((m,j)=>{if(Math.abs(m-last)<Math.abs(lad[ix]-last))ix=j;});
    const d=-dir||-1;
    for(let k=0;k<cnt;k++){ix=clamp(ix+d*(k?1:2),0,lad.length-1);let m=lad[ix];const t=st+k*step,cc=E.at(t);
      if(k===cnt-1&&!cc.pcs.has(mod12(m))){const j=clamp(ix+d,0,lad.length-1);if(cc.pcs.has(mod12(lad[j])))m=lad[j];}
      E.add(t+E.jit(r,.02),step*.9,m,50+E.e*22+(k===0?6:0));}
    n++;
  });
  return n;
}
function pianoLive(sec,plan,ctx,r,L){
  const f={block:(s,p,c,rr,l)=>texBlock(s,c,rr,l,{tierFor:b=>{const d=dBar(s,p,b);return d<.35?0:d<.7?1:2;},thin:b=>thinP(dBar(s,p,b)),push:c.sty.piano.push}),
    broken:texBroken,ostinato:texOstinato,stabs:texStabs,sustain:texSustain,octaves:texOctaves,skank:texSkank,chops:texChops}[plan.tex]||texBroken;
  const pushed=f(sec,plan,ctx,r,L);
  return{pushed,answers:answers(sec,plan,ctx,r,L)};
}

/* ---- arpeggio, pad, basso, batteria ---- */
function genArp(sec,ctx,r,L){
  const E=env(sec,ctx,L,'arp'),G=E.G,shape=ctx.sty.arp,rate16=E.e>=.5;
  for(let b=0;b<sec.bars;b++)for(let s=0;s<16;s++){
    const beat=sec.startBeat+b*4+s/4,c=E.at(beat),pat=SHAPES[shape](c.pool.length);
    if(!rate16&&s%2===1)continue;let ix=pat[s];if(ix==null)continue;ix=clamp(ix,0,c.pool.length-1);
    const d=Math.max(.15,Math.min(c.start+c.beats-beat+.05,(rate16?.6:1.2)*(shape==='lazy'?1.5:1)));
    E.add(beat+swingDelay(G,s)+E.jit(r,.02),d,c.pool[ix],E.vel(c,44+E.e*30+(s%4===0?8:0)+(ix>=c.pool.length-2?4:0)+E.jit(r,8)));
  }
  return shape;
}
function padAtmos(sec,ctx,r,L){
  const E=env(sec,ctx,L,'pad'),M=E.M,low=M.n==='Cupo'||E.G.n==='Trip hop'||r()<.25,open=new Map();
  const close=(t)=>{for(const [n,ev] of open)ev.d=Math.max(.2,t-ev.t-.02);};
  sec.chords.forEach((c,ci)=>{
    const iv=QT[c.q].iv,pcs=[];
    if(low){pcs.push(c.pc,mod12(c.pc+7));}
    else{if(iv.includes(7))pcs.push(mod12(c.pc+7));if(c.pcs.has(mod12(c.pc+2))||ctx.scalePcs.includes(mod12(c.pc+2)))pcs.push(mod12(c.pc+2));pcs.push(mod12(c.pc+iv[1]));}
    const lo=low?43:67,hi=low?60:86,want=new Set();
    pcs.slice(0,3).forEach(pc=>{let m=lo+mod12(pc-lo);for(const [n] of open)if(mod12(n)===pc)m=n;if(m<=hi)want.add(m);});
    for(const [n,ev] of [...open]){if(!want.has(n)){ev.d=Math.max(.2,c.start-ev.t-.02);open.delete(n);}}
    want.forEach(n=>{if(!open.has(n)){const ev={t:c.start,d:1,n,v:clamp(Math.round((ci===0?30:36)+E.e*22+(c.vel||0)/2),16,100)};L.pad.push(ev);open.set(n,ev);}});
  });
  close(E.end);
}
function genBass(sec,ctx,r,L,plan,prevRef){
  const E=env(sec,ctx,L,'bass'),G=E.G,M=E.M,e=E.e,tier=e<.4?0:e<.7?1:2,next=sec.next;
  let prev=prevRef.n;
  const lo=G.bass808?28:31,hi=G.bass808?43:52;
  const near=pc=>{let m=nearestPc(pc,prev);while(m<lo)m+=12;while(m>hi)m-=12;return m;};
  const nextChord=c=>{const ci=sec.chords.indexOf(c);return sec.chords[ci+1]||(next&&next.chords[0])||sec.chords[0];};
  if(G.walk&&tier>=1){
    const scale=ctx.scalePcs;
    sec.chords.forEach(c=>{
      const nxc=nextChord(c),n=Math.max(1,Math.round(c.beats)),line=[];
      let cur=near(c.bass);line.push(cur);const tgt=nearestPc(nxc.bass,cur);
      for(let k=1;k<n-1;k++){const dir=Math.sign(tgt-cur)||(r()<.5?1:-1),opts=[];
        for(let m=cur-5;m<=cur+5;m++){if(m===cur||m<lo||m>hi)continue;const pc=mod12(m);let w=c.pcs.has(pc)?2:scale.includes(pc)?1:0;if(Math.sign(m-cur)===dir)w*=2;if(Math.abs(m-cur)>4)w*=.4;opts.push([m,w]);}
        cur=pickW(r,opts);line.push(cur);}
      if(n>=2){let a=tgt+(r()<.65?(tgt>cur?-1:1):(r()<.5?7:-5));while(a<lo)a+=12;while(a>hi)a-=12;line.push(a);}
      line.forEach((m,k)=>{const t=c.start+k*c.beats/n;E.add(t+E.jit(r,.02),c.beats/n*.92,m,E.vel(c,70+e*22+(k===0?8:0)+E.jit(r,8))*1);prev=m;});
    });
    prevRef.n=prev;return'walking bass';
  }
  if(G.bass808){
    const kick=plan.drums!=null&&DRUMS[G.drums]?(DRUMS[G.drums][Math.min(plan.drums,2)].k||G.bass[tier]):G.bass[tier];
    let last=null;
    for(let b=0;b<sec.bars;b++){
      const bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs-1e-6&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
      const hits=[];for(let s=0;s<16;s++)if((kick.length===16&&kick[s]!=='.')||starts.includes(s))hits.push(s);
      if(!hits.includes(0))hits.unshift(0);
      hits.sort((a,b)=>a-b).forEach((s,i)=>{const t=bs+s/4,c=E.at(t);let m=near(c.bass);
        if(i>0&&!starts.includes(s)&&r()<.22&&m+12<=hi+7)m+=12;
        const nx=i+1<hits.length?bs+hits[i+1]/4:bs+4,ev={t:t+E.jit(r,.01),d:(nx-t)*.98,n:m,v:clamp(Math.round(E.vel(c,84+e*20)),20,124)};
        if(last!=null&&last!==m&&r()<.55)ev.gl=last;
        L.bass.push(ev);last=m;prev=m;});
    }
    prevRef.n=prev;return'808 con glide';
  }
  for(let b=0;b<sec.bars;b++){
    const pat=G.bass[tier],hits=[],bs=sec.startBeat+b*4,starts=sec.chords.filter(c=>c.start>=bs-1e-6&&c.start<bs+4).map(c=>Math.round((c.start-bs)*4));
    for(let s=0;s<16;s++){if(pat[s]!=='.')hits.push({s,k:pat[s]});else if(starts.includes(s))hits.push({s,k:'R'});}
    hits.forEach((h,i)=>{
      const beat=bs+h.s/4,c=E.at(beat),nxc=nextChord(c),root=near(c.bass);let m=root;
      switch(h.k){case'5':m=root+7>hi?root-5:root+7;break;case'8':m=root+12<=hi+3?root+12:root;break;case'3':m=root+QT[c.q].iv[1];break;
        case'6':m=root+9;break;case'a':{const nb=nearestPc(nxc.bass,root);m=nb+(nb>root?-1:1);break;}
        case'n':m=(Math.abs(nxc.start-(bs+4))<1e-6&&h.s>=14)?near(nxc.bass):root;break;}
      const nxB=i+1<hits.length?bs+hits[i+1].s/4:bs+4,d=Math.max(.15,(Math.min(nxB,c.start+c.beats+(h.k==='n'?1:0))-beat)*(G.leg<.6?.7:.9));
      E.add(beat+swingDelay(G,h.s)+E.jit(r,.02),d,m,E.vel(c,68+e*24+(h.k==='R'?8:0)+E.jit(r,8)));
      if(M.oct&&h.k==='R'&&m-12>=28)E.add(beat+swingDelay(G,h.s),d,m-12,60+e*20);
      prev=m;
    });
  }
  prevRef.n=prev;return null;
}
function genDrums(sec,ctx,r,L,tier,plan,grid,pdens){
  const E=env(sec,ctx,L,'drums'),G=E.G,M=E.M,set=DRUMS[G.drums],p=set[Math.min(tier,set.length-1)],e=E.e,kit=drumKit(G);
  const R=()=>r();
  const G2=grid||{},PD=pdens||{};
  const hit=(t,pad,v,d,man)=>{if(G2[pad]&&!man)return;const k=PD[pad];
    if(k!=null&&k<1&&pad!==9&&pad!==8){const rel=(t-sec.startBeat)%1;if(Math.abs(rel)>.01&&r()>k)return;}
    L.drums.push({t,d:d||.12,n:kit.notes[pad],pad,v:clamp(Math.round(v*E.dyn(t)),12,127)});
    if(k!=null&&k>1&&!man&&(pad===3||pad===5||pad===6||pad===7)&&r()<k-1)L.drums.push({t:t+.25,d:.1,n:kit.notes[pad],pad,v:clamp(Math.round(v*.6*E.dyn(t)),12,127)});};
  const lvl=d=>34+d*9*(.8+e*.3);
  const space=plan?plan.space:.3,dens=plan?plan.dens:.5,shape=plan?plan.shape:[1,1,1,1];
  const ghostSn=['boombap','soul','gospel','jazz','afrobeat','lofi','blues'].includes(G.drums),groove=!!G.groove;
  const nx=sec.next,riseTo=nx&&nx.energy-sec.energy>=.2;
  // stop prima di una sezione più forte (vuoto che prepara il pieno)
  const stop=riseTo&&R()<(groove?.35:.15)*(1+space);
  for(let b=0;b<sec.bars;b++){
    const bs=sec.startBeat+b*4,lastBar=b===sec.bars-1,fill=lastBar&&nx&&e>=.4&&!stop&&R()<.85;
    const empty=space>.5&&shape[b%4]<.6;   // battuta "vuota": meno colpi
    const taken=new Set();
    for(const ins in p){if(ins==='g')continue;const line=p[ins],n=line.length,st=4/n,pad=LINE_PAD[ins];if(pad==null)continue;
      for(let i=0;i<n;i++){const ch=line[i];if(ch==='.')continue;const t=bs+i*st,beat=t-bs;
        if(fill&&beat>=3-1e-6&&(pad===0||pad===1||pad>=6))continue;
        if(stop&&lastBar&&beat>=2-1e-6)continue;
        if(empty&&pad===0&&beat>.01&&R()<.6)continue;
        if(empty&&pad===3&&i%2===1&&R()<.5)continue;
        const sw=n===16?swingDelay(G,i):0,jt=E.jit(r,.012),vel=pad===3&&ins==='p'?.8:1;
        if(n===16)taken.add(pad+':'+i);
        if(ch==='r'){hit(t+sw+jt,pad,lvl(6));hit(t+st/2+sw,pad,lvl(5));}
        else if(ch==='z'){for(let k=0;k<3;k++)hit(t+sw+k*st*2/3,pad,lvl(5+(k===0?1:0)));}
        else hit(t+sw+jt,pad,(lvl(+ch)+E.jit(r,10))*vel);}}
    if(stop&&lastBar)continue;
    // aggiunte secondo la densità
    if(dens>.55&&!empty)for(let i=1;i<16;i+=2)if(!taken.has('3:'+i)&&!taken.has('3:'+(i-1))&&R()<(dens-.55)*.9)hit(bs+i/4+swingDelay(G,i),3,lvl(3)+E.jit(r,6));
    if(ghostSn)for(const i of [3,7,9,15])if(!taken.has('1:'+i)&&R()<dens*.22*(1-space*.5))hit(bs+i/4+swingDelay(G,i),1,26+R()*18);
    if(groove&&e>=.6&&!p.w&&dens>.45)for(let i=0;i<16;i++)if(R()<.85)hit(bs+i/4+swingDelay(G,i),5,(i%2?28:40)+R()*10);
    if(b%2===1&&!lastBar&&!taken.has('0:14')&&R()<.3*dens)hit(bs+3.5+swingDelay(G,14),0,lvl(6));
    if(p.g)for(let s=2;s<16;s+=4)if(R()<.25*p.g)hit(bs+s/4+swingDelay(G,s),1,28+R()*25);
    if(fill){const fl=G.drums==='afrorage'||kit.names[6].startsWith('Conga')?pick(r,[[6,0,6,7,7,7],[6,6,7,6,7,7]]):pick(r,[[1,1,1,1],[6,6,7,7],[1,1,6,7],[1,0,1,1]]);
      const st=1/fl.length;fl.forEach((pd,k)=>{if(pd||k===0)hit(bs+3+k*st,pd,70+k*8+e*20);});}
    if(b===0&&(e>=.6||(sec.prev&&sec.prev.type==='intro')))hit(bs,8,80+e*30,1);
  }
  // colpi scritti a mano sulla griglia (sostituiscono quelli generati per quel pad)
  for(const pd in G2){const steps=G2[pd];if(!steps)continue;for(let b=0;b<sec.bars;b++){const bs=sec.startBeat+b*4;
    steps.forEach((v,i)=>{if(v>0)hit(bs+i/4+swingDelay(G,i)+E.jit(r,.01),+pd,v,+pd===9?1:.12,true);});}}
  // FX: risalita prima di una sezione più forte, impatto all'attacco del ritornello
  if(riseTo)hit(sec.startBeat+(sec.bars-1)*4,9,64+e*20,4);
  if(sec.type==='chorus'&&sec.prev&&['trap','afrorage','cinematic','triphop'].includes(G.drums))hit(sec.startBeat,9,88,.6);
}

function snapMelody(notes,sec,ctx){
  const sc=ctx.scalePcs;
  const at=b=>{let c=sec.chords[0];for(const x of sec.chords)if(x.start<=b+1e-6)c=x;return c;};
  return notes.map(x=>{const c=at(x.t),pc=mod12(x.n),ok=x.strong?c.pcs.has(pc):(c.pcs.has(pc)||sc.includes(pc));
    if(ok)return x;let best=x.n;for(let k=1;k<=6;k++){if((x.strong?c.pcs.has(mod12(x.n-k)):(c.pcs.has(mod12(x.n-k))||sc.includes(mod12(x.n-k))))){best=x.n-k;break;}
      if((x.strong?c.pcs.has(mod12(x.n+k)):(c.pcs.has(mod12(x.n+k))||sc.includes(mod12(x.n+k))))){best=x.n+k;break;}}
    return{...x,n:best};});
}

function generateSong(o){
  const G=GENRES[o.genre],M=MOODS[o.mood],seeds=o.seeds,RS=o.reseed||{};
  const tagx=(tag,layer,type,kind)=>{const l=layer&&RS.L?RS.L[layer]||0:0,x=type&&RS.S?RS.S[type+'.'+kind]||0:0;return tag+(l?'#L'+l:'')+(x?'#S'+x:'');};
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
  let autoBpm=Math.round(clamp((G.tempo[0]+(G.tempo[1]-G.tempo[0])*hr())*M.tempo,52,180));
  if(G.groove)autoBpm=clamp(autoBpm,G.tempo[0],G.tempo[1]);
  const ctx={G,M,mode,key,color,tense:!!M.tense,human:o.human!==false,feel:o.feel||{},scalePcs:MODES[mode].scale.map(x=>mod12(key+x)),
    allowDim:!!(M.tense||G.n==='Jazz'||G.n==='Gospel'||G.n==='Classico')};
  const info=new Set();
  const spell=speller(key,mode);
  // struttura
  let secs=(o.secs&&o.secs.length?o.secs:STRUCTS[o.structure||'loop4'].secs).map(x=>x.slice());
  if(G.blues)secs=secs.map(([t,b])=>[t,(t==='verse'||t==='chorus')&&!(o.secs&&o.secs.length)?12:b]);
  const types=[...new Set(secs.map(s=>s[0]))];
  const order=[...(o.typeOrder||[]),...TYPE_ORDER].filter((t,i,a)=>a.indexOf(t)===i&&types.includes(t)&&TYPE_ORDER.includes(t));
  const barsOf=t=>secs.find(s=>s[0]===t)[1];
  const fillTo=(cell,n)=>{const a=[];while(a.length<n)a.push(...cell.map(x=>({...x})));return a.slice(0,n);};
  const mat={},cells={},others=[],still={};
  // se una sezione viene rigenerata, le altre continuano a confrontarsi con le scelte originali: restano identiche
  let baseOthers=null;
  if(!G.blues&&order.some(t=>RS.S&&RS.S[t+'.h'])){baseOthers={};const oth=[];for(const t of order){baseOthers[t]=oth.slice();oth.push(chooseCell(t,ctx,subRng(seeds.h,'sec-'+t),oth));}}
  for(const t of order){
    const r=subRng(seeds.h,tagx('sec-'+t,null,t,'h')),n=barsOf(t);
    if(G.blues){mat[t]=fillTo(bluesBars(ctx,n,t==='special'?'bridge':t,r),n);continue;}
    let cell=chooseCell(t,ctx,r,baseOthers?baseOthers[t]:others);others.push(cell);
    const stz=clamp(M.stasis*.8+(G.stasis||0)*.6,0,.9)*(t==='bridge'||t==='special'?.5:1);
    if(r()<stz){const c0=cell[0],c1=cell.find(x=>x.r!==c0.r||x.tri!==c0.tri)||cell[1];
      cell=pick(r,[[c0,c0,c1,c1],[c0,c1,c0,c1],[c0,c0,c0,c1],[c0,c1,c1,c1]]).map(x=>({...x}));still[t]=1;info.add('armonia ferma: pochi accordi ripetuti');}
    cells[t]=cell;
    let bars=[];
    for(let i=0;i<n;i+=4){const last=i+4>=n;
      bars=bars.concat(!still[t]&&((last&&n>4)||((t==='pre'||t==='bridge'||t==='special')&&last))?cadenceVariant(cell,ctx,r,t==='special'?'bridge':t):cell.map(x=>({...x})));}
    mat[t]=bars.slice(0,n);
  }
  const src=order.find(t=>cells[t]);
  for(const t of ['intro','outro'])if(types.includes(t)){
    const r=subRng(seeds.h,tagx('sec-'+t,null,t,'h')),n=barsOf(t);
    if(G.blues){mat[t]=fillTo(bluesBars(ctx,4,t,r),n);continue;}
    const base=src?cells[src]:chooseCell('loop',ctx,r,others);
    const c=fillTo(base,n);
    if(t==='outro'){c[n-1]={r:0,tri:MODES[mode].minor?'min':'maj'};if(n>1&&c[n-2].r===0)c[n-2]=parseRN(MODES[mode].minor?(r()<.5?'iv':'bVII'):(r()<.5?'IV':'V'));}
    mat[t]=c;
  }
  // trasformazioni e colori per tipo di sezione
  const segs={};
  for(const t of types){
    const r=subRng(seeds.h,tagx('tr-'+t,null,t,'h'));
    const s=still[t]?mat[t].map(x=>({...x,beats:4})):transformBars(mat[t].map(x=>({...x})),t,ctx,r,info);
    s.forEach((c,i)=>{c.q=colorize(c,s[i+1]||s[0],ctx,r);});
    if(t==='outro'){const l=s[s.length-1];if(l.r===0)l.q=l.tri==='min'?(color>.5?'m9':'min'):(color>.5?'maj9':(color>.2?'add9':'maj'));}
    segs[t]=s;
  }
  // istanze di sezione
  const sections=[];let bar=0;const occ={};
  secs.forEach(([t,n])=>{
    occ[t]=(occ[t]||0)+1;
    const sec={type:t,name:SEC_NAME[t]+(secs.filter(x=>x[0]===t).length>1?' '+occ[t]:''),bars:n,startBar:bar,startBeat:bar*4,
      energy:SPEC[t].energy+(t==='chorus'&&occ[t]>=3?.06:0),occ:occ[t],chords:segs[t].map(c=>({...c}))};
    sections.push(sec);bar+=n;
  });
  sections.forEach(s=>{let b=s.startBeat;s.chords.forEach(c=>{c.start=b;b+=c.beats;});});
  applyEdits(sections,o.edits);
  const chords=[];sections.forEach((s,si)=>s.chords.forEach(c=>{c.si=si;chords.push(c);}));
  // dettagli accordi
  chords.forEach(c=>{
    c.pc=mod12(key+c.r);
    const iv=QT[c.q].iv;c.pcs=new Set(iv.map(x=>mod12(c.pc+x)));c.core=new Set(iv.filter(x=>x<12).map(x=>mod12(c.pc+x)));
  });
  sections.forEach(sec=>chooseInversions(sec.chords,ctx,subRng(seeds.h,tagx('inv-'+sec.type,null,sec.type,'h'))));
  chords.forEach(c=>{
    if(c.bassIv!=null){const iv=QT[c.q].iv;c.bass=mod12(c.pc+(iv[c.bassIv]!=null?iv[c.bassIv]:0));}
    const tn=x=>numeral(x.r,x.tri==='min'?'min':x.tri==='dim'?'dim':'maj');
    c.rn=c.tt&&c.ttTarget?'subV'+(QT[c.q].r||'')+'/'+tn(c.ttTarget):c.sec&&c.target&&c.target.r!==0?'V'+(QT[c.q].r||'')+'/'+tn(c.target):c.sec&&c.target?'V'+(QT[c.q].r||''):numeral(c.r,c.q);
    c.inv=c.bass!==c.pc?1:0;
    const rs=rootSpell(c.rn,spell);c.name=rs(c.pc)+QT[c.q].n+(c.bass!==c.pc?'/'+spell(c.bass):'');
  });
  // modulazione opzionale: dall'ultimo ritornello in poi, un tono sopra
  if(o.modLast){const ch=sections.map((x,i)=>i).filter(i=>sections[i].type==='chorus');
    if(ch.length>=2){const k0=ch[ch.length-1],sp2=speller(key+2,mode);
      for(let i=k0;i<sections.length;i++){sections[i].mod=2;sections[i].chords.forEach(c=>{c.pc=mod12(c.pc+2);c.bass=mod12(c.bass+2);
        const iv=QT[c.q].iv;c.pcs=new Set(iv.map(x=>mod12(c.pc+x)));c.core=new Set(iv.filter(x=>x<12).map(x=>mod12(c.pc+x)));
        c.name=rootSpell(c.rn,sp2)(c.pc)+QT[c.q].n+(c.bass!==c.pc?'/'+sp2(c.bass):'');});}
      const pv=sections[k0-1];if(pv&&pv.type!=='chorus'){const c=pv.chords[pv.chords.length-1];
        Object.assign(c,{r:mod12(9),q:'7',tri:'maj',pc:mod12(key+9),bass:mod12(key+9),pivot:1});const iv=QT['7'].iv;
        c.pcs=new Set(iv.map(x=>mod12(c.pc+x)));c.core=new Set(iv.map(x=>mod12(c.pc+x)));c.name=sp2(c.pc)+'7';c.rn='V7→+1';}
      info.add('modulazione: ultimo ritornello un tono sopra');}}
  // voicing
  let pr=null,pl=null,pa=null;
  chords.forEach(c=>{const e=sections[c.si].energy,top=[G.top[0]+(e>.7?1:0),G.top[1]+(e>.7?2:0)];
    c.rh=voiceRH(c,pr,ctx,top);pr=c.rh;c.lh=voiceLH(c,pl,ctx,e);pl=c.lh[0];c.pool=arpPool(c,pa);pa=c.pool[0];});
  // strati, sezione per sezione, secondo il piano di arrangiamento
  const L={mel:[],piano:[],arp:[],pad:[],bass:[],drums:[]},melCache={},bassRef={n:38};
  const rA=(tag,layer,type)=>subRng(seeds.a,tagx(tag,layer,type,'a'));
  ctx.sty=songStyle(G,M,rA('sty-piano','piano'),rA('sty-arp','arp'));
  const mr=subRng(seeds.m,tagx('mel-dna','mel')),md=clamp(G.mel.dens+(M.dens-.5)*.4,.15,.9);
  const melDNA=[rhythmBar(mr,md*.9,G.mel.sync,G.mel.s16),rhythmBar(mr,md,G.mel.sync,G.mel.s16)];
  sections.forEach((sec,si)=>{
    sec.next=sections[si+1]||null;sec.prev=sections[si-1]||null;
    const t=sec.type,plan=sec.plan=planSection(sec,ctx,rA('plan-'+t,null,t),o);
    sec.mel=[];
    if(plan.mel){
      if(!melCache[t])melCache[t]=genMelody(sec,ctx,subRng(seeds.m,tagx('mel-'+t,'mel',t,'m')),melDNA);
      const r=rA('melh-'+t,'mel',t),dyn=secDyn(sec),MF=ctx.feel.mel||{},MG=MF.swing!=null?{...G,swing:MF.swing}:G,mh=ctx.human?(MF.hum!=null?MF.hum:1):0;
      const raw=melCache[t].map(x=>({t:sec.startBeat+x.s/4+swingDelay(MG,x.s%16)+(mh?((r()-.5)*.02+.01)*mh:0)+(sec.mod||0)*0,d:Math.max(.2,x.len/4*G.mel.leg),n:x.n,strong:x.strong,
        v:clamp(Math.round((68+sec.energy*22+(x.strong?8:0)+(x.peak?6:0)+(mh?(r()+r()-1)*6*mh:0))*dyn(sec.startBeat+x.s/4)),30,122)}));
      sec.mel=sec.mod?raw.map(x=>({...x,n:x.n+sec.mod})):snapMelody(raw,sec,ctx);
      sec.mel.forEach(x=>L.mel.push({t:Math.max(0,x.t),d:x.d,n:x.n,v:x.v}));
    }
    const pr2=rA('piano-'+t,'piano',t);
    if(o.pianoStyle==='classic'){if(pianoClassic(sec,ctx,pr2,L))info.add('anticipi in levare');}
    else{const res=pianoLive(sec,plan,ctx,pr2,L);sec.texture=plan.tex;if(res.pushed)info.add('anticipi in levare');if(res.answers)info.add('il pianoforte risponde alla melodia nelle pause');}
    if(plan.arp){info.add('arpeggio: '+genArp(sec,ctx,rA('arp-'+t,'arp',t),L));}
    if(plan.pad){padAtmos(sec,ctx,rA('pad-'+t,'pad',t),L);info.add('pad atmosferico che entra ed esce');}
    if(plan.bass){const x=genBass(sec,ctx,rA('bass-'+t,'bass',t),L,plan,bassRef);if(x)info.add(x);}
    if(plan.drums!=null&&G.drums)genDrums(sec,ctx,rA('drums-'+t,'drums',t),L,plan.drums,plan,(o.drumGrid||{})[t],o.padDens);
  });
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
    info:[...info],swing:G.swing,prog:G.prog,secs,typeOrder:order,
    defaults:{mel:1,piano:1,arp:1,pad:1,bass:1,drums:G.groove?1:0}};
}
const rootSpell=(rn,spell)=>/^b/.test(rn)?(pc=>FLAT[mod12(pc)]):/^#/.test(rn)?(pc=>SHARP[mod12(pc)]):spell;
const Q_LEVEL={maj:0,min:0,dim:0,aug:0,sus2:1,sus4:1,add9:1,madd9:1,'6':1,m6:1,maj7:2,m7:2,'7':2,m7b5:2,dim7:2,mmaj7:2,'7sus4':2,
  '9':3,maj9:3,m9:3,'69':3,'9sus4':3,'13':4,m11:4,'maj7#11':4,'7b9':4,'7#9':4};
const ROLE_TXT={T:'tonica: riposo',Tm:'sostituto di tonica',S:'sottodominante: prepara',D:'dominante: tensione',Dm:'dominante debole (v)'};
const fitQ=(scale,r)=>Object.keys(QT).filter(q=>QT[q].iv.every(x=>scale.includes(mod12(r+x))));
function bestQ(scale,r,color,tri){
  const target=color<.16?0:color<.4?1:color<.66?2:color<.86?3:4;
  const f=fitQ(scale,r).filter(q=>qTri(q)===tri||(tri==='maj'&&/sus/.test(q)));
  if(!f.length)return tri;
  return f.sort((a,b)=>Math.abs(Q_LEVEL[a]-target)-Math.abs(Q_LEVEL[b]-target)+(/sus/.test(a)?.3:0)-(/sus/.test(b)?.3:0))[0];
}
// consigli per un accordo: prima ciò che sta nella scala (ordinato per come si lega a prima e dopo), poi interscambio modale e cromatismi
function chordAdvice(song,si,ci){
  const md=MODES[song.mode],sc=md.scale,sec=song.sections[si],c=sec.chords[ci],all=song.chords,gi=all.indexOf(c);
  const prev=all[gi-1]||null,next=all[gi+1]||null,sp=song.spell,col=song.color;
  const degOf=x=>{if(!x)return-1;const d=sc.indexOf(mod12(x.r));return d>=0&&inScale(sc,x.r,qTri(x.q))?d:-1;};
  const pd=degOf(prev),nd=degOf(next),cd=degOf(c),curRole=role({r:c.r,tri:qTri(c.q)},md.minor);
  const nm=(r,q,rn)=>rootSpell(rn,sp)(song.key+r)+QT[q].n;
  const scale=[];
  for(let d=0;d<7;d++){const dc=degChord(song.mode,d),q=bestQ(sc,dc.r,col,dc.tri),rn=numeral(dc.r,q),rl=role(dc,md.minor);
    let s=0;const why=[ROLE_TXT[rl]];
    if(pd>=0)s+=Math.log(1+W[pd][d]);if(nd>=0){s+=Math.log(1+W[d][nd]);if(W[d][nd]>=3)why.push('porta bene a '+next.name.split('/')[0]);}
    if(rl===curRole||(rl==='Tm'&&curRole==='T')||(rl==='T'&&curRole==='Tm')){s+=1;why.push('stessa funzione di '+c.name.split('/')[0]);}
    if(d===pd||d===nd)s-=1.4;
    if(d===cd)s-=.5;
    if(dc.tri==='dim'&&!song.opts?.mode)s-=.6;
    scale.push({r:dc.r,q,lab:rn,name:nm(dc.r,q,rn),why:why.join(' · '),score:s,cur:mod12(c.r)===dc.r&&qTri(c.q)===dc.tri,fit:'in'});}
  const rec=scale.filter(x=>!x.cur).sort((a,b)=>b.score-a.score).slice(0,5);
  // interscambio modale: accordi dei modi paralleli che non stanno nella scala
  const PRI={'5min':1,'8maj':2,'10maj':3,'3maj':4,'5maj':5,'7maj':6,'1maj':7,'2maj':8,'0min':9,'0maj':9,'7min':10,'9min':11};
  const bor=[],seen=new Set();
  MODE_ORDER.filter(m=>m!==song.mode).forEach(m=>{for(let d=0;d<7;d++){const dc=degChord(m,d);if(dc.tri==='dim'||dc.tri==='aug')continue;
    const k=dc.r+dc.tri;if(seen.has(k)||inScale(sc,dc.r,dc.tri))continue;seen.add(k);
    const q=bestQ(MODES[m].scale,dc.r,col,dc.tri),rn=numeral(dc.r,q);
    bor.push({r:dc.r,q,lab:rn,name:nm(dc.r,q,rn),why:'preso dal '+MODES[m].n.toLowerCase(),src:MODES[m].n,p:PRI[k]||20,fit:'borrow'});}});
  bor.sort((a,b)=>a.p-b.p);
  // cromatici in funzione dell'accordo successivo
  const chrom=[];
  if(next){const t=next.name.split('/')[0];
    chrom.push({r:mod12(next.r+7),q:col>.7?'9':'7',lab:'V7/'+numeral(next.r,qTri(next.q)==='min'?'min':'maj'),why:'dominante secondaria: spinge verso '+t,fit:'chrom'});
    chrom.push({r:mod12(next.r+1),q:'7',lab:'subV7',why:'sostituzione di tritono verso '+t+' (basso cromatico)',fit:'chrom'});
    chrom.push({r:mod12(next.r-1),q:'dim7',lab:'#°7',why:'diminuito di passaggio che sale a '+t,fit:'chrom'});
    if(qTri(next.q)!=='dim')chrom.push({r:mod12(next.r+7),q:'7sus4',lab:'Vsus',why:'sospensione prima di '+t,fit:'chrom'});
    chrom.forEach(x=>{x.name=(x.q==='dim7'?SHARP[mod12(song.key+x.r)]:sp(song.key+x.r))+QT[x.q].n;});}
  // qualità per la radice attuale: dentro / fuori scala
  const inQ=fitQ(sc,mod12(c.r));
  return{rec,scale,bor:bor.slice(0,10),chrom,inQ:new Set(inQ),curRole:ROLE_TXT[curRole]||'',inScale:cd>=0||inScale(sc,c.r,qTri(c.q))};
}
function suggestChords(song){
  const md=MODES[song.mode],rich=song.color>=.5,out=[];
  const q7=(tri,r,dom)=>!rich?tri:tri==='maj'?(dom?'7':'maj7'):tri==='min'?'m7':tri==='dim'?'m7b5':'aug';
  for(let d=0;d<7;d++){const c=degChord(song.mode,d),q=q7(c.tri,c.r,c.r===7||(md.scale.includes(mod12(c.r+10))&&!md.scale.includes(mod12(c.r+11))&&c.tri==='maj'));out.push({g:'modo',r:c.r,q,lab:numeral(c.r,q)});}
  const bor=md.minor?['IV','V','bII','I','II','iv']:['iv','bVI','bVII','bIII','bII','II','v','i'];
  bor.forEach(s=>{const c=parseRN(s),q=q7(c.tri,c.r,s==='V'||s==='bVII'||s==='II');if(!out.some(x=>x.r===c.r&&x.q===q))out.push({g:'prestito',r:c.r,q,lab:numeral(c.r,q)});});
  return out;
}

/* ---------------- MIDI ---------------- */
const LAYERS=[
  {id:'mel',n:'Melodia',ch:0},{id:'piano',n:'Pianoforte',ch:1},{id:'arp',n:'Arpeggio',ch:2},
  {id:'pad',n:'Pad',ch:3},{id:'bass',n:'Basso',ch:4},{id:'drums',n:'Batteria',ch:9}];
function vlq(n){const b=[n&127];while(n>>=7)b.unshift((n&127)|128);return b;}
const txt=s=>Array.from(unescape(encodeURIComponent(s))).map(c=>c.charCodeAt(0));
function toMidi(song,{layers,from=0,to=song.beats,bpm=song.bpm,title='Piano Generativo',oct={},vol={},padMute={},shift={},padVol={}}={}){
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
    const ev=song.layers[l.id].filter(e=>e.t>=from-1e-6&&e.t<to&&!(l.id==='drums'&&padMute[e.pad]));if(!ev.length)return;
    const list=[{tick:0,o:0,b:meta(3,txt(l.n))}];
    if(l.ch!==9)list.push({tick:0,o:0,b:[0xC0|l.ch,progs[l.id]||0]});
    list.push({tick:0,o:0,b:[0xB0|l.ch,7,clamp(Math.round(100*(vol[l.id]!=null?vol[l.id]:1)),0,127)]});
    const sh=l.ch===9?0:12*(oct[l.id]||0);
const sf=(shift[l.id]||0)*bpm/60000;
    ev.forEach(e=>{const a=tk(Math.max(from,e.t+sf)),z=Math.max(a+1,tk(Math.min(e.t+sf+e.d,to))),n=clamp(e.n+sh,0,127),v=l.id==='drums'?e.v*(padVol[e.pad]!=null?padVol[e.pad]:1):e.v;
      list.push({tick:a,o:1,b:[0x90|l.ch,n,clamp(Math.round(v),1,127)]},{tick:z,o:0,b:[0x80|l.ch,n,0]});});
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
if(typeof module!=='undefined'&&module.exports)module.exports={chordAdvice,fitQ,PADS,drumKit,SEC_NAME,SEC_BARS,TYPE_ORDER,TEX_NAME,suggestChords,qTri,GENRES,MOODS,MODES,MODE_ORDER,STRUCTS,QT,KEY_NAMES,LAYERS,generateSong,toMidi,chordChart,makeZip,crc32,
  degChord,numeral,parseRN,speller,rngFrom,mod12,stackVoicings,SHAPES,DRUMS};
