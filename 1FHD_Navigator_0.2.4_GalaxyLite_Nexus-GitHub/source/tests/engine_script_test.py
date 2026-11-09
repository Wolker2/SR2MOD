import sys,struct,json
from pathlib import Path
import pefile
from unicorn import *
from unicorn.x86_const import *
import argparse,math
parser=argparse.ArgumentParser();parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]/'src/Navigator.code');parser.add_argument('--run',action='store_true');args=parser.parse_args()
pe=pefile.PE(str(args.exe));im=pe.get_memory_mapped_image();cpu=Uc(UC_ARCH_X86,UC_MODE_32)
cpu.mem_map(0x400000,(len(im)+4095)&~4095);cpu.mem_write(0x400000,im)
cpu.mem_map(0,4096);cpu.mem_map(0x60000000,0x4000000);cpu.mem_map(0x70000000,0x200000)
def put(p,v):cpu.mem_write(p,struct.pack('<I',v&0xffffffff))
def u32(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
heap=0x60001000;sizes={};calls=[]
def alloc(n):
 global heap
 if n<=0:return 0
 n=max(16,n);p=heap;heap+=(n+15)&~15
 if heap>0x63ffffff:raise RuntimeError('heap limit')
 sizes[p]=n;return p
def wide(p):
 if not p:return ''
 n=u32(p-4)
 if n>1000000:return '<bad wide>'
 return bytes(cpu.mem_read(p,n)).decode('utf-16le',errors='replace')
def bstr(source,n):
 if not n:return 0
 p=alloc(n*2+6)+4;put(p-4,n*2)
 if source:cpu.mem_write(p,bytes(cpu.mem_read(source,n*2)))
 cpu.mem_write(p+n*2,b'\0\0');return p
def ret(value=None,pop=0):
 sp=cpu.reg_read(UC_X86_REG_ESP)
 if value is not None:cpu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
 cpu.reg_write(UC_X86_REG_EIP,u32(sp));cpu.reg_write(UC_X86_REG_ESP,sp+4+pop)
apis={};api=0x70010000
for dll in pe.DIRECTORY_ENTRY_IMPORT:
 for i in dll.imports:
  put(i.address,api);apis[api]=i.name.decode() if i.name else '#'+str(i.ordinal);cpu.mem_write(api,b'\xc3');api+=16
seen=[]
def oncode(uc,address,size,user):
 a,d=uc.reg_read(UC_X86_REG_EAX),uc.reg_read(UC_X86_REG_EDX)
 if address==0x402f04:ret(alloc(a))
 elif address==0x402f20:ret(0)
 elif address==0x402f38:
  old=u32(a);p=alloc(d)
  if old and p:cpu.mem_write(p,bytes(cpu.mem_read(old,min(sizes.get(old,0),d))))
  put(a,p);ret(p)
 elif address in apis:
  name=apis[address];sp=uc.reg_read(UC_X86_REG_ESP);seen.append(name)
  if name=='SysAllocStringLen':ret(bstr(u32(sp+4),u32(sp+8)),8)
  elif name=='SysReAllocStringLen':put(u32(sp+4),bstr(u32(sp+8),u32(sp+12)));ret(1,12)
  elif name=='SysFreeString':ret(0,4)
  elif name=='GetProcessHeap':ret(0x1234)
  elif name=='HeapAlloc':ret(alloc(u32(sp+12)),12)
  elif name=='HeapFree':ret(1,12)
  elif name=='HeapReAlloc':
   old=u32(sp+12);n=u32(sp+16);p=alloc(n)
   if old and p:uc.mem_write(p,bytes(uc.mem_read(old,min(sizes.get(old,0),n))))
   ret(p,16)
  elif name=='WideCharToMultiByte':
   cp,flags,src,n,dest,cap,default,used=[u32(sp+4+4*i) for i in range(8)]
   if n==0xffffffff:
    raw=bytearray();q=src
    while bytes(uc.mem_read(q,2))!=b'\0\0':raw.extend(uc.mem_read(q,2));q+=2
    raw.extend(b'\0\0')
   else:raw=bytes(uc.mem_read(src,n*2))
   data=raw.decode('utf-16le').encode('utf-8' if cp==65001 else 'cp1251',errors='replace')
   if dest:uc.mem_write(dest,data[:cap])
   ret(len(data),32)
  elif name=='MultiByteToWideChar':
   cp,flags,src,n,dest,cap=[u32(sp+4+4*i) for i in range(6)]
   if n==0xffffffff:
    raw=bytearray();q=src
    while uc.mem_read(q,1)!=b'\0':raw.extend(uc.mem_read(q,1));q+=1
    raw.extend(b'\0')
   else:raw=bytes(uc.mem_read(src,n))
   data=raw.decode('utf-8' if cp==65001 else 'cp1251',errors='replace').encode('utf-16le')
   if dest:uc.mem_write(dest,data[:cap*2])
   ret(len(data)//2,24)
  else:raise RuntimeError('unsupported API '+name)
 elif address==0x404e20:
  message=u32(a+4);print('Exception message',bytes(cpu.mem_read(message,min(u32(message-4),512))).decode('cp1251',errors='replace'),flush=True);raise RuntimeError('Delphi exception object '+hex(a)+' at '+hex(u32(uc.reg_read(UC_X86_REG_ESP))))
 if len(calls)<100000:calls.append(address)
for addr in [0x402f04,0x402f20,0x402f38,0x404e20]:cpu.hook_add(UC_HOOK_CODE,oncode,begin=addr,end=addr)
cpu.hook_add(UC_HOOK_CODE,oncode,begin=0x70010000,end=api);cpu.reg_write(UC_X86_REG_FPCW,0x37f)
source=args.source.read_text()
encoded=source.encode('utf-16le');p=alloc(len(encoded)+6)+4;put(p-4,len(encoded));cpu.mem_write(p,encoded+b'\0\0')
stack=0x701f0000;stop=0x700ff000;put(stack,stop);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_EAX,p)
try:
 cpu.emu_start(0x6515a0,stop,count=30000000)
 assert cpu.reg_read(UC_X86_REG_EIP)==stop,'instruction budget'
 code=cpu.reg_read(UC_X86_REG_EAX);print('COMPILE PASS',hex(code),'heap',heap-0x60001000,'source chars',len(source))
except Exception:
 print('EIP',hex(cpu.reg_read(UC_X86_REG_EIP)),'trace',list(map(hex,calls[-15:])),'APIs',set(seen))
 raise
if args.run:
 def invoke(address,a=0,d=0,c=0):
  put(stack,stop);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_EAX,a);cpu.reg_write(UC_X86_REG_EDX,d);cpu.reg_write(UC_X86_REG_ECX,c)
  cpu.emu_start(address,stop,count=30000000)
  assert cpu.reg_read(UC_X86_REG_EIP)==stop,('budget',hex(cpu.reg_read(UC_X86_REG_EIP)))
  return cpu.reg_read(UC_X86_REG_EAX)
 def pybstr(s):
  encoded=str(s).encode('utf-16le');p=alloc(len(encoded)+6)+4;put(p-4,len(encoded));cpu.mem_write(p,encoded+b'\0\0');return p
 scope=invoke(0x4647d4,u32(0x45f23c),1)
 invoke(0x46e4ac,scope)
 import re,math
 local=set(re.findall(r'function\s+(\w+)',source));built={'int','sin','cos','sqrt','sqr','min','max','abs','round','count','substr','findsubstr'}
 used=set(re.findall(r'\b([A-Za-z_]\w*)\s*\(',source))-local-built-{'if','while','for','function'}
 hosts={};hostaddr=0x70050000
 for name in sorted(used):
  cell=invoke(0x464fa4,scope,pybstr(name),5);put(cell+0x20,hostaddr);hosts[hostaddr]=name;cpu.mem_write(hostaddr,b'\xc3');hostaddr+=16
 for name,number in [('t_ArtGiperJump',34),('t_ArtefactFuel',11),('t_FuelTanks',43)]:
  constant=invoke(0x464fa4,scope,pybstr(name),1);put(constant+0xc,number)
 invoke(0x46ca80,code,scope,0);cpu.mem_write(code+0x24,b'\x01')
 process=alloc(12);invoke(0x469f9c,process,0)
 def resolve(cell):
  for _ in range(20):
   if cpu.mem_read(cell+8,1)!=b'\x0a':return cell
   cell=u32(cell+0x34)
  raise AssertionError('reference cycle')
 def value(cell):
  cell=resolve(cell);kind=cpu.mem_read(cell+8,1)[0]
  if kind==0:return 0
  if kind==1:return struct.unpack('<i',cpu.mem_read(cell+12,4))[0]
  if kind==2:return u32(cell+16)
  if kind==3:return struct.unpack('<d',cpu.mem_read(cell+24,8))[0]
  if kind==4:return wide(u32(cell+20))
  raise RuntimeError(('cell kind',kind,hex(cell)))
 def write_value(cell,v,kind=None):
  cell=resolve(cell)
  if isinstance(v,str):cpu.mem_write(cell+8,b'\x04');put(cell+20,pybstr(v))
  elif isinstance(v,float):cpu.mem_write(cell+8,b'\x03');cpu.mem_write(cell+24,struct.pack('<d',v))
  else:
   if kind==2:cpu.mem_write(cell+8,b'\x02');put(cell+16,int(v))
   else:cpu.mem_write(cell+8,b'\x01');put(cell+12,int(v))
 # Model four displayed stars; the native script interpreter is unmodified.
 stars=[{'x':0.,'y':0.,'id':10,'sx':100,'sy':100}, {'x':10.,'y':0.,'id':11,'sx':200,'sy':100}, {'x':10.,'y':10.,'id':12,'sx':200,'sy':200}, {'x':0.,'y':10.,'id':13,'sx':100,'sy':200}]
 ship=0x71000000;planet=0x73000000;tank=0x74000000;station=0x75000000
 current_route=[];overrides={};speed=[200];boost=[0];docked=[0];frames=[0]
 fuel=[1000];capacity=[1000];goo=[0];goo_boost=[0];available=[True]*4;stations=[False]*4;planet_owner=[2]*4;relation=[50]*4;station_bad=[False]*4;has_tank=[True];at_station=[False];ui_calls=[]
 star_ptr=lambda i:0x72000000+i*4096
 def idx(p):
  i=(p-0x72000000)//4096;assert 0<=i<len(stars),(p,i);return i
 def uid(name,query):
  if name=='FHDQuestRouteInfo' and query=='Text':return overrides.get(name,'')
  if name in ['Map','FHDNavigatorMapObserver']:
   if query in ['PosX','PosY']:return 0
   return 768 if query=='SizeX' else 527
  if name=='Distance':
   if query=='Text':return '' if not current_route else str(sum(round(math.hypot(stars[b]['x']-stars[a]['x'],stars[b]['y']-stars[a]['y'])) for a,b in zip([0]+current_route,current_route)))
   if query=='PosX':return stars[current_route[-1]]['sx']+15
   if query=='PosY':return stars[current_route[-1]]['sy']-12
   if query=='SizeY':return 24
  if name=='PathDesPos':
   if query=='IsActive':return int(bool(current_route))
   return stars[current_route[0]]['sx' if query=='PosX' else 'sy']
  if name.startswith('gs_'):
   st=next(x for x in stars if str(x['id'])==name[3:]);return st['sx' if query=='PosX' else 'sy']
  raise RuntimeError(('UI query',name,query))
 def callback(uc,address,size,user):
  if address not in hosts:return
  name=hosts[address];arr=uc.reg_read(UC_X86_REG_EAX);high=uc.reg_read(UC_X86_REG_EDX)
  args=[value(u32(arr+i*4)) for i in range(1,high+1)];v=0;kind=None
  if name=='CurrentForm':v='Galaxy'
  elif name=='Player':v=ship;kind=2
  elif name=='ShipStar':v=star_ptr(0);kind=2
  elif name=='GalaxyStar':v=star_ptr(args[0]);kind=2
  elif name=='GalaxyStars':v=len(stars)
  elif name=='CurTurn':v=100
  elif name=='ShipSpeed':v=speed[0]
  elif name=='Id':v=99 if args[0]==ship else stars[idx(args[0])]['id']
  elif name=='StarToCon':v=1;kind=2
  elif name=='SectorVisible':v=1
  elif name=='UICheckElement':assert args[0]=='Galaxy';v=uid(args[1],args[2])
  elif name=='InterfaceText':
   assert args[0]=='Galaxy';v=overrides.get(args[1],'')
   if len(args)==3:overrides[args[1]]=args[2]
  elif name=='InterfaceState':v=1
  elif name in ['InterfaceSize','InterfacePos']:ui_calls.append((name,args));v=1
  elif name=='Dist2Star':
   a,b=stars[idx(args[0])],stars[idx(args[1])];v=(a['x']-b['x'])**2+(a['y']-b['y'])**2
  elif name=='StarPlanets':v=1 if len(args)==1 else planet+idx(args[0])*4096;kind=2 if len(args)>1 else None
  elif name=='StarRuins':v=int(stations[idx(args[0])]) if len(args)==1 else station;kind=2 if len(args)>1 else None
  elif name=='PlanetOwner':v=planet_owner[(args[0]-planet)//4096] if available[(args[0]-planet)//4096] else 6
  elif name=='GetRelationPlanet':v=relation[(args[0]-planet)//4096]
  elif name=='ShipTypeN':v=6
  elif name=='ShipStanding':v=2
  elif name=='ShipGetBad':v=ship if station_bad[0] else 0;kind=2
  elif name=='ShipEqInSlot':v=tank if has_tank[0] else 0;kind=2
  elif name=='ShipFuel':v=fuel[0]
  elif name=='GetEquipmentStats':v=capacity[0] if args[0] else 0
  elif name=='ShipJump':v=200
  elif name=='CT':v='5' if args[0].endswith('kArtefactFuel') else '3'
  elif name=='PlanetOrbitRadius':v=1100.
  elif name=='PlanetSize':v=100
  elif name=='CoordX':v=0. if args[0]==ship or planet<=args[0]<planet+4*4096 or args[0]==station else stars[idx(args[0])]['x']
  elif name=='CoordY':v=0. if args[0]==ship or planet<=args[0]<planet+4*4096 or args[0]==station else stars[idx(args[0])]['y']
  elif name=='ShipInHyperSpace':v=0
  elif name=='ArtefactTypeInUse':v=boost[0] if args[1]==34 else goo[0]
  elif name=='ArtefactTypeBoosted':v=0 if args[1]==34 else goo_boost[0]
  elif name=='GetShipPlanet':v=planet if docked[0] else 0;kind=2
  elif name=='GetShipRuins':v=station if at_station[0] else 0;kind=2
  else:raise RuntimeError(('unhandled host',name,args))
  write_value(u32(arr),v,kind);ret()
 cpu.hook_add(UC_HOOK_CODE,callback,begin=0x70050000,end=hostaddr)
 def run_route(route):
  current_route[:]=route;invoke(0x46cb84,code,process)
  return overrides.get('FHDNavigatorState',''),overrides.get('FHDQuestRouteInfo','')
 def oracle(route):
  x=y=0.;days=int(bool(docked[0] or at_station[0]));prev=0
  for dest in route:
   a,b=stars[prev],stars[dest];dx=b['x']-a['x'];dy=b['y']-a['y'];length=math.hypot(dx,dy)
   px,py=dx/length*1600,dy/length*1600
   days+=math.ceil(math.hypot(px-x,py-y)/speed[0])+1
   counter=max(2,round(length*0.1)+1)
   days+=1+math.ceil((counter-1)/(boost[0]+1))
   x,y=-dx/length*1700,-dy/length*1700;prev=dest
  return days
 checks=0
 for route in [[],[1],[1,2],[1,2,3],[1,2],[1],[3],[3,2],[3],[],[1],[1,2],[1,2,1],[1,2],[],[2],[2,1],[2]]:
  state,text=run_route(route)
  if route:
   assert state.split('|')[1]==','.join(map(str,route)),(route,state,text)
   assert text==f'Полёт: ~ {oracle(route)} дн. | Прыжков: {len(route)} | Заправок: 0',(route,text,oracle(route))
  else:assert text==''
  checks+=1
 # Current equipment changes must invalidate the cached signature.
 for sp in [200,400,500,0,300]:
  speed[0]=sp;state,text=run_route([2])
  if sp:assert text==f'Полёт: ~ {oracle([2])} дн. | Прыжков: 1 | Заправок: 0'
  else:assert 'не может двигаться' in text
  checks+=1
 speed[0]=200
 for row in stars:row['x']*=4;row['y']*=4
 run_route([])
 for bs in [0,1,2,0]:
  boost[0]=bs;state,text=run_route([1,2])
  # A composed route must first be entered through ordinary individual edits.
  if not state:run_route([1]);state,text=run_route([1,2])
  assert text==f'Полёт: ~ {oracle([1,2])} дн. | Прыжков: 2 | Заправок: 0'
  checks+=1
 # Planet/station departure adds a day without changing game objects.
 run_route([]);docked[0]=1;state,text=run_route([1]);assert text==f'Полёт: ~ {oracle([1])} дн. | Прыжков: 1 | Заправок: 0';docked[0]=0;run_route([]);checks+=1
 # Real fuel planning and changes while the plotted path stays unchanged.
 def setup(route):
  run_route([])
  for j in range(1,len(route)+1):state,text=run_route(route[:j])
  return state,text
 capacity[0]=60;fuel[0]=60
 state,text=setup([1,2])
 assert text==f'Полёт: ~ {oracle([1,2])+7} дн. | Прыжков: 2 | Заправок: 1',text
 checks+=1
 # In space: a necessary stop adds landing + takeoff, with no initial launch.
 fuel[0]=20;state,text=setup([1])
 assert text==f'Полёт: ~ {oracle([1])+2} дн. | Прыжков: 1 | Заправок: 1',text
 # On a planet: refuel at the current dock; only one initial takeoff, no second landing.
 docked[0]=1;state,text=run_route([1])
 assert text==f'Полёт: ~ {oracle([1])} дн. | Прыжков: 1 | Заправок: 1',text
 docked[0]=0;at_station[0]=True;state,text=run_route([1])
 assert text==f'Полёт: ~ {oracle([1])} дн. | Прыжков: 1 | Заправок: 1',text
 at_station[0]=False;fuel[0]=60;state,text=run_route([1])
 assert text==f'Полёт: ~ {oracle([1])} дн. | Прыжков: 1 | Заправок: 0',text
 checks+=4
 # Several stops and exactly enough fuel.
 state,text=setup([1,2,3]);assert text.endswith('Прыжков: 3 | Заправок: 2'),text
 capacity[0]=80;fuel[0]=80;state,text=setup([1,2]);assert text.endswith('Заправок: 0'),text
 checks+=2
 # Look ahead across a system without service; refuel BEFORE entering it.
 capacity[0]=100;fuel[0]=50;available[1]=False;state,text=setup([1,2])
 assert text==f'Полёт: ~ {oracle([1,2])+2} дн. | Прыжков: 2 | Заправок: 1',text
 available[0]=False;state,text=setup([1,2]);assert 'нет доступного пункта' in text,text
 available[:]=[True]*4;capacity[0]=60;available[1]=False;state,text=setup([1,2])
 assert 'не хватает топлива до следующей заправки' in text,text
 checks+=3
 # Station-only refueling and hostile/uninhabited planets must not be used.
 available[:]=[False]*4;stations[0]=True;fuel[0]=20;state,text=setup([1]);assert text.endswith('Заправок: 1'),text
 stations[0]=False;available[:]=[True]*4;relation[0]=0;state,text=setup([1]);assert 'нет доступного пункта' in text,text
 relation[0]=50;planet_owner[0]=5;state,text=setup([1]);assert 'нет доступного пункта' in text,text
 planet_owner[0]=2;checks+=3
 # Black goo regenerates enough fuel during ordinary flight: no fictitious stop.
 goo[0]=1;state,text=setup([1,2]);assert text.endswith('Заправок: 0'),text
 goo_boost[0]=1;state,text=run_route([1,2]);assert text.endswith('Заправок: 0'),text
 goo[0]=goo_boost[0]=0;checks+=2
 # Invalid equipment/path does not emit a feasible ETA.
 has_tank[0]=False;state,text=setup([1]);assert 'нет топливного бака' in text,text
 has_tank[0]=True;capacity[0]=30;fuel[0]=30;state,text=setup([1]);assert 'превышает дальность или бак' in text,text
 capacity[0]=fuel[0]=1000;checks+=2
 # Unknown programmatic paths are not guessed; existing state is cleared.
 run_route([]);state,text=run_route([1,2,3]);assert not state
 # Duplicate on-screen star coordinates cannot produce a wrong selection.
 run_route([]);stars[2]['sx']=stars[1]['sx'];stars[2]['sy']=stars[1]['sy']
 state,text=run_route([1]);assert not state and not text
 checks+=2
 # QuestMarker owns the shared line when it sets its own quest summary.
 stars[2]['sx']=200;stars[2]['sy']=200
 run_route([]);overrides['FHDQuestRouteInfo']='До системы: 50 дн.'
 state,text=run_route([1,2,3]);assert not state and text=='До системы: 50 дн.'
 state,text=run_route([]);assert not state and text=='До системы: 50 дн.'
 checks+=2
 print('EXECUTION PASS:',checks,'scenarios in the actual game interpreter with synthetic host/UI data')
