import sys
sys.dont_write_bytecode = True
from local_workspace import local_workspace
"""Repair hidden-component buffers in the private generated model.
Original project code only. User-supplied native data stays outside the repository.
"""
from pathlib import Path
import copy,json,shutil,xml.etree.ElementTree as E
R=local_workspace();P=R/'private-generated';old=P/'lamball-shared-v011';out=P/'lamball-shared-v012';out.mkdir(exist_ok=True)
tree=E.parse(old/'a_c_goat_01_merge_4.ydd.xml');native=E.parse(P/'goat-native/xml/a_c_goat_01_merge_4.ydd.xml')
by_hash={d.findtext('Hash'):d for d in native.getroot().find('Drawables')}
body=('0xE5B2E10E','0x4D67172B','0x58A83221','0x8E962B04')
report=[]
for d in tree.getroot().find('Drawables'):
 if d.findtext('Hash').endswith(body):continue
 source=by_hash[d.findtext('Hash')]
 for lod in list(d):
  if lod.tag not in ('LodHigh','LodMed','LodLow','LodVlow'):continue
  original=source.find(lod.tag);replacement=copy.deepcopy(original)
  for geom in replacement.findall('.//Geometries/Item'):
   rows=geom.findtext('Vertices').strip().splitlines();first=rows[0].split('\t');first[0]='0 0 0'
   # Keep the original vertex/index buffer lengths for external native facial
   # data. Identical positions and influences hide triangles without resizing.
   geom.find('Vertices').text='\n'+'\n'.join(['\t'.join(first)]*len(rows))+'\n'
   report.append({'component':d.findtext('Hash'),'lod':lod.tag,'vertices':len(rows),'indices':len(geom.findtext('Indices').split())})
  i=list(d).index(lod);d.remove(lod);d.insert(i,replacement)
E.indent(tree,space=' ');tree.write(out/'a_c_goat_01_merge_4.ydd.xml',encoding='utf-8',xml_declaration=True)
(out/'repair-report.json').write_text(json.dumps(report,indent=2));print('Repaired',len(report),'hidden geometry buffers')

shutil.copytree(P/'goat-native/xml/a_c_goat_01_merge_4',out/'a_c_goat_01_merge_4',dirs_exist_ok=True)

