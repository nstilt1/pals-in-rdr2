import sys
sys.dont_write_bytecode = True
from local_workspace import local_workspace
"""Write private, mipmapped DDS inputs from the user's local images.
No content from either game is distributed with this original script.
"""
from pathlib import Path
import json,struct
from PIL import Image
R=local_workspace();O=R/'private-generated/lamball-prototype';D=O/'a_c_goat_01_merge_4';D.mkdir(exist_ok=True)
def dds(image,path):
 image=image.convert('RGBA');w,h=image.size;mips=[]
 while True:
  mips.append(image)
  if image.size==(1,1):break
  image=image.resize((max(1,image.width//2),max(1,image.height//2)),Image.Resampling.LANCZOS)
 header=[124,0x2100f,h,w,w*4,0,len(mips)]+[0]*11+[32,0x41,0,32,0x00ff0000,0x0000ff00,0x000000ff,0xff000000,0x401008,0,0,0,0]
 assert len(header)==31
 with path.open('wb') as f:
  f.write(b'DDS '+struct.pack('<31I',*header))
  for im in mips:f.write(im.tobytes('raw','BGRA'))
for t in json.loads((O/'texture-inputs.json').read_text()):
 source=R/'assets/source/lamball'/Path(t['source']).name
 if not source.is_file():raise FileNotFoundError(source)
 dds(Image.open(source),D/(t['name']+'.dds'))
dds(Image.new('RGBA',(4,4),(128,128,255,255)),D/'local_creature_normal.dds')
dds(Image.new('RGBA',(4,4),(0,255,0,255)),D/'local_creature_material.dds')
for p in D.glob('*.dds'):
 im=Image.open(p);im.load();print(p.name,im.size,p.stat().st_size)
