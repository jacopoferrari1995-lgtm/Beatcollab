'use strict';
/* =====================================================================
   ESERCIZI — orecchio e teoria, a giri di 10 domande
   ===================================================================== */
const Ex=(()=>{
  const LIST=[
    {id:'quality',ic:'♫',t:'Qualità dell’accordo',d:'Riconosci il tipo di accordo'},
    {id:'interval',ic:'↕',t:'Intervalli',d:'La distanza tra due note'},
    {id:'inversion',ic:'⇅',t:'Rivolti',d:'Quale nota è al basso?'},
    {id:'mode',ic:'◐',t:'Modi',d:'Maggiore, dorico, frigio, lidio…'},
    {id:'prog',ic:'⟶',t:'Progressioni',d:'Riconosci il giro di accordi'},
    {id:'fill',ic:'?',t:'Accordo mancante',d:'Completa la progressione'},
    {id:'build',ic:'⌨',t:'Costruisci l’accordo',d:'Suonalo sulla tastiera'},
    {id:'degree',ic:'Ⅳ',t:'Gradi della scala',d:'Armonia diatonica in ogni tonalità'}];
  const ROUND=10,LVL=['Facile','Medio','Difficile'];
  const QLAB={maj:'Maggiore',min:'Minore',dim:'Diminuito',aug:'Aumentato',sus2:'Sus2',sus4:'Sus4',maj7:'Maj7',m7:'Minore 7',
    '7':'Dominante 7',m7b5:'Semidiminuito',dim7:'Diminuito 7','6':'Sesta',add9:'Add9'};
  const IVN=['Unisono','2ª minore','2ª maggiore','3ª minore','3ª maggiore','4ª giusta','Tritono','5ª giusta','6ª minore','6ª maggiore','7ª minore','7ª maggiore','Ottava'];
  const INV=['Stato fondamentale','Primo rivolto','Secondo rivolto','Terzo rivolto'],ROLE=['fondamentale','terza','quinta','settima'];
  const VAMP={major:'I IV V I',minor:'i iv v i',harmonic:'i iv V i',dorian:'i IV i IV',phrygian:'i bII i bII',lydian:'I II I II',mixolydian:'I bVII IV I'};
  const NM=pc=>KEY_NAMES[mod12(pc)];
  const R=Math.random,ri=n=>Math.floor(R()*n),pk=a=>a[ri(a.length)];
  const shuffle=a=>{a=a.slice();for(let i=a.length-1;i>0;i--){const j=ri(i+1);[a[i],a[j]]=[a[j],a[i]];}return a;};
  let stats=LS.get('pg_ex',{});
  const cur={lesson:null,openT:LS.get('pg_theoryOpen',true),id:'quality',lvl:1,n:0,ok:0,streak:0,q:null,answered:false,done:false,sel:new Set()};

  /* ---------- audio ---------- */
  function P(notes,{t0=0,gap=0,dur=1.4,v=82}={}){Synth.init();const t=Synth.now()+.06+t0;notes.forEach((n,i)=>Synth.note('piano',n,t+i*gap,dur,v,'ex'));}
  function playSong(S,{skip=null,bpm=92,layers=['piano','bass']}={}){
    Synth.init();const t=Synth.now()+.08,spb=60/bpm;
    layers.forEach(l=>S.layers[l].forEach(e=>{if(e.t>=16)return;if(skip&&e.t>=skip[0]-.02&&e.t<skip[1]-.02)return;
      Synth.note(l==='bass'?'bass':'piano',e.n,t+e.t*spb,Math.min(e.d,4)*spb,e.v,'ex');}));
  }
  const chordNotes=(root,q)=>QT[q].iv.map(x=>root+x);
  const genLoop=(L,extra={})=>generateSong({genre:pk(L===1?['pop','rock']:L===2?['pop','rock','classical','cinematic','gospel']:['pop','jazz','soul','lofi','bossa','classical','gospel']),
    mood:pk(Object.keys(MOODS)),mode:L===1?'major':L===2?pk(['major','minor']):'auto',key:'auto',structure:'loop4',color:L<3?'triadi':'settime',human:false,
    seeds:{h:rndSeed(),a:rndSeed(),m:rndSeed()},...extra});
  const seventhOf=(dc,mode)=>{const s=MODES[mode].scale,t7=mod12(s[(dc.deg+6)%7]-dc.r);
    return dc.tri==='maj'?(t7===11?'maj7':'7'):dc.tri==='min'?(t7===10?'m7':'mmaj7'):dc.tri==='dim'?(t7===10?'m7b5':'dim7'):'aug';};

  /* ---------- generatori di domande ---------- */
  const GEN={
    quality(L){
      const pool={1:['maj','min'],2:['maj','min','dim','aug','sus4'],3:['maj7','m7','7','m7b5','dim7']}[L];
      const q=pk(pool),root=48+ri(12),notes=chordNotes(root,q);
      const play=()=>{P(notes,{gap:.3,dur:1.1});P(notes,{t0:notes.length*.3+.35,dur:1.9});};
      return{q:'Che accordo senti?',sub:'Prima arpeggiato, poi suonato insieme.',auto:play,
        tools:[['🔁 Riascolta',play],['Arpeggio',()=>P(notes,{gap:.35,dur:1})],['Accordo',()=>P(notes,{dur:1.9})]],
        opts:pool.map(x=>({l:QLAB[x],ok:x===q})),explain:`Era <b>${NM(root)}${QT[q].n}</b> (${QLAB[q]}): ${notes.map(NM).join(' – ')}.`};
    },
    interval(L){
      const pool={1:[3,4,7,12],2:[2,3,4,5,7,9,10,12],3:[1,2,3,4,5,6,7,8,9,10,11,12]}[L];
      const iv=pk(pool),lo=55+ri(10),down=L===3&&R()<.4,a=down?lo+iv:lo,b=down?lo:lo+iv;
      const play=()=>{P([a,b],{gap:.7,dur:.9});P([a,b],{t0:1.7,dur:1.5});};
      return{q:'Che intervallo è?',sub:down?'Melodico (anche discendente), poi armonico.':'Melodico ascendente, poi armonico.',auto:play,
        tools:[['🔁 Riascolta',play],['Melodico',()=>P([a,b],{gap:.7,dur:.9})],['Armonico',()=>P([a,b],{dur:1.6})]],
        opts:pool.map(x=>({l:IVN[x],ok:x===iv})),explain:`<b>${IVN[iv]}</b> (${iv} semitoni): ${NM(a)} → ${NM(b)}.`};
    },
    inversion(L){
      const q=L===1?'maj':L===2?pk(['maj','min']):pk(['maj7','m7','7']),root=ri(12),iv=QT[q].iv,inv=ri(iv.length);
      const order=iv.slice(inv).concat(iv.slice(0,inv));let p=48+mod12(root+order[0]);const notes=[p];
      order.slice(1).forEach(x=>{const pc=mod12(root+x);let m=p+1;while(mod12(m)!==pc)m++;notes.push(m);p=m;});
      const play=()=>{P(notes,{gap:.3,dur:1.1});P(notes,{t0:notes.length*.3+.35,dur:1.9});};
      return{q:'In che rivolto è l’accordo?',sub:`Accordo ${QLAB[q].toLowerCase()} — ascolta soprattutto la nota più grave.`,auto:play,
        tools:[['🔁 Riascolta',play],['Solo il basso',()=>P([notes[0]],{dur:1.5})]],
        opts:INV.slice(0,iv.length).map((x,i)=>({l:x,ok:i===inv})),
        explain:`<b>${INV[inv]}</b>: ${NM(root)}${QT[q].n} con la ${ROLE[inv]} (${NM(notes[0])}) al basso.`};
    },
    mode(L){
      const pool={1:['major','minor'],2:['major','minor','dorian','mixolydian'],3:MODE_ORDER}[L];
      const m=pk(pool),key=ri(12),ch=VAMP[m].split(' ').map(parseRN);
      const play=()=>{Synth.init();const t0=Synth.now()+.06;
        ch.forEach((c,i)=>{const r=mod12(key+c.r),t=t0+i*.95;Synth.note('bass',36+r,t,.9,80,'ex');
          const tri={maj:[0,4,7],min:[0,3,7],dim:[0,3,6],aug:[0,4,8]}[c.tri].map(x=>r+x);
          stackVoicings(tri,64,74)[0].forEach(n=>Synth.note('piano',n,t,.9,72,'ex'));});
        const s=MODES[m].scale.map(x=>60+mod12(key)+x).concat([72+mod12(key)]);
        s.forEach((n,i)=>Synth.note('lead',n,t0+ch.length*.95+.3+i*.24,.3,85,'ex'));};
      return{q:'In che modo è questo frammento?',sub:'Quattro accordi tipici del modo, poi la scala.',auto:play,tools:[['🔁 Riascolta',play]],
        opts:pool.map(x=>({l:MODES[x].n,ok:x===m})),explain:`<b>${MODES[m].n}</b> su ${NM(key)}: ${MODE_DESC[m]}. Giro: ${VAMP[m].replace(/ /g,' – ')}.`};
    },
    prog(L){
      const S=genLoop(L),lab=s=>s.chords.map(c=>c.rn).join(' – '),good=lab(S),set=new Set([good]);
      for(let i=0;i<50&&set.size<4;i++)set.add(lab(genLoop(L,{mode:S.mode})));
      const play=()=>playSong(S);
      return{q:'Quale progressione senti?',sub:'Gradi rispetto alla tonica del brano.',auto:play,tools:[['🔁 Riascolta',play],['Solo basso',()=>playSong(S,{layers:['bass']})]],
        opts:[...set].map(x=>({l:x,ok:x===good})),explain:`${S.keyName} ${MODES[S.mode].n.toLowerCase()}: <b>${S.chords.map(c=>c.name).join(' – ')}</b>.`,wide:1};
    },
    fill(L){
      const S=genLoop(L),cand=S.chords.map((c,i)=>i).filter(i=>S.chords[i].beats===4),hi=pk(cand.length?cand:[0]),c0=S.chords[hi];
      const sp=speller(S.key,S.mode),good=c0.name,set=new Set([good]);
      shuffle([0,1,2,3,4,5,6]).forEach(d=>{if(set.size>=4)return;const dc=degChord(S.mode,d),q=L<3?dc.tri:seventhOf(dc,S.mode);set.add(sp(S.key+dc.r)+QT[q].n);});
      const skip=[c0.start,c0.start+c0.beats],play=()=>playSong(S,{skip});
      return{q:'Quale accordo manca?',sub:'Un accordo è stato tolto: scegli quello che suona giusto al suo posto.',auto:play,
        tools:[['🔁 Riascolta',play]],chips:S.chords.map((c,i)=>i===hi?'?':c.name),hidden:hi,
        opts:[...set].map(x=>({l:x,ok:x===good})),explain:`Era <b>${good}</b> (${c0.rn}) in ${S.keyName} ${MODES[S.mode].n.toLowerCase()}.`,after:()=>playSong(S)};
    },
    build(L){
      const pool={1:['maj','min'],2:['maj','min','dim','aug','sus2','sus4'],3:['maj7','m7','7','m7b5','dim7','6','add9']}[L];
      const q=pk(pool),root=ri(12),want=new Set(QT[q].iv.map(x=>mod12(root+x)));
      return{q:`Suona ${NM(root)}${QT[q].n}`,sub:`${QLAB[q]} — clicca i tasti (o usa la tastiera del computer: A W S E D F T G Y H U J K).`,kind:'keys',want,
        explain:`<b>${NM(root)}${QT[q].n}</b> = ${QT[q].iv.map(x=>NM(root+x)).join(' – ')}.`};
    },
    degree(L){
      const m=L===1?'major':L===2?pk(['major','minor','harmonic']):pk(MODE_ORDER),key=ri(12),sp=speller(key,m),d=ri(7);
      const nm=x=>{const dc=degChord(m,x),q=L===3?seventhOf(dc,m):dc.tri;return{n:sp(key+dc.r)+QT[q].n,rn:numeral(dc.r,q),dc,q};};
      const g=nm(d),set=new Set([g.n]);shuffle([0,1,2,3,4,5,6]).forEach(x=>{if(set.size<4)set.add(nm(x).n);});
      return{q:`In ${sp(key)} ${MODES[m].n.toLowerCase()}, qual è l’accordo di grado ${g.rn}?`,sub:L===3?'Accordi di settima costruiti sulla scala del modo.':'Triadi costruite sulla scala.',
        opts:[...set].map(x=>({l:x,ok:x===g.n})),explain:`${g.rn} in ${sp(key)} ${MODES[m].n.toLowerCase()} = <b>${g.n}</b> (${QT[g.q].iv.map(x=>sp(key+g.dc.r+x)).join(' – ')}).`,
        after:()=>P(chordNotes(48+mod12(key+g.dc.r),g.q),{dur:1.8})};
    }};

  /* ---------- interfaccia ---------- */
  const acc=id=>{const s=stats[id];return s&&s.tot?Math.round(s.ok/s.tot*100)+'%':'—';};
  function renderList(){
    const T=Theory.LESSONS;
    $('#exList').innerHTML=`<div class="exgrp"><button class="exgh" data-grp="t">${cur.openT?'▾':'▸'} Teoria <small>${T.length} lezioni</small></button></div>`+
      (cur.openT?T.map((e,i)=>`<button class="exi th ${e.id===cur.lesson?'on':''}" data-th="${e.id}"><span class="ic">${e.ic}</span>
      <span><b>${i+1}. ${e.t}</b><small>${e.d}</small></span></button>`).join(''):'')+
      `<div class="exgrp"><span class="exgh">Esercizi</span></div>`+
      LIST.map(e=>`<button class="exi ${e.id===cur.id&&!cur.lesson?'on':''}" data-ex="${e.id}"><span class="ic">${e.ic}</span>
      <span><b>${e.t}</b><small>${e.d}</small></span><span class="acc" title="Precisione complessiva">${acc(e.id)}</span></button>`).join('');
    $$('#exList [data-ex]').forEach(b=>b.onclick=()=>{cur.id=b.dataset.ex;cur.lesson=null;reset();});
    $$('#exList [data-th]').forEach(b=>b.onclick=()=>openLesson(b.dataset.th));
    $('#exList [data-grp]').onclick=()=>{cur.openT=!cur.openT;LS.set('pg_theoryOpen',cur.openT);renderList();};
  }
  // lezione di teoria; "mettiti alla prova" apre l'esercizio collegato
  function openLesson(id){cur.lesson=id;cur.openT=true;renderList();Theory.render(id,(x,isLesson)=>{if(isLesson)openLesson(x);else{cur.id=x;cur.lesson=null;reset();}});
    const m=$('#exMain'),tp=m.getBoundingClientRect().top;if(tp<0||tp>innerHeight*.6)m.scrollIntoView({behavior:'smooth'});}
  function reset(){Object.assign(cur,{n:0,ok:0,streak:0,q:null,answered:false,done:false});renderList();render();}
  function newQ(){const q=GEN[cur.id](cur.lvl);if(q.opts&&q.mix!==false&&['prog','fill','degree'].includes(cur.id))q.opts=shuffle(q.opts);cur.q=q;cur.answered=false;cur.sel=new Set();}
  function head(){
    const e=LIST.find(x=>x.id===cur.id);
    const th=Theory.LESSONS.find(x=>x.ex===cur.id);
    return`<div class="exbar"><div><h2>${e.t}</h2><div class="muted" style="font-size:13px">${e.d}${th?` · <a href="#" class="thlink" data-thl="${th.id}">📖 ripassa la teoria</a>`:''}</div></div><span class="spacer"></span>
      <div class="levels">${LVL.map((l,i)=>`<button data-l="${i+1}" class="${cur.lvl===i+1?'on':''}">${l}</button>`).join('')}</div></div>
      <div class="progress"><i style="width:${(cur.n+(cur.answered?1:0))/ROUND*100}%"></i></div>
      <div class="dim" style="font-size:12px;margin-top:6px">Domanda ${Math.min(cur.n+1,ROUND)} di ${ROUND} · corrette ${cur.ok} · serie ${cur.streak}</div>`;
  }
  function render(){
    if(cur.lesson){openLesson(cur.lesson);return;}
    const m=$('#exMain');
    if(cur.done){
      const pct=Math.round(cur.ok/ROUND*100),s=stats[cur.id]||{},msg=pct>=90?'Eccellente!':pct>=70?'Molto bene':pct>=50?'Ci sei quasi':'Continua ad allenarti';
      m.innerHTML=head()+`<div class="summary"><div class="big">${cur.ok}/${ROUND}</div><div class="q" style="margin-top:4px">${msg}</div>
        <p class="muted" style="margin-top:6px">Miglior giro a livello ${LVL[cur.lvl-1].toLowerCase()}: ${s['best'+cur.lvl]||0}/${ROUND}</p>
        <div class="qtools" style="justify-content:center;margin-top:18px"><button class="btn primary" id="again">Nuovo giro</button>
        ${cur.lvl<3&&pct>=80?`<button class="btn" id="up">Passa a ${LVL[cur.lvl]}</button>`:''}</div></div>`;
      bindLevels();$('#again').onclick=reset;const up=$('#up');if(up)up.onclick=()=>{cur.lvl++;reset();};return;
    }
    if(!cur.q)newQ();
    const q=cur.q;
    let html=head()+`<div class="q">${q.q}</div><div class="qsub">${q.sub||''}</div>`;
    if(q.chips)html+=`<div class="chips" style="margin-top:14px">${q.chips.map(c=>`<span class="chip" style="font-size:16px;padding:8px 14px;${c==='?'?'border-color:var(--acc);color:var(--acc)':''}"><b>${c}</b></span>`).join('')}</div>`;
    if(q.tools)html+=`<div class="qtools">${q.tools.map((t,i)=>`<button class="btn sm" data-tool="${i}">${t[0]}</button>`).join('')}</div>`;
    if(q.kind==='keys'){
      html+=`<div class="kb" id="kb"></div><div class="qtools"><button class="btn primary" id="chk">Verifica</button><button class="btn sm" id="lis">🔊 Ascolta la tua scelta</button>
        <button class="btn sm" id="clr">Pulisci</button><button class="btn sm ghost" id="sol">Mostra soluzione</button></div>`;
    }else html+=`<div class="opts" style="${q.wide?'grid-template-columns:repeat(auto-fill,minmax(230px,1fr))':''}">${q.opts.map((o,i)=>`<button class="opt" data-o="${i}">${o.l}</button>`).join('')}</div>`;
    html+=`<div id="fbox"></div>`;
    m.innerHTML=html;bindLevels();
    if(q.tools)m.querySelectorAll('[data-tool]').forEach(b=>b.onclick=()=>q.tools[+b.dataset.tool][1]());
    if(q.kind==='keys')bindKeys();
    else m.querySelectorAll('[data-o]').forEach(b=>b.onclick=()=>answer(q.opts[+b.dataset.o].ok,b));
    if(q.auto&&!q.played){q.played=1;setTimeout(q.auto,150);}
  }
  function bindLevels(){$$('#exMain [data-thl]').forEach(x=>x.onclick=ev=>{ev.preventDefault();openLesson(x.dataset.thl);});
    $$('#exMain .levels button').forEach(b=>b.onclick=()=>{cur.lvl=+b.dataset.l;reset();});}
  function answer(ok,btn){
    if(cur.answered)return;cur.answered=true;const q=cur.q;
    if(ok){cur.ok++;cur.streak++;}else cur.streak=0;
    const s=stats[cur.id]=stats[cur.id]||{tot:0,ok:0};s.tot++;if(ok)s.ok++;LS.set('pg_ex',stats);
    if(btn&&q.opts){$$('#exMain .opt').forEach((b,i)=>{b.disabled=true;if(q.opts[i].ok)b.classList.add('ok');});if(!ok)btn.classList.add('ko');}
    if(q.chips){const el=$$('#exMain .chips .chip')[q.hidden];if(el){el.innerHTML='<b>'+q.opts.find(o=>o.ok).l+'</b>';el.style.color='var(--ok)';el.style.borderColor='var(--ok)';}}
    const last=cur.n+1>=ROUND;
    $('#fbox').innerHTML=`<div class="fb ${ok?'good':'bad'}"><div class="t">${ok?'✅ <b>Esatto!</b> ':'❌ <b>Non proprio.</b> '}${q.explain}</div>
      <button class="btn primary" id="next">${last?'Vedi il risultato':'Avanti →'}</button></div>`;
    $('#exMain .progress i').style.width=(cur.n+1)/ROUND*100+'%';
    $('#next').onclick=next;$('#next').focus({preventScroll:true});
    if(q.after)setTimeout(q.after,ok?250:500);
    renderList();
  }
  function next(){
    cur.n++;
    if(cur.n>=ROUND){cur.done=true;const s=stats[cur.id]=stats[cur.id]||{tot:0,ok:0};s['best'+cur.lvl]=Math.max(s['best'+cur.lvl]||0,cur.ok);LS.set('pg_ex',stats);}
    else cur.q=null;
    render();
  }
  const KEYMAP={a:60,w:61,s:62,e:63,d:64,f:65,t:66,g:67,y:68,h:69,u:70,j:71,k:72,o:73,l:74,p:75,';':76};
  let keyEls={};
  function bindKeys(){
    const kb=$('#kb'),lo=60,hi=83,whites=[];keyEls={};
    for(let m=lo;m<=hi;m++)if(![1,3,6,8,10].includes(m%12))whites.push(m);
    const w=100/whites.length,hint=Object.fromEntries(Object.entries(KEYMAP).map(([k,v])=>[v,k.toUpperCase()]));
    whites.forEach((m,i)=>{const e=document.createElement('div');e.className='w';e.style.cssText=`left:${i*w}%;width:${w}%`;
      e.innerHTML=(hint[m]?`<span class="hint">${hint[m]}</span>`:'')+KEY_NAMES[m%12]+(m%12===0?Math.floor(m/12)-1:'');e.onclick=()=>toggle(m);kb.append(e);keyEls[m]=e;});
    for(let m=lo;m<=hi;m++)if([1,3,6,8,10].includes(m%12)){const i=whites.indexOf(m-1),e=document.createElement('div');e.className='k';
      e.style.cssText=`left:${(i+1)*w-w*.31}%;width:${w*.62}%`;if(hint[m])e.innerHTML=`<span class="hint" style="color:#777">${hint[m]}</span>`;e.onclick=()=>toggle(m);kb.append(e);keyEls[m]=e;}
    $('#chk').onclick=check;$('#clr').onclick=()=>{cur.sel.clear();Object.values(keyEls).forEach(e=>e.classList.remove('sel'));};
    $('#lis').onclick=()=>P([...cur.sel].sort((a,b)=>a-b),{dur:1.8});
    $('#sol').onclick=()=>{Object.entries(keyEls).forEach(([m,e])=>{e.classList.remove('sel');if(cur.q.want.has(+m%12)&&+m<72)e.classList.add('sol');});
      if(!cur.answered)answer(false,null);};
  }
  function toggle(m){if(cur.answered)return;cur.sel.has(m)?cur.sel.delete(m):cur.sel.add(m);keyEls[m].classList.toggle('sel',cur.sel.has(m));
    Synth.init();Synth.note('piano',m,Synth.now()+.01,1.1,80,'ex');}
  function check(){
    if(cur.answered||!cur.sel.size)return;const got=new Set([...cur.sel].map(m=>m%12)),want=cur.q.want;
    const ok=got.size===want.size&&[...want].every(p=>got.has(p));
    Object.entries(keyEls).forEach(([m,e])=>{if(want.has(+m%12)&&(cur.sel.has(+m)||(!ok&&+m<72)))e.classList.add('sol');});
    P([...cur.sel].sort((a,b)=>a-b),{dur:1.8});answer(ok,null);
  }
  document.addEventListener('keydown',e=>{
    if($('#tab-ex').hidden||!cur.q||cur.q.kind!=='keys'||/INPUT|SELECT|TEXTAREA/.test(e.target.tagName)||e.metaKey||e.ctrlKey||e.repeat)return;
    const m=KEYMAP[e.key.toLowerCase()];if(m!=null){e.preventDefault();toggle(m);}
    else if(e.key==='Enter'){e.preventDefault();cur.answered?next():check();}});
  let inited=false;
  return{show(){if(!inited){inited=true;renderList();}render();},GEN};
})();
