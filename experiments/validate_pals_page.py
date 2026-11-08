"""Compare an independently re-exported local Pals UI against its native input."""
import json,xml.etree.ElementTree as E
from local_workspace import local_workspace
if not __debug__:raise RuntimeError('Run without optimization.')
root=local_workspace()/'private-generated/weapon-wheel-research'
old=E.parse(root/'update_4.rpf/x64/data/ui/screens.rpf/hud/quick_select/wheel_menu_host.ymt.xml').getroot()
new=E.parse(root/'pals-page-v060/wheel_menu_host.ymt.reexport.xml').getroot()
def canonical(n):return n.tag,tuple(sorted(n.attrib.items())),(n.text or '').strip(),tuple(canonical(c) for c in n)
parents={c:p for p in new.iter() for c in p}
for name in ('PAN_PalsMenu','PALS_PAGE_LABEL'):
    entries=[n for n in new.iter() if n.findtext('Name')==name] if name=='PAN_PalsMenu' else [
        n for n in new.iter('Item') if n.attrib.get('type')=='UITemplateInstancingItem'
        and any(v.text=='PALS_PAGE_LABEL' for v in n.iter('Value'))]
    assert len(entries)==1,name
    node=entries[0]
    if name=='PAN_PalsMenu':
        assert any(n.text=='PALS_QUICK_SELECT' for n in node.iter('EventChannel'))
        assert not any(n.attrib.get('type')=='uiFocusedCodeEventBehavior' for n in node.iter())
    parent=parents[node];parent.remove(node)
    if not len(parent) and parent.tag=='Children':parents[parent].remove(parent)
assert canonical(old)==canonical(new),'original UI data changed beyond Pals additions'
report={'native_wheel_preserved':True,'original_pane_added':True,'private_event_channel':True,'live_verified':False}
(root/'pals-page-v060/validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
