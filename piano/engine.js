'use strict';
/* =====================================================================
   MOTORE GENERATIVO — armonia per genere/mood, accompagnamenti, melodia, MIDI
   ===================================================================== */
const NOTES=['C','Db','D','Eb','E','F','F#','G','Ab','A','Bb','B'];
function mulberry(a){return()=>{a|=0;a=a+0x6D2B79F5|0;let t=Math.imul(a^a>>>15,1|a);t=t+Math.imul(t^t>>>7,61|t)^t;return((t^t>>>14)>>>0)/4294967296;};}
const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
const pick=(r,a)=>a[Math.floor(r()*a.length)];
function pickW(r,arr){let s=0;for(const e of arr)s+=e[1];let x=r()*s;for(const e of arr){if((x-=e[1])<=0)return e[0];}return arr[arr.length-1][0];}

/* ---------- Teoria ---------- */
const SC={major:[0,2,4,5,7,9,11],minor:[0,2,3,5,7,8,10],dorian:[0,2,3,5,7,9,10],lydian:[0,2,4,6,7,9,11],
  mixolydian:[0,2,4,5,7,9,10],phrygian:[0,1,3,5,7,8,10],harmonic:[0,2,3,5,7,8,11]};
const MODE_NAME={major:'Maggiore (ionico)',minor:'Minore (eolio)',dorian:'Dorico',lydian:'Lidio',mixolydian:'Misolidio',phrygian:'Frigio',harmonic:'Minore armonico'};
const MODE_SHIFT={major:0,minor:3,dorian:10,lydian:7,mixolydian:5,phrygian:8,harmonic:3};
const SF=[0,-5,2,-3,4,-1,6,1,-4,3,-2,5];
const QT={
 maj:{iv:[0,4,7],n:'',rn:'',m:0},min:{iv:[0,3,7],n:'m',rn:'',m:1},dim:{iv:[0,3,6],n:'dim',rn:'°',m:1},aug:{iv:[0,4,8],n:'aug',rn:'+',m:0},
 sus2:{iv:[0,2,7],n:'sus2',rn:'sus2',m:0},sus4:{iv:[0,5,7],n:'sus4',rn:'sus4',m:0},
 maj7:{iv:[0,4,7,11],n:'maj7',rn:'maj7',m:0},m7:{iv:[0,3,7,10],n:'m7',rn:'7',m:1},dom7:{iv:[0,4,7,10],n:'7',rn:'7',m:0},
 m7b5:{iv:[0,3,6,10],n:'m7b5',rn:'ø7',m:1},dim7:{iv:[0,3,6,9],n:'dim7',rn:'°7',m:1},mmaj7:{iv:[0,3,7,11],n:'m(maj7)',rn:'maj7',m:1},
 add9:{iv:[0,4,7,14],n:'add9',rn:'add9',m:0},madd9:{iv:[0,3,7,14],n:'m(add9)',rn:'add9',m:1},
 6:{iv:[0,4,7,9],n:'6',rn:'6',m:0},m6:{iv:[0,3,7,9],n:'m6',rn:'6',m:1},
 maj9:{iv:[0,4,7,11,14],n:'maj9',rn:'maj9',m:0},m9:{iv:[0,3,7,10,14],n:'m9',rn:'9',m:1},dom9:{iv:[0,4,7,10,14],n:'9',rn:'9',m:0},
 dom13:{iv:[0,4,10,14,21],n:'13',rn:'13',m:0},'7b9':{iv:[0,4,7,10,13],n:'7b9',rn:'7b9',m:0},'7#9':{iv:[0,4,7,10,15],n:'7#9',rn:'7#9',m:0},
 '7sus4':{iv:[0,5,7,10],n:'7sus4',rn:'7sus4',m:0}};
const DOMQ=new Set(['dom7','dom9','dom13','7b9','7#9']);
const MAJ=[0,2,4,5,7,9,11],ROM=['I','II','III','IV','V','VI','VII'];
function numeral(r,q,bare){
  let acc='',i=MAJ.indexOf(r);
  if(r===6){acc='#';i=3;}else if(i<0){acc='b';i=MAJ.indexOf((r+1)%12);}
  let s=ROM[i];if(QT[q].m)s=s.toLowerCase();
  return acc+s+(bare?'':QT[q].rn);
}
const chordName=(pc,q)=>NOTES[pc%12]+QT[q].n;

function degChord(mode,d){
  const s=SC[mode],r=s[d],t3=(s[(d+2)%7]-r+12)%12,t5=(s[(d+4)%7]-r+12)%12,t7=(s[(d+6)%7]-r+12)%12;
  const tri=t3===4?(t5===7?'maj':'aug'):(t5===7?'min':'dim');
  let sev;
  if(tri==='maj')sev=t7===11?'maj7':'dom7';else if(tri==='min')sev=t7===11?'mmaj7':'m7';
  else if(tri==='dim')sev=t7===9?'dim7':'m7b5';else sev='maj7';
  return{r,deg:d,tri,sev};
}
const TOK={b7:{r:10,tri:'maj',sev:'dom7'},b6:{r:8,tri:'maj',sev:'maj7'},b3:{r:3,tri:'maj',sev:'maj7'},iv:{r:5,tri:'min',sev:'m7'},b2:{r:1,tri:'maj',sev:'maj7'}};
const TOKDEG={b7:6,b6:5,b3:2,iv:3,b2:1};

