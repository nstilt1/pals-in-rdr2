"""Adapt locally prepared Lamball geometry to native sheep components.

Requires private XML/DDS exports and the confirmed local 0.1.2 model.
No game content is bundled. Run with python -B to avoid repository bytecode.
Native writer compilation and in-game validation are separate steps.
"""
import copy
import json
import shutil
import xml.etree.ElementTree as E
from local_workspace import local_workspace


def build():
    private = local_workspace() / 'private-generated'
    native = private / 'sheep-native/xml'
    output = private / 'lamball-sheep-v013'
    output.mkdir(exist_ok=True)
    # The local export list includes base first and patchpack001 second.
    # Use the patch resource so hidden buffers match the installed game.
    tree = E.parse(native / 'a_c_sheep_01_merge_6.ydd_2.xml')
    confirmed = E.parse(private / 'lamball-shared-v012/a_c_goat_01_merge_4.ydd.xml')
    body = next(d for d in confirmed.getroot().find('Drawables')
                if d.findtext('Hash').endswith('0xE5B2E10E'))
    # Native sheep torso, intact-skin torso and carcass components. Retain
    # their identities/skeletons; do not overwrite sheep metadata or behavior.
    body_hashes = ('0xD76FD006', '0xBBB31E09', '0x65E050FC')
    sheep_tags = {int(b.find('Tag').get('value')) for b in
                  E.parse(native / 'meta_base_sheep_01.yft.xml').findall('.//Bones/Item')}
    used_tags = {int(t) for m in body.findall('.//Models/Item')
                 for t in (m.findtext('BoneMapping') or '').split()}
    if not used_tags or not used_tags <= sheep_tags:
        raise ValueError('Lamball weights reference bones absent from native sheep.')
    lods = ('LodHigh', 'LodMed', 'LodLow', 'LodVlow')
    replaced, hidden = [], []
    for drawable in tree.getroot().find('Drawables'):
        identity = drawable.findtext('Hash')
        if identity.endswith(body_hashes):
            replaced.append(identity)
            for name in ('ShaderGroup', 'BoundingBoxMin', 'BoundingBoxMax',
                         'BoundingSphereCenter', 'BoundingSphereRadius', *lods):
                old = drawable.find(name)
                new = body.find(name)
                if old is not None:
                    index = list(drawable).index(old)
                    drawable.remove(old)
                    if new is not None:
                        drawable.insert(index, copy.deepcopy(new))
                elif new is not None:
                    drawable.append(copy.deepcopy(new))
        else:
            # Preserve native facial/deformation buffer lengths, layouts,
            # indices and bone palettes. Collapse triangles without resizing.
            for lod in lods:
                for geom in drawable.findall(f'./{lod}/Models/Item/Geometries/Item'):
                    rows = geom.findtext('Vertices').strip().splitlines()
                    first = rows[0].split('\t')
                    if len(first) < 2:
                        raise ValueError('Unexpected native vertex layout.')
                    first[0] = '0 0 0'
                    geom.find('Vertices').text = '\n' + '\n'.join(['\t'.join(first)] * len(rows)) + '\n'
                    hidden.append({'component': identity, 'lod': lod,
                                   'vertices': len(rows),
                                   'indices': len(geom.findtext('Indices').split())})
    if len(replaced) != len(body_hashes):
        raise ValueError('Missing expected sheep body variant.')
    stem = 'a_c_sheep_01_merge_6'
    shutil.copytree(native / stem, output / stem, dirs_exist_ok=True)
    E.indent(tree, space=' ')
    tree.write(output / (stem + '.ydd.xml'), encoding='utf-8', xml_declaration=True)

    textures = E.parse(native / 'a_c_sheep_01_merge.ytd.xml')
    items = textures.getroot().find('Textures')
    source = private / 'lamball-shared-v011'
    shared = E.parse(source / 'a_c_goat_01_merge.ytd.xml')
    texture_dir = output / 'a_c_sheep_01_merge'
    shutil.copytree(native / 'a_c_sheep_01_merge', texture_dir, dirs_exist_ok=True)
    additions = []
    for item in shared.getroot().find('Textures'):
        name = item.findtext('Name')
        if name.startswith('local_creature_'):
            items.append(copy.deepcopy(item))
            shutil.copy2(source / 'a_c_goat_01_merge' / (name + '.dds'), texture_dir)
            additions.append(name)
    if len(additions) != 5:
        raise ValueError('Expected five locally prepared Lamball textures.')
    E.indent(textures, space=' ')
    textures.write(output / 'a_c_sheep_01_merge.ytd.xml', encoding='utf-8', xml_declaration=True)
    report = {'build': 'lamball-sheep-0.1.3', 'replacement_model': 'a_c_sheep_01',
              'body_components': replaced, 'bone_tags': sorted(used_tags),
              'hidden_buffers': hidden, 'shared_textures': additions,
              'in_game': 'pending'}
    (output / 'build-report.json').write_text(json.dumps(report, indent=2))
    print(f'Prepared {len(replaced)} sheep body variants and {len(hidden)} preserved hidden buffers: {output}')


if __name__ == '__main__':
    build()
