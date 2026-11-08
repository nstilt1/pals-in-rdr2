import copy,struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'experiments'))
from rbf_records import Node,read,write,walk,text,child
from prepare_pals_page import group,value,boolean,number,build
class PalsPageTests(unittest.TestCase):
    def test_records_and_typed_attributes(self):
        root=group('Synthetic',[value('Name','Original test'),boolean('Enabled',True),
            boolean('Disabled',False),number('Width',42),Node('Data',96,struct.pack('<H',3)+b'abc')])
        self.assertEqual(read(write(root)),root)
        self.assertEqual(child(root,'Enabled').data[-2:],b'\x01\x00')
        self.assertEqual(child(root,'Width').data[-2:],b'\x01\x00')
        self.assertEqual(text(child(root,'Name')),'Original test')
        for bad in (b'',b'RBF0',b'RBF0\xff\xff',write(root)[:-1],write(root)+b'x'):
            with self.assertRaises((AssertionError,ValueError,struct.error,IndexError)):read(bad)
    def test_additive_pane_preserves_native_placeholders(self):
        host=group('Item',[value('Name','PAN_WheelMenuHost'),group('Instancing',[value('Type','synthetic_native_wheel')])])
        horse_tab=group('Item',[value('Type','wheel_tab'),group('Properties',[
            group('Item',[value('Name','label'),value('Value','SATCHEL_HORSE_ITEMS')],[('type','SF_String')])])],
            [('type','UITemplateInstancingItem')])
        tabs=group('Item',[value('Name','SP_TabHeader'),group('Instancing',[horse_tab])])
        original=group('Synthetic',[host,tabs]);before=copy.deepcopy(original)
        candidate=build(original)
        self.assertEqual(original,before)
        self.assertEqual(read(write(candidate)),candidate)
        names=[text(child(n,'Name')) for n in walk(candidate)]
        self.assertEqual(names.count('PAN_PalsMenu'),1)
        self.assertEqual(sum(n.name=='Value' and text(n)=='PALS_PAGE_LABEL' for n in walk(candidate)),1)
        for name in ('PAN_WheelMenuHost','SP_TabHeader'):
            old=next(n for n in walk(before) if text(child(n,'Name'))==name)
            new=next(n for n in walk(candidate) if text(child(n,'Name'))==name)
            if name=='SP_TabHeader':
                self.assertEqual(child(old,'Instancing').children,child(new,'Instancing').children[:-1])
            else:self.assertEqual(child(old,'Instancing'),child(new,'Instancing'))
        self.assertIn('PALS_QUICK_SELECT',[text(n) for n in walk(candidate) if n.name=='EventChannel'])
        with self.assertRaises(AssertionError):build(candidate)
if __name__=='__main__':unittest.main()