/* ---------- Mood ---------- */
const MOODS={
 felice:{n:'Felice',modes:[['major',.7],['lydian',.15],['mixolydian',.15]],tempo:1.08,vel:1,dens:.65,borrow:.03,tension:.15,ext:-.08,reg:2,arp:.3},
 malinconico:{n:'Malinconico',modes:[['minor',.7],['dorian',.2],['harmonic',.1]],tempo:.88,vel:.85,dens:.35,borrow:.3,tension:.25,ext:.1,reg:-2,arp:.45},
 epico:{n:'Epico',modes:[['minor',.55],['major',.25],['dorian',.2]],tempo:1,vel:1.12,dens:.55,borrow:.35,tension:.2,ext:-.15,reg:0,arp:.35,oct:1},
 rilassato:{n:'Rilassato',modes:[['major',.45],['dorian',.25],['lydian',.15],['mixolydian',.15]],tempo:.82,vel:.8,dens:.3,borrow:.08,tension:.2,ext:.25,reg:0,arp:.45},
 teso:{n:'Teso / Oscuro',modes:[['minor',.35],['phrygian',.35],['harmonic',.3]],tempo:1.02,vel:1,dens:.6,borrow:.3,tension:.6,ext:.1,reg:-4,arp:.4,tense:1},
 sognante:{n:'Sognante',modes:[['lydian',.4],['major',.3],['dorian',.2],['minor',.1]],tempo:.78,vel:.78,dens:.25,borrow:.1,tension:.15,ext:.25,reg:4,arp:.7},
 romantico:{n:'Romantico',modes:[['major',.5],['minor',.5]],tempo:.85,vel:.88,dens:.35,borrow:.15,tension:.3,ext:.1,reg:0,arp:.5}};

/* ---------- Pattern ritmici: 16 sedicesimi per battuta, cifra = forza, '.' = niente ---------- */
const COMP={
 pop:['9.......7.......','9.....7.9.....7.','9..7..8.7..8..7.'],rock:['9.......8.......','9...7...8...7...','9.7.8.7.9.7.8.7.'],
 jazz:['9...........6...','9.....6.....7...','9..6....7..6..7.'],blues:['9.....7.....8...','9..7..8..7..8...','9.7.8.7.9.7.8.7.'],
 soul:['9.....7...8.....','9..7..7.8..7..7.','9.7..8.7.8.7..8.'],lofi:['9.......6.....7.','9..7....8..6....','9..7..6.8..7.6..'],
 classical:['9.......7.......','9...7...8...7...','9.6.7.6.8.6.7.6.'],cinematic:['9...............','9.......8.......','9...7...8...7...'],
 gospel:['9.7.8...9.7.8...','9..7.8.79..7.8..','9.7.8.7.9.7.8.79'],bossa:['9..8..7...8..7..','9..8..7...8..7.8','9..7..8.7.8..7.8']};
const BASS={
 pop:['R...............','R.......5.......','R..R..5.R..R..5.'],rock:['R...R...R...R...','R.R.R.R.R.R.R.R.','R.R.R.R.R.R.5.R.'],
 jazz:['R.......5.......','R...3...5...a...','R..3..5..a.3..5.'],blues:['R...5...R...5...','R...3...5...6...','R.R.3.R.5.R.6.5.'],
 soul:['R.......5.......','R..R..5.R..5..a.','R.R...5.R..R.a5.'],lofi:['R...............','R.....5.....R...','R.....5...R...5.'],
 classical:['R...............','R...5...R...5...','R.5.3.5.R.5.3.5.'],cinematic:['R...............','R.......R.......','R...R...R...R...'],
 gospel:['R...............','R...5...R..5..a.','R..R..5.R.5.R.a.'],bossa:['R.....5.R.....5.','R..R..5.R..R..5.','R..5..R...5..R..']};
const ARP={
 pop:['0.1.2.1.0.1.2.1.','0.1.2.3.2.1.2.1.','0.121.2.0.121.2.'],rock:['0.1.2.1.0.1.2.1.','0.1.2.1.2.1.2.1.','0.12.12.0.12.12.'],
 jazz:['0.1.2.3.2.1.0.1.','0.1.2.3.4.3.2.1.','0.1.2.3.4.2.1.0.'],blues:['0.1.2.1.0.1.2.1.','0.1.2.3.2.1.2.1.','0..1.2..1.2.1.0.'],
 soul:['0..1..2..1..2...','0..1.2..3.2..1.2','0.1.2.3.2.3.2.1.'],lofi:['0..1..2..1..2.1.','0..1.2..1.2..1.2','0.1.2.1.0.1.3.2.'],
 classical:['0.2.1.2.0.2.1.2.','0.1.2.1.0.1.2.1.','0123210301232103'],cinematic:['0.1.2.1.0.1.2.1.','0123012301230123','0.1.2.3.2.1.2.3.'],
 gospel:['0.1.2.3.2.1.0.1.','0.12.3.20.12.3.2','0.1.2.3.4.3.2.1.'],bossa:['0..1.2..0..1.2..','0..1..2.0..1..2.','0.1.2.1.0.1.2.1.']};
