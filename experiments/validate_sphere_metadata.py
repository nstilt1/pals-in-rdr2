import json,xml.etree.ElementTree as E
from pathlib import Path
from prepare_sphere_metadata import joaat
from local_workspace import local_workspace
if not __debug__:raise RuntimeError('Run without Python optimization; validation assertions are required.')
root=local_workspace()/'private-generated/weapon-wheel-research'
old=E.parse(root/'raw/update_4.rpf/x64/packs/base/data/ai/weapons.ymt.reexport.xml').getroot()
new=E.parse(root/'weapons-sphere-candidate.ymt.reexport.xml').getroot()
def normalized(n):return n.tag,tuple(sorted(n.attrib.items())),(n.text or '').strip(),tuple(normalized(c) for c in n)
targets={f'hash_{joaat(n):08X}' for n in ('weapon_thrown_pal_sphere','ammo_pal_sphere')}
name_tag=f'hash_{joaat("Name",False):08X}'
parents={c:p for p in new.iter() for c in p}
added=[]
for node in list(new.iter()):
    if node.findtext(name_tag) in targets:
        added.append(node);parents[node].remove(node)
assert len(added)==2,[(n.tag,(n.text or '').strip()) for n in new.iter() if n.tag==name_tag][-12:]
slot_name=f'hash_{joaat("slot_thrown_pal_sphere"):08X}'
slots=[n for n in new.iter('Item') if len(n)==2 and any((c.text or '').strip()==slot_name for c in n)]
assert len(slots)==1
parents[slots[0]].remove(slots[0])
assert normalized(old)==normalized(new),'native definitions changed outside additive items'
weapon=next(n for n in added if n.findtext(name_tag)==f'hash_{joaat("weapon_thrown_pal_sphere"):08X}')
damage_tag=f'hash_{joaat("Damage",False):08X}'
damage=[n for n in weapon.iter(damage_tag)]
assert damage and all(float(n.attrib['value'])==0 for n in damage)
ammo_tag=f'hash_{joaat("AmmoInfo",False):08X}'
ammo=[n for n in weapon.iter(ammo_tag)]
assert len(ammo)==1 and ammo[0].text==f'hash_{joaat("ammo_pal_sphere"):08X}',[n.text for n in ammo]
pickup_tag=f'hash_{joaat("PickupHash",False):08X}'
assert all((n.text or '').strip() in ('','hash_00000000') for a in added for n in a.iter(pickup_tag))
assert not any((n.text or '').strip()==f'hash_{joaat("reward_ammo_throwing_knives"):08X}' for a in added for n in a.iter())
report={'native_definitions_preserved':True,'new_entries':2,'sphere_damage_fields_zero':len(damage),
        'sphere_ammo_modes':len(ammo),'native_pickup_rewards_removed':True,'dedicated_weapon_slot':True,'in_game_verified':False}
old_catalog=E.parse(root/'raw/update_4.rpf/x64/data/itemdatabase/catalog_sp.rpf/catalog_sp.ymt.reexport.xml').getroot()
new_catalog=E.parse(root/'catalog-sphere-candidate.ymt.reexport.xml').getroot()
parents={c:p for p in new_catalog.iter() for c in p}
entries=[n for n in new_catalog.iter() if n.attrib.get('key') in targets]
assert len(entries)==2,len(entries)
for entry in entries:parents[entry].remove(entry)
assert normalized(old_catalog)==normalized(new_catalog),'existing native catalog changed'
report.update(native_catalog_preserved=True,new_catalog_items=2)
(root/'throwable-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
