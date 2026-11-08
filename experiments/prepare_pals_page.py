"""Add an original Pals pane to a locally extracted native wheel host.

Game-derived input/output stays private. Navigation and rendering require live QA.
"""
from pathlib import Path
import copy,struct
from rbf_records import Node,read,write,walk,text,child
from local_workspace import local_workspace

def group(name,children=(),attrs=()):
    attributes=[Node(k,96,struct.pack('<H',len(v.encode()))+v.encode()) for k,v in attrs]
    return Node(name,0,bytes(4)+struct.pack('<H',len(attributes)),attributes+list(children))
def value(name,content):return group(name,[Node('',253,content.encode()+b'\0')])
def boolean(name,on):return Node(name,0,bytes(4)+struct.pack('<H',1),[Node('value',32 if on else 48)])
def number(name,n):return Node(name,0,bytes(4)+struct.pack('<H',1),[Node('value',64,struct.pack('<f',n))])
def binding(member,path,path_type='PATH_ABSOLUTE',converter=None):
    items=[value('MemberName',member),group('Setup',[value('Path',path),value('PathType',path_type)])]
    if converter:items.append(group('Converter',[value('ID',converter)],[('type','rage__fwuiValueConversionFromRegistry')]))
    return group('Item',items)
def layout(width,height,top=0):
    return group('LayoutParams',[group('params',[
        value('horizontalAlignment','HALIGN_CENTER'),value('verticalAlignment','VALIGN_CENTER'),
        value('widthContentMode','CONTENT_MODE_RAW'),value('heightContentMode','CONTENT_MODE_RAW'),
        number('width',width),number('height',height),number('topMargin',top)])],[('type','rage__LayoutParamInstanceAsset')])
def label(name,path,width=700,top=0,style='RADIAL_MENU_ITEM_NAME'):
    return group('Item',[value('Name',name),boolean('UseLayoutEngine',True),layout(width,42,top),
        value('Style',style),group('MemberBindings',[binding('RawText',path)])],[('type','UIText')])
def build(tree):
    tree=copy.deepcopy(tree)
    assert not any(text(child(n,'Name'))=='PAN_PalsMenu' for n in walk(tree))
    host=next(n for n in walk(tree) if text(child(n,'Name'))=='PAN_WheelMenuHost')
    header=next(n for n in walk(tree) if text(child(n,'Name'))=='SP_TabHeader')
    pane=group('Item',[
        value('Name','PAN_PalsMenu'),boolean('UseLayoutEngine',True),
        group('BehaviorSet',[group('Behaviors',[
            group('Item',[value('Name','focusable'),boolean('SetDefaultFocusOnChildren',False)], [('type','uiFocusableBehavior')]),
            group('Item',[value('EventChannel','PALS_QUICK_SELECT'),value('HashParameter','PALS_WHEEL'),
                boolean('DispatchUnfocusEvent',True)],[('type','uiFocusedScriptEventBehavior')])])]),
        group('MemberBindings',[
            binding('Visible','pals_ui.enabled'),
            binding('Style','^.focusable.HasFocus','PATH_LOCAL','hud/quick_select/wheel_focus')]),
        group('Children',[
            group('Item',[value('Name','IMG_PalsBackground'),boolean('UseLayoutEngine',True),layout(780,780),
                value('Style','HUD_BG'),value('TextureDictionary','hud_quick_select'),value('Texture','WHEEL_BG_1B')],[('type','UIImage')]),
            label('TXT_PalsTitle','pals_ui.title',top=-140,style='RADIAL_MENU_SLOT_NAME'),
            label('TXT_PalsSelected','pals_ui.selectedName',top=-50),
            label('TXT_PalsStatus','pals_ui.status',top=40,style='RADIAL_MENU_SLOT_NAME'),
            label('TXT_PalsControls','pals_ui.controls',top=140,style='RADIAL_MENU_SLOT_NAME')])
    ],[('type','UIPanel')])
    children=child(host,'Children')
    if children is None:children=group('Children');host.children.append(children)
    children.children.append(pane)
    tabs=child(header,'Instancing')
    template=next(n for n in tabs.children if any(text(c)=='SATCHEL_HORSE_ITEMS' for c in walk(n) if c.name=='Value'))
    tab=copy.deepcopy(template)
    for n in walk(tab):
        if n.kind==253:
            content=n.data.rstrip(b'\0').decode('utf8')
            if content=='SATCHEL_HORSE_ITEMS':n.data=b'PALS_PAGE_LABEL\0'
            elif 'PAN_SatchelHorseItemMenu' in content:n.data=content.replace('PAN_SatchelHorseItemMenu','PAN_PalsMenu').encode()+b'\0'
        if n.name=='pathElements':n.children=[value('Item','pals_ui')]
    tabs.children.append(tab)
    return tree

if __name__=='__main__':
    if not __debug__:raise RuntimeError('Run without optimization.')
    root=local_workspace()/'private-generated/weapon-wheel-research'
    source=root/'raw/update_4.rpf/x64/data/ui/screens.rpf/hud/quick_select/wheel_menu_host.ymt'
    original=source.read_bytes();tree=read(original)
    assert write(tree)==original,'native RBF roundtrip must be lossless'
    candidate=build(tree)
    out=root/'pals-page-v060';out.mkdir(exist_ok=True)
    (out/'wheel_menu_host.ymt').write_bytes(write(candidate))
    (out/'pals_labels.gxt2').write_text('PALS_PAGE_LABEL = PALS\n',encoding='utf8')
    assert read(write(candidate))==candidate
    print('Prepared private native Pals pane candidate; live navigation/rendering not verified.')
