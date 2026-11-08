"""Original sphere geometry/textures, using private native rig/material templates.

All inputs and generated resources stay outside the public source tree.
"""
import copy,json,math,xml.etree.ElementTree as E
from local_workspace import local_workspace
root=local_workspace();out=root/'private-generated/sphere-v070';out.mkdir(exist_ok=True)
native=root/'private-generated/sphere-native/xml'
tree=E.parse(native/'w_melee_knife05.ydr.xml');drawable=tree.getroot()
drawable.find('Name').text='w_pals_sphere01.#dr'
r=.075;center=.035
low=(-r,-r,center-r);high=(r,r,center+r)
def vector(n,tag,p):
    element=n.find(tag)
    if element is None:element=E.SubElement(n,tag)
    element.attrib={k:f'{v:.9g}' for k,v in zip('xyz',p)}
def scalar(n,tag,v):
    element=n.find(tag)
    if element is None:element=E.SubElement(n,tag)
    element.set('value',str(v))
for tag,p in [('BoundingBoxMin',low),('BoundingBoxMax',high),('BoundingSphereCenter',(0,0,center))]:vector(drawable,tag,p)
scalar(drawable,'BoundingSphereRadius',r)
goat=E.parse(root/'private-generated/goat-native/xml/a_c_goat_01_merge_4.ydd.xml')
body=next(n for n in goat.getroot().find('Drawables') if n.findtext('Hash').endswith('E5B2E10E'))
shader=copy.deepcopy(body.find('./ShaderGroup/Shaders/Item'))
for p in shader.findall('./Parameters/Items/Item'):
    if p.get('type')=='Texture':
        key=p.get('name');name='pals_sphere_body' if key=='diffusetex' else 'pals_sphere_normal' if key=='bumptex' else 'pals_sphere_material' if key in ('speculartex','speculartex2') else None
        if name:p.set('texture',name)
        else:p.attrib.pop('texture',None)
    if p.get('name') in ('bumpiness','specularcolor'):p.set('x','0')
