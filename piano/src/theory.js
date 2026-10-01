'use strict';
/* =====================================================================
   TEORIA — lezioni brevi con esempi da ascoltare (tastiera che si illumina)
   ===================================================================== */
const Theory=(()=>{
  const NI=['Do','Do♯','Re','Mi♭','Mi','Fa','Fa♯','Sol','La♭','La','Si♭','Si'];
  const L2M={C:0,D:2,E:4,F:5,G:7,A:9,B:11};
  // 'C4' 'Eb3' 'F#4' → midi
  const M=s=>{const m=s.match(/^([A-G])([#b]?)(-?\d)$/);return 12*(+m[3]+1)+L2M[m[1]]+(m[2]==='#'?1:m[2]==='b'?-1:0);};
  const ns=s=>s.trim().split(/\s+/).map(M);
  // accordo: radice ('C3'), qualità, rivolto
  const ch=(root,q,inv=0)=>{let n=QT[q].iv.map(x=>M(root)+x);for(let i=0;i<inv;i++){n.push(n.shift()+12);}return n;};
  const J=x=>JSON.stringify(x).replace(/"/g,'&quot;');
  // pulsanti esempio: note insieme, arpeggio, sequenza di accordi
  const ex=(label,notes,arp)=>`<button class="tex" data-n="${J(notes)}"${arp?' data-arp="1"':''}>▶ ${label}</button>`;
  const seq=(label,chords,names)=>`<button class="tex tseq" data-seq="${J(chords)}"${names?` data-names="${J(names)}"`:''}>▶ ${label}</button>`;
  const tip=t=>`<div class="ttip">💡 ${t}</div>`;
  const tbl=(head,rows)=>`<div class="ttbl"><table><thead><tr>${head.map(h=>`<th>${h}</th>`).join('')}</tr></thead><tbody>${rows.map(r=>`<tr>${r.map(c=>`<td>${c}</td>`).join('')}</tr>`).join('')}</tbody></table></div>`;
  const prog=(key,rns,oct=3)=>rns.map(rn=>{const p=parseRN(rn.replace(/7|maj|ø|°/g,'')),q=/maj7/.test(rn)?'maj7':/ø/.test(rn)?'m7b5':/°7/.test(rn)?'dim7':/7/.test(rn)?(p.tri==='min'?'m7':'7'):p.tri;
    let root=M(key+oct)+p.r;if(root>M('E'+oct)+2)root-=12;return QT[q].iv.map(x=>root+x);});

  const LESSONS=[
  {id:'t-note',ic:'𝄞',t:'Note e tastiera',d:'Semitoni, toni, diesis e bemolle',ex:'interval',body:()=>`
    <p>Nella musica occidentale ci sono <b>12 note</b> che si ripetono a ogni ottava. Sulla tastiera sono i 7 tasti bianchi e i 5 neri.
    In italiano si chiamano Do Re Mi Fa Sol La Si, nella notazione anglosassone (quella degli accordi: C, Dm, G7…) si usano le lettere.</p>
    ${tbl(['Italiano','Do','Re','Mi','Fa','Sol','La','Si'],[['Lettera','C','D','E','F','G','A','B']])}
    <p>La distanza più piccola tra due tasti vicini (bianco o nero) è il <b>semitono</b>. Due semitoni fanno un <b>tono</b>.
    Tra Mi–Fa e Si–Do non c'è il tasto nero: sono semitoni “naturali”.</p>
    <p>Il <b>diesis ♯</b> alza di un semitono, il <b>bemolle ♭</b> abbassa di un semitono. Lo stesso tasto può avere due nomi (Do♯ = Re♭): si chiamano <b>enarmonici</b>.</p>
    <div class="texs">${ex('Scala cromatica (12 semitoni)',ns('C4 C#4 D4 Eb4 E4 F4 F#4 G4 Ab4 A4 Bb4 B4 C5'),1)}${ex('Semitono: Mi–Fa',ns('E4 F4'),1)}${ex('Tono: Do–Re',ns('C4 D4'),1)}</div>
    ${tip('L\'ottava è la stessa nota 12 semitoni più in alto: suona “uguale ma più acuta”.')}`},

  {id:'t-int',ic:'↕',t:'Intervalli',d:'Le distanze che costruiscono tutto',ex:'interval',body:()=>`
    <p>Un <b>intervallo</b> è la distanza tra due note, contata in semitoni. Gli accordi e le melodie sono fatti di intervalli: riconoscerli è la base dell'orecchio.</p>
    ${tbl(['Semitoni','Nome','Carattere','Ascolta'],[
      [1,'2ª minore','molto teso, “stridente”',ex('Do–Re♭',ns('C4 Db4'),1)],[2,'2ª maggiore','passo di scala',ex('Do–Re',ns('C4 D4'),1)],
      [3,'3ª minore','scuro, malinconico',ex('Do–Mi♭',ns('C4 Eb4'),1)],[4,'3ª maggiore','luminoso, aperto',ex('Do–Mi',ns('C4 E4'),1)],
      [5,'4ª giusta','solido, “sospeso”',ex('Do–Fa',ns('C4 F4'),1)],[6,'Tritono','instabile, cerca di risolvere',ex('Do–Fa♯',ns('C4 F#4'),1)],
      [7,'5ª giusta','vuoto, stabilissimo',ex('Do–Sol',ns('C4 G4'),1)],[8,'6ª minore','dolce-amaro',ex('Do–La♭',ns('C4 Ab4'),1)],
      [9,'6ª maggiore','caldo, cantabile',ex('Do–La',ns('C4 A4'),1)],[10,'7ª minore','blues, da dominante',ex('Do–Si♭',ns('C4 Bb4'),1)],
      [11,'7ª maggiore','sognante, tesa',ex('Do–Si',ns('C4 B4'),1)],[12,'Ottava','la stessa nota',ex('Do–Do',ns('C4 C5'),1)]])}
    <p><b>Giusto</b> si usa per 4ª, 5ª e ottava; <b>maggiore/minore</b> per 2ª, 3ª, 6ª, 7ª. Un intervallo è <b>melodico</b> se le note sono una dopo l'altra, <b>armonico</b> se suonano insieme.</p>
    ${tip('Gli intervalli sopra l\'ottava sono le <b>estensioni</b>: 9ª = 2ª + ottava, 11ª = 4ª + ottava, 13ª = 6ª + ottava.')}`},

  {id:'t-scale',ic:'≡',t:'Scale maggiore e minore',d:'Le formule di toni e semitoni',ex:'degree',body:()=>`
    <p>Una <b>scala</b> è una scelta di 7 note dentro le 12. Quello che la definisce è la sequenza di toni (T) e semitoni (S).</p>
    ${tbl(['Scala','Formula','Esempio da Do'],[
      ['Maggiore','T T S T T T S','Do Re Mi Fa Sol La Si'],
      ['Minore naturale','T S T T S T T','Do Re Mi♭ Fa Sol La♭ Si♭'],
      ['Minore armonica','come la naturale ma 7ª alzata','Do Re Mi♭ Fa Sol La♭ <b>Si</b>'],
      ['Minore melodica','6ª e 7ª alzate','Do Re Mi♭ Fa Sol <b>La Si</b>']])}
    <div class="texs">${ex('Do maggiore',ns('C4 D4 E4 F4 G4 A4 B4 C5'),1)}${ex('Do minore naturale',ns('C4 D4 Eb4 F4 G4 Ab4 Bb4 C5'),1)}${ex('Do minore armonica',ns('C4 D4 Eb4 F4 G4 Ab4 B4 C5'),1)}${ex('Do minore melodica',ns('C4 D4 Eb4 F4 G4 A4 B4 C5'),1)}</div>
    <p>Ogni scala maggiore ha una <b>relativa minore</b> con le stesse note, che parte dal 6° grado: Do maggiore ↔ La minore. Cambia il “centro”, cioè la nota su cui l'orecchio si riposa.</p>
    <div class="texs">${ex('Do maggiore',ns('C4 D4 E4 F4 G4 A4 B4 C5'),1)}${ex('La minore (stesse note)',ns('A3 B3 C4 D4 E4 F4 G4 A4'),1)}</div>
    ${tip('La minore armonica esiste per avere un accordo di dominante <b>maggiore</b> (Sol–Si–Re in Do minore): la 7ª alzata è la “sensibile” che spinge verso la tonica.')}`},

  {id:'t-triadi',ic:'△',t:'Triadi',d:'Maggiore, minore, diminuita, aumentata',ex:'quality',body:()=>`
    <p>Una <b>triade</b> sono tre note sovrapposte per terze: <b>fondamentale</b>, <b>terza</b>, <b>quinta</b>. Il tipo di terze decide il carattere.</p>
    ${tbl(['Triade','Terze','Semitoni dalla fondamentale','Sigla','Ascolta'],[
      ['Maggiore','3ª magg + 3ª min','0 · 4 · 7','C',ex('Do',ch('C4','maj'))],
      ['Minore','3ª min + 3ª magg','0 · 3 · 7','Cm',ex('Do minore',ch('C4','min'))],
      ['Diminuita','3ª min + 3ª min','0 · 3 · 6','Cdim / C°',ex('Do dim',ch('C4','dim'))],
      ['Aumentata','3ª magg + 3ª magg','0 · 4 · 8','Caug / C+',ex('Do aum',ch('C4','aug'))]])}
    <p>La differenza tra maggiore e minore è <b>una sola nota</b>: la terza. Mi naturale = maggiore, Mi♭ = minore.</p>
    <div class="texs">${seq('Maggiore → minore → maggiore',[ch('C4','maj'),ch('C4','min'),ch('C4','maj')],['C','Cm','C'])}</div>
    <p>Gli accordi <b>sus</b> (sospesi) tolgono la terza e la sostituiscono con la 2ª (sus2) o la 4ª (sus4): né maggiori né minori, aperti, “in attesa”.</p>
    <div class="texs">${ex('Csus2',ch('C4','sus2'))}${ex('Csus4',ch('C4','sus4'))}${seq('Sus4 che risolve',[ch('C4','sus4'),ch('C4','maj')],['Csus4','C'])}</div>
    ${tip('Per costruire una triade: dalla fondamentale salta una nota della scala, prendi la successiva, salta ancora. Do (salta Re) Mi (salta Fa) Sol.')}`},

  {id:'t-rivolti',ic:'⇅',t:'Rivolti',d:'Quale nota sta al basso',ex:'inversion',body:()=>`
    <p>Le note di un accordo si possono disporre in ordini diversi. Quello che conta è <b>quale nota è la più grave</b>:</p>
    ${tbl(['Posizione','Nota al basso','Do maggiore','Sigla','Ascolta'],[
      ['Stato fondamentale','fondamentale','Do · Mi · Sol','C',ex('C',ch('C4','maj',0))],
      ['Primo rivolto','terza','Mi · Sol · Do','C/E',ex('C/E',ch('C4','maj',1))],
      ['Secondo rivolto','quinta','Sol · Do · Mi','C/G',ex('C/G',ch('C4','maj',2))]])}
    <p>Gli accordi di quattro note hanno anche il <b>terzo rivolto</b>, con la settima al basso (es. G7/F): molto instabile, vuole scendere.</p>
    <div class="texs">${ex('G7',ch('G3','7',0))}${ex('G7/B',ch('G3','7',1))}${ex('G7/D',ch('G3','7',2))}${ex('G7/F',ch('G3','7',3))}</div>
    <p><b>La sigla “/”</b> (slash chord) indica la nota al basso: <b>C/E</b> = accordo di Do con Mi al basso. Può anche indicare una nota che non fa parte dell'accordo (es. D/C).</p>
    <p><b>Perché si usano:</b></p>
    <ul><li><b>Collegare gli accordi</b> con poco movimento: invece di saltare, le mani si spostano di poco (voice leading).</li>
      <li><b>Bassi che camminano</b>: con i rivolti il basso può scendere o salire per gradi.</li>
      <li><b>Colore</b>: il primo rivolto è più leggero, il secondo è sospeso e instabile (tipico prima della dominante nelle cadenze classiche).</li></ul>
    <div class="texs">${seq('Basso che scende: C – G/B – Am – F/A',[[48,64,67,72],[47,62,67,71],[45,64,69,72],[45,65,69,72]],['C','G/B','Am','F/A'])}
      ${seq('Senza rivolti (salti)',[ch('C3','maj'),ch('G3','maj'),ch('A3','min'),ch('F3','maj')],['C','G','Am','F'])}${seq('Con rivolti (mano ferma)',[ch('C4','maj'),ch('G3','maj',2),ch('A3','min',1),ch('F3','maj',2)],['C','G','Am','F'])}</div>
    ${tip('Per riconoscerli a orecchio ascolta <b>solo la nota più grave</b>: se è la stessa che dà il nome all\'accordo è stato fondamentale, altrimenti è un rivolto.')}`},

  {id:'t-sett',ic:'7',t:'Accordi di settima',d:'Maj7, 7, m7, semidiminuito, diminuito',ex:'quality',body:()=>`
    <p>Aggiungendo un'altra terza sopra la triade si ottiene un accordo di <b>settima</b>: quattro note, più ricco e con più “direzione”.</p>
    ${tbl(['Accordo','Struttura','Sigla','Carattere','Ascolta'],[
      ['Settima maggiore','triade magg + 7ª magg','Cmaj7 / CΔ','morbido, sognante',ex('Cmaj7',ch('C4','maj7'))],
      ['Settima di dominante','triade magg + 7ª min','C7','teso, vuole risolvere',ex('C7',ch('C4','7'))],
      ['Minore settima','triade min + 7ª min','Cm7','caldo, soul',ex('Cm7',ch('C4','m7'))],
      ['Semidiminuito','triade dim + 7ª min','Cm7♭5 / Cø','scuro, introduce la dominante in minore',ex('Cø7',ch('C4','m7b5'))],
      ['Diminuito','triade dim + 7ª dim','Cdim7 / C°7','drammatico, simmetrico',ex('C°7',ch('C4','dim7'))],
      ['Minore maj7','triade min + 7ª magg','Cm(maj7)','noir, cinematico',ex('Cm(maj7)',ch('C4','mmaj7'))]])}
    <p>Sulla scala maggiore le settime “naturali” sono sempre le stesse:</p>
    ${tbl(['I','ii','iii','IV','V','vi','vii'],[['maj7','m7','m7','maj7','7','m7','ø7'],['Cmaj7','Dm7','Em7','Fmaj7','G7','Am7','Bø7']])}
    <div class="texs">${seq('ii – V – I in Do',[ch('D3','m7'),ch('G3','7'),ch('C3','maj7')],['Dm7','G7','Cmaj7'])}</div>
    ${tip('Il V7 (G7 in Do) è l\'unico accordo di dominante “naturale”: contiene il tritono Si–Fa, che risolve su Do–Mi.')}`},

  {id:'t-est',ic:'✦',t:'Estensioni e colori',d:'add9, 6, 9, 11, 13',ex:'quality',body:()=>`
    <p>Le <b>estensioni</b> sono note sopra l'ottava che aggiungono colore senza cambiare la funzione dell'accordo.</p>
    ${tbl(['Sigla','Note (da Do)','Effetto','Ascolta'],[
      ['Cadd9','Do Mi Sol + Re','aperto, pop',ex('Cadd9',ch('C4','add9'))],
      ['C6','Do Mi Sol La','vintage, rilassato',ex('C6',ch('C4','6'))],
      ['C6/9','Do Mi Sol La Re','jazz, bossa',ex('C6/9',ch('C4','69'))],
      ['Cmaj9','Cmaj7 + Re','neo soul, lo-fi',ex('Cmaj9',ch('C3','maj9'))],
      ['Cm9','Cm7 + Re','caldo, morbido',ex('Cm9',ch('C3','m9'))],
      ['C9','C7 + Re','funk, blues',ex('C9',ch('C3','9'))],
      ['Cm11','Cm7 + Re + Fa','modale, spazioso',ex('Cm11',ch('C3','m11'))],
      ['C13','C7 + Re + La','gospel, jazz',ex('C13',ch('C3','13'))],
      ['Cmaj7♯11','Cmaj7 + Fa♯','lidio, sospeso',ex('Cmaj7♯11',ch('C3','maj7#11'))]])}
    <p><b>add9</b> aggiunge la 9ª senza la 7ª; <b>9</b> significa che c'è anche la 7ª. Sul pianoforte spesso si <b>omette la 5ª</b> per fare spazio alle estensioni.</p>
    ${tip('Sugli accordi maggiori l\'11ª naturale (Fa su Do) stona con la terza: si usa la ♯11. Sui minori invece l\'11ª suona benissimo.')}`},

  {id:'t-gradi',ic:'Ⅳ',t:'Gradi e numeri romani',d:'Armonia diatonica in ogni tonalità',ex:'degree',body:()=>`
    <p>Costruendo una triade su ogni nota della scala si ottengono gli <b>accordi diatonici</b>. Si indicano con i <b>numeri romani</b>: maiuscolo = maggiore, minuscolo = minore, ° = diminuito.</p>
    ${tbl(['Grado','I','ii','iii','IV','V','vi','vii°'],[['Do maggiore','C','Dm','Em','F','G','Am','B°'],['Sol maggiore','G','Am','Bm','C','D','Em','F♯°']])}
    ${tbl(['Grado (minore)','i','ii°','III','iv','v / V','VI','VII'],[['La minore','Am','B°','C','Dm','Em / E','F','G']])}
    <p>Il vantaggio dei numeri romani è che valgono <b>in tutte le tonalità</b>: “I–V–vi–IV” è lo stesso giro in Do (C G Am F) e in Sol (G D Em C).</p>
    <div class="texs">${seq('I–V–vi–IV in Do',prog('C',['I','V','vi','IV']),['C','G','Am','F'])}${seq('I–V–vi–IV in Sol',prog('G',['I','V','vi','IV'],2),['G','D','Em','C'])}</div>
    ${tip('Nelle sigle dei brani trovi anche gradi “fuori scala” col ♭: ♭VII, ♭VI, ♭III. Sono prestiti dal modo minore (vedi Interscambio modale).')}`},

  {id:'t-funz',ic:'⟳',t:'Funzioni armoniche',d:'Tonica, sottodominante, dominante',ex:'prog',body:()=>`
    <p>Ogni accordo ha un <b>ruolo</b> nel discorso armonico. Le funzioni sono tre:</p>
    ${tbl(['Funzione','Ruolo','Accordi (in maggiore)'],[
      ['<span class="fdot fT"></span>Tonica (T)','riposo, “casa”','I, vi, (iii)'],
      ['<span class="fdot fS"></span>Sottodominante (S)','si allontana, prepara','IV, ii'],
      ['<span class="fdot fD"></span>Dominante (D)','tensione, vuole tornare a casa','V, vii°']])}
    <p>Il movimento tipico è <b>T → S → D → T</b>: partenza, allontanamento, tensione, ritorno. Quasi tutte le progressioni sono variazioni di questo ciclo.</p>
    <div class="texs">${seq('T – S – D – T: C F G C',prog('C',['I','IV','V','I']),['C','F','G','C'])}${seq('Con sostituti: Am Dm G7 C',prog('C',['vi','ii','V7','I']),['Am','Dm','G7','C'])}</div>
    <p>Accordi della stessa funzione si possono <b>sostituire</b>: Am al posto di C (tonica), Dm al posto di F (sottodominante).</p>
    ${tip('Nell\'editor degli accordi del generatore i colori dei blocchi sono proprio queste funzioni: verde tonica, blu sottodominante, arancio dominante.')}`},

  {id:'t-cad',ic:'⤓',t:'Cadenze',d:'Come finisce una frase',ex:'prog',body:()=>`
    <p>Una <b>cadenza</b> è la coppia di accordi che chiude una frase, come la punteggiatura in una frase scritta.</p>
    ${tbl(['Cadenza','Accordi','Effetto','Ascolta'],[
      ['Autentica (perfetta)','V → I','punto fermo',seq('G7 → C',[ch('G3','7'),ch('C3','maj')])],
      ['Plagale','IV → I','“amen”, morbida',seq('F → C',[ch('F3','maj'),ch('C3','maj')])],
      ['Sospesa (semicadenza)','… → V','virgola, domanda',seq('C → Am → Dm → G',[ch('C3','maj'),ch('A3','min'),ch('D3','min'),ch('G3','maj')])],
      ['D\'inganno','V → vi','sorpresa, “finta fine”',seq('G7 → Am',[ch('G3','7'),ch('A3','min')])],
      ['Minore','iv → i / V → i','malinconica',seq('Fm → Cm',[ch('F3','min'),ch('C3','min')])]])}
    ${tip('Le strofe spesso finiscono con una semicadenza (sul V) per “lanciare” il ritornello, che invece chiude sulla tonica.')}`},

  {id:'t-prog',ic:'⟶',t:'Progressioni tipiche',d:'I giri che si sentono ovunque',ex:'prog',body:()=>`
    <p>Alcuni giri di accordi tornano in migliaia di brani. Conoscerli aiuta a riconoscerli e a variarli.</p>
    ${tbl(['Giro','In Do','Dove','Ascolta'],[
      ['I – V – vi – IV','C G Am F','pop',seq('Ascolta',prog('C',['I','V','vi','IV']))],
      ['vi – IV – I – V','Am F C G','pop malinconico',seq('Ascolta',prog('C',['vi','IV','I','V']))],
      ['I – vi – IV – V','C Am F G','anni \'50, doo-wop',seq('Ascolta',prog('C',['I','vi','IV','V']))],
      ['ii – V – I','Dm7 G7 Cmaj7','jazz',seq('Ascolta',prog('C',['ii7','V7','Imaj7']))],
      ['i – ♭VII – ♭VI – ♭VII','Cm B♭ A♭ B♭','rock, epico',seq('Ascolta',prog('C',['i','bVII','bVI','bVII']))],
      ['i – iv – v – i','Cm Fm Gm Cm','minore modale',seq('Ascolta',prog('C',['i','iv','v','i']))],
      ['i – ♭VI – ♭III – ♭VII','Am F C G (in La)','trap, cinematico',seq('Ascolta',prog('A',['i','bVI','bIII','bVII'],2))]])}
    <p><b>Blues in 12 battute</b> (in Do): C7 C7 C7 C7 · F7 F7 C7 C7 · G7 F7 C7 G7.</p>
    <div class="texs">${seq('Blues (accordi principali)',[ch('C3','7'),ch('F3','7'),ch('C3','7'),ch('G3','7'),ch('F3','7'),ch('C3','7')],['C7','F7','C7','G7','F7','C7'])}</div>
    <p><b>Circolo delle quinte</b>: ogni accordo è una quinta sotto il precedente (Am → Dm → G → C → F…). È il movimento più “naturale” per l'orecchio.</p>
    <div class="texs">${seq('Am7 – Dm7 – G7 – Cmaj7 – Fmaj7',[ch('A3','m7'),ch('D3','m7'),ch('G3','7'),ch('C3','maj7'),ch('F3','maj7')],['Am7','Dm7','G7','Cmaj7','Fmaj7'])}</div>`},

  {id:'t-secdom',ic:'→',t:'Dominanti secondarie',d:'V/V, V/vi e sostituzione di tritono',ex:'fill',body:()=>`
    <p>Qualsiasi accordo maggiore o minore della tonalità può essere preceduto dalla <b>sua</b> dominante, come se fosse per un attimo la tonica. Si scrive <b>V/x</b> (“quinto di…”).</p>
    ${tbl(['Sigla','In Do','Va verso','Ascolta'],[
      ['V/V','D7','G',seq('C – D7 – G – C',[ch('C3','maj'),ch('D3','7'),ch('G3','maj'),ch('C3','maj')])],
      ['V/vi','E7','Am',seq('C – E7 – Am',[ch('C3','maj'),ch('E3','7'),ch('A3','min')])],
      ['V/IV','C7','F',seq('C – C7 – F',[ch('C3','maj'),ch('C3','7'),ch('F3','maj')])],
      ['V/ii','A7','Dm',seq('C – A7 – Dm – G7',[ch('C3','maj'),ch('A3','7'),ch('D3','min'),ch('G3','7')])]])}
    <p>La nota “fuori scala” (Fa♯ in D7, Sol♯ in E7) diventa una <b>sensibile</b> che spinge verso l'accordo successivo.</p>
    <p><b>Sostituzione di tritono</b> (subV): al posto di G7 si usa D♭7. Hanno lo stesso tritono (Fa–Si/Do♭), ma il basso scende di semitono verso la tonica.</p>
    <div class="texs">${seq('Dm7 – G7 – Cmaj7',[ch('D3','m7'),ch('G2','7'),ch('C3','maj7')])}${seq('Dm7 – D♭7 – Cmaj7',[ch('D3','m7'),ch('Db3','7'),ch('C3','maj7')])}</div>`},

  {id:'t-prest',ic:'◑',t:'Interscambio modale',d:'Prestiti dal minore (e dal maggiore)',ex:'fill',body:()=>`
    <p>Un brano in maggiore può <b>prendere in prestito</b> accordi dal minore con la stessa tonica (Do maggiore ← Do minore). È uno dei trucchi più usati per dare emozione.</p>
    ${tbl(['Prestito','In Do','Effetto','Ascolta'],[
      ['iv','Fm','nostalgia, “commozione”',seq('C – F – Fm – C',[ch('C3','maj'),ch('F3','maj'),ch('F3','min'),ch('C3','maj')])],
      ['♭VII','B♭','rock, epico, liberatorio',seq('C – B♭ – F – C',[ch('C3','maj'),ch('Bb2','maj'),ch('F3','maj'),ch('C3','maj')])],
      ['♭VI','A♭','cinematico, grandioso',seq('C – A♭ – B♭ – C',[ch('C3','maj'),ch('Ab2','maj'),ch('Bb2','maj'),ch('C3','maj')])],
      ['♭III','E♭','sorpresa, colore',seq('C – E♭ – F – C',[ch('C3','maj'),ch('Eb3','maj'),ch('F3','maj'),ch('C3','maj')])],
      ['iiø7','Dm7♭5','tensione scura',seq('Dø7 – G7 – C',[ch('D3','m7b5'),ch('G2','7'),ch('C3','maj')])]])}
    <p>Al contrario, un brano in minore che finisce su un accordo <b>maggiore</b> di tonica usa la “terza piccarda”: un finale luminoso inaspettato.</p>
    <div class="texs">${seq('Am – Dm – E – A (piccarda)',[ch('A2','min'),ch('D3','min'),ch('E3','maj'),ch('A2','maj')])}</div>
    ${tip('Nel popup degli accordi del generatore i prestiti compaiono nella sezione “interscambio modale”.')}`},

  {id:'t-modi',ic:'◐',t:'I modi',d:'Ionio, dorico, frigio, lidio…',ex:'mode',body:()=>`
    <p>I <b>modi</b> sono le 7 scale che si ottengono partendo da ogni nota della scala maggiore. Ognuno ha una <b>nota caratteristica</b> che lo distingue dal maggiore o dal minore.</p>
    ${tbl(['Modo','Da Do','Rispetto a…','Carattere','Ascolta'],[
      ['Ionio (maggiore)','Do Re Mi Fa Sol La Si','—','luminoso',ex('scala',ns('C4 D4 E4 F4 G4 A4 B4 C5'),1)],
      ['Dorico','Do Re Mi♭ Fa Sol <b>La</b> Si♭','minore con 6ª maggiore','minore “positivo”, soul, funk',ex('scala',ns('C4 D4 Eb4 F4 G4 A4 Bb4 C5'),1)],
      ['Frigio','Do <b>Re♭</b> Mi♭ Fa Sol La♭ Si♭','minore con 2ª minore','scuro, spagnolo, trap',ex('scala',ns('C4 Db4 Eb4 F4 G4 Ab4 Bb4 C5'),1)],
      ['Lidio','Do Re Mi <b>Fa♯</b> Sol La Si','maggiore con 4ª aumentata','sognante, cinematico',ex('scala',ns('C4 D4 E4 F#4 G4 A4 B4 C5'),1)],
      ['Misolidio','Do Re Mi Fa Sol La <b>Si♭</b>','maggiore con 7ª minore','rock, blues, festoso',ex('scala',ns('C4 D4 E4 F4 G4 A4 Bb4 C5'),1)],
      ['Eolio (minore)','Do Re Mi♭ Fa Sol La♭ Si♭','—','malinconico',ex('scala',ns('C4 D4 Eb4 F4 G4 Ab4 Bb4 C5'),1)],
      ['Locrio','Do <b>Re♭</b> Mi♭ Fa <b>Sol♭</b> La♭ Si♭','instabile (tonica diminuita)','raro, inquietante',ex('scala',ns('C4 Db4 Eb4 F4 Gb4 Ab4 Bb4 C5'),1)]])}
    <p>Il modo si sente meglio con un <b>vamp</b>, due accordi che mettono in evidenza la nota caratteristica:</p>
    <div class="texs">${seq('Dorico: Cm – F',[ch('C3','m7'),ch('F3','maj'),ch('C3','m7'),ch('F3','maj')])}${seq('Frigio: Cm – D♭',[ch('C3','min'),ch('Db3','maj'),ch('C3','min'),ch('Db3','maj')])}
      ${seq('Lidio: C – D',[ch('C3','maj'),ch('D3','maj'),ch('C3','maj'),ch('D3','maj')])}${seq('Misolidio: C – B♭',[ch('C3','maj'),ch('Bb2','maj'),ch('C3','maj'),ch('Bb2','maj')])}</div>`},

  {id:'t-voice',ic:'≋',t:'Voicing e voice leading',d:'Come disporre e collegare gli accordi',ex:'build',body:()=>`
    <p>Il <b>voicing</b> è il modo in cui distribuisci le note di un accordo sulla tastiera. Lo stesso Cmaj7 può suonare chiuso e compatto o aperto e arioso.</p>
    <div class="texs">${ex('Cmaj7 chiuso',ns('C4 E4 G4 B4'))}${ex('Cmaj7 aperto',ns('C3 G3 E4 B4'))}${ex('Shell: 1–3–7',ns('C3 E3 B3'))}${ex('Rootless (mano destra jazz)',ns('E4 G4 B4 D5'))}</div>
    <ul><li><b>Mano sinistra</b>: fondamentale o fondamentale + quinta/ottava. Le terze troppo in basso suonano “fangose”.</li>
      <li><b>Mano destra</b>: terza e settima sono le note che definiscono l'accordo; la quinta si può togliere.</li>
      <li><b>Shell voicing</b> (1–3–7): il minimo indispensabile, tipico del jazz e del lo-fi.</li></ul>
    <p>Il <b>voice leading</b> è collegare gli accordi muovendo ogni voce il meno possibile: le note comuni restano ferme, le altre si spostano di un tono o semitono.</p>
    <div class="texs">${seq('ii–V–I con voice leading',[ns('D3 F4 A4 C5'),ns('G2 F4 B4 D5'),ns('C3 E4 B4 D5')],['Dm7','G7','Cmaj9'])}${seq('Stesso giro, a blocchi (salti)',[ch('D4','m7'),ch('G4','7'),ch('C4','maj7')],['Dm7','G7','Cmaj7'])}</div>
    ${tip('Nel ii–V–I la settima di un accordo scende di semitono e diventa la terza del successivo (Do → Si, Fa → Mi): è il segreto del suono “che scorre”.')}`}
  ];

  // tastiera di esempio
  function kbHtml(){let h='';const lo=48,hi=84,wh=[];for(let m=lo;m<=hi;m++)if(![1,3,6,8,10].includes(m%12))wh.push(m);const w=100/wh.length;
    wh.forEach((m,i)=>h+=`<div class="w" data-m="${m}" style="left:${i*w}%;width:${w}%">${m%12===0?'Do'+(Math.floor(m/12)-1):''}</div>`);
    for(let m=lo;m<=hi;m++)if([1,3,6,8,10].includes(m%12)){const i=wh.indexOf(m-1);h+=`<div class="k" data-m="${m}" style="left:${(i+1)*w-w*.31}%;width:${w*.62}%"></div>`;}
    return h;}
  let timers=[];
  function light(notes,nm){const kb=document.getElementById('tkb');if(!kb)return;kb.querySelectorAll('.sel').forEach(e=>e.classList.remove('sel'));
    notes.forEach(n=>{let m=n;while(m<48)m+=12;while(m>84)m-=12;const e=kb.querySelector(`[data-m="${m}"]`);if(e)e.classList.add('sel');});
    const lab=document.getElementById('tkbl');if(lab)lab.innerHTML=nm?`<b>${nm}</b> — `+notes.map(n=>NI[n%12]).join(' · '):notes.map(n=>NI[n%12]).join(' · ');}
  function play(btn){timers.forEach(clearTimeout);timers=[];Synth.init();const t0=Synth.now()+.05;
    if(btn.dataset.seq){const S=JSON.parse(btn.dataset.seq),nm=btn.dataset.names?JSON.parse(btn.dataset.names):[],d=1.15;
      S.forEach((c,i)=>{c.forEach(n=>Synth.note('piano',n,t0+i*d,d*.95,78,'ex'));timers.push(setTimeout(()=>light(c,nm[i]),i*d*1000+40));});return;}
    const n=JSON.parse(btn.dataset.n),arp=!!btn.dataset.arp,g=arp?.32:0;
    n.forEach((x,i)=>Synth.note('piano',x,t0+i*g,arp?.6:1.8,80,'ex'));
    if(arp)n.forEach((x,i)=>timers.push(setTimeout(()=>light([x]),i*g*1000+40)));else light(n,btn.textContent.replace('▶','').trim());}
  function render(id,goEx){
    const L=LESSONS.find(x=>x.id===id),i=LESSONS.indexOf(L),m=document.getElementById('exMain');
    m.innerHTML=`<div class="exbar"><div><div class="label">Teoria · lezione ${i+1} di ${LESSONS.length}</div><h2>${L.t}</h2><div class="muted" style="font-size:13px">${L.d}</div></div></div>
      <div class="tkbw"><div class="kb tkb" id="tkb">${kbHtml()}</div><div class="tkbl" id="tkbl">Premi ▶ su un esempio: lo senti e lo vedi sulla tastiera.</div></div>
      <div class="tbody">${L.body()}</div>
      <div class="tnav">${i>0?`<button class="btn sm" data-go="${LESSONS[i-1].id}">← ${LESSONS[i-1].t}</button>`:''}<span class="spacer"></span>
        ${L.ex?`<button class="btn primary" id="tTry">Mettiti alla prova →</button>`:''}${i<LESSONS.length-1?`<button class="btn sm" data-go="${LESSONS[i+1].id}">${LESSONS[i+1].t} →</button>`:''}</div>`;
    m.querySelectorAll('.tex').forEach(b=>b.onclick=()=>play(b));
    m.querySelectorAll('[data-go]').forEach(b=>b.onclick=()=>goEx(b.dataset.go,true));
    const tr=document.getElementById('tTry');if(tr)tr.onclick=()=>goEx(L.ex,false);
  }
  return{LESSONS,render};
})();
