"""Original racing score: composed phrases, layered synths, drums and stereo mix."""
import sys, pathlib, math, json
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'audio-deps'))
import numpy as np
import soundfile as sf

ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=ROOT/'work/game/assets/music'
OUT.mkdir(parents=True,exist_ok=True)
SR=24000
RNG=np.random.default_rng(811)
def freq(note):return 440*2**((note-69)/12)
def lowpass(x,cut):
    bins=np.fft.rfftfreq(len(x),1/SR)
    return np.fft.irfft(np.fft.rfft(x)/(1+(bins/cut)**4),n=len(x)).astype(np.float32)
def envelope(n,attack=.008,release=.14):
    t=np.arange(n)/SR
    return np.minimum(1,t/max(.001,attack))*np.minimum(1,(n/SR-t)/max(.001,release))
def synth(note,duration,kind):
    n=int(SR*duration);t=np.arange(n,dtype=np.float64)/SR;f=freq(note)
    if kind=='bass':
        x=.65*np.sin(2*np.pi*f*t)
        for h in range(2,9):x+=.22*np.sin(2*np.pi*f*h*t)/h
        x+=.15*np.sin(2*np.pi*f*2.004*t)
        x=lowpass(x,1100)*envelope(n,.004,.055)*(.72+.28*np.exp(-t*9))
    elif kind=='pluck':
        x=np.zeros(n)
        for detune in [.998,1,1.003]:
            for h in range(1,7):x+=np.sin(2*np.pi*f*detune*h*t)/(h*7)
        x=lowpass(x,3300)*np.exp(-t*6.5)*envelope(n,.003,.09)
    elif kind=='lead':
        x=np.zeros(n)
        vibrato=.0025*np.sin(t*2*np.pi*5.2)*np.minimum(1,t*4)
        for detune in [.992,.997,1.003,1.008]:
            for h in range(1,min(9,int(15000/f))):
                x+=np.sin(2*np.pi*f*detune*h*t+vibrato*h*12)/(h*5.3)
        x=lowpass(x,4800)*envelope(n,.018,.12)
    else:
        x=np.zeros(n)
        for detune in [.995,1.005]:
            for h in range(1,6):x+=np.sin(2*np.pi*f*detune*h*t)/(h*9)
        x=lowpass(x,1400)*envelope(n,.32,.45)
    return x.astype(np.float32)

def drum(kind):
    duration={'kick':.43,'snare':.30,'clap':.24,'hat':.075,'open':.26,'crash':1.6}[kind]
    t=np.arange(int(SR*duration))/SR
    noise=RNG.standard_normal(len(t)).astype(np.float32)
    if kind=='kick':
        f=47+105*np.exp(-t*34)
        body=np.sin(np.cumsum(f)/SR*2*np.pi)*np.exp(-t*10)
        click=lowpass(noise,6200)*np.exp(-t*200)*.12
        x=np.tanh((body+click)*1.6)*.84
    elif kind=='snare':
        hiss=noise-lowpass(noise,1800)
        x=hiss*np.exp(-t*17)*.27+np.sin(2*np.pi*185*t)*np.exp(-t*22)*.32
        x+=np.sin(2*np.pi*335*t)*np.exp(-t*28)*.1
    elif kind=='clap':
        hiss=lowpass(noise,9200)-lowpass(noise,1200)
        bursts=sum(np.exp(-np.maximum(0,t-s)*90)*(t>=s) for s in [0,.012,.027,.043])
        x=hiss*(bursts*.14+np.exp(-t*19)*.18)
    else:
        hiss=noise-lowpass(noise,6500)
        metallic=sum(np.sin(2*np.pi*q*t) for q in [6193,7439,8669,9721])*.065
        decay={'hat':58,'open':15,'crash':3.6}[kind]
        x=(hiss*.17+metallic)*np.exp(-t*decay)
    return (x*envelope(len(t),.0004,.004)).astype(np.float32)

