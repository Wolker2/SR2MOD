import json, re, hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
config=json.loads((root/'catalog/exe-hashes.json').read_text(encoding='utf-8-sig'))
assert config['schema']==1
seen={}
for obj in config['executables']:
    assert obj['variant'] in ('old','new')
    assert re.fullmatch('[a-fA-F0-9]{64}',obj['sha256'])
    k=obj['sha256'].upper()
    assert seen.get(k, obj['variant'])==obj['variant']
    seen[k]=obj['variant']
ps=(root/'installer/FHD_Setup.ps1').read_text(encoding='utf-8-sig')
assert 'Get-FileHash -LiteralPath $exe -Algorithm SHA256' in ps
assert 'Detect-ExeVariant $game' in ps
assert 'Update-ExeStatus' in ps
assert 'ComboBox' not in ps
assert 'if($mod.variant -ne \'any\' -and $mod.variant -ne $script:currentVariant){continue}' in ps
assert "Variant='unknown'" in ps
# Cannot provide authoritative old/new executable hashes without verified reference executables.
assert not config['executables'], 'New hashes should be reviewed before shipping.'
print('PASS EXE SHA256 auto-detection configured; reference hash list intentionally empty')
