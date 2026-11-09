from pathlib import Path
import sys,json,zipfile,copy,hashlib
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'src/lib'))
from dat_codec import encode
from gi_codec import encode as gi_encode

def param(k,v):return {'kind':'parameter','name':k,'value':v}
def block(k,es):return {'kind':'block','name':k,'value':{'entries':es,'sorted':False}}
def data(es,fmt='blockpar'):return {'format':fmt,'seed':1369621460,'root':{'entries':es,'sorted':False}}
def code(text):return [param(f'{i:03}',line.strip()) for i,line in enumerate(text.splitlines()) if line.strip()]
CALL="ExecuteCodeFromString(GenerateCodeStringFromBlock('Script.FHDNavigator.Update'));"
def config():
 # The GalaxyLite frame is a 1200x897 child of MainPanel and it is centered
 # by native game code. NEVER overwrite its Size.
 # Use distinct Navigator widget names, as reusing QuestMarker's
 # panel path/names creates two frame panels in the actual game GUI merge.
 frame=block('Image',[param('Active','True'),param('Image','GI,Bm.FHDNavigator.RouteFrame'),param('MouseBlocking','False'),param('MouseBlockingTest','False'),param('Name','FHDNavigatorRouteFrame'),param('Pos','0,0,0'),param('Size','758,47')])
 label=block('Label',[param('Active','True'),param('AlignX','Center'),param('AlignY','CenterEx'),param('Font','Font.2SmallBold'),param('Help','Help.FHDNavigatorRoute'),param('MouseBlocking','False'),param('MouseBlockingTest','False'),param('Name','FHDNavigatorRouteInfo'),param('Pos','89,16,-1'),param('Size','450,24'),param('TextColor','0,255,0'),param('WordWrap','True')])
 # In GalaxyLite this is relative to Galaxy.MainPanel, not to the enlarged
 # Galaxy.Panel.Panel (at -88,-87) which would otherwise double-offset it.
 dock=block('FHDNavigatorRouteOverlay',[block('Panel',[param('Active','True'),param('Name','FHDNavigatorRouteDock'),param('Pos','132,861,-6'),param('Size','758,47'),param('PosAutoCorrection','True'),param('PosAutoCorrectionXCoef','0.5'),param('PosAutoCorrectionYCoef','0.5'),frame,label])])
 state=block('FHDNavigatorStateStore',[block('Label',[param('Name','FHDNavigatorState'),param('Active','False'),param('Font','Font.2SmallBold'),param('Pos','0,0,0'),param('Size','1,1'),param('MouseBlocking','False'),param('MouseBlockingTest','False')])])
 # The native Map is inside a 24,30 parent on GalaxyLite, with 1150x786 size.
 # Keep input observer as Map's SIBLING: the native OnClose removes Map's
 # children. OnClick is not blocked by this transparent observer.
 observer=block('FHDNavigatorInput',[block('GraphButton',[param('Name','FHDNavigatorMapObserver'),param('Active','True'),param('Kind','Normal'),param('KindHit','Rect'),param('Pos','0,0,-50'),param('Size','1150,786'),param('MouseBlocking','False'),param('MouseBlockingTest','False'),block('OnPressCode',[param('990',CALL)]),block('OnMouseEnterCode',[param('990',CALL)])])])
 map_patch=block('Panel',[block('Panel',[block('OnMouseEnterCode',[param('990',CALL)])]),observer])
 buttons=[]
 for index in range(5):buttons.append(block('GraphButton',[block('OnPressCode',[param('990',CALL)])] if index in (3,4) else []))
 # No 810,704 Size: would overwrite GalaxyLite's native 1200,897 panel.
 frame_patch=block('Panel',[map_patch,*buttons,state])
 init="InterfaceText('Galaxy','FHDNavigatorState','');"
 main=data([block('BV',[block(event,[block('FHDNavigatorReset',[param('000',init)])]) for event in ['OnLoad','OnStart']]),block('ML',[block('Galaxy',[block('Panel',[dock,block('Panel',[]),frame_patch,block('OnKey',[param('990',CALL)])])])])])
 cache=data([block('Bm',[block('FHDNavigator',[param('RouteFrame',r'Mods\FHD_MODS\FHD_Navigator\Data\Image\RouteFrame.gi')])])],'cachedata')
 helptext='Примерное время до последней системы маршрута: гиперпереходы, полёт внутри систем и необходимые дозаправки. Каждая дозаправка в космосе: подлёт, день посадки, день взлёта и вылет к переходу. Старт с планеты или станции: ещё один день взлёта. Заправка до полного бака. Полёт по прямой, по текущим координатам планет и станций; бои и посадка в конечной системе не включены.'
 lang=data([block('Help',[param('FHDNavigatorRoute',helptext)]),block('Script',[block('FHDNavigator',[block('Update',code((ROOT/'src/Navigator.code').read_text()))])])])
 return {'Main':main,'CacheData':cache,'Lang':lang}

def build():
 files={}
 for name,tree in config().items():
  (ROOT/'src'/f'{name}.json').write_text(json.dumps(tree,ensure_ascii=False,indent=2)+'\n')
  files['CFG/'+({'Main':'Main.dat','CacheData':'CacheData.dat','Lang':'Rus/Lang.dat'}[name])]=encode(tree)
 files['Data/Image/RouteFrame.gi']=gi_encode(json.loads((ROOT/'src/RouteFrame.json').read_text()))
 files['ModuleInfo.txt']=(ROOT/'src/ModuleInfo.txt').read_text().encode('cp1251')
 files['README_RU.txt']=(ROOT/'README_RU.md').read_text().replace('\n','\r\n').encode('utf-8-sig')
 files['SHA256.json']=json.dumps({k:hashlib.sha256(v).hexdigest() for k,v in files.items()},indent=2).encode()
 return files

def package(files):
 out=ROOT/'dist/FHD_Navigator_0.2.4_GALAXYLITE_TEXT_RESTORE_TEST.zip';out.parent.mkdir(exist_ok=True)
 with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for name,b in sorted(files.items()):
   i=zipfile.ZipInfo('Mods/FHD_MODS/FHD_Navigator/'+name,(2026,10,30,0,0,0));i.external_attr=0o644<<16
   z.writestr(i,b,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
 print(out)
 return out
if __name__=='__main__':package(build())
