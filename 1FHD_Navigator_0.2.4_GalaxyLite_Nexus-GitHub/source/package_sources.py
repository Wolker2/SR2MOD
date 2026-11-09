from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parent
VERSION='0.2.4_GALAXYLITE_TEXT_RESTORE_TEST'
def run():
 p=[f for part in ['src','tests'] for f in (ROOT/part).rglob('*') if f.is_file() and '__pycache__' not in f.parts and f.suffix!='.pyc']
 p += [ROOT/name for name in ['build.py','package_sources.py','README_RU.md','BUILD.md','CHANGELOG.md','DIAGNOSTICS_RU.md','requirements.md']]
 digest={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p)}
 manifest={'mod':'FHD_Navigator','version':VERSION,'runtime_dlls':0,'runtime_exe_addresses':0,'source_sha256':digest}
 (ROOT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
 p += [ROOT/'manifest.json',ROOT/'dist/VERIFICATION.json']
 output=ROOT/'dist/FHD_Navigator_0.2.4_GALAXYLITE_TEXT_RESTORE_SOURCE.zip'
 with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for f in sorted(p):z.write(f,'FHD_Navigator_GalaxyLite_024_Source/'+str(f.relative_to(ROOT)))
 with zipfile.ZipFile(output) as z:assert z.testzip() is None
 print(output)
if __name__=='__main__':run()
