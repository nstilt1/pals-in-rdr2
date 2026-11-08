import sys
sys.dont_write_bytecode = True
from local_workspace import local_workspace
"""Rebuild the local Lamball source scene; does not write to either game."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = local_workspace()
sys.path.insert(0, str(ROOT / 'tools/blender-python-deps'))
sys.path.insert(0, str(ROOT / 'tools/UEFormat/UEFormat-main/plugins/blender'))
from io_scene_ueformat.importer.import_context import UEFormatImport
from io_scene_ueformat.options import UEModelOptions

SOURCE = ROOT / 'assets/source/lamball'
arguments = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
if arguments and (len(arguments) != 2 or arguments[0] != '--output'):
    raise ValueError('Expected -- --output LOCAL_DIRECTORY')
OUTPUT = Path(arguments[1]).resolve() if arguments else ROOT / 'assets/working/lamball'
OUTPUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
armature = UEFormatImport(UEModelOptions(reorient_bones=False)).import_file(SOURCE / 'SK_SheepBall.uemodel')
meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
assert armature and armature.type == 'ARMATURE', 'No imported armature'
assert meshes, 'No imported mesh'
report = {'blender_version': bpy.app.version_string, 'source': str(SOURCE),
          'status': 'Blender source preparation only; not an RDR2-compatible asset',
          'materials': [], 'meshes': [], 'bones': []}

for mat in list(bpy.data.materials):
    metadata_path = SOURCE / (mat.name + '.json')
    assert metadata_path.exists(), f'Missing metadata for {mat.name}'
    metadata = json.loads(metadata_path.read_text())
    texture_name = metadata['Textures']['Base Texture']['ObjectPath'].split('/')[-1].split('.')[0]
    image = bpy.data.images.load(str(SOURCE / (texture_name + '.png')), check_existing=True)
    image.colorspace_settings.name = 'sRGB'
    alpha = list(image.pixels)[3::4]
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    output = nodes.new('ShaderNodeOutputMaterial')
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    shader.inputs['Roughness'].default_value = 0.75 if 'Body' in mat.name else 0.45
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = image
    tex.label = 'Original Palworld base texture'
    mat.node_tree.links.new(tex.outputs['Color'], shader.inputs['Base Color'])
    # Palworld metadata specifies BLEND_Masked, not an opaque face rectangle.
    threshold = metadata.get('Properties', {}).get('BasePropertyOverrides', {}).get('OpacityMaskClipValue', 0.3333)
    mask = nodes.new('ShaderNodeMath')
    mask.operation = 'GREATER_THAN'
    mask.inputs[1].default_value = threshold
    mask.label = 'Source opacity-mask threshold'
    mat.node_tree.links.new(tex.outputs['Alpha'], mask.inputs[0])
    mat.node_tree.links.new(mask.outputs[0], shader.inputs['Alpha'])
    mat.node_tree.links.new(shader.outputs['BSDF'], output.inputs['Surface'])
    mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    report['materials'].append({'name': mat.name, 'texture': image.name,
        'size': list(image.size), 'alpha_min': min(alpha), 'alpha_max': max(alpha),
        'transparent_pixels': sum(a <= threshold for a in alpha), 'opacity_threshold': threshold,
        'note': 'Source alpha masking restored; source subsurface/clearcoat and packed mask shading not fully reproduced'})

for mesh in meshes:
    mesh.data.calc_loop_triangles()
    sums = [sum(group.weight for group in vertex.groups) for vertex in mesh.data.vertices]
    report['meshes'].append({'name': mesh.name, 'vertices': len(mesh.data.vertices),
        'triangles': len(mesh.data.loop_triangles), 'uv_layers': len(mesh.data.uv_layers),
        'materials': [mat.name for mat in mesh.data.materials], 'dimensions_m': list(mesh.dimensions),
        'unweighted_vertices': sum(s < 0.00001 for s in sums),
        'max_weight_sum_error': max(abs(s-1) for s in sums),
        'max_influences': max(len(v.groups) for v in mesh.data.vertices),
        'armature_modifier': any(m.type == 'ARMATURE' and m.object == armature for m in mesh.modifiers)})
    assert not report['meshes'][-1]['unweighted_vertices'], 'Unweighted vertices'
    assert report['meshes'][-1]['armature_modifier'], 'Missing skin deformation modifier'

for bone in armature.data.bones:
    report['bones'].append({'name': bone.name, 'parent': bone.parent.name if bone.parent else None,
        'head_m': list(bone.head_local), 'tail_m': list(bone.tail_local)})
report['bone_count'] = len(report['bones'])
armature.show_in_front = True

# Neutral studio scene used only for verifying the imported asset.
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 24
scene.render.threads_mode = 'FIXED'
scene.render.threads = 4
scene.render.resolution_x = 900
scene.render.resolution_y = 900
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.world = bpy.data.worlds.new('Lamball Studio')
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs[0].default_value = (0.12, 0.14, 0.18, 1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value = 0.45
scene.view_settings.view_transform = 'AgX'

for name, loc, energy, size in [('Key', (2,-3,4), 350, 4), ('Fill', (-3,-1,2), 150, 3), ('Rim', (0,3,3), 250, 3)]:
    light = bpy.data.lights.new(name, 'AREA')
    light.energy, light.shape, light.size = energy, 'DISK', size
    obj = bpy.data.objects.new(name, light)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector((0,0,0.5)) - obj.location).to_track_quat('-Z','Y').to_euler()

camera_data = bpy.data.cameras.new('Inspection Camera')
camera = bpy.data.objects.new('Inspection Camera', camera_data)
scene.collection.objects.link(camera)
scene.camera = camera
camera_data.type = 'ORTHO'
camera_data.ortho_scale = 1.6
camera.location = (0,-3,1.15)
camera.rotation_euler = (Vector((0,0,0.48)) - camera.location).to_track_quat('-Z','Y').to_euler()

for obj in bpy.context.selected_objects:
    obj.select_set(False)
armature.select_set(True)
bpy.context.view_layer.objects.active = armature
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces.active.region_3d.view_distance = 2.2
            area.spaces.active.region_3d.view_location = (0,0,0.5)

bpy.ops.file.pack_all()
scene.render.filepath = str(OUTPUT / 'lamball-front.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / 'lamball-source.blend'))
(OUTPUT / 'import-report.json').write_text(json.dumps(report, indent=2))
bpy.ops.render.render(write_still=True)
print('LAMBALL_IMPORT_VERIFIED', json.dumps({'bones': report['bone_count'], 'meshes': report['meshes']}))
