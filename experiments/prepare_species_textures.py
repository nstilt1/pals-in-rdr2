"""Prepare private RGBA inputs for external BC3 compression with mipmaps."""
import sys
sys.dont_write_bytecode = True
import json
from pathlib import Path
from PIL import Image
from local_workspace import local_workspace

kind = sys.argv[1]
root = local_workspace()
output = root / 'private-generated' / ('roster-' + kind + ('-v015' if kind == 'chicken' else '-v014'))
stem = {'goat':'a_c_goat_01_merge','boar':'a_c_boar_01_merge','pig':'a_c_pig_01_merge','chicken':'a_c_chicken_01_head_00_merge'}[kind]
pal = 'caprity' if kind == 'goat' else 'chikipi' if kind == 'chicken' else 'rushoar'
directory = output / stem
directory.mkdir(exist_ok=True)
inputs = json.loads((output / 'texture-inputs.json').read_text())
images = []
for item in inputs:
    source = root / 'assets/source' / pal / Path(item['source']).name
    image = Image.open(source).convert('RGBA')
    maximum = 1024 if item['name'].endswith('_body') else 512
    image.thumbnail((maximum, maximum), Image.Resampling.LANCZOS)
    images.append((item['name'], image))
images += [(f'pals_{pal}_normal', Image.new('RGBA',(4,4),(128,128,255,255))),
           (f'pals_{pal}_material', Image.new('RGBA',(4,4),(0,255,0,255)))]
manifest = []
for name, image in images:
    raw = output / (name + '.rgba')
    raw.write_bytes(image.tobytes())
    manifest.append({'raw':str(raw),'output':str(directory/(name+'.dds')),
                     'width':image.width,'height':image.height})
(output/'compression-inputs.json').write_text(json.dumps(manifest,indent=2))
print('PREPARED',len(manifest),'compressed texture inputs for',kind)
