"""Offline UI merge + source assertions. No Windows runtime claim."""
from pathlib import Path
import sys,itertools,json,re
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'src/lib'),str(ROOT/'tests')]
from dat_codec import decode,encode
from overlay_reference import plain,merge,find,one,path
import build
root=Path('/mnt/data/nav_galaxylite_work')
fn=ROOT/'src/Navigator.code'
code=fn.read_text()
source_lines=code.splitlines()
assert 'function NavSharedRoute(){' in code
assert "InterfaceText('Galaxy','FHDNavigatorRouteInfo',text);" in code
assert "InterfaceState('Galaxy','FHDNavigatorRouteInfo',1);" in code
assert "InterfaceText('Galaxy','FHDQuestRouteInfo',text);" not in code
assert "InterfaceState('Galaxy','FHDNavigatorRouteInfo',0);" not in code
assert "if(label=='')" in code and "NavCalculate(cachedOrigin,cachedRoute)" in code
assert "NavShow('Навигатор: выберите маршрут')" in code
# Only own frame is changed, QuestMarker image state remains untouched.
assert "InterfaceState('Galaxy','FHDQuestRouteFrame'" not in code
files=build.build()
for n in ('CFG/Main.dat','CFG/CacheData.dat','CFG/Rus/Lang.dat'):
 assert decode(files[n], 'cachedata' if 'CacheData' in n else 'blockpar')
assert files['ModuleInfo.txt'].decode('cp1251').count('Dependence=AModGalaxyLite')==1
assert not any(n.endswith(('.dll','.exe')) for n in files)
nav=plain(decode(files['CFG/Main.dat'],'blockpar'))
gal=plain(decode((root/'extracted/AModGalaxyLite/CFG/Main.dat').read_bytes(),'blockpar'))
qmark=plain(decode((root/'qmark/FHD_QuestMarker_Preview/CFG/Main.dat').read_bytes(),'blockpar'))

def cnt(node,name):
 return int(one(node,'Name')==name)+sum(cnt(v,name) for k,v in node if isinstance(v,list))
checked=0
for layers in [(gal,nav),(nav,gal),*itertools.permutations((gal,nav,qmark))]:
 merged=layers[0]
 for layer in layers[1:]:merged=merge(merged,layer)
 galaxy=path(merged,'ML','Galaxy','Panel')
 for n in ('FHDNavigatorRouteInfo','FHDNavigatorRouteFrame','FHDNavigatorRouteDock','FHDNavigatorMapObserver','FHDNavigatorState'):
  assert cnt(galaxy,n)==1,(n,checked)
 for n in ('FHDQuestRouteInfo','FHDQuestRouteFrame','FHDQuestRouteDock'):
  assert cnt(galaxy,n)==(1 if len(layers)==3 else 0),(n,checked)
 label=find(galaxy,'FHDNavigatorRouteInfo')
 frame=find(galaxy,'FHDNavigatorRouteFrame')
 assert one(label,'Active')=='True' and one(label,'TextColor')=='0,255,0'
 assert one(frame,'Image')=='GI,Bm.FHDNavigator.RouteFrame'
 assert one(find(galaxy,'ImagePanel'),'Size')=='1200,895'
 assert one(find(galaxy,'Map'),'Size')=='1150,786'
 checked+=1
print(f'PASS: 0.2.4 regression invariants; 8 GUI merge permutations; no QuestMarker text writes or GalaxyLite size change')
