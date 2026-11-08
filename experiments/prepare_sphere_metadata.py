"""Original additive PSO experiment; local inputs/output only, never installs.

Requires independently exported PSO schema JSON beside each native input.
The JSON is game-derived and must remain outside the source repository.
This does not supply weapon art, localization, inventory paths or runtime hooks.
"""
import base64,copy,json,struct
from pathlib import Path
from local_workspace import local_workspace

def joaat(text,lower=True):
    value=0
    for byte in (text.lower() if lower else text).encode():
        value=(value+byte)&0xffffffff
        value=(value+(value<<10))&0xffffffff
        value^=value>>6
    value=(value+(value<<3))&0xffffffff;value^=value>>11
    return (value+(value<<15))&0xffffffff

def checksum(data):
    value=0x3fac7125
    for byte in data:
        value=(value+(byte if byte<128 else byte-256))&0xffffffff
        value=(value+(value<<10))&0xffffffff;value^=value>>6
    value=(value+(value<<3))&0xffffffff;value^=value>>11
    return (value+(value<<15))&0xffffffff

class Editor:
    def __init__(self,path):
        self.path=path;self.raw=path.read_bytes()
        self.j=json.loads(Path(str(path)+'.schema.json').read_text())
        self.data=bytearray(base64.b64decode(self.j['Data']))
        if self.raw[:4]!=b'PSIN' or struct.unpack_from('>I',self.raw,4)[0]!=len(self.data) or self.raw[:len(self.data)]!=self.data:
            raise ValueError('schema data does not match the native PSIN input')
        self.blocks=copy.deepcopy(self.j['Blocks'])
        self.original_block_count=len(self.blocks)
        for block in self.blocks:
            if block['Offset']<8 or block['Length']<0 or block['Offset']+block['Length']>len(self.data):
                raise ValueError('native block is outside PSIN')
        self.schemas={int(k):v for k,v in self.j['Structs'].items()}
        self.cache={}
        self.names={joaat('weapon_thrown_throwing_knives'):joaat('weapon_thrown_pal_sphere'),
                    joaat('ammo_throwing_knives'):joaat('ammo_pal_sphere'),
                    joaat('reward_ammo_throwing_knives'):0,
                    joaat('inventory_items'):joaat('pals_sphere_ui'),
                    joaat('multiwheel_weapons'):joaat('pals_sphere_ui'),
                    joaat('item_textures'):joaat('pals_sphere_ui'),
                    joaat('ui_ammo_throwing_knives'):joaat('ammo_pal_sphere'),
                    joaat('w_melee_knife05'):joaat('w_pals_sphere01')}
        self.zero={joaat(n,False) for n in ('Damage','Penetration','Force','ForceHitPed','ForceHitVehicle',
                  'ForceHitFlyingHeli','ProjectileForce')}
    def pointer(self,pos):
        lo=struct.unpack_from('>I',self.data,pos)[0]
        block=lo&0xfffff
        if block>4095:raise ValueError('unsupported pointer block')
        offset=(lo>>20)|(struct.unpack_from('<I',self.data,pos+4)[0]<<12)
        return block,offset
    def packpointer(self,pos,block,offset):
        assert 0<=block<=4095 and 0<=offset<0x80000000
        struct.pack_into('>I',self.data,pos,block|((offset&4095)<<20))
        struct.pack_into('<I',self.data,pos+4,offset>>12)
    def resolve(self,block,offset):
        assert 1<=block<=len(self.blocks)
        b=self.blocks[block-1];assert 0<=offset<b['Length']
        return b['Offset']+offset,b['NameHash']
    def allocate(self,tid,contents):
        if len(self.blocks)>=4095:raise ValueError('PSO block capacity')
        self.data.extend(b'\x70'*((-len(self.data))%16))
        position=len(self.data);self.data.extend(contents)
        self.blocks.append({'NameHash':tid,'Offset':position,'Length':len(contents)})
        return len(self.blocks),position
    def clone(self,block,offset,tid=None):
        source,actual=self.resolve(block,offset);tid=actual if tid is None else tid
        key=(block,offset,tid)
        if key in self.cache:return self.cache[key]
        size=self.schemas[tid]['Size']
        assert offset+size<=self.blocks[block-1]['Length']
        new,pos=self.allocate(tid,self.data[source:source+size]);self.cache[key]=new
        self.rewrite(tid,pos)
        return new
    def rewrite(self,tid,pos,depth=0):
        if depth>64:raise ValueError('nested schema limit')
        schema=self.schemas[tid]
        for member in schema['Members']:
            name,kind,sub,offset,ref=(member[k] for k in ('NameHash','Type','SubType','Offset','ReferenceKey'))
            if name==256:continue # element descriptor, not stored member
            at=pos+offset
            if kind==11 and sub in (7,8):
                old=struct.unpack_from('>I',self.data,at)[0]
                if old in self.names:struct.pack_into('>I',self.data,at,self.names[old])
                if name==joaat('PickupHash',False):struct.pack_into('>I',self.data,at,0)
                if name==joaat('Model',False):struct.pack_into('>I',self.data,at,joaat('w_pals_sphere01'))
                if name==0xD6C14496:struct.pack_into('>I',self.data,at,joaat('PALS_SPHERE_NAME'))
            elif name in self.zero:
                if kind!=7:raise ValueError('damage/force field is not a float')
                struct.pack_into('>f',self.data,at,0)
            elif kind==12:
                if sub==0:self.rewrite(ref,at,depth+1)
                elif sub in (3,4):
                    block,off=self.pointer(at)
                    if block:self.packpointer(at,self.clone(block,off,ref or None),0)
                else:raise ValueError(f'unsupported struct subtype {sub}')
            elif kind==13:
                if name==0xC9B8217D:
                    # Do not inherit the native knife's catalogue description.
                    self.packpointer(at,0,0);struct.pack_into('>HH',self.data,at+8,0,0);continue
                descriptor=schema['Members'][ref&0xffff]
                if sub==0:
                    block,off=self.pointer(at);count=struct.unpack_from('>H',self.data,at+8)[0]
                    if not count:continue
                    source,_=self.resolve(block,off)
                    if descriptor['Type']==12:
                        element_schema=self.schemas.get(descriptor['ReferenceKey'])
                        if element_schema and any(m['NameHash']==joaat('Damage',False) for m in element_schema['Members']):
                            # One basic sphere mode: no improved/poisoned native ammo modes.
                            count=1;struct.pack_into('>HH',self.data,at+8,1,1)
                        pointer=descriptor['SubType'] in (3,4)
                        elem_size=8 if pointer else self.schemas[descriptor['ReferenceKey']]['Size']
                        assert off+count*elem_size<=self.blocks[block-1]['Length']
                        new,target=self.allocate(self.blocks[block-1]['NameHash'],self.data[source:source+count*elem_size])
                        self.packpointer(at,new,0)
                        for i in range(count):
                            item=target+i*elem_size
                            if pointer:
                                child,childoff=self.pointer(item)
                                if child:self.packpointer(item,self.clone(child,childoff,descriptor['ReferenceKey'] or None),0)
                            else:self.rewrite(descriptor['ReferenceKey'],item,depth+1)
                    # Primitive arrays are immutable shared data, not modified.
                elif sub==4:
                    count=ref>>16
                    if descriptor['Type']!=12 or descriptor['SubType']!=0:raise ValueError('unsupported inline array')
                    stride=self.schemas[descriptor['ReferenceKey']]['Size']
                    for i in range(count):self.rewrite(descriptor['ReferenceKey'],at+i*stride,depth+1)
                elif sub==1:pass # fixed primitive/struct arrays handled separately if needed
                else:raise ValueError(f'unsupported array subtype {sub}')
            elif kind==16:
                if self.data[at:at+16] not in (bytes(16),b'\x01'+bytes(15)):
                    raise ValueError(f'nonempty map: {tid:08x} {name:08x}')
    def find(self,tid,name,field_name='Name'):
        namehash=joaat(name);field=next(m for m in self.schemas[tid]['Members'] if m['NameHash']==joaat(field_name,False))
        matches=[]
        for bid,block in enumerate(self.blocks,1):
            if block['NameHash']!=tid:continue
            size=self.schemas[tid]['Size']
            for offset in range(0,block['Length']-size+1,size):
                if struct.unpack_from('>I',self.data,block['Offset']+offset+field['Offset'])[0]==namehash:matches.append((bid,offset))
        if len(matches)!=1:raise ValueError(f'expected one native template: {name}, found {matches}')
        return matches[0]
    def append_root_info(self,old,new):
        root=self.blocks[self.j['RootId']-1]['Offset']
        outer=next(m for m in self.schemas[self.blocks[self.j['RootId']-1]['NameHash']]['Members'] if m['NameHash']==joaat('Infos',False))
        assert outer['SubType']==4
        changed=0
        for group in range(outer['ReferenceKey']>>16):
            header=root+outer['Offset']+group*16
            bid,off=self.pointer(header);count=struct.unpack_from('>H',self.data,header+8)[0]
            source,_=self.resolve(bid,off)
            entries=[self.pointer(source+i*8) for i in range(count)]
            if old not in entries:continue
            added,target=self.allocate(12,self.data[source:source+count*8]+b'\0'*8)
            self.packpointer(target+count*8,new,0);self.packpointer(header,added,0)
            struct.pack_into('>HH',self.data,header+8,count+1,count+1);changed+=1
        assert changed==1,changed
    def add_weapon_slot(self):
        weapon=self.find(0x04eb080e,'weapon_thrown_throwing_knives')
        source,_=self.resolve(*weapon)
        slot_member=next(m for m in self.schemas[0x04eb080e]['Members'] if m['NameHash']==joaat('Slot',False))
        old_slot=struct.unpack_from('>I',self.data,source+slot_member['Offset'])[0]
        new_slot=joaat('slot_thrown_pal_sphere');self.names[old_slot]=new_slot
        root=self.blocks[self.j['RootId']-1];root_schema=self.schemas[root['NameHash']]
        found=0
        for member in root_schema['Members']:
            if member['Type']!=12 or member['SubType']!=0:continue
            schema=self.schemas[member['ReferenceKey']]
            for array in schema['Members']:
                if array['Type']!=13 or array['SubType']!=0:continue
                desc=schema['Members'][array['ReferenceKey']&0xffff]
                if desc['Type']!=12 or desc['SubType']!=0:continue
                entry_schema=self.schemas[desc['ReferenceKey']];fields=[m for m in entry_schema['Members'] if m['NameHash']!=256]
                if len(fields)!=2:continue
                keys=[m for m in fields if m['Type']==11 and m['SubType']==7]
                orders=[m for m in fields if m['Type'] in (5,6)]
                if len(keys)!=1 or len(orders)!=1:continue
                header=root['Offset']+member['Offset']+array['Offset']
                block,off=self.pointer(header);count=struct.unpack_from('>H',self.data,header+8)[0]
                if not count:continue
                at,tid=self.resolve(block,off);size=entry_schema['Size']
                entries=[self.data[at+i*size:at+(i+1)*size] for i in range(count)]
                matches=[e for e in entries if struct.unpack_from('>I',e,keys[0]['Offset'])[0]==old_slot]
                if not matches:continue
                assert len(matches)==1
                new=bytearray(matches[0]);struct.pack_into('>I',new,keys[0]['Offset'],new_slot)
                order=max(struct.unpack_from('>I',e,orders[0]['Offset'])[0] for e in entries)+1
                struct.pack_into('>I',new,orders[0]['Offset'],order)
                newblock,_=self.allocate(tid,b''.join(entries)+new);self.packpointer(header,newblock,0)
                struct.pack_into('>HH',self.data,header+8,count+1,count+1);found+=1
        assert found==1,found
    def append_catalog_pairs(self,olds):
        root=self.blocks[self.j['RootId']-1]['Offset']
        root_schema=self.schemas[self.blocks[self.j['RootId']-1]['NameHash']]
        catalog_member=next(m for m in root_schema['Members'] if m['NameHash']==joaat('catalog',False))
        wrapper=self.schemas[catalog_member['ReferenceKey']]
        member=next(m for m in wrapper['Members'] if m['Type']==16)
        header=root+catalog_member['Offset']+member['Offset']
        block,offset=self.pointer(header+8);source,pair_type=self.resolve(block,offset)
        count=struct.unpack_from('>H',self.data,header+16)[0]
        size=self.schemas[pair_type]['Size']
        assert count*size==self.blocks[block-1]['Length']
        addition=bytearray()
        for name in olds:
            old=self.find(pair_type,name,'Key');new=self.clone(*old)
            cloned=self.blocks[new-1];addition.extend(self.data[cloned['Offset']:cloned['Offset']+size])
        new,target=self.allocate(pair_type,self.data[source:source+count*size]+addition)
        self.packpointer(header+8,new,0)
        struct.pack_into('>HH',self.data,header+16,count+len(olds),count+len(olds))
    def save(self,out):
        sections=[];offset=0
        while offset<len(self.raw):
            tag,size=struct.unpack_from('>4sI',self.raw,offset)
            assert size>=8 and offset+size<=len(self.raw)
            section=bytearray(self.raw[offset:offset+size]);offset+=size
            if tag==b'PSIN':
                section=self.data;struct.pack_into('>I',section,4,len(section))
            elif tag==b'PMAP':
                assert len(section)==24+self.original_block_count*16
                assert struct.unpack_from('>H',section,16)[0]==self.original_block_count
                # Preserve native mapping records, including their unknown bits.
                for i,b in enumerate(self.blocks[:self.original_block_count]):
                    name,block_offset,_,length=struct.unpack_from('>IIII',section,24+i*16)
                    assert (name,block_offset,length)==(b['NameHash'],b['Offset'],b['Length'])
                struct.pack_into('>H',section,16,len(self.blocks))
                for b in self.blocks[self.original_block_count:]:section.extend(struct.pack('>IIII',b['NameHash'],b['Offset'],0,b['Length']))
                struct.pack_into('>I',section,4,len(section))
            sections.append((tag,section))
        # Verify the known checksum convention against the unmodified local file.
        original=bytearray(self.raw);chks=len(original)-20
        assert original[chks:chks+4]==b'CHKS'
        expected=struct.unpack_from('>I',original,chks+12)[0]
        original[chks+8:chks+16]=b'\0'*8
        actual=checksum(original)
        if actual!=expected:raise ValueError(f'checksum convention unverified: {actual:08x} != {expected:08x}')
        result=bytearray().join(section for _,section in sections);chks=len(result)-20
        result[chks+8:chks+16]=b'\0'*8
        value=checksum(result);struct.pack_into('>II',result,chks+8,len(result),value)
        out.write_bytes(result)

if __name__=='__main__':
    if not __debug__:raise RuntimeError('Run without Python optimization; validation assertions are required.')
    root=local_workspace()/'private-generated/weapon-wheel-research'
    editor=Editor(root/'raw/update_4.rpf/x64/packs/base/data/ai/weapons.ymt')
    editor.add_weapon_slot()
    for tid,name in ((0x04eb080e,'weapon_thrown_throwing_knives'),(0x7c62e030,'ammo_throwing_knives')):
        old=editor.find(tid,name);new=editor.clone(*old);editor.append_root_info(old,new)
        print('CLONED',name,'blocks',len(editor.blocks))
    editor.save(root/'weapons-sphere-candidate.ymt')
    print('WROTE PRIVATE CANDIDATE')
    catalog=Editor(root/'raw/update_4.rpf/x64/data/itemdatabase/catalog_sp.rpf/catalog_sp.ymt')
    catalog.append_catalog_pairs(('weapon_thrown_throwing_knives','ammo_throwing_knives'))
    catalog.save(root/'catalog-sphere-candidate.ymt')
    print('WROTE PRIVATE CATALOG CANDIDATE')
