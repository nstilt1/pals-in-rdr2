"""Validate re-exported private native resources before local deployment."""
import sys
sys.dont_write_bytecode = True
import hashlib
import json
import math
import xml.etree.ElementTree as E
from PIL import Image
from local_workspace import local_workspace

kind = sys.argv[1]
private = local_workspace() / 'private-generated'
output = private / ('roster-' + kind + ('-v015' if kind == 'chicken' else '-v014'))
native = private / ('goat-native/xml' if kind == 'goat' else 'chicken-native/xml' if kind == 'chicken' else 'roster-native/xml')
report = json.loads((output/'build-report.json').read_text())
buffers = appended = masks = 0
files = {}
for binary in output.glob('*.ydd'):
    stem = binary.stem
    original = native / (binary.name + '.xml')
    if kind in ('pig','chicken') and (native / (binary.name + '_2.xml')).exists():
        original = native / (binary.name + '_2.xml')
    before = E.parse(original).getroot().find('Drawables')
    after = E.parse(output/'verified-final'/(binary.name+'.xml')).getroot().find('Drawables')
    assert len(before) == len(after)
    for old, new in zip(before, after):
        assert old.findtext('Hash') == new.findtext('Hash')
        assert [b.find('Tag').get('value') for b in old.findall('.//Skeleton/Bones/Item')] == [b.find('Tag').get('value') for b in new.findall('.//Skeleton/Bones/Item')]
        for lod in ('LodHigh','LodMed','LodLow','LodVlow'):
            originals = old.findall(f'./{lod}/Models/Item')
            rebuilt = new.findall(f'./{lod}/Models/Item')
            assert len(rebuilt) in (len(originals), len(originals)+1)
            for old_model, new_model in zip(originals, rebuilt):
                assert old_model.findtext('BoneMapping') == new_model.findtext('BoneMapping')
                og = old_model.findall('./Geometries/Item'); ng = new_model.findall('./Geometries/Item')
                assert len(og) == len(ng)
                for a,b in zip(og,ng):
                    rows = b.findtext('Vertices').strip().splitlines()
                    assert len(rows) == len(a.findtext('Vertices').strip().splitlines())
                    assert a.findtext('Indices').split() == b.findtext('Indices').split()
                    assert len(set(rows)) == 1
                    buffers += 1
            if len(rebuilt) > len(originals):
                model = rebuilt[-1]; triangles = 0
                tags = list(map(int,model.findtext('BoneMapping').split()))
                assert tags == report['palette'] == sorted(set(tags))
                for geom in model.findall('./Geometries/Item'):
                    rows = geom.findtext('Vertices').strip().splitlines()
                    indices = list(map(int,geom.findtext('Indices').split()))
                    assert min(indices) >= 0 and max(indices) < len(rows) and len(indices)%3 == 0
                    triangles += len(indices)//3
                    assert int(geom.find('ShaderID').get('value')) < len(new.findall('./ShaderGroup/Shaders/Item'))
                    for row in rows:
                        fields = [list(map(float,f.split())) for f in row.split('\t')]
                        assert len(fields)==11 and all(math.isfinite(n) for f in fields for n in f)
                        assert sum(fields[3]+fields[4])==255
                        assert all(0<=i<len(tags) for i,w in zip(fields[5]+fields[6],fields[3]+fields[4]) if w)
                assert triangles == report['triangles']
                appended += 1
    source_masks = list((native/stem).glob('*.dds'))
    for mask in source_masks:
        with Image.open(output/'verified-final'/stem/mask.name) as image: image.load()
        masks += 1
    files[binary.name] = hashlib.sha256(binary.read_bytes()).hexdigest()
assert buffers == len(report['native_buffers']) and appended == len(report['appended_body_lods']) == (4 if kind == 'chicken' else 12)
texture = next(output.glob('*.ytd'))
pal = report['pal']
for entry in json.loads((output/'compression-inputs.json').read_text()):
    filename = entry['output'].replace('\\','/').split('/')[-1]
    with Image.open(output/'verified-final'/texture.stem/filename) as image:
        image.load()
        assert image.size == (entry['width'],entry['height'])
        if filename.endswith(('_eye.dds','_mouth.dds')) or (pal=='caprity' and filename.endswith('_body.dds')):
            assert image.convert('RGBA').getchannel('A').getextrema()==(0,255)
files[texture.name] = hashlib.sha256(texture.read_bytes()).hexdigest()
(output/'validation-report.json').write_text(json.dumps({'native_species':kind,'pal':pal,'native_buffers_preserved':buffers,'appended_body_lods':appended,'native_masks_decoded':masks,'files':files,'in_game':'pending'},indent=2))
print('PASS',kind,'native buffers',buffers,'body LODs',appended,'masks',masks,'DDS payloads/alpha/weights/indices')
