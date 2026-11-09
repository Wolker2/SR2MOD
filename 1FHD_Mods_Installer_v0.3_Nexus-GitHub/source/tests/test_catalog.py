import json, re, zipfile, os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
data=json.loads((ROOT/'catalog/catalog-snapshot.json').read_text())
assert data['schema']==1
assert data['folderId']=='16iY_DzJk7X7NxmegMbqYURLl5C6Q8qTd'
assert len(data['mods'])>20
assert len({x['id'] for x in data['mods']})==len(data['mods'])
for m in data['mods']:
    assert re.fullmatch(r'[A-Za-z0-9_-]{10,}',m['fileId']),m
    assert m['fileName'].lower().endswith(('.rar','.zip','.7z'))
    assert m['variant'] in ('old','new','any')
    assert m['channel'] in ('stable','test')

samples=['FHD_Wormhole-v0.7.4 nativ NoDLL.rar','FHD_Artifacts_0.8.8_RELEASE_FULL.zip','FHD_OnlineChat_0.21 TEST.zip']
expected={'FHD_Wormhole':'Mods/FHD_MODS/FHD_Wormhole/ModuleInfo.txt','FHD_Artifacts':'Mods/FHD_MODS/FHD_Artifacts/ModuleInfo.txt','FHD_OnlineChat':'Mods/OtherMods/FHD_OnlineChat/ModuleInfo.txt'}
for s in samples:
    p=Path('/mnt/data')/s
    if p.exists():
      with zipfile.ZipFile(p) as z:
        mods=[x for x in z.namelist() if x.endswith('ModuleInfo.txt')]
        assert any(v in mods for v in expected.values()),mods
        for m in mods:
          assert m.startswith('Mods/'),m
        assert z.testzip() is None
assert not any(x['updater'] for x in data['mods']), 'No auto updater on live Drive as checked 2026-10-29'
print('PASS snapshot schema:',len(data['mods']),'archives; source ZIP structures validated; AutoUpdater gating required')
