import sys
sys.dont_write_bytecode = True
from local_workspace import local_workspace
"""Local-only prototype conversion. No game content is bundled with this code.
Reads user-supplied source scene and native templates; writes private derived XML.
"""
import bpy, copy, json, math
from pathlib import Path
import xml.etree.ElementTree as E
ROOT=local_workspace()
OUT=ROOT/'private-generated/lamball-prototype'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'assets/working/lamball/lamball-source.blend'))
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
mesh=obj.data
mesh.calc_loop_triangles()
mesh.calc_tangents()
tree=E.parse(ROOT/'private-generated/goat-native/xml/a_c_goat_01_merge_4.ydd.xml')
drawables=tree.getroot().find('Drawables')
body=next(d for d in drawables if d.findtext('Hash').endswith('0xE5B2E10E'))
hair=next(d for d in drawables if d.findtext('Hash').endswith('0x97DCFB9B'))
body_shader=copy.deepcopy(body.find('./ShaderGroup/Shaders/Item'))
face_shader=copy.deepcopy(hair.find('./ShaderGroup/Shaders/Item'))
# Keep native skeleton tags and behavior. First pass deliberately maps the round
# torso/face to the spine; short legs follow the native rear legs. No new rig.
tags={'spine':14411,'foot_l':45454,'foot_r':33646,'calf_l':55120,'calf_r':43312}
tag_list=list(tags.values())
def target_group(name):
 if name.startswith('leg_'):
  side='l' if name.endswith('_l') else 'r'
  return tags[('foot_' if '_03_' in name else 'calf_')+side]
 return tags['spine']
def weights(v):
 accum={}
 for g in v.groups:
  tag=target_group(obj.vertex_groups[g.group].name)
  accum[tag]=accum.get(tag,0)+g.weight
 pairs=sorted(accum.items(),key=lambda x:-x[1])[:4] or [(tags['spine'],1)]
 total=sum(w for _,w in pairs)
 ws=[round(w/total*255) for _,w in pairs];ws[0]+=255-sum(ws)
 ids=[tag_list.index(t) for t,_ in pairs]
 return ws+[0]*(4-len(ws)),ids+[0]*(4-len(ids))
def floats(v):return ' '.join(f'{x:.8g}' for x in v)
def vec(e,name,v):
 x=e.find(name)
 if x is None:x=E.SubElement(e,name)
 x.attrib={k:f'{a:.8g}' for k,a in zip('xyz',v)}
def value(e,name,v):
 x=e.find(name)
 if x is None:x=E.SubElement(e,name)
 x.set('value',str(v))
# Native rest geometry is centered around the spine; the idle animation supplies
# the standing height. This is a prototype fit, not a claim of faithful retargeting.
scale=.9
coords=[(v.co.x*scale,v.co.y*scale,v.co.z*scale-.42) for v in mesh.vertices]
low=tuple(min(p[i] for p in coords) for i in range(3));high=tuple(max(p[i] for p in coords) for i in range(3))
shaders=[];geoms=[];textures=[]
for mat_idx,mat in enumerate(mesh.materials):
 name='local_creature_'+('eye' if 'Eye' in mat.name else 'mouth' if 'Mouth' in mat.name else 'body')
 textures.append({'name':name,'source':next(n.image.filepath for n in mat.node_tree.nodes if n.type=='TEX_IMAGE')})
 shader=copy.deepcopy(face_shader if 'Body' not in mat.name else body_shader)
 for p in shader.findall('./Parameters/Items/Item'):
  if p.get('type')=='Texture':
   if p.get('name')=='diffusetex':p.set('texture',name)
   elif p.get('name')=='bumptex':p.set('texture','local_creature_normal')
   elif p.get('name') in ('speculartex','speculartex2'):p.set('texture','local_creature_material')
   elif 'texture' in p.attrib:del p.attrib['texture']
  if p.get('name')=='bumpiness':p.set('x','0')
  if p.get('name')=='specularcolor':p.set('x','0')
 shaders.append(shader)
 geom=E.Element('Item');value(geom,'ShaderID',mat_idx);vec(geom,'BoundingBoxMin',low);vec(geom,'BoundingBoxMax',high);value(geom,'BonesCount',len(tag_list));E.SubElement(geom,'Name').text=shader.findtext('Name')
 layout=E.SubElement(geom,'VertexLayout');E.SubElement(layout,'Semantics').text='PNXWWIICCTT';E.SubElement(layout,'Formats').text='39955665522';value(layout,'NonInterleaved','True')
 rows=[];indices=[];cache={}
 for tri in mesh.loop_triangles:
  if tri.material_index!=mat_idx:continue
  for li in tri.loops:
   loop=mesh.loops[li];vi=loop.vertex_index;w,ids=weights(mesh.vertices[vi]);uv=mesh.uv_layers.active.data[li].uv
   normal=mesh.corner_normals[li].vector
   row='\t'.join([floats(coords[vi]),floats((*normal,0)),floats((*loop.tangent,loop.bitangent_sign)),floats(w),'0 0 0 0',floats(ids),'0 0 0 0','255 255 255 255','0 0 0 0',floats((uv.x,1-uv.y)),floats((uv.x,1-uv.y))])
   if row not in cache:cache[row]=len(rows);rows.append(row)
   indices.append(cache[row])
 E.SubElement(geom,'Vertices').text='\n'+'\n'.join(rows)+'\n';E.SubElement(geom,'Indices').text='\n'+ ' '.join(map(str,indices))+'\n';geoms.append(geom)
