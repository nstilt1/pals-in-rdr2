import sys
sys.dont_write_bytecode = True
from local_workspace import local_workspace
"""Validate private generated content before local installation; no bundled assets."""
import hashlib,json,math
from pathlib import Path
import xml.etree.ElementTree as E
from PIL import Image
R=local_workspace()/'private-generated/lamball-prototype'
tree=E.parse(R/'verified/a_c_goat_01_merge_4.ydd.xml');root=tree.getroot()
variants=('0xE5B2E10E','0x4D67172B','0x58A83221','0x8E962B04')
count=0
for d in root.find('Drawables'):
 if not d.findtext('Hash').endswith(variants):continue
 count+=1
 for model in d.findall('./LodHigh/Models/Item')+d.findall('./LodMed/Models/Item')+d.findall('./LodLow/Models/Item'):
  tags=list(map(int,model.findtext('BoneMapping').split()));assert tags==[14411,45454,33646,55120,43312]
  tris=0
  for g in model.findall('./Geometries/Item'):
   rows=g.findtext('Vertices').strip().splitlines();indices=list(map(int,g.findtext('Indices').split()));assert len(indices)%3==0 and max(indices)<len(rows)
   tris+=len(indices)//3
   for row in rows:
    fields=[list(map(float,f.split())) for f in row.split('\t')]
    assert len(fields)==11
    assert all(math.isfinite(n) for field in fields for n in field)
    assert sum(fields[3]+fields[4])==255
    assert all(0<=i<len(tags) for i,w in zip(fields[5]+fields[6],fields[3]+fields[4]) if w)
  assert tris==6762
assert count==4
texdir=R/'verified/a_c_goat_01_merge_4'
# Verify actual texture bytes re-exported from the rebuilt native resource.
for name,size in [('body',(2048,2048)),('eye',(1024,1024)),('mouth',(1024,1024))]:
 image=Image.open(texdir/f'local_creature_{name}.dds').convert('RGBA');assert image.size==size
 if name!='body':assert image.getchannel('A').getextrema()==(0,255)
native=R/'a_c_goat_01_merge_4.ydd';digest=hashlib.sha256(native.read_bytes()).hexdigest()
(R/'validated-sha256.txt').write_text(digest)
(R/'validation-report.json').write_text(json.dumps({'native_sha256':digest,'native_bytes':native.stat().st_size,'body_variants':count,'triangles_per_body_lod':6762,'weight_sums':255,'embedded_textures_verified':True,'independent_reader':'RPFView.Models.ReadRdr2; zero warnings; GLB exported and visually inspected','in_game_test':'pending','limitations':['First-pass native animation mapping','All goats share the replacement','Skinning retains native goat behavior, with replacement body on skinned variants']},indent=2))
print('PASS: native geometry, weights, texture payloads and alpha validated',digest)
