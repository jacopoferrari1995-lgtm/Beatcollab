'use strict';
/* =====================================================================
   SINTESI AUDIO — pianoforte, Rhodes, basso, pad, pizzico, batteria
   ===================================================================== */
const Synth=(()=>{
  let A=null,master,comp,revIn,noise,waves={};
  const bus={};
  const LAYER_GAIN={smp:.9,mel:.95,cm:.62,gtr:.8,piano:.78,arp:.5,pad:.32,bass:.9,drums:.62,ex:.9};
  function impulse(sec,decay){const len=Math.floor(A.sampleRate*sec),b=A.createBuffer(2,len,A.sampleRate);
    for(let c=0;c<2;c++){const d=b.getChannelData(c);for(let i=0;i<len;i++)d[i]=(Math.random()*2-1)*Math.pow(1-i/len,decay);}return b;}
  function wave(parts){const re=new Float32Array(parts.length+1),im=new Float32Array(parts.length+1);parts.forEach((a,i)=>im[i+1]=a);return A.createPeriodicWave(re,im);}
  function init(){
    if(A){if(A.state==='suspended')A.resume();return;}
    A=new (window.AudioContext||window.webkitAudioContext)();
    comp=A.createDynamicsCompressor();comp.threshold.value=-16;comp.knee.value=12;comp.ratio.value=3;comp.attack.value=.005;comp.release.value=.2;
    master=A.createGain();master.gain.value=.8;master.connect(comp);comp.connect(A.destination);
    const conv=A.createConvolver();conv.buffer=impulse(2.4,2.8);revIn=A.createGain();revIn.gain.value=1;revIn.connect(conv);
    const wet=A.createGain();wet.gain.value=.2;conv.connect(wet);wet.connect(comp);
    const len=A.sampleRate*1.5;noise=A.createBuffer(1,len,A.sampleRate);const nd=noise.getChannelData(0);for(let i=0;i<len;i++)nd[i]=Math.random()*2-1;
    waves.pLow=wave([1,.55,.35,.22,.16,.1,.07,.05]);waves.pMid=wave([1,.42,.22,.12,.07,.04]);waves.pHigh=wave([1,.25,.08,.03]);
    for(const k in LAYER_GAIN){const g=A.createGain();g.gain.value=LAYER_GAIN[k]*(fac[k]!=null?fac[k]:1);g.connect(master);
      const s=A.createGain();s.gain.value=k==='drums'?.08:k==='bass'?.05:.35;g.connect(s);s.connect(revIn);bus[k]=g;}
    if(A.state==='suspended')A.resume();
  }
  const now=()=>A?A.currentTime:0;
  const hz=n=>440*Math.pow(2,(n-69)/12);
  function env(g,t,peak,att,dec,sus,end,rel){
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(peak,t+att);
    g.gain.setTargetAtTime(peak*sus,t+att,dec);g.gain.setTargetAtTime(0,end,rel);
  }
  /* ---- pianoforte campionato (Salamander Grand, via Tone.js) con ritorno al sintetico ---- */
  const SAMP={on:false,state:'off',buf:{},base:'https://tonejs.github.io/audio/salamander/'};
  const SNOTES=(()=>{const nm=['C','Ds','Fs','A'],out=[];for(let m=21;m<=108;m+=3){const pc=m%12,i=[0,3,6,9].indexOf(pc);if(i>=0)out.push([m,nm[i]+(Math.floor(m/12)-1)]);}return out;})();
  function loadSamples(onprog){
    if(SAMP.state==='loading'||SAMP.state==='ready')return Promise.resolve(SAMP.state);
    init();SAMP.state='loading';let done=0,ok=0;
    return Promise.all(SNOTES.map(([m,f])=>fetch(SAMP.base+f+'.mp3').then(r=>{if(!r.ok)throw 0;return r.arrayBuffer();})
      .then(b=>new Promise((res,rej)=>A.decodeAudioData(b,res,rej))).then(buf=>{SAMP.buf[m]=buf;ok++;}).catch(()=>{})
      .finally(()=>{done++;onprog&&onprog(done/SNOTES.length);})))
      .then(()=>{SAMP.state=ok>=SNOTES.length*.8?'ready':'failed';return SAMP.state;});
  }
  function sampled(n,t,d,v,out){
    let best=null,bd=99;for(const k in SAMP.buf){const dd=Math.abs(n-k);if(dd<bd){bd=dd;best=+k;}}
    if(best==null)return false;
    const src=A.createBufferSource(),g=A.createGain(),lp=A.createBiquadFilter(),vel=Math.pow(v/127,1.6);
    src.buffer=SAMP.buf[best];src.playbackRate.value=Math.pow(2,(n-best)/12);
    lp.type='lowpass';lp.frequency.value=900+vel*9000;
    src.connect(lp);lp.connect(g);g.connect(out);const end=t+Math.max(.08,d);
    g.gain.setValueAtTime(vel*.9,t);g.gain.setTargetAtTime(0,end,.18);
    src.start(t);src.stop(end+1.2);return true;
  }
  function piano(n,t,d,v,out,bright){
    if(SAMP.on&&SAMP.state==='ready'&&sampled(n,t,d,v*(bright?1.08:1),out))return;
    const f=hz(n),vel=Math.pow(v/127,1.5),o=A.createOscillator(),o2=A.createOscillator(),g=A.createGain(),lp=A.createBiquadFilter();
    o.setPeriodicWave(n<52?waves.pLow:n<74?waves.pMid:waves.pHigh);o2.setPeriodicWave(waves.pMid);
    o.frequency.value=f;o2.frequency.value=f*1.0018;
    lp.type='lowpass';const c0=Math.min(16000,f*(3+vel*9)*(bright?1.6:1)+600);lp.frequency.setValueAtTime(c0,t);lp.frequency.setTargetAtTime(Math.max(f*2,500),t,.6);
    const g2=A.createGain();g2.gain.value=.35;
    o.connect(g);o2.connect(g2);g2.connect(g);g.connect(lp);lp.connect(out);
    const end=t+Math.max(.08,d),decay=n<48?2.4:n<72?1.6:.9;
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.42,t+.004);g.gain.setTargetAtTime(vel*.42*.25,t+.004,decay);g.gain.setTargetAtTime(0,end,.09);
    o.start(t);o2.start(t);o.stop(end+.6);o2.stop(end+.6);
  }
  function epiano(n,t,d,v,out){
    const f=hz(n),vel=Math.pow(v/127,1.3),car=A.createOscillator(),mod=A.createOscillator(),mg=A.createGain(),g=A.createGain();
    car.frequency.value=f;mod.frequency.value=f;mod.connect(mg);mg.connect(car.frequency);
    mg.gain.setValueAtTime(f*(1.2+vel*2.2),t);mg.gain.setTargetAtTime(f*.15,t,.35);
    const tine=A.createOscillator(),tg=A.createGain();tine.frequency.value=f*14;tine.connect(tg);tg.connect(g);
    tg.gain.setValueAtTime(vel*.04,t);tg.gain.setTargetAtTime(0,t,.03);
    car.connect(g);g.connect(out);
    const end=t+Math.max(.08,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.36,t+.006);g.gain.setTargetAtTime(vel*.36*.35,t+.006,1.5);g.gain.setTargetAtTime(0,end,.12);
    [car,mod,tine].forEach(x=>{x.start(t);x.stop(end+.8);});
  }
  function pluck(n,t,d,v,out){
    const f=hz(n),vel=Math.pow(v/127,1.3),o=A.createOscillator(),g=A.createGain(),lp=A.createBiquadFilter();
    o.type='triangle';o.frequency.value=f;lp.type='lowpass';lp.frequency.setValueAtTime(f*8,t);lp.frequency.setTargetAtTime(f*2,t,.15);
    o.connect(lp);lp.connect(g);g.connect(out);const end=t+Math.max(.08,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.4,t+.003);g.gain.setTargetAtTime(0,t+.003,.5);g.gain.setTargetAtTime(0,end,.08);
    o.start(t);o.stop(end+.6);
  }
  function bass(n,t,d,v,out){
    const f=hz(n),vel=Math.pow(v/127,1.2),o=A.createOscillator(),s=A.createOscillator(),g=A.createGain(),lp=A.createBiquadFilter();
    o.type='sawtooth';s.type='sine';o.frequency.value=f;s.frequency.value=f;
    lp.type='lowpass';lp.Q.value=2;lp.frequency.setValueAtTime(300+vel*900,t);lp.frequency.setTargetAtTime(220,t,.12);
    const og=A.createGain();og.gain.value=.35;o.connect(og);og.connect(lp);s.connect(lp);lp.connect(g);g.connect(out);
    const end=t+Math.max(.06,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.55,t+.008);g.gain.setTargetAtTime(vel*.38,t+.01,.25);g.gain.setTargetAtTime(0,end,.05);
    o.start(t);s.start(t);o.stop(end+.4);s.stop(end+.4);
  }
  function b808(n,t,d,v,out,gl){
    const f=hz(n),vel=Math.pow(v/127,1.1),o=A.createOscillator(),g=A.createGain(),sh=A.createWaveShaper();
    if(!waves.curve){const c=new Float32Array(256);for(let i=0;i<256;i++){const x=i/128-1;c[i]=Math.tanh(x*2.2);}waves.curve=c;}
    sh.curve=waves.curve;o.type='sine';
    if(gl!=null){o.frequency.setValueAtTime(hz(gl),t);o.frequency.exponentialRampToValueAtTime(f,t+.09);}else o.frequency.setValueAtTime(f,t);
    o.connect(sh);sh.connect(g);g.connect(out);const end=t+Math.max(.08,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.62,t+.006);g.gain.setTargetAtTime(vel*.45,t+.01,.6);g.gain.setTargetAtTime(0,end,.06);
    o.start(t);o.stop(end+.4);
  }
  function pad(n,t,d,v,out){
    const f=hz(n),vel=v/127,g=A.createGain(),lp=A.createBiquadFilter();lp.type='lowpass';lp.frequency.value=1100+vel*900;lp.Q.value=.6;
    const os=[-7,7].map(c=>{const o=A.createOscillator();o.type='sawtooth';o.frequency.value=f;o.detune.value=c;o.connect(lp);return o;});
    lp.connect(g);g.connect(out);const end=t+Math.max(.2,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.12,t+Math.min(1.2,Math.max(.35,d*.3)));g.gain.setTargetAtTime(0,end,.6);
    os.forEach(o=>{o.start(t);o.stop(end+1.8);});
  }
  function noiseHit(t,dur,type,freq,q,gain,out){
    const s=A.createBufferSource();s.buffer=noise;const f=A.createBiquadFilter();f.type=type;f.frequency.value=freq;f.Q.value=q||.7;
    const g=A.createGain();s.connect(f);f.connect(g);g.connect(out);
    g.gain.setValueAtTime(gain,t);g.gain.exponentialRampToValueAtTime(.001,t+dur);s.start(t,Math.random()*.5);s.stop(t+dur+.05);
  }
  function tone(t,f0,f1,dur,gain,out,type){
    const o=A.createOscillator(),g=A.createGain();o.type=type||'sine';o.frequency.setValueAtTime(f0,t);o.frequency.exponentialRampToValueAtTime(f1,t+dur*.6);
    o.connect(g);g.connect(out);g.gain.setValueAtTime(gain,t);g.gain.exponentialRampToValueAtTime(.001,t+dur);o.start(t);o.stop(t+dur+.05);
  }
  function drum(n,t,v,out,d){
    const k=Math.pow(v/127,1.2);
    switch(n){
      case 55:if((d||0)>1){const s=A.createBufferSource();s.buffer=noise;s.loop=true;const f=A.createBiquadFilter();f.type='bandpass';f.Q.value=1.5;
          f.frequency.setValueAtTime(300,t);f.frequency.exponentialRampToValueAtTime(7000,t+d);const g=A.createGain();g.gain.setValueAtTime(.0001,t);
          g.gain.exponentialRampToValueAtTime(k*.3,t+d*.97);g.gain.linearRampToValueAtTime(0,t+d);s.connect(f);f.connect(g);g.connect(out);s.start(t);s.stop(t+d+.05);}
        else{tone(t,90,38,1.1,k*.9,out);noiseHit(t,.7,'lowpass',900,.7,k*.35,out);}break;
      case 36:tone(t,140,44,.42,k*1.1,out);noiseHit(t,.02,'highpass',2500,.7,k*.15,out);break;
      case 38:tone(t,190,160,.13,k*.5,out);noiseHit(t,.22,'bandpass',2800,.6,k*.6,out);noiseHit(t,.09,'highpass',6000,.7,k*.22,out);break;
      case 37:tone(t,1750,1600,.035,k*.45,out,'triangle');tone(t,430,400,.06,k*.3,out);noiseHit(t,.045,'bandpass',2600,3,k*.6,out);break;
      case 39:[0,.012,.024].forEach(x=>noiseHit(t+x,.11,'bandpass',1300,1.2,k*.4,out));break;
      case 42:noiseHit(t,.05,'highpass',7500,.7,k*.25,out);break;
      case 44:noiseHit(t,.04,'highpass',6500,.7,k*.18,out);break;
      case 46:noiseHit(t,.32,'highpass',7000,.7,k*.22,out);break;
      case 51:noiseHit(t,.9,'bandpass',7200,1.8,k*.2,out);[3150,4230,5170].forEach((f,i)=>tone(t,f,f*.995,.7-i*.15,k*(.035-i*.008),out,'square'));noiseHit(t,.03,'highpass',9000,.7,k*.12,out);break;
      case 40:{const s2=A.createBufferSource();s2.buffer=noise;const f=A.createBiquadFilter();f.type='bandpass';f.frequency.value=2600;f.Q.value=.5;const g=A.createGain();
        s2.connect(f);f.connect(g);g.connect(out);g.gain.setValueAtTime(.0001,t);g.gain.exponentialRampToValueAtTime(k*.32,t+.025);g.gain.exponentialRampToValueAtTime(.001,t+.32);s2.start(t,Math.random()*.5);s2.stop(t+.36);break;}
      case 49:noiseHit(t,1.6,'highpass',4200,.6,k*.32,out);break;
      case 63:tone(t,420,330,.18,k*.5,out);noiseHit(t,.02,'bandpass',3000,2,k*.12,out);break;
      case 64:tone(t,260,210,.26,k*.55,out);noiseHit(t,.02,'bandpass',2000,2,k*.1,out);break;
      case 60:case 61:tone(t,n===60?520:400,n===60?470:360,.12,k*.4,out);break;
      case 70:case 82:noiseHit(t,.06,'highpass',6000,.7,k*.18,out);break;
      case 56:tone(t,800,800,.18,k*.18,out,'square');tone(t,540,540,.18,k*.18,out,'square');break;
      default:{const f={41:90,43:100,45:110,47:140,48:165,50:200}[n]||130;tone(t,f*1.6,f,.32,k*.7,out);}
    }
  }
  /* ---- chitarra: Karplus-Strong (corda pizzicata calcolata), corpo e amplificatore per timbro ---- */
  const TONES={acoustic:{br:.72,rho:.9965,pos:.18,len:3.4,body:[[110,4,1.2],[230,3,1.5],[2600,3,1]],lp:9000},
    nylon:{br:.42,rho:.9945,pos:.22,len:3,body:[[100,4,1.1],[210,3,1.4],[1800,1,1]],lp:5200},
    clean:{br:.6,rho:.9965,pos:.14,len:3.2,body:[[180,2,1],[2200,3,1.2]],lp:7000},
    jazz:{br:.32,rho:.995,pos:.25,len:2.8,body:[[160,3,1],[900,2,1]],lp:3200},
    crunch:{br:.85,rho:.998,pos:.12,len:3,body:[[120,3,1],[1800,4,1]],lp:4600,drive:6}};
  let gTone='clean',gIn=null;const GBUF=new Map();
  function gtrChain(){if(gIn)return gIn;const T=TONES[gTone];gIn=A.createGain();gIn.gain.value=T.drive?.55:1;let node=gIn;
    if(T.drive){const sh=A.createWaveShaper(),c=new Float32Array(1024);for(let i=0;i<1024;i++){const x=i/511.5-1;c[i]=Math.tanh(x*T.drive)/Math.tanh(T.drive);}sh.curve=c;sh.oversample='2x';node.connect(sh);node=sh;}
    T.body.forEach(([f,g,q])=>{const b=A.createBiquadFilter();b.type='peaking';b.frequency.value=f;b.gain.value=g;b.Q.value=q;node.connect(b);node=b;});
    const lp=A.createBiquadFilter();lp.type='lowpass';lp.frequency.value=T.lp;node.connect(lp);lp.connect(bus.gtr);return gIn;}
  function setGuitarTone(t){if(!TONES[t]||t===gTone)return;gTone=t;GBUF.clear();if(gIn){try{gIn.disconnect();}catch(e){}gIn=null;}}
  function ksBuf(n,art){const key=n+'|'+art+'|'+gTone;if(GBUF.has(key))return GBUF.get(key);
    const T=TONES[gTone],sr=A.sampleRate,f=hz(n),Nd=sr/f-.5,P=Math.max(2,Math.floor(Nd)),fr=Nd-P,C=(1-fr)/(1+fr);
    const mute=art==='mute',pm=art==='pm',secs=mute?.14:pm?.6:T.len,len=Math.floor(sr*secs),y=new Float32Array(len);
    const br=mute?.25:pm?T.br*.45:art==='hammer'?T.br*.5:T.br,rho0=mute?.86:pm?.982:T.rho,rho=1-(1-rho0)*Math.min(1.4,Math.sqrt(110/f));
    // eccitazione: rumore filtrato (brillantezza) con il punto di pizzico (comb)
    let lp=0;const ex=new Float32Array(P);for(let i=0;i<P;i++){const x=Math.random()*2-1;lp+=br*(x-lp);ex[i]=lp;}
    const pk=Math.max(1,Math.floor(P*T.pos));for(let i=P-1;i>=pk;i--)ex[i]-=ex[i-pk];
    let x1=0,y1=0;
    for(let i=0;i<len;i++){if(i<P){y[i]=ex[i];continue;}
      const v=rho*.5*(y[i-P]+(i-P-1>=0?y[i-P-1]:0)),o=C*v+x1-C*y1;x1=v;y1=o;y[i]=o;}
    let mx=0;for(let i=0;i<Math.min(len,P*8);i++)mx=Math.max(mx,Math.abs(y[i]));const k=mx>0?.9/mx:1;for(let i=0;i<len;i++)y[i]*=k;
    const b=A.createBuffer(1,len,sr);b.copyToChannel?b.copyToChannel(y,0):b.getChannelData(0).set(y);GBUF.set(key,b);if(GBUF.size>400)GBUF.clear();return b;}
  function gtr(n,t,d,v,out,gl,art){
    const inp=gtrChain(),src=A.createBufferSource();src.buffer=ksBuf(n,art==='mute'?'mute':art==='pm'?'pm':art==='hammer'?'hammer':'');
    const g=A.createGain(),vel=Math.pow(v/127,1.3)*.9,end=t+Math.max(.04,d);src.connect(g);g.connect(inp);
    if(art==='swell'){g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel,t+Math.min(.6,d*.4));}else g.gain.setValueAtTime(vel,t);
    g.gain.setTargetAtTime(0,end,art==='mute'?.01:.045);
    if(art==='slide'){src.playbackRate.setValueAtTime(Math.pow(2,-1/12),t);src.playbackRate.linearRampToValueAtTime(1,t+.07);}
    src.start(t);src.stop(Math.min(t+src.buffer.duration,end+.4));
  }
  /* ---- campioni caricati dall'utente ---- */
  const SMPB=new Map();
  async function loadSample(id,data){init();if(SMPB.has(id))return SMPB.get(id);const b=await A.decodeAudioData(data.slice(0));SMPB.set(id,b);return b;}
  const sampleDur=id=>SMPB.has(id)?SMPB.get(id).duration:0;
  function playSample(id,t,{dur=null,rate=1,gain=1,offset=0,layer='smp'}={}){const b=SMPB.get(id);if(!b||!A)return false;
    const src=A.createBufferSource(),g=A.createGain();src.buffer=b;src.playbackRate.value=rate;src.connect(g);g.connect(bus[layer]||bus.smp);
    g.gain.setValueAtTime(gain,t);src.start(t,Math.max(0,offset));
    if(dur!=null){const e=t+dur;g.gain.setValueAtTime(gain,Math.max(t,e-.015));g.gain.linearRampToValueAtTime(0,e);src.stop(e+.02);}
    return true;}
  const INST={piano,epiano,pluck,bass,pad,b808,gtr};
  function note(inst,n,t,d,v,layer,gl,art){init();(INST[inst]||piano)(n,t,d,v,bus[layer]||bus.ex,inst==='lead'?true:gl,art);}
  function hit(n,t,v,d){init();drum(n,t,v,bus.drums,d);}
  const fac={};
  function setLayerGain(k,x){fac[k]=x;if(bus[k])bus[k].gain.value=LAYER_GAIN[k]*x;}
  function setVolume(x){init();master.gain.value=x;}
  function useSamples(on,onprog){SAMP.on=on;return on?loadSamples(onprog):Promise.resolve('off');}
  return{loadSample,sampleDur,playSample,hasSample:id=>SMPB.has(id),setGuitarTone,init,now,note,hit,setLayerGain,setVolume,useSamples,get sampleState(){return SAMP.state;},get ctx(){return A;}};
})();
