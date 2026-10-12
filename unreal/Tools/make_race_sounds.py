"""Generate original, short game cues. No external recordings or samples."""
from pathlib import Path
import wave, math, struct, random
root=Path(__file__).resolve().parents[1]/'AssetSource'/'RaceSounds'
root.mkdir(parents=True,exist_ok=True)
rate=22050
def save(name,duration,fn):
    data=[]
    for i in range(int(rate*duration)):
        t=i/rate;fade=min(1,t/.007,(duration-t)/.025)
        data.append(int(max(-1,min(1,fn(t)*fade))*.65*32767))
    with wave.open(str(root/(name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate);f.writeframes(struct.pack('<'+'h'*len(data),*data))
save('Coin',.22,lambda t:math.sin(2*math.pi*(880 if t<.10 else 1320)*t)*math.exp(-t*12)*.7)
save('Spool',.35,lambda t:sum(math.sin(2*math.pi*f*t) for f in [440,554.37,659.25])*.18*math.exp(-t*6))
save('Fire',.28,lambda t:math.sin(2*math.pi*(750*t-1100*t*t))*math.exp(-t*10)*.65)
rng=random.Random(381)
save('Hit',.43,lambda t:(math.sin(2*math.pi*75*t)*.65+rng.uniform(-1,1)*.25)*math.exp(-t*9))
save('Ready',.18,lambda t:math.sin(2*math.pi*660*t)*.5)
save('Finish',1.2,lambda t:math.sin(2*math.pi*[523.25,659.25,783.99,1046.5][min(3,int(t/.25))]*t)*.45)
print('Six original game sounds written.')
