󯻿# Only the owner of the installer release runs this helper to register SHA256 reference hashes.
# It does not copy or upload game executables. It records only their hashes.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$file=Join-Path $root 'catalog\exe-hashes.json'
function Pick-Exe([string]$label) {
  $msg=[System.Windows.Forms.MessageBox]::Show("Выберите ОРИГИНАЛЬНЫЙ Rangers.exe для: $label`r`n`r`nЕсли такой версии нет, нажмите Нет, чтобы пропустить.", 'Регистрация хешей', [System.Windows.Forms.MessageBoxButtons]::YesNo,[System.Windows.Forms.MessageBoxIcon]::Question)
  if($msg -ne 'Yes'){return $null}
  $dlg=New-Object System.Windows.Forms.OpenFileDialog
  $dlg.Title='Выберите Rangers.exe: '+$label
  $dlg.Filter='Rangers.exe|Rangers.exe|EXE files (*.exe)|*.exe'
  $dlg.CheckFileExists=$true
  if($dlg.ShowDialog() -ne 'OK') {return $null}
  if([IO.Path]::GetFileName($dlg.FileName) -ine 'Rangers.exe') {throw 'Нужен именно Rangers.exe, а не другой исполняемый файл.'}
  $stream=[IO.File]::OpenRead($dlg.FileName)
  try {
    if($stream.ReadByte() -ne 0x4d -or $stream.ReadByte() -ne 0x5a) {throw 'Файл не похож на Windows EXE (нет сигнатуры MZ).'}
  } finally {$stream.Dispose()}
  return (Get-FileHash -LiteralPath $dlg.FileName -Algorithm SHA256).Hash.ToUpperInvariant()
}
try {
  $db=Get-Content -LiteralPath $file -Raw -Encoding UTF8 | ConvertFrom-Json
  if([int]$db.schema -ne 1){throw 'Некорректный формат базы хешей.'}
  $entries=New-Object System.Collections.ArrayList
  foreach($e in @($db.executables)){[void]$entries.Add($e)}
  $added=0
  foreach($v in @('old','new')) {
    $display=if($v -eq 'old'){'СТАРОГО EXE'}else{'НОВОГО EXE'}
    $hash=Pick-Exe $display
    if(-not $hash){continue}
    foreach($existing in $entries){
      if([string]$existing.sha256 -ieq $hash -and $existing.variant -ne $v){throw 'Один хеш не может считаться и старой, и новой версией.'}
    }
    if(@($entries | Where-Object {$_.sha256 -ieq $hash -and $_.variant -eq $v}).Count -eq 0){
      [void]$entries.Add([pscustomobject]@{variant=$v;sha256=$hash;label=('Rangers.exe '+$v);registeredAt=(Get-Date -Format o)})
      $added++
    }
  }
  $db.executables=@($entries)
  $db | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $file -Encoding UTF8
  [System.Windows.Forms.MessageBox]::Show("Эталонные хеши записаны: $added`r`nВсего в базе: $($entries.Count)`r`nФайлы EXE не копировались и никуда не отправлялись.",'Готово')|Out-Null
} catch {
  [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Ошибка',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Error)|Out-Null
  exit 1
}
