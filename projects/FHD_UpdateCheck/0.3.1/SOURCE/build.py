from pathlib import Path
import re, subprocess

root = Path(__file__).parent
src = (root / 'FHD_UpdateCheck.c').read_text()
imports = re.findall(r'IMP\s+[^;\n]*?\bWINAPI\s+(\w+)\(([^;]*)\);', src)
print('IMPORTS', len(imports))

user32_names = {
    'GetPrivateProfileIntW','GetPrivateProfileStringW','WritePrivateProfileStringW',
    'SetWindowLongW','GetWindowLongW','CallWindowProcW','GetClientRect',
    'GetWindowThreadProcessId','SetWindowsHookExW','CallNextHookEx',
    'GetModuleHandleExW','TranslateMessage'
}

groups = {'kernel32': [], 'user32': [], 'shell32': [], 'winhttp': [], 'advapi32': []}
for name, args in imports:
    if name.startswith('WinHttp'):
        group = 'winhttp'
    elif name.startswith('Crypt'):
        group = 'advapi32'
    elif name == 'ShellExecuteW':
        group = 'shell32'
    elif name in user32_names:
        group = 'user32'
    else:
        group = 'kernel32'
    count = 0 if args.strip() == 'void' or not args.strip() else len(args.split(','))
    groups[group].append((name, count * 4))

for dll, items in groups.items():
    if not items:
        continue
    assembly = '.text\n' + ''.join(f'.globl _{name}@{stack}\n_{name}@{stack}:\n ret ${stack}\n' for name, stack in items)
    (root / f'{dll}_stub.s').write_text(assembly)
    (root / f'{dll}.def').write_text('LIBRARY ' + dll + '.dll\nEXPORTS\n' + ''.join(f'_{name}@{stack}\n' for name, stack in items))
    subprocess.run(['clang', '-target', 'i686-pc-windows-msvc', '-c', str(root / f'{dll}_stub.s'), '-o', str(root / f'{dll}_stub.obj')], check=True)
    subprocess.run(['lld-link', '/dll', '/machine:x86', '/safeseh:no', '/noentry', f'/out:{root / (dll + ".dll")}', f'/def:{root / (dll + ".def")}', f'/implib:{root / (dll + ".lib")}', str(root / f'{dll}_stub.obj')], check=True)
    lib = root / f'{dll}.lib'
    b = bytearray(lib.read_bytes())
    i = 0
    while True:
        i = b.find(b'\x00\x00\xff\xff', i)
        if i < 0:
            break
        if b[i+4:i+6] == b'\x00\x00' and b[i+6:i+8] == b'\x4c\x01':
            old = int.from_bytes(b[i+18:i+20], 'little')
            b[i+18:i+20] = ((old & 3) | (3 << 2)).to_bytes(2, 'little')
        i += 4
    lib.write_bytes(b)

subprocess.run(['clang', '-target', 'i686-pc-windows-msvc', '-O2', '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-fno-exceptions', '-c', str(root / 'FHD_UpdateCheck.c'), '-o', str(root / 'FHD_UpdateCheck.obj')], check=True)
subprocess.run(['clang', '-target', 'i686-pc-windows-msvc', '-c', str(root / 'chkstk.s'), '-o', str(root / 'chkstk.obj')], check=True)
libs = [str(root / f'{dll}.lib') for dll, items in groups.items() if items]
subprocess.run(['lld-link', '/dll', '/machine:x86', '/safeseh:no', '/entry:DllMain@12', '/opt:ref', '/opt:icf', '/nodefaultlib', f'/out:{root / "FHD_UpdateCheck.XenoPlugin.dll"}', f'/def:{root / "FHD_UpdateCheck.def"}', str(root / 'FHD_UpdateCheck.obj'), str(root / 'chkstk.obj'), *libs], check=True)
print('DONE', (root / 'FHD_UpdateCheck.XenoPlugin.dll').stat().st_size)
