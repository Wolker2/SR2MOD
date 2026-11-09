"""Exercise real TObjectGI/GraphButton click dispatch in both supplied EXEs.
UI objects and callbacks are synthetic; no complete game is launched.
"""
from pathlib import Path
import argparse,struct
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
p=argparse.ArgumentParser();p.add_argument('executables',nargs='+',type=Path);args=p.parse_args()
for filename in args.executables:
 pe=pefile.PE(str(filename));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
 old=pe.OPTIONAL_HEADER.SizeOfImage==0x4d1000
 vmtref=0x4a0da0 if old else 0x4ac720
 u=lambda a:struct.unpack_from('<I',image,a-base)[0]
 buttonvmt=u(vmtref);basevmt=u(u(buttonvmt-36))
 down=u(basevmt+0x54);up=u(basevmt+0x58)
 press=0x4a2844 if old else 0x4ad910
 md=Cs(CS_ARCH_X86,CS_MODE_32)
 queue=int(next(i.op_str for i in md.disasm(image[press-base:press-base+80],press) if i.mnemonic=='call'),16)
 cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(base,(len(image)+4095)&~4095);cpu.mem_write(base,image)
 cpu.mem_map(0,4096);cpu.mem_map(0x70000000,0x200000)
 def put(a,v):cpu.mem_write(a,struct.pack('<I',v&0xffffffff))
 def u32(a):return struct.unpack('<I',cpu.mem_read(a,4))[0]
 root,button,ml,point,blocker,code=0x70010000,0x70011000,0x70012000,0x70013000,0x70014000,0x70015000
 map_panel=0x70017000
 native_down,native_up=0x70016000,0x70016100;stack=0x701f0000;stop=0x700ff000
 for a in [root,button,blocker,map_panel]:
  put(a,buttonvmt if a==button else basevmt);put(a+0x18,ml);cpu.mem_write(a+0x41,b'\x01')
  for off,v in [(0x28,768),(0x2c,527),(0x60,0),(0x64,0),(0x68,768),(0x6c,527)]:put(a+off,v)
 put(root+4,map_panel);put(root+8,button);put(map_panel+0x14,root);put(map_panel+0x10,button);put(button+0xc,map_panel);put(button+0x14,root);put(ml+0x18,root);put(ml+0x30,button)
 put(map_panel+0xa8,native_down);put(map_panel+0xac,map_panel);put(map_panel+0xb0,native_up);put(map_panel+0xb4,map_panel)
 put(button+0x1f0,code);cpu.mem_write(button+0x121,b'\x00')
 put(point,200);put(point+4,100)
 cpu.mem_write(native_down,b'\xc2\x04\x00');cpu.mem_write(native_up,b'\xc2\x04\x00')
 events=[]
 def intercept(uc,addr,size,user):
  if addr==native_down:events.append('native map down')
  elif addr==native_up:events.append('native map up')
  elif addr==queue:
   assert uc.reg_read(UC_X86_REG_EAX)==ml and uc.reg_read(UC_X86_REG_EDX)==code
   events.append('queue navigator script')
   sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EIP,u32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
 def release_memory(uc,addr,size,user):
  # Delphi TObject.Free is replaced only at the memory/destructor boundary.
  # Native FreeOwnedChildren/FreeOwnedChild/UnlinkOwnedChild run unchanged.
  assert uc.reg_read(UC_X86_REG_EAX)==blocker
  sp=uc.reg_read(UC_X86_REG_ESP);uc.reg_write(UC_X86_REG_EIP,u32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
 cpu.hook_add(UC_HOOK_CODE,release_memory,begin=0x4045ac,end=0x4045ac)
 for addr in [native_down,native_up,queue]:cpu.hook_add(UC_HOOK_CODE,intercept,begin=addr,end=addr)
 def call(address,self=root):
  put(stack,stop);cpu.reg_write(UC_X86_REG_ESP,stack);cpu.reg_write(UC_X86_REG_EAX,self);cpu.reg_write(UC_X86_REG_EDX,0);cpu.reg_write(UC_X86_REG_ECX,point)
  cpu.emu_start(address,stop,count=100000)
  assert cpu.reg_read(UC_X86_REG_EIP)==stop
  assert cpu.reg_read(UC_X86_REG_ESP)==stack+4
 call(down);call(up)
 assert events==['native map down','native map up','queue navigator script'],events
 # Map owns temporary stars; the observer is a sibling and survives clearing Map.
 assert u32(button+0x14)==u32(map_panel+0x14)==root
 assert u32(map_panel+4)==u32(map_panel+8)==0
 for _ in range(5):
  put(map_panel+4,blocker);put(map_panel+8,blocker);put(blocker+0x14,map_panel)
  # Execute the actual cleanup routine used by Galaxy.OnClose.
  call(0x4c9530 if old else 0x4c0b84,map_panel)
  assert u32(map_panel+4)==u32(map_panel+8)==u32(blocker+0x14)==0
  events.clear();call(down);call(up)
  assert events==['native map down','native map up','queue navigator script'],events
 # A blocking native window above the observer prevents its scripted click.
 put(button+0x10,blocker);put(blocker+0xc,button);put(blocker+0x14,root);put(root+8,blocker);cpu.mem_write(blocker+0x89,b'\x01')
 events.clear();call(down);call(up);assert 'queue navigator script' not in events
 # A click outside the observer does not trigger it.
 put(button+0x10,0);put(root+8,button);put(point,800);events.clear();call(down);call(up)
 assert 'queue navigator script' not in events
 print('PASS: native map callbacks retained; sibling observer survives 5 native Map cleanups; one script on release; occluded/outside ignored:',filename.name,hex(pe.OPTIONAL_HEADER.SizeOfImage))
