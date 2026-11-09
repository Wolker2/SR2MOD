󯻿# Author helper: paste a deployed Google Apps Script web app URL; writes settings.json safely.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName Microsoft.VisualBasic
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$path=Join-Path $root 'settings.json'
try {
  $settings=Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json
  $url=[Microsoft.VisualBasic.Interaction]::InputBox('Вставьте ссылку /exec из Google Apps Script (один раз).
Адрес сохранится в settings.json и будет работать у всех игроков после публикации установщика.','Настройка онлайн-каталога',[string]$settings.catalogUrl)
  if(-not $url.Trim()){exit 0}
  $url=$url.Trim()
  if($url -notmatch '^https://script\.google\.com/macros/s/[A-Za-z0-9_-]+/exec(?:\?.*)?$') {throw 'Это не ссылка опубликованного веб-приложения Google Apps Script (/exec).'}
  [System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12
  $client=New-Object System.Net.WebClient
  $client.Encoding=[Text.Encoding]::UTF8
  try {$json=$client.DownloadString($url) | ConvertFrom-Json} finally {$client.Dispose()}
  if([int]$json.schema -ne 1 -or $json.folderId -ne $settings.folderId -or -not $json.mods){throw 'Сервис не вернул каталог ожидаемой папки Google Drive.'}
  $settings.catalogUrl=$url
  $settings | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $path -Encoding UTF8
  [System.Windows.Forms.MessageBox]::Show("Каталог подключён. Модов: $(@($json.mods).Count).`r`nОстальные игроки ничего не настраивают.",'FHD MODS')|Out-Null
} catch {
  [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Не удалось подключить каталог',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Error)|Out-Null
  exit 1
}