const RC=[[[[0,16]],[[0,8],[8,8]],[[0,6],[6,10]],[[0,4],[4,12]]],
 [[[0,4],[4,4],[8,8]],[[0,6],[6,2],[8,8]],[[0,4],[4,2],[6,2],[8,8]],[[0,3],[3,3],[6,2],[8,4],[12,4]],[[0,4],[4,4],[8,4],[12,4]]],
 [[[0,2],[2,2],[4,4],[8,2],[10,2],[12,4]],[[0,3],[3,3],[6,2],[8,3],[11,3],[14,2]],[[0,4],[4,2],[6,2],[8,2],[10,2],[12,2],[14,2]],[[0,2],[2,2],[4,2],[6,2],[8,4],[12,2],[14,2]]]];

/* ---------- Generi ---------- */
const GENRES={
 pop:{n:'Pop',desc:'Giri ciclici da 4 accordi, aggiunte di nona e sospensioni.',tempo:[90,128],swing:0,tmplP:.7,cycle:1,dens:0,mel:0,
  templates:[[0,4,5,3],[5,3,0,4],[0,5,3,4],[0,3,5,4],[5,4,3,4],[0,2,3,4]],ext:{s7:.15,s9:.1,sus:.12,add9:.3,six:0,s13:0},secdom:.1,tritone:0,split:0,harmV:.4,prog:0},
 rock:{n:'Rock',desc:'Accordi pieni su I–IV–V e prestiti dal bVII, ritmo in ottavi.',tempo:[80,150],swing:0,tmplP:.75,cycle:.8,dens:.2,mel:-.1,
  templates:[[0,3,4,3],[0,'b7',3,0],[0,4,3,4],[0,'b7','b6','b7'],[5,3,0,4],[0,'b7',3,4]],ext:{s7:.05,s9:0,sus:.2,add9:.05,six:0,s13:0},secdom:0,tritone:0,split:0,harmV:.1,prog:0},
 jazz:{n:'Jazz',desc:'Cicli ii–V–I, settime/none/tredicesime, dominanti secondarie e sostituzioni di tritono.',tempo:[70,170],swing:.6,tmplP:.55,cycle:2.4,dens:0,mel:.1,
  templates:[[1,4,0,5],[2,5,1,4],[0,5,1,4],[1,4,0,3],[0,2,5,1]],ext:{s7:.95,s9:.5,sus:.04,add9:0,six:.2,s13:.5},secdom:.35,tritone:.3,split:.45,harmV:1,prog:0,alt:.35,dim:1},
 blues:{n:'Blues',desc:'Struttura blues a 12 battute con accordi di settima dominante.',tempo:[70,130],swing:.8,tmplP:0,cycle:1,dens:.1,mel:0,blues:1,
  templates:[],ext:{s7:1,s9:.15,sus:0,add9:0,six:0,s13:.25},secdom:0,tritone:.08,split:0,harmV:1,prog:0,modeW:{major:.3,mixolydian:3,minor:1,dorian:1.5,lydian:0,phrygian:0,harmonic:0,}},
 soul:{n:'Soul / R&B',desc:'Settime e none, movimenti discendenti, groove sincopato.',tempo:[60,100],swing:.15,tmplP:.6,cycle:1.3,dens:.1,mel:.1,
  templates:[[0,2,3,4],[3,4,2,5],[1,4,0,5],[0,3,1,4],[5,1,4,0]],ext:{s7:.75,s9:.45,sus:.1,add9:.05,six:.1,s13:.15},secdom:.25,tritone:.05,split:.25,harmV:.7,prog:4},
 lofi:{n:'Lo‑fi',desc:'Accordi estesi maj9/m9, swing leggero e pochi movimenti.',tempo:[68,90],swing:.35,tmplP:.65,cycle:1.5,dens:-.1,mel:-.1,
  templates:[[1,4,0,5],[0,2,5,3],[3,2,5,1],[5,3,0,4],[0,5,1,4]],ext:{s7:.95,s9:.6,sus:.05,add9:.05,six:.1,s13:.1},secdom:.15,tritone:.1,split:.1,harmV:.8,prog:4,modeW:{dorian:1.4,minor:1.2,major:1,lydian:.6,mixolydian:.5,phrygian:.1,harmonic:.2}},
 classical:{n:'Classico',desc:'Funzioni tonali (T–S–D), dominante con settima, risoluzioni e basso albertino.',tempo:[60,120],swing:0,tmplP:.6,cycle:1.8,dens:-.1,mel:0,
  templates:[[0,4,5,2],[0,5,1,4],[0,3,4,0],[5,3,4,0]],ext:{s7:.2,s9:0,sus:.06,add9:0,six:0,s13:0},secdom:.2,tritone:0,split:.15,harmV:1,prog:0,modeW:{major:1.4,minor:1.4,harmonic:.8,dorian:.1,lydian:0,mixolydian:0,phrygian:0}},
 cinematic:{n:'Cinematico',desc:'Prestiti modali (bVI, bVII), sospensioni, bassi in ottava e arpeggi ampi.',tempo:[60,95],swing:0,tmplP:.75,cycle:.9,dens:-.1,mel:-.15,
  templates:[[5,3,0,4],[0,'b6','b7',0],[5,'b6','b7',4],[0,'b7','b6','b7'],[0,2,5,3],[5,3,'b7',0]],ext:{s7:.2,s9:.15,sus:.3,add9:.4,six:0,s13:0},secdom:.05,tritone:0,split:0,harmV:.3,prog:0,arpBoost:.25},
 gospel:{n:'Gospel',desc:'Cadenze plagali, sospensioni, dominanti secondarie e colori ricchi.',tempo:[60,100],swing:.3,tmplP:.6,cycle:1.4,dens:.15,mel:.1,
  templates:[[0,3,0,4],[0,5,1,4],[3,4,2,5],[0,2,3,4],[0,3,'iv',0]],ext:{s7:.7,s9:.5,sus:.35,add9:.15,six:.2,s13:.2},secdom:.35,tritone:0,split:.3,harmV:.8,prog:0},
 bossa:{n:'Bossa Nova',desc:'Settime morbide, ii–V che si susseguono, ritmo sincopato in 16mi.',tempo:[100,150],swing:0,tmplP:.65,cycle:2,dens:.05,mel:.05,
  templates:[[0,5,1,4],[1,4,0,5],[0,2,5,1],[0,'iv',2,5]],ext:{s7:.95,s9:.4,sus:0,add9:0,six:.3,s13:.2},secdom:.3,tritone:.25,split:.4,harmV:1,prog:0,dim:1,modeW:{major:1.5,minor:1.2,dorian:.5,lydian:.3,mixolydian:.2,phrygian:0,harmonic:.3}}};

