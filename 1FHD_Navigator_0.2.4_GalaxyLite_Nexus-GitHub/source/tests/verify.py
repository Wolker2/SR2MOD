"""Build/verify the script-only mod. Runtime fixtures use the supplied EXEs."""
import argparse,hashlib,itertools,json,re,struct,subprocess,sys,zipfile
from pathlib import Path,PurePosixPath
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'src/lib'),str(ROOT/'tests')]
import build
from dat_codec import decode,encode
from gi_codec import decode as gi_decode
from overlay_reference import plain,merge,find,one,path,children
p=argparse.ArgumentParser();p.add_argument('--exe-new',type=Path);p.add_argument('--exe-old',type=Path);p.add_argument('--quest-source',type=Path);a=p.parse_args()
checks=[]
def ok(text):checks.append(text);print('PASS:',text,flush=True)
files=build.build()
for name,fmt in [('CFG/Main.dat','blockpar'),('CFG/CacheData.dat','cachedata'),('CFG/Rus/Lang.dat','blockpar')]:assert encode(decode(files[name],fmt))==files[name]
gi_decode(files['Data/Image/RouteFrame.gi'])
assert hashlib.sha256(files['Data/Image/RouteFrame.gi']).hexdigest()=='73c3017a0cbb67668e37c8b698d59e8275719f87e913731e843f69b828b34bc2'
ok('DAT CRC/roundtrip and GI validity; exact original QuestMarker frame bytes')
source=(ROOT/'src/Navigator.code').read_text()
assert not re.search(r'ImportedFunction|loadlibrary|libraryfunction|ScriptLibs|0x[0-9a-fA-F]+|ReadProcessMemory|WriteProcessMemory',source)
assert 'Native/' not in '\n'.join(files) and not any(Path(n).suffix.lower() in ['.exe','.dll','.scr'] for n in files)
main=plain(decode(files['CFG/Main.dat'],'blockpar'));lang=plain(decode(files['CFG/Rus/Lang.dat'],'blockpar'))
assert not children(main,'Data')
compiled=''.join(v for k,v in path(lang,'Script','FHDNavigator','Update'))
assert re.sub(r'\s+','',compiled)==re.sub(r'\s+','',source)
ok('Runtime contains no DLL, EXE, SCR, ScriptLibs, memory operations or fixed EXE addresses; CFG embeds the tested source')
base=plain(json.loads((ROOT/'tests/vanilla_galaxy.json').read_text()));trade=plain(json.loads((ROOT/'tests/trade_overlay.json').read_text()))
overlays=[main,trade]
if a.quest_source:overlays.append(plain(json.loads((a.quest_source/'SOURCE/CFG/Main.json').read_text())))
cases=[(main,),(main,trade),(trade,main)]
if len(overlays)==3:cases+=list(itertools.permutations(overlays))
def count_named(b,name):return int(one(b,'Name')==name)+sum(count_named(v,name) for k,v in b if isinstance(v,list))
for order in cases:
 cfg=base
 for layer in order:cfg=merge(cfg,layer)
 galaxy=path(cfg,'ML','Galaxy','Panel');parent=children(galaxy,'Panel')[1]
 f=find(parent,'FHDQuestRouteFrame');label=find(parent,'FHDQuestRouteInfo');observer=find(galaxy,'FHDNavigatorMapObserver')
 assert find(find(galaxy,'Map'),'FHDNavigatorMapObserver') is None, 'Map.OnClose deletes its children'
 for name in ['FHDQuestRouteFrame','FHDQuestRouteInfo','FHDNavigatorMapObserver','FHDNavigatorState']:assert count_named(galaxy,name)==1
 assert one(f,'Pos')=='31,636,0' and one(f,'Size')=='758,47'
 assert one(label,'Pos')=='120,652,-3' and one(label,'Size')=='450,24'
 assert one(label,'Font')=='Font.2SmallBold' and one(label,'TextColor')=='0,255,0'
 assert one(parent,'Size')=='810,704'
 assert one(observer,'MouseBlocking')==one(observer,'MouseBlockingTest')=='False'
 assert one(observer,'KindHit')=='Rect' and one(observer,'Kind')=='Normal'
 assert one(find(galaxy,'FHDNavigatorState'),'Active')=='False'
 assert find(galaxy,'ImagePanel')==find(path(base,'ML','Galaxy','Panel'),'ImagePanel')
 for button in ['ButUndo','ButDel']:
  target=find(galaxy,button);native=find(path(base,'ML','Galaxy','Panel'),button)
  for key in ['Name','Pos','Size','ImageNormal','ImageDown']:assert one(target,key)==one(native,key) or (button=='ButDel' and len(overlays)==3 and overlays[2] in order and key.startswith('Image'))
  assert one(children(target,'OnPressCode')[0],'990')==build.CALL
 assert one(children(observer,'OnPressCode')[0],'990')==build.CALL
 for w,h in [(1024,768),(1920,1080),(2560,1440),(3440,1440)]:
  ox,oy=107+(w-1024)//2,22+(h-768)//2
  for n in [f,label]:
   x,y,_=map(int,one(n,'Pos').split(','));nw,nh=map(int,one(n,'Size').split(','));assert 0<=ox+x<ox+x+nw<=w and 0<=oy+y<oy+y+nh<=h