shaders=drawable.find('./ShaderGroup/Shaders');shaders.clear();shaders.append(shader)
report=[]
for lod,steps,rings in [('LodHigh',32,16),('LodMed',24,12),('LodLow',16,8),('LodVlow',12,6)]:
    node=drawable.find(lod)
    if node is None:node=E.SubElement(drawable,lod)
    models=node.find('Models')
    if models is None:models=E.SubElement(node,'Models')
    models.clear();model=E.SubElement(models,'Item');scalar(model,'Flags',1);scalar(model,'HasSkin','True')
    vector(model,'BoundingBoxMin',low);vector(model,'BoundingBoxMax',high)
    geometries=E.SubElement(model,'Geometries');geom=E.SubElement(geometries,'Item')
    scalar(geom,'ShaderID',0);scalar(geom,'BonesCount',4);E.SubElement(geom,'Name').text=shader.findtext('Name')
    vector(geom,'BoundingBoxMin',low);vector(geom,'BoundingBoxMax',high)
    layout=E.SubElement(geom,'VertexLayout');E.SubElement(layout,'Semantics').text='PNXWWIICCTT'
    E.SubElement(layout,'Formats').text='39955665522';scalar(layout,'NonInterleaved','True')
    rows=[];indices=[]
    for j in range(rings+1):
        v=j/rings;phi=math.pi*v
        for i in range(steps+1):
            u=i/steps;theta=2*math.pi*u
            n=(math.sin(phi)*math.cos(theta),math.sin(phi)*math.sin(theta),math.cos(phi))
            p=(r*n[0],r*n[1],center+r*n[2]);t=(-math.sin(theta),math.cos(theta),0,1)
            fmt=lambda vals:' '.join(f'{x:.9g}' for x in vals)
            rows.append('\t'.join([fmt(p),fmt((*n,0)),fmt(t),'255 0 0 0','0 0 0 0','0 0 0 0','0 0 0 0','255 255 255 255','0 0 0 0',fmt((u,v)),fmt((u,v))]))
    for j in range(rings):
        for i in range(steps):
            a=j*(steps+1)+i;b=a+steps+1
            if j>0:indices.extend((a,b,a+1))
            if j<rings-1:indices.extend((a+1,b,b+1))
    E.SubElement(geom,'Vertices').text='\n'+'\n'.join(rows)+'\n'
    E.SubElement(geom,'Indices').text=' '.join(map(str,indices))
    report.append({'lod':lod,'vertices':len(rows),'triangles':len(indices)//3})
    scalar(drawable,{'LodHigh':'FlagsHigh','LodMed':'FlagsMed','LodLow':'FlagsLow','LodVlow':'FlagsVlow'}[lod],1)
    scalar(drawable,{'LodHigh':'LodDistHigh','LodMed':'LodDistMed','LodLow':'LodDistLow','LodVlow':'LodDistVlow'}[lod],9998)
bounds=drawable.find('Bounds')
for tag,p in [('BoxMin',low),('BoxMax',high),('BoxCenter',(0,0,center)),('SphereCenter',(0,0,center))]:vector(bounds,tag,p)
scalar(bounds,'SphereRadius',r);scalar(bounds,'Mass',4/3*math.pi*r**3);vector(bounds,'Inertia',(.4*r*r,)*3)
children=bounds.find('Bounds');prototype=copy.deepcopy(children[0]);children.clear();prototype.set('type','Sphere')
for tag,p in [('BoxMin',(-r,)*3),('BoxMax',(r,)*3)]:vector(prototype,tag,p)
scalar(prototype,'SphereRadius',r);scalar(prototype,'Margin',.0025);scalar(prototype,'Mass',4/3*math.pi*r**3)
vector(prototype,'Inertia',(.4*r*r,)*3)
prototype.find('Transform').text=f'\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 {center} 1\n'
children.append(prototype)
E.indent(tree,space=' ');tree.write(out/'w_pals_sphere01.ydr.xml',encoding='utf8',xml_declaration=True)
textures=E.Element('RDR2TextureDictionary',version='1');items=E.SubElement(textures,'Textures')
folder=out/'w_pals_sphere01';folder.mkdir(exist_ok=True);inputs=[]
for name,width,height in [('pals_sphere_body',512,256),('pals_sphere_normal',4,4),('pals_sphere_material',4,4)]:
    data=bytearray()
    for y in range(height):
        for x in range(width):
            u=(x+.5)/width;v=(y+.5)/height
            if name.endswith('normal'):color=(128,128,255,255)
            elif name.endswith('material'):color=(35,95,0,255)
            else:
                color=(28,112,225,255) if v<.5 else (223,229,234,255)
                if abs(v-.5)<.033:color=(22,28,37,255)
                if ((u-.25)/.035)**2+((v-.5)/.07)**2<1:color=(225,173,54,255)
            data.extend(color)
    raw=folder/(name+'.rgba');raw.write_bytes(data)
    inputs.append({'raw':str(raw),'output':str(folder/(name+'.dds')),'width':width,'height':height})
    item=E.SubElement(items,'Item');E.SubElement(item,'Name').text=name;scalar(item,'Flags',402685954)
texture_tree=E.ElementTree(textures);E.indent(texture_tree,space=' ');texture_tree.write(out/'w_pals_sphere01.ytd.xml',encoding='utf8',xml_declaration=True)
(out/'compression-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
(out/'model-build.json').write_text(json.dumps({'radius':r,'lods':report,'collision':'sphere','in_game_verified':False},indent=2)+'\n')
# A small original crystal is the collectible Paldium representation.
crystal=copy.deepcopy(drawable);crystal.find('Name').text='w_pals_paldium01.#dr'
points=[(0,0,.16),(0,0,0),(.04,0,.07),(0,.04,.07),(-.04,0,.07),(0,-.04,.07)]
faces=[(0,2,3),(0,3,4),(0,4,5),(0,5,2),(1,3,2),(1,4,3),(1,5,4),(1,2,5)]
for p in crystal.findall('./ShaderGroup/Shaders/Item/Parameters/Items/Item'):
    if p.get('texture')=='pals_sphere_body':p.set('texture','pals_paldium_body')
for lod in ('LodHigh','LodMed','LodLow','LodVlow'):
    model=crystal.find(f'./{lod}/Models/Item');geom=model.find('./Geometries/Item');rows=[]
    for face in faces:
        a,b,c=[points[i] for i in face];ab=[b[i]-a[i] for i in range(3)];ac=[c[i]-a[i] for i in range(3)]
        normal=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]);length=math.sqrt(sum(v*v for v in normal));normal=tuple(v/length for v in normal)
        for p in (a,b,c):
            fmt=lambda values:' '.join(f'{v:.9g}' for v in values)
            rows.append('\t'.join([fmt(p),fmt((*normal,0)),'1 0 0 1','255 0 0 0','0 0 0 0','0 0 0 0','0 0 0 0','255 255 255 255','0 0 0 0','0.5 0.5','0.5 0.5']))
    geom.find('Vertices').text='\n'+'\n'.join(rows)+'\n';geom.find('Indices').text=' '.join(map(str,range(len(rows))))
    for n in (model,geom):vector(n,'BoundingBoxMin',(-.04,-.04,0));vector(n,'BoundingBoxMax',(.04,.04,.16))
vector(crystal,'BoundingBoxMin',(-.04,-.04,0));vector(crystal,'BoundingBoxMax',(.04,.04,.16));vector(crystal,'BoundingSphereCenter',(0,0,.08));scalar(crystal,'BoundingSphereRadius',.09)
crystal_tree=E.ElementTree(crystal);E.indent(crystal_tree,space=' ');crystal_tree.write(out/'w_pals_paldium01.ydr.xml',encoding='utf8',xml_declaration=True)
crystal_folder=out/'w_pals_paldium01';crystal_folder.mkdir(exist_ok=True)
crystal_tex=E.Element('RDR2TextureDictionary',version='1');crystal_items=E.SubElement(crystal_tex,'Textures')
for name,color in [('pals_paldium_body',(35,160,245,255)),('pals_sphere_normal',(128,128,255,255)),('pals_sphere_material',(35,95,0,255))]:
    raw=crystal_folder/(name+'.rgba');raw.write_bytes(bytes(color)*16)
    inputs.append({'raw':str(raw),'output':str(crystal_folder/(name+'.dds')),'width':4,'height':4})
    item=E.SubElement(crystal_items,'Item');E.SubElement(item,'Name').text=name;scalar(item,'Flags',402685954)
crystal_tex_tree=E.ElementTree(crystal_tex);E.indent(crystal_tex_tree,space=' ');crystal_tex_tree.write(out/'w_pals_paldium01.ytd.xml',encoding='utf8',xml_declaration=True)
# Original inventory/wheel icon; no extracted UI texture is copied.
ui_folder=out/'pals_sphere_ui';ui_folder.mkdir(exist_ok=True)
ui_tex=E.Element('RDR2TextureDictionary',version='1');ui_items=E.SubElement(ui_tex,'Textures')
image=bytearray();size=128
for y in range(size):
    for x in range(size):
        dx=(x+.5-size/2)/(size*.45);dy=(y+.5-size/2)/(size*.45)
        if dx*dx+dy*dy>1:color=(0,0,0,0)
        else:
            color=(45,135,235,255) if dy<0 else (235,239,244,255)
            if abs(dy)<.08:color=(25,30,40,255)
            if dx*dx+dy*dy<.045:color=(231,183,59,255)
        image.extend(color)
raw=ui_folder/'sphere_icon.rgba';raw.write_bytes(image)
for name in ('weapon_thrown_pal_sphere','ammo_pal_sphere'):
    inputs.append({'raw':str(raw),'output':str(ui_folder/(name+'.dds')),'width':size,'height':size})
    item=E.SubElement(ui_items,'Item');E.SubElement(item,'Name').text=name;scalar(item,'Flags',402685954)
ui_tree=E.ElementTree(ui_tex);E.indent(ui_tree,space=' ');ui_tree.write(out/'pals_sphere_ui.ytd.xml',encoding='utf8',xml_declaration=True)
(out/'compression-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
# Register only the two new locally generated model names.
archetypes=E.parse(root/'private-generated/weapon-wheel-research/common_0.rpf/packs/base/data/weaponarchetypes.meta')
entries=archetypes.getroot().find('InitDatas');template=next(n for n in entries if n.findtext('modelName').lower()=='w_melee_knife05')
for name in ('w_pals_sphere01','w_pals_paldium01'):
    assert not any(n.findtext('modelName','').lower()==name for n in entries)
    item=copy.deepcopy(template);item.find('modelName').text=name;item.find('txdName').text=name;entries.append(item)
E.indent(archetypes,space=' ');archetypes.write(out/'weaponarchetypes.meta',encoding='utf8',xml_declaration=True)
(out/'sphere_labels.gxt2').write_text('PALS_SPHERE_NAME = Pal Sphere\nPALS_SPHERE_DESC = A sphere for capturing Pals.\n',encoding='utf8')
print('Prepared original sphere mesh/texture inputs outside repository:',out)