/* ---------- Armonia ---------- */
const W=[[0,2,1,3,3,3,.3],[.5,0,.3,1,5,.5,1],[.3,1,0,3,.5,3,.3],[2,1.5,.5,0,3,1,.5],[5,.3,.5,1.5,0,3,.3],[.5,3,.5,3,2.5,0,.5],[3,.3,1,.5,.3,1,0]];

function genHarmony(o,rng,info){
  const G=GENRES[o.genre],M=MOODS[o.mood],mode=o.mode,n=o.bars,plain=!!o.plain;
  const allowDim=!!(G.dim||M.tense)&&!plain;
  const modal=['minor','dorian','mixolydian','phrygian'].includes(mode);
  const tokDeg=t=>typeof t==='string'?TOKDEG[t]:t;
  const mkDesc=t=>typeof t==='string'?{...TOK[t]}:degChord(mode,t);
  const nextDeg=(prev,loop)=>{const row=W[tokDeg(prev)],arr=[];
    for(let d=0;d<7;d++){let w=row[d]||0;if(d===(tokDeg(prev)+3)%7)w*=G.cycle;if(modal&&d===6)w*=2.5;
      const dc=degChord(mode,d);if(!allowDim&&(dc.tri==='dim'||dc.tri==='aug'))w*=.04;if(loop)w*=W[d][0]+.2;arr.push([d,w]);}
    return pickW(rng,arr);};
  const markov=len=>{
    const st=G.n==='Jazz'||G.n==='Bossa Nova'?[[1,.4],[0,.3],[2,.15],[5,.15]]:[[0,.65],[5,.2],[3,.1],[1,.05]];
    const a=[pickW(rng,st)];while(a.length<len)a.push(nextDeg(a[a.length-1],a.length===len-1));return a;};
  const template=()=>{let t=pick(rng,G.templates).slice();if(rng()<.35){const k=1+Math.floor(rng()*3);t=t.slice(k).concat(t.slice(0,k));}return t;};
  const phrase=len=>{if(G.templates.length&&rng()<G.tmplP){info.push('template '+G.n);return template();}info.push('catena di Markov funzionale');return markov(len);};
  const variation=p=>{const q=p.slice();q[2]=nextDeg(q[1],false);q[3]=nextDeg(q[2],true);return q;};

  let degs=[];
  if(G.blues){
    const B={4:[0,3,0,4],8:[0,0,3,3,0,0,4,4],12:[0,0,0,0,3,3,0,0,4,3,0,4],16:[0,0,0,0,3,3,0,0,4,3,0,4,0,3,0,4]};
    degs=B[n].slice();info.push('struttura blues a '+n+' battute');
  }else{
    const ph=[phrase(4)];
    for(let k=1;k<n/4;k++)ph.push(k%2===1?variation(ph[0]):phrase(4));
    degs=ph.flat().slice(0,n);
  }
  let ch=degs.map(mkDesc);
  // niente dim/aug dove non servono: sostituzione con un grado vicino
  ch=ch.map(c=>{if(c.deg!=null&&(c.tri==='dim'&&!allowDim||c.tri==='aug'&&!M.tense)){
    for(const d of [(c.deg+2)%7,(c.deg+4)%7]){const x=degChord(mode,d);if(x.tri!=='dim'&&x.tri!=='aug')return x;}}return c;});
  const majorish=degChord(mode,0).tri==='maj';
  // prestiti modali
  if(!plain)ch=ch.map(c=>{
    if(majorish&&c.deg===3&&rng()<M.borrow){info.push('prestito modale: iv minore');return{r:5,tri:'min',sev:'m7',deg:null};}
    if(majorish&&c.deg===5&&rng()<M.borrow*.6){info.push('prestito modale: bVI');return{...TOK.b6,deg:null};}
    return c;});
  // dominanti secondarie
  if(!plain){let last=-9;for(let i=1;i<n-1;i++){const nx=ch[i+1],c=ch[i];
    if(i-last>1&&nx.deg!=null&&nx.tri!=='dim'&&nx.tri!=='aug'&&c.deg!==4&&rng()<G.secdom*(.5+M.tension)){
      ch[i]={r:(nx.r+7)%12,tri:'maj',sev:'dom7',deg:null,sec:nx};last=i;info.push('dominante secondaria');}}}
  // dominante armonica (V maggiore con settima)
  ch.forEach(c=>{if(c.deg===4&&c.tri!=='maj'&&rng()<(plain?.7:G.harmV+(M.tense?.3:0))){c.tri='maj';c.sev='dom7';info.push('dominante armonica (V maggiore)');}});
  // qualità / estensioni
  const jit=(rng()-.5)*.3,E=G.ext,adj=x=>clamp(x+M.ext+jit,0,1);
  const P={s7:plain?0:(E.s7>=1?1:adj(E.s7)),s9:plain?0:adj(E.s9*1.1),sus:plain?0:E.sus,add9:plain?0:clamp(E.add9+M.ext*.4,0,1),six:plain?0:E.six,s13:plain?0:E.s13,tense:M.tense};
  const domQ=c=>{if(!plain&&G.alt&&(c.sec||c.deg===4&&c.tri==='maj'&&!majorish)&&rng()<G.alt)return rng()<.6?'7b9':'7#9';
    if(P.s7<.5&&!G.blues)return 'dom7';
    return rng()<P.s13*.5?'dom13':rng()<P.s9?'dom9':'dom7';};
  const pickQ=c=>{
    if(c.tri==='maj'&&c.sev==='dom7'){
      if(c.sec||c.deg===4||G.blues||(c.deg==null&&c.r===10&&rng()<.4))return rng()<(P.s7>=.5||c.sec?1:.15)?domQ(c):'maj';
      if(rng()<P.s7*.7)return domQ(c);if(rng()<P.sus)return 'sus4';return 'maj';}
    if(c.tri==='maj'){
      if(rng()<P.s7)return rng()<P.s9?'maj9':(rng()<P.six*.5?'6':'maj7');
      const y=rng();if(y<P.sus)return rng()<.5?'sus2':'sus4';if(y<P.sus+P.add9)return 'add9';if(rng()<P.six)return '6';return 'maj';}
    if(c.tri==='min'){
      if(c.sev==='mmaj7'&&P.tense&&rng()<.6)return 'mmaj7';
      if(rng()<P.s7)return rng()<P.s9?'m9':'m7';if(rng()<P.add9*.4)return 'madd9';return 'min';}
    if(c.tri==='dim')return P.tense&&rng()<.5?'dim7':(rng()<P.s7?'m7b5':'dim');
    return 'aug';};
  ch.forEach(c=>{c.q=G.blues?(c.tri==='maj'?(rng()<P.s13*.5&&c.deg===0?'dom13':'dom7'):'m7'):pickQ(c);c.beats=4;});
  // ritmo armonico: seconda dominante in mezzo alla battuta
  let flat=[];
  ch.forEach((c,i)=>{
    const nx=ch[(i+1)%n];
    if(rng()<G.split&&nx.tri!=='dim'&&(nx.r+7)%12!==c.r&&!DOMQ.has(c.q)){
      c.beats=2;flat.push(c);
      flat.push({r:(nx.r+7)%12,tri:'maj',sev:'dom7',deg:null,sec:nx,q:domQ({sec:nx}),beats:2});info.push('turnaround a metà battuta');
    }else flat.push(c);});
  // sostituzione di tritono
  flat.forEach((c,i)=>{const nx=flat[i+1];
    if(nx&&DOMQ.has(c.q)&&(c.r+5)%12===nx.r&&rng()<G.tritone){c.r=(c.r+6)%12;c.q=rng()<.5?'dom9':'dom7';c.tri='tt';c.sec=null;c.tt=1;info.push('sostituzione di tritono');}});
  let t=0;flat.forEach(c=>{c.start=t;t+=c.beats;c.pc=(o.key+c.r)%12;
    c.pcs=new Set(QT[c.q].iv.map(x=>(c.pc+x)%12));c.name=chordName(c.pc,c.q);});
  flat.forEach(c=>{c.rn=c.sec?'V'+(QT[c.q].rn||'')+'/'+numeral(c.sec.r,c.sec.q||'maj',true):numeral(c.r,c.q);});
  return flat;
}