ok(f'GUI in {len(cases)} merge orders: one shared frame/label, exact original placement/font/layer, native controls retained; 4 resolutions')
if a.exe_new:
 subprocess.run([sys.executable,str(ROOT/'tests/engine_script_test.py'),'--exe',str(a.exe_new),'--run'],check=True)
 ok('Actual new-EXE compiler and interpreter: 49 synthetic route/equipment/state scenarios, independent ETA oracle, QuestMarker text ownership')
if a.exe_new and a.exe_old:
 subprocess.run([sys.executable,str(ROOT/'tests/mouse_events_test.py'),str(a.exe_new),str(a.exe_old)],check=True)
 ok('Actual old/new native UI click handlers: map callbacks retained, script queued once on release, blocked/outside clicks ignored')
 import pefile
 local=set(re.findall(r'function\s+(\w+)',source));mathfunc={'int','sin','cos','sqrt','sqr','min','max','abs','round','count','substr','findsubstr'}
 calls=set(re.findall(r'\b([A-Za-z_]\w*)\s*\(',source))-local-mathfunc-{'if','while','for','function'}
 for exe in [a.exe_new,a.exe_old]:
  image=pefile.PE(str(exe)).get_memory_mapped_image()
  for name in sorted(calls|{'t_ArtGiperJump','t_FuelTanks','t_ArtefactFuel'}):assert struct.pack('<I',len(name)*2)+name.encode('utf-16le')+b'\0\0' in image,(exe,name)
  assert b'gs_' in image or 'gs_'.encode('utf-16le') in image
 ok(f'All {len(calls)} referenced game functions, hyper-accelerator constant and gs_ marker prefix present in both supplied EXEs')
report={'mod':'FHD_Navigator','version':'0.2.1 SCRIPT NODLL TEST','checks':checks,'runtime_dlls':0,'fixed_exe_addresses':0,'exe_hash_gate':False,'windows_game_session_tested':False,'actual_flight_accuracy_tested':False,'scope':'Native game compiler/interpreter and UI dispatch run in Unicorn with synthetic Windows APIs, game data and UI objects; full game not launched.','limits':['In-system flight uses straight-line estimate, omitting star avoidance and gate angle jitter.','Tracks manual routes up to 256 waypoints. Does not infer intermediate points of a programmatic route installed by another mod.','Needs stock UICheckElement and gs_<ID> Galaxy controls. Ambiguous on-screen star coordinates are rejected.','Refueling assumes full tank, current ports/coordinates/relations; standard inhabited planets and station types 6..11. Does not predict future battles, access restrictions, port destruction, funds or moving planets.','Black goo uses current configured regeneration; custom fuel scripts, leaking tanks and cargo fuel transfers are not predicted.','Full target mod set and real Windows play remain untested.']}
(ROOT/'dist/VERIFICATION.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');files['VERIFICATION.json']=(ROOT/'dist/VERIFICATION.json').read_bytes()
target=build.package(files)
with zipfile.ZipFile(target) as z:
 assert z.testzip() is None
 names=z.namelist();assert len(names)==len(set(n.lower() for n in names))
 for n in names:assert n.startswith('Mods/FHD_MODS/FHD_Navigator/') and '..' not in PurePosixPath(n).parts
ok('Final installation ZIP: CRC, prefix, path safety, Windows case uniqueness')
print('Windows gameplay: NOT TESTED. Ready for in-game test.',flush=True)
