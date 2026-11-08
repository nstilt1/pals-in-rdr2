"""Check privately compiled/re-exported sheep resources; no game data bundled."""
import hashlib
import json
import math
import xml.etree.ElementTree as E
from PIL import Image
from local_workspace import local_workspace


def validate():
    private = local_workspace() / 'private-generated'
    output = private / 'lamball-sheep-v013'
    native = E.parse(private / 'sheep-native/xml/a_c_sheep_01_merge_6.ydd_2.xml')
    rebuilt = E.parse(output / 'verified-final/a_c_sheep_01_merge_6.ydd.xml')
    source = {d.findtext('Hash'): d for d in native.getroot().find('Drawables')}
    bodies = ('0xD76FD006', '0xBBB31E09', '0x65E050FC')
    hidden, variants, lod_count = 0, 0, 0
    for d in rebuilt.getroot().find('Drawables'):
        identity = d.findtext('Hash')
        assert identity in source
        is_body = identity.endswith(bodies)
        if is_body:
            variants += 1
        for lod in ('LodHigh', 'LodMed', 'LodLow', 'LodVlow'):
            models = d.findall(f'./{lod}/Models/Item')
            if is_body and models:
                lod_count += 1
            native_geoms = source[identity].findall(f'./{lod}/Models/Item/Geometries/Item')
            geoms = d.findall(f'./{lod}/Models/Item/Geometries/Item')
            if not is_body:
                assert len(geoms) == len(native_geoms)
            triangles = 0
            for i, geom in enumerate(geoms):
                rows = geom.findtext('Vertices').strip().splitlines()
                indices = list(map(int, geom.findtext('Indices').split()))
                assert len(indices) % 3 == 0 and min(indices) >= 0 and max(indices) < len(rows)
                triangles += len(indices) // 3
                if not is_body:
                    old = native_geoms[i]
                    assert len(rows) == len(old.findtext('Vertices').strip().splitlines())
                    assert indices == list(map(int, old.findtext('Indices').split()))
                    assert len(set(rows)) == 1
                    hidden += 1
                else:
                    for row in rows:
                        fields = [list(map(float, field.split())) for field in row.split('\t')]
                        assert all(math.isfinite(n) for field in fields for n in field)
                        assert sum(fields[3] + fields[4]) == 255
            if is_body and models:
                assert triangles == 6762
                for model in models:
                    tags = list(map(int, model.findtext('BoneMapping').split()))
                    assert tags == sorted(tags) and len(tags) == 5
                    for geom in model.findall('./Geometries/Item'):
                        for row in geom.findtext('Vertices').strip().splitlines():
                            f = [list(map(float, field.split())) for field in row.split('\t')]
                            assert all(0 <= index < len(tags) for index, weight
                                       in zip(f[5] + f[6], f[3] + f[4]) if weight)
    assert variants == 3 and hidden == 23 and lod_count == 9
    texture_dir = output / 'verified-final/a_c_sheep_01_merge'
    for name, size in [('body', (1024, 1024)), ('eye', (512, 512)), ('mouth', (512, 512))]:
        with Image.open(texture_dir / f'local_creature_{name}.dds') as im:
            assert im.size == size
            if name != 'body':
                assert im.convert('RGBA').getchannel('A').getextrema() == (0, 255)
    masks = list((output / 'verified-final/a_c_sheep_01_merge_6').glob('*.dds'))
    assert len(masks) == 2
    for mask in masks:
        with Image.open(mask) as im:
            im.load()
    report = {'build': 'lamball-sheep-0.1.3', 'body_variants': variants,
              'body_lods': lod_count, 'hidden_buffers_preserved': hidden,
              'embedded_masks_decoded': len(masks), 'in_game': 'pending',
              'files': {name: hashlib.sha256((output / name).read_bytes()).hexdigest()
                        for name in ('a_c_sheep_01_merge_6.ydd', 'a_c_sheep_01_merge.ytd')}}
    (output / 'validation-report.json').write_text(json.dumps(report, indent=2))
    print('PASS: sheep geometry, weights, native hidden indices/counts, DDS payloads and alpha.')


if __name__ == '__main__':
    validate()