/* ---------- Voicing ---------- */
function voiceLead(c,last,lo,hi){
  const pcs=QT[c.q].iv.map(x=>(c.pc+x)%12);let best=null,bs=1e9;
  for(let inv=0;inv<pcs.length;inv++)for(let base=lo;base<=lo+9;base++){
    const a=[];let p=base-1;
    for(let j=0;j<pcs.length;j++){let m=p+1;while(m%12!==pcs[(inv+j)%pcs.length])m++;a.push(m);p=m;}
    if(a[a.length-1]>hi)continue;
    let s;
    if(last)s=a.reduce((x,m)=>x+Math.min(...last.map(l=>Math.abs(m-l))),0)+2*Math.abs(a[a.length-1]-last[last.length-1]);
    else s=Math.abs(a.reduce((x,m)=>x+m,0)/a.length-(lo+hi)/2);
    if(s<bs){bs=s;best=a;}
  }
  return best||pcs.map((_,i)=>lo+i*3);
}
const bassMidi=pc=>{let m=36+pc;if(m>43)m-=12;return m;};

/* ---------- Costruzione eventi ---------- */
function buildEvents(S,rng){
  const{chords,bars,G,M,out,lh,human}=S,total=bars*4,ev=[];
  const sw=st=>((st%4)>=2?S.swing*.1667:0);
  const jit=()=>human?(rng()-.5)*.03:0,vj=()=>human?(rng()-.5)*14:0;
  const V=(d,k)=>clamp(Math.round((20+d*8)*M.vel*(k||1)+vj()),25,122);
  const comp=out==='compose';
  const lo=comp?50:(lh?52:55),hi=comp?70:(lh?72:77);
  let last=null;chords.forEach(c=>{c.v=voiceLead(c,last,lo,hi);last=c.v;});
  const at=t=>{for(let i=chords.length-1;i>=0;i--)if(chords[i].start<=t+1e-6)return chords[i];return chords[0];};
  const dens=clamp(M.dens+G.dens+(rng()-.5)*.2,0,1),tier=dens<.33?0:dens<.66?1:2;
  const pat=(tab,t)=>tab[clamp(t,0,2)];
  const useArp=out==='arp'||(comp&&rng()<clamp(M.arp+(G.arpBoost||0)-.1,0,.9));
  const gid=Object.keys(GENRES).find(k=>GENRES[k]===G);
  const add=(t,d,n,v,tr)=>{ev.push({t:Math.max(0,t),d:Math.max(.08,d),n,v,tr});};
  // --- mano destra
  for(let b=0;b<bars;b++){
    const fill=b%4===3&&rng()<.6,tr=clamp(tier+(fill?1:0),0,2);
    const p=useArp?pat(ARP[gid],tr):pat(COMP[gid],tr);
    const starts=chords.filter(c=>c.start>=b*4&&c.start<b*4+4).map(c=>Math.round((c.start-b*4)*4));
    const hits=[];
    for(let s=0;s<16;s++){
      if(p[s]!=='.')hits.push({s,k:p[s]});
      else if(starts.includes(s)&&s>0)hits.push({s,k:useArp?'0':'8'});
    }
    if(!hits.find(h=>h.s===0))hits.unshift({s:0,k:useArp?'0':'9'});
    hits.sort((a,c)=>a.s-c.s);
    hits.forEach((h,i)=>{
      const t0=b*4+h.s*.25,c=at(t0),endChord=c.start+c.beats;
      const nextS=i+1<hits.length?b*4+hits[i+1].s*.25:b*4+4,t=t0+sw(h.s)+jit();
      if(useArp){
        const T=(lh||comp)?c.v:[48+c.pc%12,...c.v],idx=+h.k,L=T.length;
        const n=T[idx%L]+12*Math.floor(idx/L);
        const d=Math.min(Math.max(nextS-t0,.25)*2,endChord-t0);
        add(t,d,n,V(h.s%4===0?8:6),0);
      }else{
        const dv=+h.k,full=dv>=7||h.s===0||starts.includes(h.s),ns=full?c.v:c.v.slice(-2);
        const d=Math.min(nextS-t0,endChord-t0)*.97;
        ns.forEach((n,j)=>add(t+j*(human?.009:0),d,n,V(dv,j===ns.length-1?1.12:.9),0));
      }
    });
  }
  // --- basso (mano sinistra)
  if(lh||comp){
    for(let b=0;b<bars;b++){
      const p=pat(BASS[gid],clamp(tier+(rng()<.2?1:0),0,2));
      const starts=chords.filter(c=>c.start>=b*4&&c.start<b*4+4).map(c=>Math.round((c.start-b*4)*4));
      const hits=[];for(let s=0;s<16;s++){if(p[s]!=='.')hits.push({s,k:p[s]});else if(starts.includes(s))hits.push({s,k:'R'});}
      hits.sort((a,c)=>a.s-c.s);
      hits.forEach((h,i)=>{
        const t0=b*4+h.s*.25,c=at(t0),nc=at(Math.min(t0+1,total-.01)),bm=bassMidi(c.pc);
        let n=bm;
        if(h.k==='5')n=bm+7;else if(h.k==='8')n=bm+12;else if(h.k==='6')n=bm+9;
        else if(h.k==='3')n=bm+(QT[c.q].iv[1]);
        else if(h.k==='a'){const nb=bassMidi(chords[(chords.indexOf(c)+1)%chords.length].pc);n=nb+(rng()<.5?-1:1);}
        const nextS=i+1<hits.length?b*4+hits[i+1].s*.25:b*4+4,end=Math.min(nextS,c.start+c.beats);
        add(t0+sw(h.s)+jit(),(end-t0)*.92,n,V(h.k==='R'?8:6,1.05),1);
        if(M.oct&&h.k==='R')add(t0+sw(h.s)+jit(),(end-t0)*.92,n+12,V(6),1);
      });
    }
  }
  // --- melodia
  if(comp)genMelody(S,rng,ev,at,sw,tier);
  // evita note uguali sovrapposte (stesso tasto, stessa traccia)
  const by={};ev.forEach(e=>{(by[e.tr+'_'+e.n]=by[e.tr+'_'+e.n]||[]).push(e);});
  Object.values(by).forEach(a=>{a.sort((x,y)=>x.t-y.t);for(let i=0;i<a.length-1;i++)a[i].d=Math.min(a[i].d,a[i+1].t-a[i].t-.01);});
  ev.forEach(e=>{if(e.t+e.d>total+.5)e.d=Math.max(.08,total+.5-e.t);});
  return ev.filter(e=>e.d>.02).sort((a,b)=>a.t-b.t);
}