DRUMS={k:drum(k) for k in ['kick','snare','clap','hat','open','crash']}
def compose(name,bpm,root,style):
    beat=60/bpm;bar=4*beat;bars=48;length=int(bars*bar*SR)
    drums=np.zeros((length+int(4*SR),2),np.float32)
    synths=np.zeros_like(drums);bass=np.zeros_like(drums)
    def add(bus,sound,at,amp,pan=0):
        start=int(at*SR);end=min(len(bus),start+len(sound));n=end-start
        if n<=0:return
        bus[start:end,0]+=sound[:n]*amp*math.sqrt((1-pan)*.5)
        bus[start:end,1]+=sound[:n]*amp*math.sqrt((1+pan)*.5)
    progression=[(0,[0,3,7,10]),(-4,[0,4,7,11]),(3,[0,4,7,9]),(-2,[0,4,7,10])]
    # An original eight-bar motif, with longer responses between faster phrases.
    motif=[
        [(0,12,.5),(.75,15,.25),(1.25,19,.75),(2.25,17,.5),(3,15,.75)],
        [(0,14,.75),(1,12,.5),(1.75,7,.5),(2.5,10,.5),(3.25,12,.65)],
        [(0,15,.5),(.75,19,.5),(1.5,22,.75),(2.5,19,.5),(3.25,17,.65)],
        [(0,14,1.25),(1.5,10,.5),(2.25,12,.75),(3.25,7,.6)],
        [(0,12,.5),(.5,15,.5),(1.25,19,.5),(2,24,.75),(3,22,.8)],
        [(0,19,.75),(1,17,.5),(1.75,15,.5),(2.5,14,.5),(3.25,12,.7)],
        [(0,15,.75),(1,17,.5),(1.75,19,.75),(2.75,22,.75)],
        [(0,19,.5),(.75,17,.5),(1.5,14,.75),(2.5,12,1.2)]
    ]
    arp_pattern=[0,2,1,3,2,1,0,2]
    for b in range(bars):
        start=b*bar;chroot,intervals=progression[(b//2)%4]
        chord=root+chroot
        breakdown=24<=b<28
        build=28<=b<32
        intro=b<8
        big=32<=b
        for voice,interval in enumerate(intervals):
            sound=synth(chord+12+interval,bar*1.2,'pad')
            add(synths,sound,start,.23 if breakdown else .13,(-.55,.55,-.25,.25)[voice])
        if not breakdown:
            if style=='dnb':kick_beats=[0,1.75,2.5] if b%2==0 else [0,1.5,2.75]
            else:kick_beats=[0,1,2,3]
            if intro and b<4:kick_beats=[0,2]
            if build and b>29:kick_beats=[0,1,2,3] if b==30 else [0,1,2]
            for k in kick_beats:add(drums,DRUMS['kick'],start+k*beat,.8)
            if b>=4:
                for k in [1,3]:
                    add(drums,DRUMS['snare'],start+k*beat,.6 if style=='dnb' else .36)
                    add(drums,DRUMS['clap'],start+k*beat,.26, .08)
            for step in range(8 if intro else 16):
                k=step*(.5 if intro else .25)
                amp=.20 if step%2==0 else .11
                add(drums,DRUMS['hat'],start+k*beat,amp,(-.25 if step%2 else .25))
            if b>=8:
                for k in [.5,1.5,2.5,3.5]:add(drums,DRUMS['open'],start+k*beat,.22,.35)
        if b in [8,16,32,40]:add(drums,DRUMS['crash'],start,.3,-.3)
        if b%8==7 and b>=7:
            for k in [3,3.25,3.5,3.75]:add(drums,DRUMS['snare'],start+k*beat,.18+(k-3)*.25,-.15)
        if b>=4 and not breakdown and not (build and b==31):
            bass_pattern=[0,0,0,7,0,0,12,7]
            for step in range(8):
                note=chord-12+bass_pattern[step]
                add(bass,synth(note,beat*.46,'bass'),start+step*.5*beat,.64 if big else .53)
        if b>=4:
            for step in range(8):
                note=chord+24+intervals[arp_pattern[step]]
                add(synths,synth(note,beat*.55,'pluck'),start+step*.5*beat,.16 if breakdown else .20,math.sin(step*1.7)*.55)
        if 8<=b<24 or big or breakdown:
            for at,note,duration in motif[b%8]:
                octave=0 if breakdown else 12
                sound=synth(root+note+octave,duration*beat+.10,'lead' if not breakdown else 'pluck')
                add(synths,sound,start+at*beat,.23 if big else .19 if not breakdown else .21,math.sin(b*.7)*.12)
        if build:
            n=int(bar*SR);t=np.arange(n)/SR
            noise=RNG.standard_normal(n)
            riser=(noise-lowpass(noise,2800))*np.linspace(.015,.10+(b-28)*.045,n)
            riser*=envelope(n,.1,.15)
            add(synths,riser,start,.35,.15)
    # Stereo delays and diffusion put the melody around, rather than on top of, the drums.
    dry=synths.copy()
    for delay,gain in [(beat*.75,.22),(beat*1.5,.12),(.061,.08),(.097,.07),(.151,.055),(.223,.045),(.347,.035),(.487,.025)]:
        offset=int(delay*SR)
        synths[offset:]+=dry[:-offset,::-1]*gain
    phase=(np.arange(len(synths))/SR)%beat
    duck=.46+.54*(1-np.exp(-phase*18))
    synths*=duck[:,None];bass*=duck[:,None]
    mix=drums+bass+synths
    # Wrap remaining reverb into the beginning for a genuine seamless music loop.
    tail=mix[length:];mix=mix[:length].copy();mix[:len(tail)]+=tail
    mix=np.tanh(mix*1.1)
    peak=float(np.max(np.abs(mix)));mix*=.92/peak
    path=OUT/(name+'.ogg')
    print('Encoding',name,'peak',peak,'finite',bool(np.isfinite(mix).all()),flush=True)
    with sf.SoundFile(str(path),'w',samplerate=SR,channels=2,format='OGG',subtype='VORBIS') as encoder:
        for offset in range(0,len(mix),24000):encoder.write(mix[offset:offset+24000])
    print(name,round(length/SR,2),'seconds',path.stat().st_size,'bytes',flush=True)
    if style=='electro':
        probe=mix[int(8*bar*SR):int((8*bar+24)*SR)]
        with sf.SoundFile(str(ROOT/'outputs/BenchyRacers-Musikprobe.ogg'),'w',samplerate=SR,channels=2,format='OGG',subtype='VORBIS') as encoder:
            for offset in range(0,len(probe),24000):encoder.write(probe[offset:offset+24000])
    return {'name':name,'bpm':bpm,'duration':length/SR,'peak':float(np.max(np.abs(mix))),'rms':float(np.sqrt(np.mean(mix**2)))}

reports=[]
for spec in [('wake_runner',128,50,'electro'),('northern_drift',160,52,'dnb'),('afterglow_circuit',132,53,'electro_night')]:
    reports.append(compose(*spec))
(ROOT/'work/music-report.json').write_text(json.dumps(reports,indent=2))
