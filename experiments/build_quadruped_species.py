"""Experimental native component builder for private, locally exported Pals.

Blender --background --python build_quadruped_species.py -- goat|boar|pig
Keeps native geometry buffers collapsed in place and appends custom models.
External native writer compilation and independent validation remain required.
"""
import sys
sys.dont_write_bytecode = True
import copy
import json
import shutil
import xml.etree.ElementTree as E
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from local_workspace import local_workspace
import bpy

root = local_workspace()
kind = sys.argv[sys.argv.index('--') + 1]
configs = {
    'goat': ('caprity', 'goat-native/xml', 'a_c_goat_01_merge_4.ydd.xml', 'a_c_goat_01_merge', 'meta_base_goat_01.yft.xml', ('E5B2E10E', '4D67172B', '58A83221', '8E962B04'), .43, -.444),
    'boar': ('rushoar', 'roster-native/xml', 'a_c_boar_01_merge_3.ydd.xml', 'a_c_boar_01_merge', 'meta_base_boar_01.yft_2.xml', ('54EE8327', 'A1D3B056', '195CD2B9', '7D4278B6'), .58, -.391),
    'pig': ('rushoar', 'roster-native/xml', 'a_c_pig_01_merge_2.ydd_2.xml', 'a_c_pig_01_merge', 'meta_base_pig_01.yft_2.xml', ('303F549D', '11190B91', 'A199F9D9', '7F212E2C'), .45, -.31),
    'chicken': ('chikipi', 'chicken-native/xml', 'a_c_chicken_01_head_00_merge_2.ydd_2.xml', 'a_c_chicken_01_head_00_merge', 'meta_base_chicken_01.yft_2.xml', ('1F6603BE','5121E735'), .55, -.208),
}
pal, native_dir, model_xml, texture_stem, skeleton_xml, body_hashes, scale, z_offset = configs[kind]
native = root / 'private-generated' / native_dir
output = root / 'private-generated' / ('roster-' + kind + ('-v015' if kind == 'chicken' else '-v014'))
output.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root / 'assets/working' / pal / (pal + '-source.blend')))
obj = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
mesh = obj.data
mesh.calc_loop_triangles(); mesh.calc_tangents()
native_bones = {b.findtext('Name'): int(b.find('Tag').get('value'))
                for b in E.parse(native / skeleton_xml).findall('.//Bones/Item')}

def native_name(group):
    name = group.lower()
    side = 'L' if name.endswith('_l') else 'R'
    if kind == 'chicken':
        side = 'L' if '_l' in name else 'R'
        if name.startswith('foot_'): return f'SKEL_{side}_Calf' if '_01_' in name else f'SKEL_{side}_Foot'
        if name.startswith('ball_'): return f'SKEL_{side}_Toe0'
        if name.startswith(('wing_', 'closedwing_')): return f'SKEL_{side}_UpperArm' if '_01_' in name else f'SKEL_{side}_Forearm'
        if name.startswith('finger_'): return f'SKEL_{side}_Forearm'
        if name.startswith('tail_'): return 'SKEL_Tail0'
        if name.startswith('hair_'): return 'SKEL_Comb' if name != 'hair_03' else 'SKEL_Head'
    if name.startswith(('leg_front_', 'foot_front_')):
        part = 'Hand' if name.startswith('foot_') or '_04_' in name or '_03_' in name else 'Forearm' if '_02_' in name else 'UpperArm'
        return f'SKEL_{side}_{part}'
    if name.startswith(('leg_back_', 'foot_back_')):
        part = 'Foot' if name.startswith('foot_') or '_04_' in name or '_03_' in name else 'Calf' if '_02_' in name else 'Thigh'
        return f'SKEL_{side}_{part}'
    if name.startswith('ear_'): return f'SKEL_{side}_Ear0'
    if name.startswith('neck'): return 'SKEL_Neck0'
    if name.startswith(('head', 'nose', 'jow', 'tongue', 'hair', 'socket_mouth')): return 'SKEL_Head'
    if name.startswith('tail_'): return 'SKEL_Tail' + str(min(int(name.split('_')[1]) - 1, 2))
    if name == 'pelvis' or name == 'root': return 'SKEL_Pelvis'
    if name.startswith('spine_'): return 'SKEL_Spine' + str(min(int(name.split('_')[1]) - 1, 3))
    return 'SKEL_Spine1'