function genMelody(S,rng,ev,at,sw,tier0){
  const{bars,key,mode,G,M,human}=S;
  const scs=new Set(SC[mode]);const list=[];for(let m=64;m<=91;m++)if(scs.has((((m-key)%12)+12)%12))list.push(m);
  const center=76+M.reg;
  const near=p=>{let bi=0;list.forEach((m,i)=>{if(Math.abs(m-p)<Math.abs(list[bi]-p))bi=i;});return bi;};
  const snap=(idx,ch)=>{for(let k=0;k<6;k++)for(const s of [idx+k,idx-k]){if(s>=0&&s<list.length&&ch.pcs.has(list[s]%12))return s;}return clamp(idx,0,list.length-1);};
  const dens=clamp(M.dens+G.mel+(rng()-.5)*.25,0,1),tier=dens<.33?0:dens<.66?1:2;
  const rA=pick(rng,RC[tier]),rB=pick(rng,RC[clamp(tier+(rng()<.5?1:-1),0,2)]);
  let cur=near(center),dir=1,first=true,motif=[];
  for(let b=0;b<bars;b++){
    const pos=b%4,cell=pos===0?rA:pos===1?(rng()<.7?rA:rB):pos===2?rB:(rng()<.6?rA:rB);
    const reuse=pos>=1&&cell===rA&&motif.length===rA.length&&rng()<(pos===1?.7:.5);
    const shift=reuse?pick(rng,pos===3?[0,0,-1]:[-1,0,0,1,2]):0,drift=[0,2,4,-2][pos],idxs=[];
    cell.forEach(([st,len],k)=>{
      const s=b*16+st,t=s*.25,ch=at(t),strong=st%4===0||len>=4,lastNote=b===bars-1&&k===cell.length-1;
      let idx;
      if(first){idx=snap(near(center),ch);first=false;}
      else if(reuse){idx=motif[k]+shift;if(strong)idx=snap(idx,ch);}
      else if(strong){
        const c=[];for(let i=Math.max(0,cur-4);i<=Math.min(list.length-1,cur+4);i++){
          if(!ch.pcs.has(list[i]%12))continue;
          let w=Math.exp(-Math.abs(i-cur)/1.8)*Math.exp(-Math.abs(list[i]-(center+drift))/9);
          if(i===cur)w*=.35;if((i-cur)*dir>0)w*=1.25;c.push([i,w]);}
        idx=c.length?pickW(rng,c):clamp(cur+dir,0,list.length-1);
      }else{
        const x=rng();let dd=x<.62?dir:x<.74?-dir:x<.88?2*dir:0;idx=cur+dd;
        if(list[clamp(idx,0,list.length-1)]>center+drift+8)dir=-1;else if(list[clamp(idx,0,list.length-1)]<center+drift-8)dir=1;
      }
      if(lastNote)idx=snap(idx,ch);
      idx=clamp(idx,0,list.length-1);
      if(idx!==cur)dir=idx>cur?1:-1;cur=idx;idxs.push(idx);
      if(!strong&&tier>0&&rng()<.07)return;
      const v=clamp(Math.round((strong?84:70)*M.vel+(human?(rng()-.5)*10:0)+(pos===2?4:0)),30,120);
      ev.push({t:Math.max(0,t+sw(st)+(human?(rng()-.5)*.02+.012:0)),d:Math.max(.12,len*.25*(strong?.95:.85)),n:list[idx],v,tr:2});
    });
    if(pos===0)motif=idxs.slice();
  }
}