# Replace the complete visible body/pelt variants. Other parts remain valid
# dictionary entries but are collapsed to degenerate geometry to hide goat pieces.
for d in drawables:
 is_body=d.findtext('Hash').endswith(('0xE5B2E10E','0x4D67172B','0x58A83221','0x8E962B04'))
 for lod in [x for x in d if x.tag.startswith('Lod') and x.tag in ('LodHigh','LodMed','LodLow','LodVlow')]:
  if is_body:
   lod.clear();models=E.SubElement(lod,'Models');model=E.SubElement(models,'Item');value(model,'Flags',1);value(model,'HasSkin','True');E.SubElement(model,'BoneMapping').text=' '.join(map(str,tag_list));vec(model,'BoundingBoxMin',low);vec(model,'BoundingBoxMax',high);g=E.SubElement(model,'Geometries');g.extend(copy.deepcopy(geoms))
  else:
   for g in lod.findall('.//Geometries/Item'):
    lines=(g.findtext('Vertices') or '').strip().splitlines()
    if lines:g.find('Vertices').text='\n'+'\n'.join([lines[0]]*3)+'\n';g.find('Indices').text='0 1 2'
 if is_body:
  group=d.find('ShaderGroup');group.clear();td=E.SubElement(group,'TextureDictionary',version='1');ts=E.SubElement(td,'Textures')
  for n in [t['name'] for t in textures]+['local_creature_normal','local_creature_material']:
   it=E.SubElement(ts,'Item');E.SubElement(it,'Name').text=n;value(it,'Flags',402685954)
  ss=E.SubElement(group,'Shaders');ss.extend(copy.deepcopy(shaders))
  vec(d,'BoundingBoxMin',low);vec(d,'BoundingBoxMax',high);vec(d,'BoundingSphereCenter',tuple((a+b)/2 for a,b in zip(low,high)));value(d,'BoundingSphereRadius',1.2)
E.indent(tree,space=' ');tree.write(OUT/'a_c_goat_01_merge_4.ydd.xml',encoding='utf-8',xml_declaration=True)
(OUT/'texture-inputs.json').write_text(json.dumps(textures,indent=2))
(OUT/'mesh-report.json').write_text(json.dumps({'source_triangles':len(mesh.loop_triangles),'native_geometries':[{'vertices':len(g.findtext('Vertices').strip().splitlines()),'triangles':len(g.findtext('Indices').split())//3} for g in geoms],'bone_tags':tag_list,'rest_bounds':[low,high],'status':'Experimental native replacement; in-game validation pending'},indent=2))
print('WROTE',OUT)