group_tags = {g.index: native_bones[native_name(g.name)] for g in obj.vertex_groups}
palette = sorted(set(group_tags.values()))
def weights(vertex):
    combined = {}
    for group in vertex.groups:
        tag = group_tags[group.group]
        combined[tag] = combined.get(tag, 0) + group.weight
    pairs = sorted(combined.items(), key=lambda x: -x[1])[:4]
    assert pairs and sum(w for _, w in pairs) > 0
    total = sum(w for _, w in pairs)
    values = [round(w / total * 255) for _, w in pairs]
    values[0] += 255 - sum(values)
    ids = [palette.index(t) for t, _ in pairs]
    return values + [0] * (4-len(values)), ids + [0] * (4-len(ids))

def value(node, tag, val): E.SubElement(node, tag).set('value', str(val))
def vector(node, tag, xyz): E.SubElement(node, tag).attrib = {k: f'{v:.8g}' for k, v in zip('xyz', xyz)}
def floats(items): return ' '.join(f'{v:.8g}' for v in items)

coords = [(v.co.x*scale, v.co.y*scale, v.co.z*scale+z_offset) for v in mesh.vertices]
low = tuple(min(v[i] for v in coords) for i in range(3))
high = tuple(max(v[i] for v in coords) for i in range(3))
goat = E.parse(root / 'private-generated/goat-native/xml/a_c_goat_01_merge_4.ydd.xml')
drawables = list(goat.getroot().find('Drawables'))
body_shader = next(d for d in drawables if d.findtext('Hash').endswith('E5B2E10E')).find('./ShaderGroup/Shaders/Item')
face_shader = next(d for d in drawables if d.findtext('Hash').endswith('97DCFB9B')).find('./ShaderGroup/Shaders/Item')
shaders, geometries, texture_inputs = [], [], []
for material_index, material in enumerate(mesh.materials):
    part = 'eye' if 'Eye' in material.name else 'mouth' if 'Mouth' in material.name else 'body'
    name = f'pals_{pal}_{part}'
    texture_inputs.append({'name': name, 'source': next(bpy.path.abspath(n.image.filepath) for n in material.node_tree.nodes if n.type == 'TEX_IMAGE')})
    shader = copy.deepcopy(body_shader if part == 'body' and pal != 'caprity' else face_shader)
    for parameter in shader.findall('./Parameters/Items/Item'):
        if parameter.get('type') == 'Texture':
            key = parameter.get('name')
            texture = name if key == 'diffusetex' else f'pals_{pal}_normal' if key == 'bumptex' else f'pals_{pal}_material' if key in ('speculartex','speculartex2') else None
            if texture: parameter.set('texture', texture)
            else: parameter.attrib.pop('texture', None)
        if parameter.get('name') in ('bumpiness','specularcolor'): parameter.set('x','0')
    shaders.append(shader)
    geom = E.Element('Item'); value(geom, 'ShaderID', material_index)
    vector(geom,'BoundingBoxMin',low); vector(geom,'BoundingBoxMax',high)
    value(geom,'BonesCount',len(palette)); E.SubElement(geom,'Name').text = shader.findtext('Name')
    layout = E.SubElement(geom,'VertexLayout')
    E.SubElement(layout,'Semantics').text='PNXWWIICCTT'; E.SubElement(layout,'Formats').text='39955665522'; value(layout,'NonInterleaved','True')
    rows, indices, cache = [], [], {}
    for triangle in mesh.loop_triangles:
        if triangle.material_index != material_index: continue
        for li in triangle.loops:
            loop = mesh.loops[li]; vi = loop.vertex_index; w, ids = weights(mesh.vertices[vi])
            uv = mesh.uv_layers.active.data[li].uv
            row = '\t'.join([floats(coords[vi]),floats((*mesh.corner_normals[li].vector,0)),floats((*loop.tangent,loop.bitangent_sign)),floats(w),'0 0 0 0',floats(ids),'0 0 0 0','255 255 255 255','0 0 0 0',floats((uv.x,1-uv.y)),floats((uv.x,1-uv.y))])
            if row not in cache: cache[row]=len(rows); rows.append(row)
            indices.append(cache[row])
    E.SubElement(geom,'Vertices').text='\n'+'\n'.join(rows)+'\n'
    E.SubElement(geom,'Indices').text=' '.join(map(str,indices))
    geometries.append(geom)