/* ---------- Generazione completa ---------- */
function generate(o){
  const seed=o.seed,hr=mulberry(seed),vr=mulberry(seed*31+(o.vseed||0)*7919+1);
  const G=GENRES[o.genre],M=MOODS[o.mood];
  const key=o.key==='auto'||o.key==null?Math.floor(hr()*12):+o.key;
  let mw=M.modes.map(([m,w])=>[m,w*(G.modeW&&G.modeW[m]!=null?G.modeW[m]:1)]);
  if(mw.every(x=>x[1]<=0))mw=Object.keys(G.modeW||{major:1}).map(m=>[m,G.modeW?G.modeW[m]:1]);
  const mode=o.mode||pickW(hr,mw);
  let bars=o.bars||4;if(G.blues&&![4,8,12,16].includes(bars))bars=12;
  const bpm=Math.round(clamp((G.tempo[0]+(G.tempo[1]-G.tempo[0])*hr())*M.tempo,50,170));
  const info=[];
  const chords=genHarmony({genre:o.genre,mood:o.mood,mode,bars,key,plain:o.plain},hr,info);
  const S={chords,bars,key,mode,G,M,out:o.out||'chords',lh:!!o.lh,human:o.human!==false,swing:G.swing};
  const events=buildEvents(S,vr);
  const beats=chords.reduce((a,c)=>a+c.beats,0);
  const uniq=[...new Set(info)];
  return{seed,vseed:o.vseed||0,key,mode,bpm,bars,beats,chords,events,info:uniq,genre:o.genre,mood:o.mood,out:S.out,lh:S.lh,swing:G.swing,prog:G.prog};
}

