'use strict';
/* =====================================================================
   TEORIA — un corso in moduli. Ogni lezione si legge nella tonalità scelta
   (tutti i nomi, le tabelle e gli esempi si trasportano) e ogni esempio si ascolta
   e si vede sulla tastiera. In più: esploratore di scale (anche dal mondo, con i quarti di tono).
   ===================================================================== */
const Theory=(()=>{
  /* ---------- nomi e ortografia ---------- */
  const IT=['Do','Re','Mi','Fa','Sol','La','Si'],LET=['C','D','E','F','G','A','B'],LPC=[0,2,4,5,7,9,11];
  const ACC={'-2':'𝄫','-1':'♭','0':'','1':'♯','2':'𝄪'};
  // tonica: lettera e alterazione preferite per ogni altezza
  const TON=[[0,0],[1,-1],[1,0],[2,-1],[2,0],[3,0],[3,1],[4,0],[5,-1],[5,0],[6,-1],[6,0]];
  const FLATKEY=new Set([1,3,5,8,10]);
  const SH=['Do','Do♯','Re','Re♯','Mi','Fa','Fa♯','Sol','Sol♯','La','La♯','Si'],FL=['Do','Re♭','Re','Mi♭','Mi','Fa','Sol♭','Sol','La♭','La','Si♭','Si'];
  const SHL=['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'],FLL=['C','D♭','D','E♭','E','F','G♭','G','A♭','A','B♭','B'];
  const mod=x=>((x%12)+12)%12;
  // ortografia di una scala di 7 note: una lettera per grado
  function spell7(k,ivs,lat){const [l0]=TON[k];return ivs.map((iv,i)=>{const L=(l0+i)%7,pc=mod(k+Math.round(iv)),d=((pc-LPC[L]+18)%12)-6,q=iv%1?(iv%1>0?'½':''):'';
    return (lat?LET:IT)[L]+(Math.abs(d)<=2?ACC[d]:'?')+(iv%1?'↑¼':'');});}
  /* ---------- scale ---------- */
  const SC=[
    // occidentali
    {id:'major',n:'Maggiore (ionio)',c:'Maggiori e minori',iv:[0,2,4,5,7,9,11],d:'La scala di riferimento: luminosa, stabile.',u:'pop, rock, classica, quasi tutto'},
    {id:'minor',n:'Minore naturale (eolio)',c:'Maggiori e minori',iv:[0,2,3,5,7,8,10],d:'Malinconica; stesse note della maggiore relativa.',u:'pop, rock, trap, cinematica'},
    {id:'harmonic',n:'Minore armonica',c:'Maggiori e minori',iv:[0,2,3,5,7,8,11],d:'7ª alzata: dominante maggiore, salto di 1½ tono tra 6ª e 7ª, sapore “orientale”.',u:'classica, metal, flamenco, trap scura'},
    {id:'melodic',n:'Minore melodica',c:'Maggiori e minori',iv:[0,2,3,5,7,9,11],d:'6ª e 7ª alzate: minore che sale liscio. Nel jazz si usa sia salendo sia scendendo.',u:'jazz, classica'},
    {id:'dorian',n:'Dorico',c:'Modi',iv:[0,2,3,5,7,9,10],d:'Minore con la 6ª maggiore: scuro ma “aperto”.',u:'soul, funk, jazz, lo-fi'},
    {id:'phrygian',n:'Frigio',c:'Modi',iv:[0,1,3,5,7,8,10],d:'Minore con la 2ª minore: cupo, spagnolo.',u:'trap, metal, flamenco'},
    {id:'lydian',n:'Lidio',c:'Modi',iv:[0,2,4,6,7,9,11],d:'Maggiore con la 4ª aumentata: sospeso, sognante.',u:'colonne sonore, dream pop'},
    {id:'mixolydian',n:'Misolidio',c:'Modi',iv:[0,2,4,5,7,9,10],d:'Maggiore con la 7ª minore: festoso, blues.',u:'rock, blues, folk, funk'},
    {id:'locrian',n:'Locrio',c:'Modi',iv:[0,1,3,5,6,8,10],d:'Tonica diminuita: instabile, raramente usata come centro.',u:'metal, jazz (sul m7♭5)'},
    {id:'pmaj',n:'Pentatonica maggiore',c:'Pentatoniche e blues',iv:[0,2,4,7,9],d:'Maggiore senza 4ª e 7ª: nessun semitono, impossibile “sbagliare”.',u:'country, pop, soul, musica cinese (gong)'},
    {id:'pmin',n:'Pentatonica minore',c:'Pentatoniche e blues',iv:[0,3,5,7,10],d:'La scala degli assoli rock e blues.',u:'blues, rock, hip hop, R&B'},
    {id:'blues',n:'Blues (minore)',c:'Pentatoniche e blues',iv:[0,3,5,6,7,10],d:'Pentatonica minore + la “blue note” (♭5).',u:'blues, jazz, rock'},
    {id:'bluesmaj',n:'Blues maggiore',c:'Pentatoniche e blues',iv:[0,2,3,4,7,9],d:'Pentatonica maggiore + ♭3 di passaggio.',u:'country, soul, gospel'},
    {id:'bebop',n:'Bebop dominante',c:'Jazz',iv:[0,2,4,5,7,9,10,11],d:'Misolidio + 7ª maggiore di passaggio: 8 note, le note dell’accordo cadono sui battiti.',u:'jazz'},
    {id:'lyddom',n:'Lidio dominante',c:'Jazz',iv:[0,2,4,6,7,9,10],d:'4ª aumentata e 7ª minore: dominante “aperta” (sul ♭VII7, sul subV).',u:'jazz, fusion, colonne sonore'},
    {id:'altered',n:'Alterata (superlocria)',c:'Jazz',iv:[0,1,3,4,6,8,10],d:'Tutte le tensioni alterate su una dominante: ♭9 ♯9 ♯11 ♭13.',u:'jazz (V7alt)'},
    {id:'whole',n:'Esatonale (toni interi)',c:'Simmetriche',iv:[0,2,4,6,8,10],d:'Solo toni: niente centro, effetto sogno/sospensione.',u:'Debussy, colonne sonore'},
    {id:'dimHW',n:'Diminuita semitono-tono',c:'Simmetriche',iv:[0,1,3,4,6,7,9,10],d:'Alterna semitono e tono: sulle dominanti 7♭9.',u:'jazz, colonne sonore'},
    {id:'dimWH',n:'Diminuita tono-semitono',c:'Simmetriche',iv:[0,2,3,5,6,8,9,11],d:'Alterna tono e semitono: sugli accordi diminuiti.',u:'jazz, metal'},
    // dal mondo
    {id:'phrdom',n:'Frigia dominante (spagnola, ebraica, Hijaz)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1,4,5,7,8,10],d:'5° modo della minore armonica: 2ª minore e 3ª maggiore. Chiamata Freygish/Ahava Raba nella musica ebraica, maqam Hijaz in quella araba.',u:'flamenco, klezmer, musica araba, trap'},
    {id:'dharm',n:'Doppia armonica (bizantina, “araba”)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1,4,5,7,8,11],d:'Due seconde aumentate simmetriche. Maqam Hijaz Kar, raga Bhairav.',u:'musica araba, gitana, colonne sonore'},
    {id:'hijazkar',n:'Maqam Hijaz Kar',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1,4,5,7,8,11],d:'Come la doppia armonica: due tetracordi Hijaz.',u:'musica araba e turca'},
    {id:'rast',n:'Maqam Rast (quarti di tono)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,2,3.5,5,7,9,10.5],d:'3ª e 7ª “neutre”, a metà tra maggiore e minore (¼ di tono). Il maqam “madre” della musica araba.',u:'musica araba classica'},
    {id:'bayati',n:'Maqam Bayati (quarti di tono)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1.5,3,5,7,8,10],d:'2ª a tre quarti di tono: dolce e malinconico.',u:'musica araba, turca'},
    {id:'saba',n:'Maqam Saba (quarti di tono)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1.5,3,4,7,8,10],d:'4ª diminuita: il maqam della tristezza.',u:'musica araba'},
    {id:'nahawand',n:'Maqam Nahawand',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,2,3,5,7,8,11],d:'Equivale alla minore armonica.',u:'musica araba, turca'},
    {id:'kurd',n:'Maqam Kurd',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1,3,5,7,8,10],d:'Equivale al frigio.',u:'musica araba, curda'},
    {id:'persian',n:'Persiana',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,1,4,5,6,8,11],d:'2ª minore, 3ª maggiore, 5ª diminuita: molto tesa ed esotica.',u:'colonne sonore, metal'},
    {id:'egyptian',n:'Egiziana (pentatonica sospesa)',c:'Dal mondo · Mediterraneo e Medio Oriente',iv:[0,2,5,7,10],d:'Pentatonica senza terza: né maggiore né minore. Corrisponde anche al modo cinese Shang.',u:'musica egiziana, folk, ambient'},
    {id:'romanian',n:'Rumena (dorico ♯4)',c:'Dal mondo · Europa dell’Est',iv:[0,2,3,6,7,9,10],d:'Dorico con la 4ª aumentata: tipica della doina rumena e del klezmer (Misheberekh), maqam Nikriz.',u:'folk rumeno, klezmer, gypsy'},
    {id:'hungmin',n:'Ungherese minore (gitana)',c:'Dal mondo · Europa dell’Est',iv:[0,2,3,6,7,8,11],d:'Minore armonica con la 4ª aumentata: due seconde aumentate.',u:'musica gitana, Liszt'},
    {id:'hungmaj',n:'Ungherese maggiore',c:'Dal mondo · Europa dell’Est',iv:[0,3,4,6,7,9,10],d:'2ª aumentata, 4ª aumentata, 7ª minore.',u:'musica ungherese, jazz manouche'},
    {id:'neapmin',n:'Napoletana minore',c:'Dal mondo · Europa dell’Est',iv:[0,1,3,5,7,8,11],d:'Minore armonica con la 2ª minore.',u:'classica, canzone napoletana'},
    {id:'neapmaj',n:'Napoletana maggiore',c:'Dal mondo · Europa dell’Est',iv:[0,1,3,5,7,9,11],d:'Minore melodica con la 2ª minore.',u:'classica'},
    {id:'gong',n:'Cinese Gong (pentatonica maggiore)',c:'Dal mondo · Asia orientale',iv:[0,2,4,7,9],d:'Il sistema cinese usa cinque note; i cinque modi partono da ognuna (gong, shang, jue, zhi, yu).',u:'musica tradizionale cinese'},
    {id:'jue',n:'Cinese Jue',c:'Dal mondo · Asia orientale',iv:[0,3,5,8,10],d:'Terzo modo cinese: scuro, sospeso.',u:'musica cinese'},
    {id:'zhi',n:'Cinese Zhi',c:'Dal mondo · Asia orientale',iv:[0,2,5,7,9],d:'Quarto modo cinese: aperto, senza terza.',u:'musica cinese'},
    {id:'yu',n:'Cinese Yu (pentatonica minore)',c:'Dal mondo · Asia orientale',iv:[0,3,5,7,10],d:'Quinto modo cinese, uguale alla pentatonica minore.',u:'musica cinese'},
    {id:'hirajoshi',n:'Giapponese Hirajoshi',c:'Dal mondo · Asia orientale',iv:[0,2,3,7,8],d:'Pentatonica con semitoni: il suono del koto.',u:'musica giapponese, colonne sonore'},
    {id:'insen',n:'Giapponese In (Miyako-bushi)',c:'Dal mondo · Asia orientale',iv:[0,1,5,7,8],d:'Scura e raffinata, con la 2ª minore.',u:'musica giapponese tradizionale'},
    {id:'yo',n:'Giapponese Yo',c:'Dal mondo · Asia orientale',iv:[0,2,5,7,9],d:'Pentatonica senza semitoni: canti popolari.',u:'folk giapponese'},
    {id:'kumoi',n:'Giapponese Kumoi',c:'Dal mondo · Asia orientale',iv:[0,2,3,7,9],d:'Minore dolce, a cinque note.',u:'musica giapponese'},
    {id:'iwato',n:'Giapponese Iwato',c:'Dal mondo · Asia orientale',iv:[0,1,5,6,10],d:'La più tesa: 2ª minore e 5ª diminuita.',u:'musica giapponese, metal'},
    {id:'pelog',n:'Balinese Pelog (approssimata)',c:'Dal mondo · Asia orientale',iv:[0,1,3,7,8],d:'Scala dei gamelan; le intonazioni reali variano da orchestra a orchestra.',u:'gamelan'},
    {id:'slendro',n:'Giavanese Slendro (approssimata)',c:'Dal mondo · Asia orientale',iv:[0,2.4,4.8,7.2,9.6],d:'Cinque intervalli quasi uguali (circa 240 cent): fuori dal temperamento occidentale.',u:'gamelan'},
    {id:'bhairav',n:'Raga Bhairav',c:'Dal mondo · India',iv:[0,1,4,5,7,8,11],d:'Raga dell’alba, solenne: 2ª e 6ª minori. Stesse altezze della doppia armonica.',u:'musica classica indiana'},
    {id:'yaman',n:'Raga Yaman',c:'Dal mondo · India',iv:[0,2,4,6,7,9,11],d:'Raga della sera, devozionale: come il lidio.',u:'musica classica indiana, Bollywood'},
    {id:'kafi',n:'Raga Kafi',c:'Dal mondo · India',iv:[0,2,3,5,7,9,10],d:'Come il dorico: romantico, popolare.',u:'musica indiana, folk'},
    {id:'bhairavi',n:'Raga Bhairavi',c:'Dal mondo · India',iv:[0,1,3,5,7,8,10],d:'Come il frigio: il raga della chiusura dei concerti.',u:'musica classica indiana'},
    {id:'todi',n:'Raga Todi',c:'Dal mondo · India',iv:[0,1,3,6,7,8,11],d:'Molto tesa e intensa: 2ª, 3ª e 6ª minori con 4ª aumentata.',u:'musica classica indiana'},
    {id:'purvi',n:'Raga Purvi',c:'Dal mondo · India',iv:[0,1,4,6,7,8,11],d:'Raga del tramonto: 2ª e 6ª minori, 4ª aumentata.',u:'musica classica indiana'},
    {id:'marwa',n:'Raga Marwa',c:'Dal mondo · India',iv:[0,1,4,6,7,9,11],d:'Senza 5ª nella pratica: inquieta, del crepuscolo.',u:'musica classica indiana'},
    {id:'enigmatic',n:'Enigmatica (Verdi)',c:'Curiosità',iv:[0,1,4,6,8,10,11],d:'Inventata da Verdi per l’Ave Maria: ambigua, cromatica.',u:'classica'},
    {id:'prometheus',n:'Prometeo (Skrjabin)',c:'Curiosità',iv:[0,2,4,6,9,10],d:'L’“accordo mistico” di Skrjabin disteso in scala.',u:'classica del ’900'}];
  const SCM=Object.fromEntries(SC.map(s=>[s.id,s]));
  const IVLAB=['1','♭2','2','♭3','3','4','♯4','5','♭6','6','♭7','7'];
  const ivLab=x=>x%1?(x<1.6?'2½♭':x<4?'3½♭':x<11?'7½♭':'¼'):IVLAB[x];

  /* ---------- contesto della tonalità scelta ---------- */
  function ctxOf(k){
    const flat=FLATKEY.has(k);
    const nm=pc=>(flat?FL:SH)[mod(pc)],nl=pc=>(flat?FLL:SHL)[mod(pc)];
    const m=(off,oct=4)=>12*(oct+1)+k+off;               // nota relativa alla tonica (oct = ottava della tonica)
    const lowOct=k>=7?2:3;                                 // ottava degli accordi (resta nel registro centrale)
    const ch=(off,q,inv=0,oct=lowOct)=>{let r=12*(oct+1)+k+off;while(r>=12*(oct+1)+12)r-=12;while(r<12*(oct+1))r+=12;let n=QT[q].iv.map(x=>r+x);for(let i=0;i<inv;i++)n.push(n.shift()+12);return n;};
    const cn=(off,q)=>nl(k+off)+QT[q].n;                   // sigla (C, Dm7…)
    const sc=id=>spell7(k,SCM[id].iv);
    return{k,flat,nm,nl,m,ch,cn,sc,ton:nm(k),tonL:nl(k)};
  }
  /* ---------- pezzi di interfaccia ---------- */
  const J=x=>JSON.stringify(x).replace(/"/g,'&quot;');
  const ex=(label,notes,arp)=>`<button class="tex" data-n="${J(notes)}"${arp?' data-arp="1"':''}>▶${label==='▶'?'':' '+label}</button>`;
  const seq=(label,chords,names)=>`<button class="tex tseq" data-seq="${J(chords)}"${names?` data-names="${J(names)}"`:''}>▶${label==='▶'?'':' '+label}</button>`;
  const rhy=(label,pat,bpm,notes,beats)=>`<button class="tex" data-rhy="${J({pat,bpm:bpm||90,notes:notes||[38],beats:beats||4})}">▶${label==='▶'?'':' '+label}</button>`;
  const tip=t=>`<div class="ttip">💡 ${t}</div>`;
  const tbl=(head,rows)=>`<div class="ttbl"><table><thead><tr>${head.map(h=>`<th>${h}</th>`).join('')}</tr></thead><tbody>${rows.map(r=>`<tr>${r.map(c=>`<td>${c}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`;
  const ex2=x=>`<div class="texs">${x.join('')}</div>`;
  const h=t=>`<h4 class="th4">${t}</h4>`;
  // accordi diatonici di una scala di 7 note (terze sovrapposte nella scala)
  function diatonic(X,id,sev){const iv=SCM[id].iv;return iv.map((r,i)=>{const t3=mod(iv[(i+2)%7]-r),t5=mod(iv[(i+4)%7]-r),t7=mod(iv[(i+6)%7]-r);
    let q=t3===4&&t5===7?'maj':t3===3&&t5===7?'min':t3===3&&t5===6?'dim':t3===4&&t5===8?'aug':t3===4&&t5===6?'maj':'min';
    if(sev)q=q==='maj'?(t7===11?'maj7':'7'):q==='min'?(t7===10?'m7':'mmaj7'):q==='dim'?(t7===10?'m7b5':'dim7'):'maj7';
    const RN=['I','II','III','IV','V','VI','VII'][i],acc=mod(r-[0,2,4,5,7,9,11][i]),pre=acc===11?'♭':acc===1?'♯':acc===10?'♭♭':'';
    const low=QT[q].m||q==='dim'||q==='m7b5'||q==='dim7';const rn=pre+(low?RN.toLowerCase():RN)+(q==='dim'?'°':q==='aug'?'+':q==='m7b5'?'ø7':q==='dim7'?'°7':QT[q].r||'');
    return{off:r,q,rn};});}
  // gradi romani → accordi nella tonalità
  const RN_OFF={I:0,II:2,III:4,IV:5,V:7,VI:9,VII:11};
  function rn2(X,s){const m=s.match(/^([b#♭♯]?)([ivIV]+)(.*)$/);const acc=m[1]==='b'||m[1]==='♭'?-1:m[1]?1:0,up=m[2]===m[2].toUpperCase(),off=mod(RN_OFF[m[2].toUpperCase()]+acc),suf=m[3];
    let q=up?'maj':'min';if(/°7/.test(suf))q='dim7';else if(/°/.test(suf))q='dim';else if(/ø/.test(suf))q='m7b5';else if(/maj7/.test(suf))q='maj7';else if(/7/.test(suf))q=up?'7':'m7';else if(/sus4/.test(suf))q='sus4';
    return{off,q,name:X.cn(off,q)};}
  const prog=(X,rns,label)=>{const C=rns.map(s=>rn2(X,s));return seq(label||rns.join(' – '),C.map(c=>X.ch(c.off,c.q)),C.map(c=>c.name));};
  const progN=(X,rns)=>rns.map(s=>rn2(X,s).name).join(' ');

  /* ---------- il corso ---------- */
  const MODS=['Fondamenti','Accordi','Armonia','Modi e scale','Scrivere e arrangiare'];
  const L=[];
  const add=(mod,o)=>L.push({mod,...o});

  add(0,{id:'t-note',ic:'𝄞',t:'Note e tastiera',d:'Semitoni, toni, diesis e bemolle',ex:'interval',body:X=>`
    <p>Nella musica occidentale ci sono <b>12 note</b> che si ripetono a ogni ottava: sulla tastiera sono 7 tasti bianchi e 5 neri.
    In italiano si chiamano Do Re Mi Fa Sol La Si; nelle sigle degli accordi (C, Dm, G7…) si usano le lettere.</p>
    ${tbl(['Italiano',...IT],[['Lettera',...LET]])}
    <p>Il <b>semitono</b> è la distanza tra due tasti vicini (bianco o nero); due semitoni fanno un <b>tono</b>. Tra Mi–Fa e Si–Do non c'è tasto nero: sono semitoni naturali.
    Il <b>diesis ♯</b> alza di un semitono, il <b>bemolle ♭</b> abbassa. Lo stesso tasto può avere due nomi (Do♯ = Re♭): si dicono <b>enarmonici</b>, e quale si usa dipende dalla tonalità.</p>
    ${ex2([ex('Scala cromatica da '+X.ton,Array.from({length:13},(_,i)=>X.m(i)),1),ex('Semitono',[X.m(0),X.m(1)],1),ex('Tono',[X.m(0),X.m(2)],1),ex('Ottava',[X.m(0),X.m(12)],1)])}
    ${h('L’ottava e i registri')}
    <p>L'<b>ottava</b> è la stessa nota al doppio della frequenza. Si numera: Do4 è il “Do centrale” (MIDI 60). Il basso vive tra Do1 e Do3, gli accordi tra Do3 e Do5, la melodia tra Do4 e Do6: tenere ogni strumento nel suo registro è la prima regola dell'arrangiamento.</p>
    ${ex2([ex(X.ton+'2 (basso)',[X.m(0,2)]),ex(X.ton+'4 (centrale)',[X.m(0,4)]),ex(X.ton+'5 (melodia)',[X.m(0,5)])])}
    ${tip('Cambia la tonalità in alto: tutti gli esempi e i nomi del corso si spostano lì.')}`});

  add(0,{id:'t-ritmo',ic:'♩',t:'Ritmo, tempo e metro',d:'Battiti, figure, sincope, terzine, swing',ex:null,body:X=>`
    <p>Il <b>tempo</b> (BPM) è quanti battiti ci sono in un minuto. Il <b>metro</b> dice come si raggruppano: in 4/4 ogni battuta ha 4 battiti, il primo è il più forte.</p>
    ${tbl(['Figura','Durata in 4/4','Ascolta'],[
      ['Semibreve','4 battiti (battuta intera)',rhy('semibreve','9...............',90,[60])],
      ['Minima','2 battiti',rhy('minime','9.......9.......',90,[60])],
      ['Semiminima','1 battito',rhy('semiminime','9...9...9...9...',90,[60])],
      ['Croma','½ battito (ottavi)',rhy('crome','9.9.9.9.9.9.9.9.',90,[60])],
      ['Semicroma','¼ di battito (sedicesimi)',rhy('semicrome','9999999999999999',90,[60])]])}
    ${h('Accenti, sincope e controtempo')}
    <p>Il <b>battere</b> sono i battiti (1 2 3 4), il <b>levare</b> le “e” in mezzo. La <b>sincope</b> sposta l'accento sul levare e lo tiene: è ciò che fa “ballare” un ritmo.</p>
    ${ex2([rhy('Dritto sui battiti','9...9...9...9...',96,[64]),rhy('Sincopato','9..9..9...9.....',96,[64]),rhy('Controtempo (solo levare)','..9...9...9...9.',96,[64])])}
    ${h('Terzine e swing')}
    <p>Una <b>terzina</b> divide il battito in 3 invece che in 2. Lo <b>swing</b> è suonare gli ottavi “lungo-corto”, come una terzina senza la nota centrale.</p>
    ${ex2([rhy('Ottavi dritti','9.9.9.9.9.9.9.9.',100,[67]),rhy('Terzine','999999999999',100,[67]),rhy('Swing (lungo-corto)','9.99.99.99.9',100,[67])])}
    ${h('Metri diversi')}
    ${ex2([rhy('3/4 (valzer)','9...6...6...',110,[60],3),rhy('6/8 (due gruppi da tre)','9.....7.....',140,[60],3),rhy('Half-time (rullante sul 3)','9.......9.......',140,[38])])}
    ${tip('Nel generatore il “half-time” della trap mette il rullante sul 3: a 140 BPM si sente come un pezzo a 70.')}`});

  add(0,{id:'t-int',ic:'↕',t:'Intervalli',d:'Le distanze che costruiscono tutto',ex:'interval',body:X=>`
    <p>Un <b>intervallo</b> è la distanza tra due note. Si conta in semitoni e si nomina contando le lettere: da ${X.ton} a ${X.sc('major')[2]} sono tre lettere → una <b>terza</b>.</p>
    ${tbl(['Semitoni','Nome','Da '+X.ton,'Carattere','Ascolta'],[[1,'2ª minore',X.nm(X.k+1),'stridente'],[2,'2ª maggiore',X.nm(X.k+2),'passo di scala'],[3,'3ª minore',X.nm(X.k+3),'scuro'],[4,'3ª maggiore',X.nm(X.k+4),'luminoso'],
      [5,'4ª giusta',X.nm(X.k+5),'solido, sospeso'],[6,'Tritono',X.nm(X.k+6),'instabile'],[7,'5ª giusta',X.nm(X.k+7),'vuoto, stabile'],[8,'6ª minore',X.nm(X.k+8),'dolce-amaro'],
      [9,'6ª maggiore',X.nm(X.k+9),'caldo'],[10,'7ª minore',X.nm(X.k+10),'blues'],[11,'7ª maggiore',X.nm(X.k+11),'tesa, sognante'],[12,'Ottava',X.ton,'la stessa nota']].map(r=>[...r,ex('melodico',[X.m(0),X.m(r[0])],1)+' '+ex('armonico',[X.m(0),X.m(r[0])])]))}
    ${h('Consonanze e dissonanze')}
    <p><b>Consonanze</b> (riposo): unisono, ottava, 5ª, 4ª, 3ª e 6ª. <b>Dissonanze</b> (tensione): 2ª, 7ª, tritono. La musica vive del passaggio dalla tensione alla risoluzione.</p>
    ${ex2([seq('Tritono che risolve',[[X.m(-1,4),X.m(5,4)],[X.m(0,4),X.m(4,4)]]),seq('7ª che scende sulla 3ª',[[X.m(7,3),X.m(5,4)],[X.m(0,3),X.m(4,4)]])])}
    ${h('Rivolto di un intervallo')}
    <p>Capovolgendo un intervallo (la nota bassa va un'ottava sopra) la somma fa sempre 9: una 3ª diventa 6ª, una 2ª diventa 7ª, la 4ª diventa 5ª. Maggiore ↔ minore, giusto resta giusto.</p>
    ${ex2([ex('3ª maggiore',[X.m(0),X.m(4)],1),ex('→ 6ª minore',[X.m(4),X.m(12)],1)])}
    ${tip('Le estensioni degli accordi sono intervalli oltre l\'ottava: 9ª = 2ª + ottava, 11ª = 4ª + ottava, 13ª = 6ª + ottava.')}`});

  add(0,{id:'t-scale',ic:'≡',t:'Scale maggiore e minore',d:'Le formule e le relative',ex:'degree',body:X=>{const rel=mod(X.k+9);return`
    <p>Una <b>scala</b> sceglie 7 note tra le 12. La identifica la sequenza di toni (T) e semitoni (S) e la nota su cui si “riposa”: la <b>tonica</b>.</p>
    ${tbl(['Scala','Formula',X.ton,'Ascolta'],[
      ['Maggiore','T T S T T T S',X.sc('major').join(' '),ex('▶',[...SCM.major.iv,12].map(x=>X.m(x)),1)],
      ['Minore naturale','T S T T S T T',X.sc('minor').join(' '),ex('▶',[...SCM.minor.iv,12].map(x=>X.m(x)),1)],
      ['Minore armonica','7ª alzata',X.sc('harmonic').join(' '),ex('▶',[...SCM.harmonic.iv,12].map(x=>X.m(x)),1)],
      ['Minore melodica','6ª e 7ª alzate',X.sc('melodic').join(' '),ex('▶',[...SCM.melodic.iv,12].map(x=>X.m(x)),1)]])}
    ${h('I gradi e i loro nomi')}
    ${tbl(['Grado','I','II','III','IV','V','VI','VII'],[['Nome','tonica','sopratonica','mediante','sottodominante','dominante','sopradominante','sensibile'],['In '+X.ton+' magg.',...X.sc('major')]])}
    <p>La <b>sensibile</b> è il VII grado a un semitono dalla tonica: “tira” verso casa. Per questo nel minore si alza la 7ª (minore armonica).</p>
    ${h('Relativa e parallela')}
    <p><b>Relativa</b>: stesse note, tonica diversa. La relativa minore di ${X.ton} maggiore è <b>${X.nm(rel)} minore</b> (parte dal VI grado). <b>Parallela</b>: stessa tonica, modo diverso (${X.ton} maggiore ↔ ${X.ton} minore).</p>
    ${ex2([ex(X.ton+' maggiore',[...SCM.major.iv,12].map(x=>X.m(x)),1),ex(X.nm(rel)+' minore (relativa)',[...SCM.minor.iv,12].map(x=>X.m(x+9-12)),1),ex(X.ton+' minore (parallela)',[...SCM.minor.iv,12].map(x=>X.m(x)),1)])}
    ${tip('Vuoi vedere qualsiasi scala con i suoi accordi? Apri l\'<b>Esploratore di scale</b> in fondo all\'elenco della teoria.')}`;}});

  add(0,{id:'t-ton',ic:'◎',t:'Tonalità e circolo delle quinte',d:'Armature, tonalità vicine, scelta della tonalità',ex:'degree',body:X=>{
    const ring=[0,7,2,9,4,11,6,1,8,3,10,5],acc=['nessuna','1♯','2♯','3♯','4♯','5♯','6♯ / 6♭','5♭','4♭','3♭','2♭','1♭'];
    return`<p>Una <b>tonalità</b> è una scala con la sua tonica e i suoi accordi. L'<b>armatura</b> elenca le alterazioni fisse. Ogni tonalità maggiore ha una relativa minore con la stessa armatura.</p>
    <div class="circle">${ring.map((pc,i)=>{const a=i/12*2*Math.PI-Math.PI/2,x=50+40*Math.cos(a),y=50+40*Math.sin(a),xi=50+26*Math.cos(a),yi=50+26*Math.sin(a);
      return`<button class="cnode ${pc===X.k?'on':''}" style="left:${x}%;top:${y}%" data-cof="${pc}" title="${acc[i]}">${(FLATKEY.has(pc)?FLL:SHL)[pc]}</button><span class="cmin" style="left:${xi}%;top:${yi}%">${(FLATKEY.has(pc)||pc===5?FLL:SHL)[mod(pc+9)]}m</span>`;}).join('')}<span class="cmid">quinte →<br>in senso orario</span></div>
    <p>Girando in senso orario si sale di una quinta e si aggiunge un ♯; in senso antiorario si scende di una quinta e si aggiunge un ♭. Clicca una tonalità per trasportare il corso.</p>
    ${ex2([ex(X.ton+' maggiore',[...SCM.major.iv,12].map(x=>X.m(x)),1),ex(X.nl(X.k+7)+' maggiore (un ♯ in più)',[...SCM.major.iv,12].map(x=>X.m(x+7,3)),1),ex(X.nl(X.k+5)+' maggiore (un ♭ in più)',[...SCM.major.iv,12].map(x=>X.m(x+5,3)),1),prog(X,['I','IV','V','I'],'Cadenza in '+X.ton)])}
    ${h('Tonalità vicine')}
    <p>Le tonalità accanto sul cerchio condividono 6 note su 7: modulare tra loro è morbido. Da ${X.ton} maggiore sono vicine: <b>${X.nl(X.k+7)}</b> (dominante), <b>${X.nl(X.k+5)}</b> (sottodominante) e le loro relative minori.</p>
    ${h('Ordine dei diesis e dei bemolle')}
    <p>Diesis: Fa Do Sol Re La Mi Si. Bemolle: Si Mi La Re Sol Do Fa (al contrario).</p>
    ${h('Come scegliere la tonalità')}
    <ul><li><b>Voce</b>: la tonalità giusta è quella in cui la nota più alta del ritornello sta comoda.</li><li><b>Strumenti</b>: chitarra ama Mi, La, Re, Sol (corde vuote); fiati preferiscono i bemolli (Si♭, Mi♭, Fa).</li>
      <li><b>Colore</b>: nel temperamento equabile non cambia il carattere, ma cambia il registro (più acuto = più brillante).</li></ul>`;}});

  add(1,{id:'t-triadi',ic:'△',t:'Triadi',d:'Maggiore, minore, diminuita, aumentata',ex:'quality',body:X=>`
    <p>Una <b>triade</b> sono tre note sovrapposte per terze: <b>fondamentale</b>, <b>terza</b>, <b>quinta</b>. Le terze decidono il carattere.</p>
    ${tbl(['Triade','Terze','Semitoni','Su '+X.ton,'Sigla','Ascolta'],[
      ['Maggiore','3M + 3m','0·4·7',[0,4,7].map(x=>X.nm(X.k+x)).join(' '),X.cn(0,'maj'),ex('▶',X.ch(0,'maj',0,4))],
      ['Minore','3m + 3M','0·3·7',[0,3,7].map(x=>X.nm(X.k+x)).join(' '),X.cn(0,'min'),ex('▶',X.ch(0,'min',0,4))],
      ['Diminuita','3m + 3m','0·3·6',[0,3,6].map(x=>X.nm(X.k+x)).join(' '),X.cn(0,'dim'),ex('▶',X.ch(0,'dim',0,4))],
      ['Aumentata','3M + 3M','0·4·8',[0,4,8].map(x=>X.nm(X.k+x)).join(' '),X.cn(0,'aug'),ex('▶',X.ch(0,'aug',0,4))]])}
    <p>Tra maggiore e minore cambia <b>una sola nota</b>: la terza.</p>
    ${ex2([seq('Maggiore → minore → maggiore',[X.ch(0,'maj',0,4),X.ch(0,'min',0,4),X.ch(0,'maj',0,4)],[X.cn(0,'maj'),X.cn(0,'min'),X.cn(0,'maj')])])}
    ${h('Sus: senza terza')}
    <p>Gli accordi <b>sus</b> sostituiscono la terza con la 2ª (sus2) o la 4ª (sus4): aperti, in attesa. La sus4 di solito <b>risolve</b> sulla terza.</p>
    ${ex2([ex(X.cn(0,'sus2'),X.ch(0,'sus2',0,4)),ex(X.cn(0,'sus4'),X.ch(0,'sus4',0,4)),seq('sus4 → maggiore',[X.ch(0,'sus4',0,4),X.ch(0,'maj',0,4)],[X.cn(0,'sus4'),X.cn(0,'maj')])])}
    ${h('Le triadi nella scala')}
    <p>Su ogni grado della scala maggiore nasce una triade: <b>tre maggiori</b> (I, IV, V), <b>tre minori</b> (ii, iii, vi) e <b>una diminuita</b> (vii°).</p>
    ${ex2(diatonic(X,'major').map(c=>ex(c.rn+' '+X.cn(c.off,c.q),X.ch(c.off,c.q))))}
    ${tip('Per costruire una triade: dalla fondamentale salta una nota della scala, prendi la successiva, salta ancora.')}`});

  add(1,{id:'t-rivolti',ic:'⇅',t:'Rivolti',d:'Quale nota sta al basso',ex:'inversion',body:X=>{const I=[0,4,7].map(x=>X.nm(X.k+x));return`
    <p>Conta <b>quale nota è la più grave</b>:</p>
    ${tbl(['Posizione','Al basso','Note','Sigla','Ascolta'],[
      ['Stato fondamentale','fondamentale',I.join(' · '),X.cn(0,'maj'),ex('▶',X.ch(0,'maj',0))],
      ['Primo rivolto','terza',[I[1],I[2],I[0]].join(' · '),X.cn(0,'maj')+'/'+X.nl(X.k+4),ex('▶',X.ch(0,'maj',1))],
      ['Secondo rivolto','quinta',[I[2],I[0],I[1]].join(' · '),X.cn(0,'maj')+'/'+X.nl(X.k+7),ex('▶',X.ch(0,'maj',2))]])}
    <p>Gli accordi di 4 note hanno il <b>terzo rivolto</b>, con la settima al basso: instabile, vuole scendere.</p>
    ${ex2([0,1,2,3].map(i=>ex(X.cn(7,'7')+(i?'/'+X.nl(X.k+7+[0,4,7,10][i]):''),X.ch(7,'7',i))))}
    ${h('Le sigle con la barra')}
    <p><b>${X.cn(0,'maj')}/${X.nl(X.k+4)}</b> = accordo di ${X.ton} con ${X.nm(X.k+4)} al basso. La nota dopo la barra può anche non far parte dell'accordo (es. ${X.cn(2,'maj')}/${X.nl(X.k)}): è un <b>basso pedale</b> o un accordo “ibrido”.</p>
    ${h('Perché si usano')}
    <ul><li><b>Collegare gli accordi</b> con poco movimento (voice leading).</li><li><b>Bassi che camminano</b> per gradi.</li><li><b>Colore</b>: il primo rivolto è leggero, il secondo sospeso (si usa prima della dominante: “6/4 cadenzale”).</li></ul>
    ${ex2([prog(X,['I','V','vi','IV'],'Senza rivolti'),seq('Basso che scende: I – V/7 – vi – IV/6',[X.ch(0,'maj',0),X.ch(7,'maj',1),X.ch(9,'min',0),X.ch(5,'maj',1)],[X.cn(0,'maj'),X.cn(7,'maj')+'/'+X.nl(X.k+11),X.cn(9,'min'),X.cn(5,'maj')+'/'+X.nl(X.k+9)]),
      seq('6/4 cadenzale',[X.ch(5,'maj'),X.ch(0,'maj',2),X.ch(7,'7'),X.ch(0,'maj')],[X.cn(5,'maj'),X.cn(0,'maj')+'/'+X.nl(X.k+7),X.cn(7,'7'),X.cn(0,'maj')])])}
    ${tip('A orecchio ascolta solo la nota più grave: se coincide col nome dell\'accordo è stato fondamentale.')}`;}});

  add(1,{id:'t-sett',ic:'7',t:'Accordi di settima',d:'maj7, 7, m7, ø7, °7',ex:'quality',body:X=>`
    <p>Aggiungendo un'altra terza alla triade nasce l'accordo di <b>settima</b>: più ricco e con più direzione.</p>
    ${tbl(['Accordo','Struttura','Sigla','Carattere','Ascolta'],[
      ['Settima maggiore','magg + 7M',X.cn(0,'maj7'),'morbido, sognante',ex('▶',X.ch(0,'maj7',0,4))],['Dominante','magg + 7m',X.cn(0,'7'),'teso, vuole risolvere',ex('▶',X.ch(0,'7',0,4))],
      ['Minore settima','min + 7m',X.cn(0,'m7'),'caldo, soul',ex('▶',X.ch(0,'m7',0,4))],['Semidiminuito','dim + 7m',X.cn(0,'m7b5'),'scuro, prepara la dominante in minore',ex('▶',X.ch(0,'m7b5',0,4))],
      ['Diminuito','dim + 7dim',X.cn(0,'dim7'),'drammatico, simmetrico',ex('▶',X.ch(0,'dim7',0,4))],['Minore maj7','min + 7M',X.cn(0,'mmaj7'),'noir',ex('▶',X.ch(0,'mmaj7',0,4))]])}
    ${h('Le settime nella scala')}
    ${tbl(diatonic(X,'major',1).map(c=>c.rn),[diatonic(X,'major',1).map(c=>X.cn(c.off,c.q))])}
    ${ex2(diatonic(X,'major',1).map(c=>ex(c.rn,X.ch(c.off,c.q))))}
    ${h('Il tritono della dominante')}
    <p>Solo il V7 contiene il tritono tra la sensibile e la 4ª (${X.nm(X.k+11)}–${X.nm(X.k+5)}): risolve per moto contrario sulla 3ª della tonica. È il motore dell'armonia tonale.</p>
    ${ex2([prog(X,['ii7','V7','Imaj7'],'ii – V – I'),seq('Il tritono risolve',[[X.m(-1),X.m(5)],[X.m(0),X.m(4)]])])}`});

  add(1,{id:'t-est',ic:'✦',t:'Estensioni e colori',d:'add9, 6, 9, 11, 13',ex:'quality',body:X=>`
    <p>Le <b>estensioni</b> sono note sopra l'ottava (9ª, 11ª, 13ª) che aggiungono colore senza cambiare la funzione.</p>
    ${tbl(['Sigla','Cosa aggiunge','Effetto','Ascolta'],[[X.cn(0,'add9'),'9ª senza 7ª','aperto, pop',ex('▶',X.ch(0,'add9',0,4))],[X.cn(0,'6'),'6ª','vintage',ex('▶',X.ch(0,'6',0,4))],
      [X.cn(0,'69'),'6ª e 9ª','jazz, bossa',ex('▶',X.ch(0,'69',0,3))],[X.cn(0,'maj9'),'maj7 + 9ª','neo soul, lo-fi',ex('▶',X.ch(0,'maj9',0,3))],[X.cn(0,'m9'),'m7 + 9ª','morbido',ex('▶',X.ch(0,'m9',0,3))],
      [X.cn(0,'9'),'7 + 9ª','funk',ex('▶',X.ch(0,'9',0,3))],[X.cn(0,'m11'),'m7 + 9ª + 11ª','modale, spazioso',ex('▶',X.ch(0,'m11',0,3))],[X.cn(0,'13'),'7 + 9ª + 13ª','gospel',ex('▶',X.ch(0,'13',0,3))],
      [X.cn(0,'maj7#11'),'maj7 + ♯11','lidio',ex('▶',X.ch(0,'maj7#11',0,3))]])}
    ${h('Tensioni disponibili ed evitate')}
    <p>Le estensioni “buone” sono quelle della scala dell'accordo che non stanno un semitono sopra una nota dell'accordo. Sul maggiore l'11ª naturale (${X.nm(X.k+5)}) urta la terza → si usa la ♯11. Sul minore la 11ª suona benissimo.</p>
    ${ex2([ex('maj con 11 (urta)',[X.m(0,3),X.m(4,3),X.m(7,3),X.m(5,4)]),ex('maj con ♯11 (aperto)',[X.m(0,3),X.m(4,3),X.m(7,3),X.m(6,4)]),ex('m11 (morbido)',X.ch(0,'m11',0,3))])}
    ${tip('Al pianoforte si toglie spesso la 5ª per fare spazio alle estensioni.')}`});

  add(1,{id:'t-alt',ic:'♯',t:'Dominanti alterate e accordi speciali',d:'7♭9, 7♯9, 7sus4, alt, diminuiti di passaggio',ex:'quality',body:X=>`
    <p>Sulla dominante si possono alterare 9ª e 13ª per aumentare la tensione prima della risoluzione.</p>
    ${tbl(['Sigla','Note aggiunte','Uso','Ascolta'],[[X.cn(7,'7b9'),'♭9','tensione drammatica, verso il minore',seq('→ '+X.cn(0,'min'),[X.ch(7,'7b9'),X.ch(0,'min')])],
      [X.cn(7,'7#9'),'♯9 (“accordo di Hendrix”)','rock, funk, blues',seq('▶',[X.ch(7,'7#9'),X.ch(0,'maj')])],[X.cn(7,'7sus4'),'4ª al posto della 3ª','dominante morbida, gospel/neo soul',seq('→ '+X.cn(7,'7')+' → '+X.cn(0,'maj'),[X.ch(7,'7sus4'),X.ch(7,'7'),X.ch(0,'maj')])],
      [X.cn(7,'9sus4'),'sus4 + 7 + 9','pop moderno, “IV/V”',ex('▶',X.ch(7,'9sus4'))]])}
    ${h('Il diminuito come accordo di passaggio')}
    <p>Il °7 è simmetrico (terze minori): ogni nota può fare da fondamentale. Si usa tra due accordi a un tono di distanza per collegarli cromaticamente.</p>
    ${ex2([seq('I – ♯I°7 – ii – V',[X.ch(0,'maj'),X.ch(1,'dim7'),X.ch(2,'min'),X.ch(7,'7')],[X.cn(0,'maj'),X.nl(X.k+1)+'°7',X.cn(2,'min'),X.cn(7,'7')]),seq('IV – ♯IV°7 – I/5',[X.ch(5,'maj'),X.ch(6,'dim7'),X.ch(0,'maj',2)],[X.cn(5,'maj'),X.nl(X.k+6)+'°7',X.cn(0,'maj')+'/'+X.nl(X.k+7)])])}
    ${h('Line cliché')}
    <p>Una nota interna dell'accordo scende di semitono mentre l'accordo resta: tipico delle ballate e del cinema.</p>
    ${ex2([seq('i – i(maj7) – i7 – i6',[X.ch(0,'min',0,3),X.ch(0,'mmaj7',0,3),X.ch(0,'m7',0,3),X.ch(0,'m6',0,3)],[X.cn(0,'min'),X.cn(0,'mmaj7'),X.cn(0,'m7'),X.cn(0,'m6')])])}`});

  add(1,{id:'t-voice',ic:'≋',t:'Voicing e voice leading',d:'Come disporre e collegare gli accordi',ex:'build',body:X=>`
    <p>Il <b>voicing</b> è come distribuisci le note sulla tastiera. Lo stesso accordo può suonare compatto o arioso.</p>
    ${ex2([ex(X.cn(0,'maj7')+' chiuso',X.ch(0,'maj7',0,4)),ex('aperto',[X.m(0,3),X.m(7,3),X.m(4,4),X.m(11,4)]),ex('Shell 1–3–7',[X.m(0,3),X.m(4,3),X.m(11,3)]),ex('Drop 2',[X.m(0,3),X.m(7,3),X.m(11,3),X.m(4,4)]),ex('Rootless',[X.m(4,4),X.m(7,4),X.m(11,4),X.m(14,4)])])}
    <ul><li><b>Sinistra</b>: fondamentale, o fondamentale + quinta/ottava. Le terze sotto il Do3 suonano “fangose”.</li><li><b>Destra</b>: terza e settima definiscono l'accordo; la quinta si può togliere.</li>
      <li><b>Raddoppi</b>: si raddoppia la fondamentale, quasi mai la terza dell'accordo maggiore né la sensibile.</li></ul>
    ${h('Voice leading')}
    <p>Collegare gli accordi muovendo ogni voce il meno possibile: le note comuni restano, le altre si spostano di tono o semitono. Evita le <b>quinte e ottave parallele</b> tra le voci esterne: tolgono indipendenza.</p>
    ${ex2([seq('ii–V–I con voice leading',[[X.m(2,3),X.m(5,4),X.m(9,4),X.m(12,4)],[X.m(-5,3),X.m(5,4),X.m(11,4),X.m(14,4)],[X.m(0,3),X.m(4,4),X.m(11,4),X.m(14,4)]],[X.cn(2,'m7'),X.cn(7,'7'),X.cn(0,'maj9')]),
      seq('Stesso giro a blocchi',[X.ch(2,'m7',0,4),X.ch(7,'7',0,4),X.ch(0,'maj7',0,4)])])}
    ${tip('Nel ii–V–I la settima di un accordo scende di semitono e diventa la terza del successivo.')}`});

  add(2,{id:'t-gradi',ic:'Ⅳ',t:'Gradi e numeri romani',d:'Armonia diatonica in maggiore e in minore',ex:'degree',body:X=>{const mj=diatonic(X,'major'),mn=diatonic(X,'minor'),hm=diatonic(X,'harmonic');return`
    <p>Costruendo un accordo su ogni grado si ottengono gli <b>accordi diatonici</b>. I numeri romani valgono in tutte le tonalità: maiuscolo = maggiore, minuscolo = minore, ° = diminuito.</p>
    ${tbl(['',...mj.map(c=>c.rn)],[['In '+X.ton+' maggiore',...mj.map(c=>X.cn(c.off,c.q))]])}
    ${ex2(mj.map(c=>ex(c.rn,X.ch(c.off,c.q))))}
    ${tbl(['',...mn.map(c=>c.rn)],[['In '+X.ton+' minore',...mn.map(c=>X.cn(c.off,c.q))],['con la armonica',...hm.map(c=>X.cn(c.off,c.q))]])}
    <p>Nel minore si usano insieme la naturale e l'armonica: <b>v</b> minore per un suono modale, <b>V</b> maggiore per una cadenza forte.</p>
    ${ex2([prog(X,['i','iv','v','i'],'i – iv – v – i (naturale)'),prog(X,['i','iv','V','i'],'i – iv – V – i (armonica)')])}
    ${tip('Nei generi moderni trovi gradi col ♭ (♭VII, ♭VI, ♭III): sono prestiti dal minore. Vedi “Interscambio modale”.')}`;}});

  add(2,{id:'t-funz',ic:'⟳',t:'Funzioni armoniche',d:'Tonica, sottodominante, dominante',ex:'prog',body:X=>`
    <p>Ogni accordo ha un <b>ruolo</b>:</p>
    ${tbl(['Funzione','Ruolo','Accordi','In '+X.ton],[['<span class="fdot fT"></span>Tonica (T)','riposo','I, vi, (iii)',progN(X,['I','vi','iii'])],['<span class="fdot fS"></span>Sottodominante (S)','si allontana','IV, ii',progN(X,['IV','ii'])],['<span class="fdot fD"></span>Dominante (D)','tensione','V, vii°',progN(X,['V','vii°'])]])}
    <p>Il ciclo base è <b>T → S → D → T</b>. Accordi della stessa funzione si sostituiscono: vi al posto di I, ii al posto di IV.</p>
    ${ex2([prog(X,['I','IV','V','I']),prog(X,['vi','ii','V7','I']),prog(X,['I','iii','IV','V'])])}
    ${h('Retrogressione')}
    <p>Il movimento D → S (V → IV) è “all'indietro”: in classica è raro, nel rock e nel blues è comunissimo e dà un sapore rilassato.</p>
    ${ex2([prog(X,['I','V','IV','I'],'I – V – IV – I (rock)')])}`});

  add(2,{id:'t-cad',ic:'⤓',t:'Cadenze',d:'Come finisce una frase',ex:'prog',body:X=>`
    <p>La <b>cadenza</b> è la punteggiatura dell'armonia.</p>
    ${tbl(['Cadenza','Gradi','Effetto','Ascolta'],[['Autentica perfetta','V → I','punto fermo',prog(X,['V7','I'],'▶')],['Plagale','IV → I','“amen”',prog(X,['IV','I'],'▶')],
      ['Sospesa (semicadenza)','… → V','virgola, domanda',prog(X,['I','vi','ii','V'],'▶')],['D’inganno','V → vi','finta fine',prog(X,['V7','vi'],'▶')],
      ['Minore plagale','iv → I','nostalgia',prog(X,['IV','iv','I'],'▶')],['Frigia','iv⁶ → V (in minore)','barocca, spagnola',seq('▶',[X.ch(5,'min',1),X.ch(7,'maj')])],
      ['Backdoor','♭VII7 → I','soul, jazz',prog(X,['iv7','bVII7','I'],'▶')]])}
    ${tip('Le strofe spesso chiudono sospese sul V per lanciare il ritornello, che chiude sulla tonica.')}`});

  add(2,{id:'t-prog',ic:'⟶',t:'Progressioni tipiche',d:'I giri che si sentono ovunque',ex:'prog',body:X=>`
    ${tbl(['Giro','In '+X.ton,'Dove','Ascolta'],[[ 'I – V – vi – IV',progN(X,['I','V','vi','IV']),'pop',prog(X,['I','V','vi','IV'],'▶')],['vi – IV – I – V',progN(X,['vi','IV','I','V']),'pop malinconico',prog(X,['vi','IV','I','V'],'▶')],
      ['I – vi – IV – V',progN(X,['I','vi','IV','V']),'anni ’50',prog(X,['I','vi','IV','V'],'▶')],['ii – V – I',progN(X,['ii7','V7','Imaj7']),'jazz',prog(X,['ii7','V7','Imaj7'],'▶')],
      ['I – IV – ♭VII – IV',progN(X,['I','IV','bVII','IV']),'rock',prog(X,['I','IV','bVII','IV'],'▶')],['i – ♭VII – ♭VI – ♭VII',progN(X,['i','bVII','bVI','bVII']),'epico',prog(X,['i','bVII','bVI','bVII'],'▶')],
      ['i – ♭VI – ♭III – ♭VII',progN(X,['i','bVI','bIII','bVII']),'trap, cinematico',prog(X,['i','bVI','bIII','bVII'],'▶')],['i – iv – v – i',progN(X,['i','iv','v','i']),'minore modale',prog(X,['i','iv','v','i'],'▶')],
      ['I – iii – IV – iv',progN(X,['I','iii','IV','iv']),'ballad',prog(X,['I','iii','IV','iv'],'▶')],['Imaj7 – vi7 – ii7 – V7',progN(X,['Imaj7','vi7','ii7','V7']),'jazz, “turnaround”',prog(X,['Imaj7','vi7','ii7','V7'],'▶')]])}
    ${h('Blues in 12 battute')}
    <p>${[['I7','I7','I7','I7'],['IV7','IV7','I7','I7'],['V7','IV7','I7','V7']].map(r=>r.map(s=>rn2(X,s).name).join(' · ')).join(' &nbsp;|&nbsp; ')}</p>
    ${ex2([prog(X,['I7','IV7','I7','V7','IV7','I7'],'Blues (accordi principali)')])}
    ${h('Circolo delle quinte')}
    <p>Ogni accordo scende di una quinta: il movimento più naturale per l'orecchio.</p>
    ${ex2([prog(X,['vi7','ii7','V7','Imaj7','IVmaj7'],'vi – ii – V – I – IV')])}`});

  add(2,{id:'t-secdom',ic:'→',t:'Dominanti secondarie',d:'V/V, V/vi, sostituzione di tritono',ex:'fill',body:X=>`
    <p>Ogni accordo maggiore o minore della tonalità può essere preceduto dalla <b>sua</b> dominante, come se fosse per un attimo la tonica: si scrive <b>V/x</b>.</p>
    ${tbl(['Sigla','In '+X.ton,'Va verso','Ascolta'],[['V/V',X.cn(2,'7'),X.cn(7,'maj'),seq('▶',[X.ch(0,'maj'),X.ch(2,'7'),X.ch(7,'maj'),X.ch(0,'maj')])],
      ['V/vi',X.cn(4,'7'),X.cn(9,'min'),seq('▶',[X.ch(0,'maj'),X.ch(4,'7'),X.ch(9,'min')])],['V/IV',X.cn(0,'7'),X.cn(5,'maj'),seq('▶',[X.ch(0,'maj'),X.ch(0,'7'),X.ch(5,'maj')])],
      ['V/ii',X.cn(9,'7'),X.cn(2,'min'),seq('▶',[X.ch(0,'maj'),X.ch(9,'7'),X.ch(2,'min'),X.ch(7,'7')])]])}
    <p>Si può fare anche <b>ii–V secondario</b>: (ii–V)/x, e la <b>catena di dominanti</b> (ogni dominante risolve su un'altra dominante).</p>
    ${ex2([seq('Catena: '+[4,9,2,7].map(o=>X.cn(o,'7')).join(' → ')+' → '+X.cn(0,'maj'),[X.ch(4,'7'),X.ch(9,'7'),X.ch(2,'7'),X.ch(7,'7'),X.ch(0,'maj')])])}
    ${h('Sostituzione di tritono')}
    <p>Al posto di ${X.cn(7,'7')} si usa ${X.cn(1,'7')}: stesso tritono, ma il basso scende di semitono sulla tonica.</p>
    ${ex2([seq('ii – V – I',[X.ch(2,'m7'),X.ch(7,'7'),X.ch(0,'maj7')]),seq('ii – subV – I',[X.ch(2,'m7'),X.ch(1,'7'),X.ch(0,'maj7')])])}`});

  add(2,{id:'t-prest',ic:'◑',t:'Interscambio modale',d:'Come prendere accordi da un altro modo',ex:'fill',body:X=>{const par=diatonic(X,'minor');return`
    <p>L'<b>interscambio modale</b> (o “prestito”) è usare in una tonalità accordi del <b>modo parallelo</b>: stessa tonica, scala diversa. È il trucco più usato per dare emozione a un giro semplice.</p>
    ${h('Come si fa, passo per passo')}
    <ol><li>Scrivi gli accordi della tonalità (${X.ton} maggiore): ${diatonic(X,'major').map(c=>X.cn(c.off,c.q)).join(' ')}.</li>
      <li>Scrivi quelli del parallelo minore (${X.ton} minore): ${par.map(c=>X.cn(c.off,c.q)).join(' ')}.</li>
      <li>Prendi un accordo del secondo elenco e mettilo dove nel primo c'è uno della <b>stessa funzione</b> (iv al posto di IV, ♭VI al posto di vi o IV, ♭VII al posto di V).</li>
      <li>Torna subito alla tonalità: il prestito colora, non sostituisce.</li></ol>
    ${tbl(['Prestito','In '+X.ton,'Effetto','Ascolta'],[['iv',X.cn(5,'min'),'commozione',prog(X,['I','IV','iv','I'],'▶')],['♭VII',X.cn(10,'maj'),'liberatorio, rock',prog(X,['I','bVII','IV','I'],'▶')],
      ['♭VI',X.cn(8,'maj'),'grandioso',prog(X,['I','bVI','bVII','I'],'▶')],['♭III',X.cn(3,'maj'),'sorpresa',prog(X,['I','bIII','IV','I'],'▶')],['iiø7',X.cn(2,'m7b5'),'scuro',prog(X,['iiø7','V7','I'],'▶')],
      ['v (dal misolidio)',X.cn(7,'min'),'morbido, senza sensibile',prog(X,['I','v','IV','I'],'▶')],['II (dal lidio)',X.cn(2,'maj'),'sognante',prog(X,['I','II','IV','I'],'▶')]])}
    ${h('Al contrario: in minore')}
    <p>In minore si prendono accordi dal maggiore: il <b>IV maggiore</b> (sapore dorico), il <b>V maggiore</b> (dall'armonica) e la <b>terza piccarda</b>: finire sull'accordo maggiore di tonica.</p>
    ${ex2([prog(X,['i','IV','i','IV'],'i – IV (dorico)'),prog(X,['i','iv','V','I'],'Terza piccarda')])}
    ${h('Cambio modale vs interscambio')}
    <p>L'<b>interscambio</b> è un colore di un accordo. Il <b>cambio modale</b> è passare a un altro modo per un'intera sezione (es. strofa in ${X.ton} minore, ritornello in ${X.ton} maggiore): vedi “Modulare e cambiare modo”.</p>
    ${tip('Nel popup degli accordi del generatore i prestiti compaiono nella sezione “interscambio modale”.')}`;}});

  add(2,{id:'t-mod',ic:'⇪',t:'Modulare e cambiare modo',d:'Cambi di tonalità: perno, diretta, per dominante, di un tono',ex:null,body:X=>{const D=7,Dn=X.nl(X.k+D);return`
    <p><b>Modulare</b> è spostare la tonica in un'altra tonalità per un tratto o per sempre. Dà energia, sorpresa o un nuovo colore. Ci sono tecniche diverse:</p>
    ${h('1 · Accordo perno (la più morbida)')}
    <p>Si trova un accordo comune alle due tonalità e lo si usa come “ponte”. Da ${X.ton} a ${Dn}: ${X.cn(9,'min')} è vi in ${X.tonL} e ii in ${Dn}.</p>
    ${ex2([seq(`${X.cn(0,'maj')} – ${X.cn(5,'maj')} – [${X.cn(9,'min')}] – ${X.cn(2,'7')} – ${X.cn(7,'maj')}`,[X.ch(0,'maj'),X.ch(5,'maj'),X.ch(9,'min'),X.ch(2,'7'),X.ch(7,'maj')])])}
    ${h('2 · Per dominante')}
    <p>Si arriva alla nuova tonalità con la <b>sua</b> dominante (o ii–V). Funziona verso qualsiasi tonalità.</p>
    ${ex2([seq(`${X.cn(0,'maj')} → ${X.cn(4,'m7')} ${X.cn(9,'7')} → ${X.cn(2,'maj')}`,[X.ch(0,'maj'),X.ch(4,'m7'),X.ch(9,'7'),X.ch(2,'maj')])])}
    ${h('3 · Diretta (brusca)')}
    <p>Si cambia senza preparazione, spesso tra due sezioni. Più è lontana la tonalità, più è sorprendente.</p>
    ${ex2([seq(`${X.cn(0,'maj')} ${X.cn(7,'maj')} | ${X.cn(3,'maj')} ${X.cn(10,'maj')}`,[X.ch(0,'maj'),X.ch(7,'maj'),X.ch(3,'maj'),X.ch(10,'maj')])])}
    ${h('4 · Un tono sopra (“truck driver”)')}
    <p>L'ultimo ritornello sale di un tono (o di un semitono), spesso preceduto dalla dominante della nuova tonalità. Il generatore lo fa con “Modula l'ultimo ritornello”.</p>
    ${ex2([seq(`… ${X.cn(7,'maj')} → ${X.cn(9,'7')} → ${X.cn(2,'maj')}`,[X.ch(5,'maj'),X.ch(7,'maj'),X.ch(9,'7'),X.ch(2,'maj'),X.ch(7,'maj',0,X.k>=7?2:3).map(n=>n+2)])])}
    ${h('5 · Alla relativa o alla parallela (cambio modale)')}
    <p><b>Relativa</b> (${X.ton} ↔ ${X.nm(X.k+9)} minore): stesse note, cambia il centro. <b>Parallela</b> (${X.ton} maggiore ↔ ${X.ton} minore): stessa tonica, cambia il modo: luce/ombra immediata. Si può cambiare anche verso un modo (dorico, lidio, misolidio) mantenendo la tonica.</p>
    ${ex2([seq('Maggiore → parallela minore',[X.ch(0,'maj'),X.ch(5,'maj'),X.ch(0,'min'),X.ch(8,'maj'),X.ch(10,'maj'),X.ch(0,'min')]),seq('Ionio → lidio (stessa tonica)',[X.ch(0,'maj'),X.ch(5,'maj'),X.ch(0,'maj'),X.ch(2,'maj')])])}
    ${h('Come tornare')}
    <p>Si torna con lo stesso sistema al contrario (perno o dominante della tonalità d'origine). Se la modulazione è al ritornello finale, di solito non si torna.</p>
    ${tip('Regola pratica: modula verso tonalità vicine sul circolo delle quinte per un effetto naturale, lontane per un effetto cinematico.')}`;}});

  add(3,{id:'t-modi',ic:'◐',t:'I modi',d:'Ionio, dorico, frigio, lidio, misolidio, eolio, locrio',ex:'mode',body:X=>{const md=['major','dorian','phrygian','lydian','mixolydian','minor','locrian'],nm=['Ionio','Dorico','Frigio','Lidio','Misolidio','Eolio','Locrio'],ch=['—','6ª maggiore','2ª minore','4ª aumentata','7ª minore','—','2ª minore e 5ª dim.'];
    return`<p>I <b>modi</b> sono 7 scale che nascono partendo da ogni grado della maggiore. Qui sono tutti su <b>${X.ton}</b> (paralleli), così senti solo il colore che cambia.</p>
    ${tbl(['Modo','Note','Nota caratteristica','Carattere','Ascolta'],md.map((id,i)=>[nm[i],X.sc(id).join(' '),ch[i],SCM[id].d,ex('▶',[...SCM[id].iv,12].map(x=>X.m(x)),1)]))}
    ${h('Dal più luminoso al più scuro')}
    <p>Lidio → Ionio → Misolidio → Dorico → Eolio → Frigio → Locrio: ogni passo abbassa una nota.</p>
    ${h('Vamp: far sentire il modo')}
    <p>Un modo si riconosce quando un accordo mette in evidenza la sua nota caratteristica, alternato alla tonica:</p>
    ${ex2([seq('Dorico: i – IV',[X.ch(0,'m7'),X.ch(5,'maj'),X.ch(0,'m7'),X.ch(5,'maj')]),seq('Frigio: i – ♭II',[X.ch(0,'min'),X.ch(1,'maj'),X.ch(0,'min'),X.ch(1,'maj')]),
      seq('Lidio: I – II',[X.ch(0,'maj'),X.ch(2,'maj'),X.ch(0,'maj'),X.ch(2,'maj')]),seq('Misolidio: I – ♭VII',[X.ch(0,'maj'),X.ch(10,'maj'),X.ch(0,'maj'),X.ch(10,'maj')])])}
    ${tip('Nel generatore puoi scegliere il modo nel menu “Modo”: armonia, basso e melodia lo rispettano.')}`;}});

  add(3,{id:'t-modiuso',ic:'✎',t:'Scrivere in un modo',d:'Evitare di ricadere nel maggiore/minore',ex:'mode',body:X=>`
    <p>Il rischio, scrivendo in un modo, è che l'orecchio lo senta come la scala maggiore “madre”. Tre regole:</p>
    <ol><li><b>Rimarca la tonica</b>: inizia e finisci sulla tonica del modo, tienila al basso (pedale).</li>
      <li><b>Usa la nota caratteristica</b> spesso, negli accordi e nella melodia.</li>
      <li><b>Evita la cadenza V–I della scala madre</b> e gli accordi che contengono il tritono (portano verso la madre).</li></ol>
    ${tbl(['Modo','Accordi caratteristici su '+X.ton,'Da evitare','Ascolta'],[
      ['Dorico',`i – IV – ♭VII (${progN(X,['i','IV','bVII'])})`,'vi° (il tritono)',prog(X,['i','IV','bVII','i'],'▶')],
      ['Frigio',`i – ♭II – ♭VII (${progN(X,['i','bII','bVII'])})`,'v°',prog(X,['i','bII','bVII','i'],'▶')],
      ['Lidio',`I – II – vii (${X.cn(0,'maj')} ${X.cn(2,'maj')} ${X.cn(11,'min')})`,'♯iv°',seq('▶',[X.ch(0,'maj'),X.ch(2,'maj'),X.ch(11,'min'),X.ch(0,'maj')])],
      ['Misolidio',`I – ♭VII – IV (${progN(X,['I','bVII','IV'])})`,'iii°',prog(X,['I','bVII','IV','I'],'▶')],
      ['Eolio',`i – ♭VI – ♭VII (${progN(X,['i','bVI','bVII'])})`,'ii° verso V',prog(X,['i','bVI','bVII','i'],'▶')]])}
    ${h('Pedale e ostinato')}
    <p>Un basso fermo sulla tonica sotto accordi che cambiano è il modo più sicuro di far sentire un modo.</p>
    ${ex2([seq('Pedale di '+X.ton+' (lidio)',[[X.m(0,2),...X.ch(0,'maj',0,4)],[X.m(0,2),...X.ch(2,'maj',0,4)],[X.m(0,2),...X.ch(11,'min',0,3)],[X.m(0,2),...X.ch(0,'maj',0,4)]])])}`});

  add(3,{id:'t-penta',ic:'⬠',t:'Pentatoniche e blues',d:'Cinque note che funzionano sempre',ex:null,body:X=>`
    <p>Le <b>pentatoniche</b> tolgono dalla scala le due note che creano semitoni: niente “note sbagliate”, perfette per melodie e assoli.</p>
    ${tbl(['Scala','Note da '+X.ton,'Ascolta'],[['Pentatonica maggiore',SCM.pmaj.iv.map(x=>X.nm(X.k+x)).join(' '),ex('▶',[...SCM.pmaj.iv,12].map(x=>X.m(x)),1)],['Pentatonica minore',SCM.pmin.iv.map(x=>X.nm(X.k+x)).join(' '),ex('▶',[...SCM.pmin.iv,12].map(x=>X.m(x)),1)],
      ['Blues',SCM.blues.iv.map(x=>X.nm(X.k+x)).join(' '),ex('▶',[...SCM.blues.iv,12].map(x=>X.m(x)),1)],['Blues maggiore',SCM.bluesmaj.iv.map(x=>X.nm(X.k+x)).join(' '),ex('▶',[...SCM.bluesmaj.iv,12].map(x=>X.m(x)),1)]])}
    <p>La pentatonica minore di ${X.nm(X.k+9)} ha le stesse note della maggiore di ${X.ton}: relative, come le scale.</p>
    ${h('Le blue notes')}
    <p>Nel blues la ♭3, la ♭5 e la ♭7 si suonano sopra accordi maggiori: l'attrito è il suono stesso del genere. Spesso si “piega” la ♭3 verso la 3 (bending).</p>
    ${ex2([ex('Lick blues',[X.m(0),X.m(3),X.m(4),X.m(7),X.m(10),X.m(7),X.m(6),X.m(5),X.m(3),X.m(0)],1),seq('Su '+X.cn(0,'7'),[X.ch(0,'7')])])}`});

  add(3,{id:'t-simm',ic:'⬡',t:'Scale simmetriche e jazz',d:'Esatonale, diminuite, alterata, bebop',ex:null,body:X=>`
    ${tbl(['Scala','Note da '+X.ton,'Su quale accordo','Ascolta'],[['Esatonale',SCM.whole.iv.map(x=>X.nm(X.k+x)).join(' '),'7♯5, aumentati',ex('▶',[...SCM.whole.iv,12].map(x=>X.m(x)),1)],
      ['Diminuita ST',SCM.dimHW.iv.map(x=>X.nm(X.k+x)).join(' '),'7♭9',ex('▶',[...SCM.dimHW.iv,12].map(x=>X.m(x)),1)],['Diminuita TS',SCM.dimWH.iv.map(x=>X.nm(X.k+x)).join(' '),'°7',ex('▶',[...SCM.dimWH.iv,12].map(x=>X.m(x)),1)],
      ['Alterata',SCM.altered.iv.map(x=>X.nm(X.k+x)).join(' '),'7alt',ex('▶',[...SCM.altered.iv,12].map(x=>X.m(x)),1)],['Lidia dominante',SCM.lyddom.iv.map(x=>X.nm(X.k+x)).join(' '),'7♯11',ex('▶',[...SCM.lyddom.iv,12].map(x=>X.m(x)),1)],
      ['Bebop dominante',SCM.bebop.iv.map(x=>X.nm(X.k+x)).join(' '),'7',ex('▶',[...SCM.bebop.iv,12].map(x=>X.m(x)),1)]])}
    <p>Le scale <b>simmetriche</b> si ripetono a distanza fissa (l'esatonale ogni tono, le diminuite ogni terza minore): per questo non hanno un vero centro e suonano sospese.</p>
    ${tip('Regola jazz: su ogni accordo si sceglie la scala che contiene le sue note e le tensioni volute (accordo-scala).')}`});

  add(3,{id:'t-mondo',ic:'🌍',t:'Scale del mondo',d:'Arabe, indiane, cinesi, giapponesi, rumene…',ex:null,body:X=>{const cats=[...new Set(SC.filter(s=>s.c.startsWith('Dal mondo')).map(s=>s.c))];return`
    <p>Fuori dalla tradizione occidentale le scale non sono solo “insiemi di note”: hanno regole su come salire e scendere, note di riposo, ornamenti e spesso <b>intervalli più piccoli del semitono</b>. Qui le senti su ${X.ton}; quelle con i quarti di tono (Rast, Bayati, Saba, Slendro) sono suonate con l'intonazione vera.</p>
    ${cats.map(c=>h(c.replace('Dal mondo · ',''))+tbl(['Scala','Note','Carattere','Ascolta'],SC.filter(s=>s.c===c).map(s=>[`<b>${s.n}</b>`,(s.iv.length===7&&s.iv.every(x=>x%1===0)?spell7(X.k,s.iv):s.iv.map(x=>x%1?X.nm(X.k+Math.floor(x))+'*':X.nm(X.k+x))).join(' '),s.d,ex('scala',[...s.iv,12].map(x=>X.m(x)),1)+` <button class="tex ghost" data-explore="${s.id}">accordi →</button>`]))).join('')}
    <p class="dim" style="font-size:12px">* nota alterata di un quarto di tono (non esiste sulla tastiera: si sente nell'esempio).</p>
    ${h('Come usarle in un brano')}
    <ul><li>Tieni un <b>bordone</b> (la tonica al basso) e lascia che la melodia giri attorno ai gradi caratteristici.</li>
      <li>Molte di queste scale non hanno una vera armonia a accordi: usa accordi semplici (tonica, ♭II, IV o la quinta vuota).</li>
      <li>La <b>frigia dominante</b> e la <b>doppia armonica</b> sono le più usate nella musica moderna (trap, reggaeton, colonne sonore) per un sapore “mediorientale”.</li></ul>
    ${ex2([seq('Frigia dominante: I – ♭II – I',[X.ch(0,'maj'),X.ch(1,'maj'),X.ch(0,'maj'),X.ch(1,'maj'),X.ch(0,'maj')]),seq('Doppia armonica su bordone',[[X.m(0,2),X.m(7,2),X.m(0,4)],[X.m(0,2),X.m(7,2),X.m(1,4)],[X.m(0,2),X.m(7,2),X.m(4,4)],[X.m(0,2),X.m(7,2),X.m(1,4)],[X.m(0,2),X.m(7,2),X.m(0,4)]])])}`;}});

  add(4,{id:'t-mel',ic:'♪',t:'Costruire una melodia',d:'Motivo, ripetizione, sequenza, tensione',ex:null,body:X=>{const S=X.sc('major');return`
    <p>Una melodia memorabile nasce da un'idea breve, il <b>motivo</b>, che poi si ripete e si trasforma.</p>
    ${h('Gli strumenti')}
    <ul><li><b>Ripetizione</b>: la stessa idea 2-3 volte la fissa nella memoria.</li><li><b>Sequenza</b>: lo stesso motivo spostato più in alto o in basso.</li>
      <li><b>Variazione</b>: cambia il finale, il ritmo o una nota.</li><li><b>Domanda e risposta</b>: una frase che resta aperta (finisce sulla 2ª o sulla 5ª) e una che chiude (finisce sulla tonica).</li></ul>
    ${ex2([ex('Motivo',[0,2,4,2].map(x=>X.m([0,2,4,5,7,9,11][x])),1),ex('Sequenza',[0,2,4,2,1,3,5,3].map(x=>X.m([0,2,4,5,7,9,11,12][x])),1),ex('Domanda (finisce sulla 5ª)',[4,3,2,1,4].map(x=>X.m([0,2,4,5,7,9,11][x])),1),ex('Risposta (chiude)',[4,3,2,1,0].map(x=>X.m([0,2,4,5,7,9,11][x])),1)])}
    ${h('Note dell\'accordo e note di passaggio')}
    <p>Sui tempi forti metti <b>note dell'accordo</b>; sui deboli puoi usare note di passaggio (tra due note dell'accordo) e di volta (salgono/scendono e tornano). Le <b>appoggiature</b> sono note estranee sul tempo forte che risolvono: molto espressive.</p>
    ${ex2([seq('Note dell\'accordo su '+X.cn(0,'maj'),[[X.m(0,3),X.m(4,3),X.m(7,3),X.m(4,5)]]),ex('Appoggiatura '+S[3]+'→'+S[2],[X.m(5),X.m(4)],1)])}
    ${h('Contorno e culmine')}
    <p>Una buona frase ha un <b>profilo</b> (arco, salita, discesa) e un <b>culmine</b>: la nota più alta, una volta sola, di solito verso i due terzi della frase. Alterna passi (gradi vicini) e salti; dopo un salto grande torna indietro per grado.</p>
    ${tip('Il motore del generatore usa proprio motivo → sequenza → sviluppo → cadenza, e il controcanto risponde nelle pause.')}`;}});

  add(4,{id:'t-forma',ic:'▤',t:'Forma della canzone',d:'Strofa, pre, ritornello, bridge…',ex:null,body:X=>`
    ${tbl(['Sezione','Ruolo','Cosa cambia di solito'],[['Intro','presenta il mondo del brano','pochi strumenti, il motivo del ritornello'],['Strofa','racconta','energia media, melodia più bassa e parlata'],
      ['Pre-ritornello','prepara il salto','armonia che sale o si sospende sul V, densità che cresce'],['Ritornello','il messaggio, il “gancio”','registro più alto, tutti gli strumenti, melodia ripetuta'],
      ['Bridge / Special','cambia prospettiva','accordi nuovi (prestiti, relativa), spesso più vuoto'],['Outro','chiude','riprende il ritornello o l\'intro, si svuota']])}
    ${h('Il contrasto è tutto')}
    <p>Tra strofa e ritornello deve cambiare <b>qualcosa di evidente</b>: registro della melodia, densità, ritmo armonico (accordi più lunghi o più corti), energia della batteria. Nel generatore lo controlli con energia, densità e vuoto/pieno per sezione.</p>
    ${h('Strutture comuni')}
    <p><b>Strofa–Ritornello</b> (V C V C B C) · <b>AABA</b> (standard jazz, 32 battute) · <b>Blues 12 battute</b> · <b>Loop</b> (hip hop, elettronica: lo stesso giro con strati che entrano ed escono).</p>`});

  add(4,{id:'t-arr',ic:'☰',t:'Arrangiamento',d:'Ruoli, registri, spazio',ex:null,body:X=>`
    <p>Arrangiare è dare a ogni strumento un <b>ruolo</b> e un <b>posto</b>: nel registro, nel ritmo e nel tempo.</p>
    ${tbl(['Ruolo','Chi','Regola'],[['Fondamenta','basso, cassa','suonano insieme; il basso segue la cassa'],['Ritmo armonico','piano, chitarra ritmica','uno solo comanda il ritmo, l\'altro tiene o tace'],
      ['Melodia','voce, lead','sopra a tutti, con spazio intorno'],['Controcanto','seconda voce, archi','risponde nelle pause, più basso'],['Colore','pad, arpeggi, percussioni','riempiono senza coprire']])}
    ${h('Tre regole pratiche')}
    <ol><li><b>Registri separati</b>: basso sotto Do3, accordi tra Do3 e Do5, melodia sopra. Se due strumenti stanno nella stessa zona, uno dei due deve tacere o cambiare ottava.</li>
      <li><b>Domanda e risposta</b>: quando la melodia canta, gli altri tengono; quando la melodia respira, qualcuno risponde.</li>
      <li><b>Sottrarre</b>: il ritornello sembra più grande se la strofa è più vuota. Usa vuoti ritmici (stop, mezza battuta senza batteria) prima dei momenti importanti.</li></ol>
    ${tip('Il generatore applica queste regole: piano e arpeggio scendono sotto la melodia, il controcanto entra nelle pause, la chitarra ritmica fa calmare il piano.')}`});

  add(4,{id:'t-groove',ic:'🥁',t:'Groove e generi',d:'Backbeat, half-time, one drop, dembow, clave',ex:null,body:X=>`
    <p>Ogni genere ha un'<b>ossatura ritmica</b> riconoscibile. Ascolta cassa (K) e rullante (S):</p>
    ${tbl(['Groove','Dove','BPM','Ascolta'],[['Backbeat (rullante 2 e 4)','pop, rock, soul','80–130',rhy('▶','K...S...K.K.S...',100,[36,38])],['Boom bap','hip hop anni ’90','84–96',rhy('▶','K......KS.K...S.',90,[36,38])],
      ['Half-time','trap, drill','130–160',rhy('▶','K......K..K.S...',140,[36,38])],['One drop','reggae','70–90',rhy('▶','........B.......',76,[36,37])],['Steppers','reggae, dub','70–90',rhy('▶','K...K...B...K...',76,[36,37])],
      ['Dembow','dancehall, reggaeton','90–105',rhy('▶','K..SK.S.K..SK.S.',96,[36,38])],['Four on the floor','house, disco','118–128',rhy('▶','K...K...K...K...',122,[36])],
      ['Clave 3-2','salsa, afro','—',rhy('▶','9..9..9...9.9...',100,[37])],['Tresillo','afrobeats, reggaeton','—',rhy('▶','9..9..9.9..9..9.',100,[37])]])}
    <p>Nei pattern: K = cassa, S = rullante, B = cassa e cross-stick insieme. Il generatore ha questi groove come stili della batteria (🎲 nella traccia → “Rigenera come…”).</p>`});

  /* ---------- esploratore di scale ---------- */
  function explorer(X,sid){const s=SCM[sid]||SCM.major,seven=s.iv.length===7&&s.iv.every(x=>x%1===0),notes=[...s.iv,12].map(x=>X.m(x));
    const names=seven?spell7(X.k,s.iv):s.iv.map(x=>x%1?X.nm(X.k+Math.round(x))+'*':X.nm(X.k+x));
    const steps=s.iv.map((x,i)=>{const d=(s.iv[i+1]!=null?s.iv[i+1]:12)-x;return d===1?'S':d===2?'T':d===3?'T½':d===.5?'¼':d===1.5?'¾':d===2.5?'T¼':String(d);});
    const dia=seven?diatonic(X,sid):null,dia7=seven?diatonic(X,sid,1):null;
    const gm={major:1,minor:1,harmonic:1,dorian:1,phrygian:1,lydian:1,mixolydian:1}[sid];
    let cat='';const opts=[...new Set(SC.map(x=>x.c))].map(c=>`<optgroup label="${c}">${SC.filter(x=>x.c===c).map(x=>`<option value="${x.id}" ${x.id===s.id?'selected':''}>${x.n}</option>`).join('')}</optgroup>`).join('');
    return`<div class="explore"><label class="field"><span class="label">Scala</span><select id="xScale">${opts}</select></label></div>
      <p><b>${X.ton} ${s.n.toLowerCase()}</b> — ${s.d} <span class="dim">· ${s.u}</span></p>
      ${tbl(['Note',...names],[['Intervalli',...s.iv.map(ivLab)],['Passi',...steps]])}
      ${ex2([ex('Scala in salita',notes,1),ex('In discesa',notes.slice().reverse(),1),ex('Tutte insieme (cluster)',notes.slice(0,-1))])}
      ${dia?h('Accordi della scala')+tbl(['Grado',...dia.map(c=>c.rn)],[['Triade',...dia.map(c=>X.cn(c.off,c.q))],['Settima',...dia7.map(c=>X.cn(c.off,c.q))]])+ex2(dia.map(c=>ex(c.rn,X.ch(c.off,c.q))))+ex2(dia7.map(c=>ex(c.rn.replace(/7$/,'')+' (7)',X.ch(c.off,c.q))))
        :`<p class="dim">Questa scala non ha 7 note: si armonizza con accordi semplici (tonica, quinta vuota, bordone).</p>${ex2([seq('Su bordone',s.iv.map(x=>[X.m(0,2),X.m(7,2),X.m(x,4)]))])}`}
      ${gm?`<div class="tnav" style="border:0;margin-top:6px"><button class="btn primary" id="xUse">Usa ${X.ton} ${s.n.toLowerCase()} nel generatore →</button></div>`:''}`;}

  /* ---------- tastiera e riproduzione ---------- */
  const NI=['Do','Do♯','Re','Mi♭','Mi','Fa','Fa♯','Sol','La♭','La','Si♭','Si'];
  function kbHtml(){let h='';const lo=48,hi=84,wh=[];for(let m=lo;m<=hi;m++)if(![1,3,6,8,10].includes(m%12))wh.push(m);const w=100/wh.length;
    wh.forEach((m,i)=>h+=`<div class="w" data-m="${m}" style="left:${i*w}%;width:${w}%">${m%12===0?'Do'+(Math.floor(m/12)-1):''}</div>`);
    for(let m=lo;m<=hi;m++)if([1,3,6,8,10].includes(m%12)){const i=wh.indexOf(m-1);h+=`<div class="k" data-m="${m}" style="left:${(i+1)*w-w*.31}%;width:${w*.62}%"></div>`;}
    return h;}
  let timers=[],curX=null;
  function light(notes,nm){const kb=document.getElementById('tkb');if(!kb)return;kb.querySelectorAll('.sel,.qt').forEach(e=>e.classList.remove('sel','qt'));
    notes.forEach(n=>{let m=Math.floor(n);while(m<48)m+=12;while(m>84)m-=12;const e=kb.querySelector(`[data-m="${m}"]`);if(e)e.classList.add(n%1?'qt':'sel');});
    const lab=document.getElementById('tkbl'),names=notes.map(n=>(n%1?NI[Math.floor(n)%12]+'↑¼':(curX?curX.nm(n):NI[n%12])));
    if(lab)lab.innerHTML=nm?`<b>${nm}</b> — `+names.join(' · '):names.join(' · ');}
  function play(btn){timers.forEach(clearTimeout);timers=[];Synth.init();const t0=Synth.now()+.05;
    if(btn.dataset.rhy){const R=JSON.parse(btn.dataset.rhy),n=R.pat.length,spb=60/R.bpm,B=R.beats||4,step=B/n*spb;
      for(let rep=0;rep<2;rep++)R.pat.split('').forEach((c,i)=>{if(c==='.')return;const t=t0+rep*B*spb+i*step;
        if(c==='K')Synth.hit(36,t,105,.2);else if(c==='S')Synth.hit(38,t,100,.2);else if(c==='B'){Synth.hit(36,t,105,.2);Synth.hit(37,t,95,.1);}
        else if(R.notes[0]>=48)Synth.note('piano',R.notes[0],t,step*.9,40+(+c)*9,'ex');else Synth.hit(R.notes[0],t,40+(+c)*9,.12);});
      for(let b=0;b<2*B;b++)Synth.hit(42,t0+b*spb,38,.05);return;}
    if(btn.dataset.seq){const S=JSON.parse(btn.dataset.seq),nm=btn.dataset.names?JSON.parse(btn.dataset.names):[],d=1.15;
      S.forEach((c,i)=>{c.forEach(n=>Synth.note('piano',n,t0+i*d,d*.95,78,'ex'));timers.push(setTimeout(()=>light(c,nm[i]),i*d*1000+40));});return;}
    const n=JSON.parse(btn.dataset.n),arp=!!btn.dataset.arp,g=arp?.3:0;
    n.forEach((x,i)=>Synth.note('piano',x,t0+i*g,arp?.6:1.8,80,'ex'));
    if(arp)n.forEach((x,i)=>timers.push(setTimeout(()=>light([x]),i*g*1000+40)));else light(n,btn.textContent.replace('▶','').trim());}
  let key=LS.get('pg_theoryKey',0),scaleSel=LS.get('pg_theoryScale','major');
  const LESSONS=L.slice();LESSONS.splice(LESSONS.findIndex(x=>x.id==='t-mondo')+1,0,{mod:3,id:'t-explore',ic:'🔎',t:'Esploratore di scale',d:'Qualsiasi scala in qualsiasi tonalità, con i suoi accordi',ex:null,body:X=>explorer(X,scaleSel)});
  function render(id,goEx){
    const Ls=LESSONS.find(x=>x.id===id)||LESSONS[0],i=LESSONS.indexOf(Ls),m=document.getElementById('exMain'),X=curX=ctxOf(key);
    const keys=Array.from({length:12},(_,k)=>`<button class="kk ${k===key?'on':''}" data-key="${k}">${(FLATKEY.has(k)?FL:SH)[k]}</button>`).join('');
    m.innerHTML=`<div class="exbar"><div><div class="label">${MODS[Ls.mod]} · lezione ${i+1} di ${LESSONS.length}</div><h2>${Ls.t}</h2><div class="muted" style="font-size:13px">${Ls.d}</div></div></div>
      <div class="keysel"><span class="label">Tonalità</span>${keys}</div>
      <div class="tkbw"><div class="kb tkb" id="tkb">${kbHtml()}</div><div class="tkbl" id="tkbl">Premi ▶ su un esempio: lo senti e lo vedi sulla tastiera.</div></div>
      <div class="tbody">${Ls.body(X)}</div>
      <div class="tnav">${i>0?`<button class="btn sm" data-go="${LESSONS[i-1].id}">← ${LESSONS[i-1].t}</button>`:''}<span class="spacer"></span>
        ${Ls.ex?`<button class="btn primary" id="tTry">Mettiti alla prova →</button>`:''}${i<LESSONS.length-1?`<button class="btn sm" data-go="${LESSONS[i+1].id}">${LESSONS[i+1].t} →</button>`:''}</div>`;
    m.querySelectorAll('.tex[data-n],.tex[data-seq],.tex[data-rhy]').forEach(b=>b.onclick=()=>play(b));
    m.querySelectorAll('[data-go]').forEach(b=>b.onclick=()=>goEx(b.dataset.go,true));
    m.querySelectorAll('[data-key]').forEach(b=>b.onclick=()=>{key=+b.dataset.key;LS.set('pg_theoryKey',key);render(Ls.id,goEx);});
    m.querySelectorAll('[data-cof]').forEach(b=>b.onclick=()=>{key=+b.dataset.cof;LS.set('pg_theoryKey',key);render(Ls.id,goEx);});
    m.querySelectorAll('[data-explore]').forEach(b=>b.onclick=()=>{scaleSel=b.dataset.explore;LS.set('pg_theoryScale',scaleSel);goEx('t-explore',true);});
    const xs=document.getElementById('xScale');if(xs)xs.onchange=()=>{scaleSel=xs.value;LS.set('pg_theoryScale',scaleSel);render(Ls.id,goEx);};
    const xu=document.getElementById('xUse');if(xu)xu.onclick=()=>{St.opts.mode=scaleSel;St.opts.key=String(key);syncControls();saveOpts();dropEdits();St.reseed={L:{},S:{}};regen();
      document.querySelector('.tabs [data-tab="gen"]').click();toast(`Generatore: ${X.ton} ${SCM[scaleSel].n.toLowerCase()}`);};
    const tr=document.getElementById('tTry');if(tr)tr.onclick=()=>goEx(Ls.ex,false);
  }
  return{LESSONS,MODS,render,SC};
})();
