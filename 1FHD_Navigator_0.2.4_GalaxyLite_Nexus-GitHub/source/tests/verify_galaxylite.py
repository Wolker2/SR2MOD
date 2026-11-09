#!/usr/bin/env python3
"""Standalone source/ZIP static QA; game UI rendering needs a Windows playtest."""
from pathlib import Path
import json,sys,re,zipfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT));sys.path.insert(0,str(ROOT/'src/lib'))
from dat_codec import encode,decode
import build
files=build.build()
for filename,format in [('CFG/Main.dat','blockpar'),('CFG/CacheData.dat','cachedata'),('CFG/Rus/Lang.dat','blockpar')]:
 assert encode(decode(files[filename],format))==files[filename],filename
source=(ROOT/'src/Navigator.code').read_text()
main=files['CFG/Main.dat']
assert "InterfaceState('Galaxy','FHDNavigatorRouteFrame',0);" in source
assert "InterfaceState('Galaxy','FHDNavigatorRouteFrame',1);" in source
assert "InterfaceText('Galaxy','FHDNavigatorRouteInfo'" in source
assert "InterfaceText('Galaxy','FHDQuestRouteInfo',text);" not in source
assert "InterfaceText('Galaxy','FHDNavigatorRouteInfo',text);" in source
assert "InterfaceState('Galaxy','FHDNavigatorRouteInfo',0);" not in source
assert "int frame=UICheckElement('Galaxy','FHDQuestRouteFrame','SizeX');" in source
assert "signature=signature+':';signature=signature+NavSharedRoute();" not in source
assert 'NavCalculate(int origin,str route)' in source
assert "NavShow('Навигатор: выберите маршрут')" in source
assert "if(label=='')" in source
assert "NavCalculate(cachedOrigin,cachedRoute)" in source
assert "NavShow('Навигатор: не удалось распознать систему')" in source
assert 'ImportedFunction(' not in source and 'SetActive' not in source
names=[]
def collect(root):
 for e in root['entries']:
  if e['kind']=='parameter' and e['name']=='Name':names.append(e['value'])
  if e['kind']=='block':collect(e['value'])
collect(json.loads((ROOT/'src/Main.json').read_text())['root'])
assert len(set(names))==len(names)
assert {'FHDNavigatorRouteDock','FHDNavigatorRouteFrame','FHDNavigatorRouteInfo'}.issubset(names)
assert not any(n.startswith('FHDQuestRoute') for n in names)
assert 'Dependence=AModGalaxyLite' in files['ModuleInfo.txt'].decode('cp1251')
assert not any(f.lower().endswith(('.dll','.exe')) for f in files)
print('PASS: native DAT, unique Navigator UI, independent Navigator line, QuestMarker frame reuse, no DLL/EXE hooks, GalaxyLite dependence')
