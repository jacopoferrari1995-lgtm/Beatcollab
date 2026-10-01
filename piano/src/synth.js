'use strict';
/* =====================================================================
   SINTESI AUDIO — pianoforte, Rhodes, basso, pad, pizzico, batteria
   ===================================================================== */
const Synth=(()=>{
  let A=null,master,comp,revIn,noise,waves={};
  const bus={};
  const LAYER_GAIN={mel:.95,piano:.78,arp:.5,pad:.32,bass:.9,drums:.62,ex:.9};
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
    for(const k in LAYER_GAIN){const g=A.createGain();g.gain.value=LAYER_GAIN[k];g.connect(master);
      const s=A.createGain();s.gain.value=k==='drums'?.08:k==='bass'?.05:.35;g.connect(s);s.connect(revIn);bus[k]=g;}
    if(A.state==='suspended')A.resume();
  }
  const now=()=>A?A.currentTime:0;
  const hz=n=>440*Math.pow(2,(n-69)/12);
  function env(g,t,peak,att,dec,sus,end,rel){
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(peak,t+att);
    g.gain.setTargetAtTime(peak*sus,t+att,dec);g.gain.setTargetAtTime(0,end,rel);
  }
  function piano(n,t,d,v,out,bright){
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
  function pad(n,t,d,v,out){
    const f=hz(n),vel=v/127,g=A.createGain(),lp=A.createBiquadFilter();lp.type='lowpass';lp.frequency.value=1100+vel*900;lp.Q.value=.6;
    const os=[-7,7].map(c=>{const o=A.createOscillator();o.type='sawtooth';o.frequency.value=f;o.detune.value=c;o.connect(lp);return o;});
    lp.connect(g);g.connect(out);const end=t+Math.max(.2,d);
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vel*.12,t+.35);g.gain.setTargetAtTime(0,end,.35);
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
  function drum(n,t,v,out){
    const k=Math.pow(v/127,1.2);
    switch(n){
      case 36:tone(t,140,44,.42,k*1.1,out);noiseHit(t,.02,'highpass',2500,.7,k*.15,out);break;
      case 38:noiseHit(t,.2,'highpass',1400,.7,k*.55,out);tone(t,220,170,.12,k*.4,out,'triangle');break;
      case 37:noiseHit(t,.05,'bandpass',1800,3,k*.5,out);tone(t,900,800,.04,k*.25,out,'square');break;
      case 39:[0,.012,.024].forEach(x=>noiseHit(t+x,.11,'bandpass',1300,1.2,k*.4,out));break;
      case 42:noiseHit(t,.05,'highpass',7500,.7,k*.25,out);break;
      case 44:noiseHit(t,.04,'highpass',6500,.7,k*.18,out);break;
      case 46:noiseHit(t,.32,'highpass',7000,.7,k*.22,out);break;
      case 51:noiseHit(t,.6,'bandpass',6500,2.5,k*.22,out);tone(t,3200,3150,.4,k*.03,out,'square');break;
      case 49:noiseHit(t,1.6,'highpass',4200,.6,k*.32,out);break;
      default:{const f={45:110,47:140,48:165,50:200}[n]||130;tone(t,f*1.6,f,.32,k*.7,out);}
    }
  }
  const INST={piano,epiano,pluck,bass,pad};
  function note(inst,n,t,d,v,layer){init();(INST[inst]||piano)(n,t,d,v,bus[layer]||bus.ex,inst==='lead');}
  function hit(n,t,v){init();drum(n,t,v,bus.drums);}
  function setLayerGain(k,x){if(bus[k])bus[k].gain.value=LAYER_GAIN[k]*x;}
  function setVolume(x){init();master.gain.value=x;}
  return{init,now,note,hit,setLayerGain,setVolume,get ctx(){return A;}};
})();
