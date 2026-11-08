"""Synthetic format fixtures only; no game data or native schema is bundled."""
import base64
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'experiments'))
from prepare_sphere_metadata import Editor,joaat,checksum

class SphereMetadataTests(unittest.TestCase):
    def fixture(self,folder):
        tid=0x12345678
        data=bytearray(b'PSIN'+bytes(4)+b'\x70'*8+bytes(24))
        struct.pack_into('>I',data,4,len(data))
        struct.pack_into('>IfII',data,16,joaat('weapon_thrown_throwing_knives'),55.0,
                         joaat('pickup_test'),joaat('ammo_throwing_knives'))
        pmap=bytearray(b'PMAP'+struct.pack('>II',40,1)+bytes(4)+struct.pack('>H',1)+b'\x70'*6)
        pmap.extend(struct.pack('>IIII',tid,16,0,24))
        chks=bytearray(b'CHKS'+struct.pack('>I',20)+bytes(8)+b'yppp')
        raw=data+pmap+chks
        value=checksum(raw);struct.pack_into('>II',raw,len(raw)-12,len(raw),value)
        path=folder/'synthetic.ymt';path.write_bytes(raw)
        members=[{'NameHash':joaat('Name',False),'Type':11,'SubType':7,'Offset':0,'ReferenceKey':0},
                 {'NameHash':joaat('Damage',False),'Type':7,'SubType':0,'Offset':4,'ReferenceKey':0},
                 {'NameHash':joaat('PickupHash',False),'Type':11,'SubType':7,'Offset':8,'ReferenceKey':0},
                 {'NameHash':joaat('AmmoInfo',False),'Type':11,'SubType':7,'Offset':12,'ReferenceKey':0}]
        schema={'Data':base64.b64encode(data).decode(),'RootId':1,
                'Blocks':[{'NameHash':tid,'Offset':16,'Length':24}],
                'Structs':{str(tid):{'NameHash':tid,'Size':24,'Members':members}}}
        Path(str(path)+'.schema.json').write_text(json.dumps(schema))
        return path,tid

    def test_pointer_boundaries_and_known_encoding(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('RDR2PAL_TEST_TMP')) as tmp:
            path,_=self.fixture(Path(tmp));editor=Editor(path)
            for block in (0,1,13,4095):
                for offset in (0,1,4095,4096,5544,0x7fffffff):
                    editor.packpointer(16,block,offset)
                    self.assertEqual(editor.pointer(16),(block,offset))
            editor.packpointer(16,10,5544)
            self.assertEqual(bytes(editor.data[16:24]),bytes.fromhex('5a80000a01000000'))
            with self.assertRaises(AssertionError):editor.packpointer(16,4096,0)
            with self.assertRaises(AssertionError):editor.packpointer(16,1,0x80000000)

    def test_clone_isolated_and_native_checksum_unchanged(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('RDR2PAL_TEST_TMP')) as tmp:
            folder=Path(tmp);path,tid=self.fixture(folder)
            before=path.read_bytes();editor=Editor(path)
            clone=editor.clone(1,0);position=editor.blocks[clone-1]['Offset']
            self.assertEqual(editor.data[16:40],before[16:40])
            self.assertEqual(struct.unpack_from('>IfII',editor.data,position),
                 (joaat('weapon_thrown_pal_sphere'),0.0,0,joaat('ammo_pal_sphere')))
            out=folder/'candidate.ymt';editor.save(out)
            result=out.read_bytes()
            self.assertEqual(path.read_bytes(),before)
            self.assertEqual(result[16:40],before[16:40])
            self.assertEqual(struct.unpack_from('>I',result,len(result)-12)[0],len(result))
            # Independent reference arithmetic for the signed-byte checksum.
            stored=struct.unpack_from('>I',result,len(result)-8)[0]
            checked=bytearray(result);checked[-12:-4]=bytes(8)
            value=0x3fac7125
            for byte in checked:
                value=(value+(byte if byte<128 else byte-256))&0xffffffff
                value=(value*1025)&0xffffffff
                value^=value>>6
            value=(value*9)&0xffffffff;value^=value>>11
            self.assertEqual(stored,(value*32769)&0xffffffff)

    def test_corrupt_checksum_cannot_produce_output(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('RDR2PAL_TEST_TMP')) as tmp:
            folder=Path(tmp);path,_=self.fixture(folder)
            raw=bytearray(path.read_bytes());raw[-1]^=1;path.write_bytes(raw)
            editor=Editor(path);out=folder/'candidate.ymt'
            with self.assertRaisesRegex(ValueError,'checksum convention unverified'):editor.save(out)
            self.assertFalse(out.exists())

if __name__=='__main__':unittest.main()
