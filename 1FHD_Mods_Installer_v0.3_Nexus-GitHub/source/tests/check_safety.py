from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'installer/FHD_Setup.ps1').read_text(encoding='utf-8-sig')
assert s.replace("StartsWith('{')","StartsWith('x')").count('{')==s.count('}')
assert 'FHD_AutoUpdater' in s
assert 'Write-ModCfg' in s
assert 'ModuleInfo.txt' in s
assert 'Download-DriveFile' in s
assert 'if($commitStarted)' in s
assert "if($auto.Count -eq 0)" in s
assert "Copy-Item -LiteralPath $op.Source" in s
assert 'GameRoot' in s
assert 'Rangers.exe' in s
print('PASS installer static safety smoke checks')
