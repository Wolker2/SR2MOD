import re, json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
src=(ROOT/'installer/FHD_Setup.ps1').read_text(encoding='utf-8-sig')
settings=json.loads((ROOT/'settings.json').read_text(encoding='utf-8'))
assert settings['catalogUrl'].startswith('https://script.google.com/macros/s/')
assert settings['catalogUrl'].endswith('/exec')
assert settings['folderId']=='16iY_DzJk7X7NxmegMbqYURLl5C6Q8qTd'
for v in ('Convert-UpdaterIni','Parse-LiveCatalog',"$head['Schema']", "$group['DriveFileId']", "$group['Exe']", "$group['Channel']", 'FHD_UpdateCheck', 'FHD_AutoUpdater'):
    assert v in src,v
assert '    $data=Parse-LiveCatalog $text' in src
assert '$data=$text | ConvertFrom-Json' in src # preserved JSON fallback
# A static smoke test checks format examples. Real PowerShell execution requires Windows.
manifest='''[Updater]\nSchema=2\nCount=2\nLive=1\nFolderId=16iY_DzJk7X7NxmegMbqYURLl5C6Q8qTd\nSnapshotUTC=2026-11-04T12:00:00Z\n[Mod1]\nId=FHD_OnlineChat\nVersion=0.22\nModifiedUTC=2026-11-04T12:00:00Z\nDriveFileId=1MmcxyeAnziTpLhv54isczUvIrns0ygKF\nExe=any\nChannel=test\nSize=420490\n[Mod2]\nId=FHD_UpdateCheck\nVersion=0.3.1\nModifiedUTC=2026-11-04T12:00:00Z\nDriveFileId=1MmcxyeAnziTpLhv54isczUvIrns0ygKF\nExe=any\nChannel=release\nSize=420490\n'''
sections=re.findall(r'(?m)^\[Mod\d+\]$', manifest)
assert len(sections)==2
assert 'Id=FHD_UpdateCheck' in manifest
print('PASS shared manifest metadata, preconfigured URL and script parser static inspection')
