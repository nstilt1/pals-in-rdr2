"""Prepare a user-exported UEFormat creature scene outside the repository.

Blender --background --python prepare_species.py -- NAME MESH_STEM
"""
import sys
sys.dont_write_bytecode = True
import json
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from local_workspace import local_workspace
import bpy

root = local_workspace()
name, stem = sys.argv[sys.argv.index('--') + 1:]
source = root / 'assets/source' / name
output = root / 'assets/working' / name
output.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(root / 'tools/blender-python-deps'))
sys.path.insert(0, str(root / 'tools/UEFormat/UEFormat-main/plugins/blender'))
from io_scene_ueformat.importer.import_context import UEFormatImport
from io_scene_ueformat.options import UEModelOptions
bpy.ops.wm.read_factory_settings(use_empty=True)
armature = UEFormatImport(UEModelOptions(reorient_bones=False)).import_file(source / (stem + '.uemodel'))
assert armature and armature.type == 'ARMATURE'
for material in bpy.data.materials:
    metadata = json.loads((source / (material.name + '.json')).read_text())
    texture = metadata['Textures']['Base Texture']['ObjectPath'].split('/')[-1].split('.')[0]
    image = bpy.data.images.load(str(source / (texture + '.png')), check_existing=True)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    shader.inputs['Roughness'].default_value = 0.8
    tex = nodes.new('ShaderNodeTexImage'); tex.image = image
    end = nodes.new('ShaderNodeOutputMaterial')
    mask = nodes.new('ShaderNodeMath'); mask.operation = 'GREATER_THAN'
    mask.inputs[1].default_value = metadata.get('Properties', {}).get('BasePropertyOverrides', {}).get('OpacityMaskClipValue', 0.3333)
    material.node_tree.links.new(tex.outputs['Color'], shader.inputs['Base Color'])
    material.node_tree.links.new(tex.outputs['Alpha'], mask.inputs[0])
    material.node_tree.links.new(mask.outputs[0], shader.inputs['Alpha'])
    material.node_tree.links.new(shader.outputs[0], end.inputs[0])
    material.surface_render_method = 'DITHERED'
report = {'meshes': [], 'bones': []}
for obj in bpy.context.scene.objects:
    if obj.type == 'MESH':
        obj.data.calc_loop_triangles()
        sums = [sum(g.weight for g in v.groups) for v in obj.data.vertices]
        assert all(abs(s - 1) < 0.001 for s in sums)
        report['meshes'].append({'name': obj.name, 'vertices': len(obj.data.vertices),
                                'triangles': len(obj.data.loop_triangles),
                                'dimensions': list(obj.dimensions),
                                'bounds': [[min(v.co[i] for v in obj.data.vertices), max(v.co[i] for v in obj.data.vertices)] for i in range(3)],
                                'materials': [m.name for m in obj.data.materials]})
for bone in armature.data.bones:
    report['bones'].append({'name': bone.name, 'head': list(bone.head_local),
                            'parent': bone.parent.name if bone.parent else None})
assert report['meshes']
bpy.ops.wm.save_as_mainfile(filepath=str(output / (name + '-source.blend')))
(output / 'source-report.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