model_files = [model_xml] + (['a_c_pig_01_merge_3.ydd.xml'] if kind == 'pig' else [])
preserved, appended = [], []
for filename in model_files:
    tree = E.parse(native / filename)
    # Exporter's _2 suffix distinguishes patch from base, not the native name.
    stem = filename.replace('.ydd_2.xml','.ydd.xml').replace('.ydd.xml','')
    if (native / stem).exists():
        shutil.copytree(native / stem, output / stem, dirs_exist_ok=True)
    for drawable in tree.getroot().find('Drawables'):
        identity = drawable.findtext('Hash')
        is_body = any(identity.endswith(h) for h in body_hashes)
        shader_items = drawable.find('./ShaderGroup/Shaders')
        offset = len(shader_items)
        if is_body: shader_items.extend(copy.deepcopy(shaders))
        for lod in ('LodHigh','LodMed','LodLow','LodVlow'):
            container = drawable.find(f'./{lod}/Models')
            if container is None: continue
            for geom in container.findall('./Item/Geometries/Item'):
                rows = geom.findtext('Vertices').strip().splitlines(); first=rows[0].split('\t'); first[0]='0 0 0'
                geom.find('Vertices').text='\n'+'\n'.join(['\t'.join(first)]*len(rows))+'\n'
                preserved.append({'file':stem,'hash':identity,'lod':lod,'vertices':len(rows),'indices':len(geom.findtext('Indices').split())})
            if is_body:
                model=E.SubElement(container,'Item'); value(model,'Flags',1); value(model,'HasSkin','True')
                E.SubElement(model,'BoneMapping').text=' '.join(map(str,palette))
                vector(model,'BoundingBoxMin',low); vector(model,'BoundingBoxMax',high)
                gs=E.SubElement(model,'Geometries'); gs.extend(copy.deepcopy(geometries))
                for geom in gs: geom.find('ShaderID').set('value',str(int(geom.find('ShaderID').get('value'))+offset))
                appended.append({'file':stem,'hash':identity,'lod':lod})
        if is_body:
            for tag, xyz in [('BoundingBoxMin',low),('BoundingBoxMax',high),('BoundingSphereCenter',tuple((a+b)/2 for a,b in zip(low,high)))]:
                drawable.remove(drawable.find(tag)); vector(drawable,tag,xyz)
            drawable.find('BoundingSphereRadius').set('value','2.5')
    E.indent(tree,space=' '); tree.write(output/(stem+'.ydd.xml'),encoding='utf-8',xml_declaration=True)
textures=E.parse(native/(texture_stem+'.ytd.xml'))
items=textures.getroot().find('Textures')
for name in [t['name'] for t in texture_inputs]+[f'pals_{pal}_normal',f'pals_{pal}_material']:
    item=E.SubElement(items,'Item'); E.SubElement(item,'Name').text=name; value(item,'Flags',402685954)
shutil.copytree(native/texture_stem,output/texture_stem,dirs_exist_ok=True)
E.indent(textures,space=' '); textures.write(output/(texture_stem+'.ytd.xml'),encoding='utf-8',xml_declaration=True)
(output/'texture-inputs.json').write_text(json.dumps(texture_inputs,indent=2))
(output/'build-report.json').write_text(json.dumps({'pal':pal,'native_species':kind,'native_buffers':preserved,'appended_body_lods':appended,'triangles':len(mesh.loop_triangles),'palette':palette,'scale':scale,'bounds':[low,high],'in_game':'pending'},indent=2))
print('PREPARED',output,'preserved buffers',len(preserved),'body LODs',len(appended))