/* ---------- Esportazione MIDI (SMF formato 1, 480 PPQ) ---------- */
function vlq(n){const b=[n&127];while(n>>=7)b.unshift((n&127)|128);return b;}
function toMidi(song,bpm){
  const PPQ=480,txt=s=>[...new TextEncoder().encode(s)];
  const meta=(t,d)=>[0xFF,t,...vlq(d.length),...d];
  const chunk=(id,d)=>[...id].map(c=>c.charCodeAt(0)).concat([d.length>>>24&255,d.length>>>16&255,d.length>>>8&255,d.length&255],d);
  const body=list=>{list.sort((a,b)=>a.tick-b.tick||a.o-b.o);let p=0,o=[];list.forEach(e=>{o.push(...vlq(e.tick-p),...e.b);p=e.tick;});o.push(0,0xFF,0x2F,0);return o;};
  const mpq=Math.round(60000000/bpm);
  const sf=SF[(song.key+MODE_SHIFT[song.mode])%12];
  const t0=[{tick:0,o:0,b:meta(3,txt(`${GENRES[song.genre].n} · ${MOODS[song.mood].n}`))},
    {tick:0,o:0,b:meta(0x51,[mpq>>16&255,mpq>>8&255,mpq&255])},{tick:0,o:0,b:meta(0x58,[4,2,24,8])},{tick:0,o:0,b:meta(0x59,[sf&255,0])}];
  song.chords.forEach(c=>t0.push({tick:Math.round(c.start*PPQ),o:1,b:meta(6,txt(c.name))}));
  const names=['Piano — accordi','Piano — basso','Piano — melodia'],tracks=[chunk('MTrk',body(t0))];
  [0,1,2].forEach(tr=>{
    const es=song.events.filter(e=>e.tr===tr);if(!es.length)return;
    const l=[{tick:0,o:0,b:meta(3,txt(names[tr]))},{tick:0,o:0,b:[0xC0|tr,song.prog]}];
    es.forEach(e=>{const a=Math.round(e.t*PPQ),z=Math.max(a+1,Math.round((e.t+e.d)*PPQ));
      l.push({tick:a,o:1,b:[0x90|tr,e.n,e.v]},{tick:z,o:0,b:[0x80|tr,e.n,0]});});
    tracks.push(chunk('MTrk',body(l)));});
  return new Uint8Array([...chunk('MThd',[0,1,0,tracks.length,PPQ>>8,PPQ&255]),...tracks.flat()]);
}
if(typeof module!=='undefined')module.exports={GENRES,MOODS,COMP,BASS,ARP,RC,QT,SC,generate,toMidi,degChord,numeral,chordName,NOTES,MODE_NAME,mulberry,pick,pickW,voiceLead,bassMidi};
