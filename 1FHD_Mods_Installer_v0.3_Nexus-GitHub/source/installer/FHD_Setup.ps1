󯻿# FHD Mods Installer 0.3. Windows PowerShell 5.1 / Windows Forms.
# No PowerShell profile, no execution of downloaded scripts or installers.
param([switch]$TestMode)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.Net.Http
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
$script:base = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$script:settingsPath = Join-Path $script:base 'settings.json'
$script:logPath = Join-Path $script:base 'fhd_setup.log'
$script:items = @()
$script:checkboxes = @{}
$script:installed = @()
$script:cfg = Get-Content -LiteralPath $script:settingsPath -Raw -Encoding UTF8 | ConvertFrom-Json
$script:catalogData = $null
$script:online = $false
$script:currentVariant = 'unknown'
$script:currentHash = ''

function Log([string]$message) {
  $line = ('{0} {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'),$message)
  [IO.File]::AppendAllText($script:logPath, $line + [Environment]::NewLine, [Text.Encoding]::UTF8)
}
function Alert([string]$s) { [void][System.Windows.Forms.MessageBox]::Show($s,'FHD Mods Installer',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::Information) }
function Fail([string]$s) { throw [System.Exception]::new($s) }
function Tick([string]$s,[int]$value=-1) {
  $script:state.Text=$s
  if($value -ge 0) {$script:progress.Value=[Math]::Min(100,[Math]::Max(0,$value))}
  [System.Windows.Forms.Application]::DoEvents()
}
function UriBase([string]$url) {
  if(-not $url -or -not $url.StartsWith('https://')) { Fail 'Для обновления каталога нужен HTTPS-адрес опубликованного Google Apps Script.' }
  return $url
}
function Get-RemoteText([string]$url) {
  $client = New-Object System.Net.WebClient
  $client.Encoding = [Text.Encoding]::UTF8
  $client.Headers.Add('User-Agent','FHD-Mods-Installer/0.3')
  try {return $client.DownloadString((UriBase $url))} finally {$client.Dispose()}
}
# The already-published FHD_UpdateCheck manifest is INI (Schema=2), not the
# older JSON catalog (Schema=1). Both formats are accepted; no second GAS app.
function Convert-UpdaterIni([string]$raw) {
  $groups=New-Object System.Collections.ArrayList
  $head=@{}
  $section=''
  $current=$null
  foreach($original in ($raw -split "`r?`n")) {
    $line=$original.Trim().TrimStart([char]0xFEFF)
    if(-not $line -or $line.StartsWith(';') -or $line.StartsWith('#')) {continue}
    if($line -match '^\[([^\]]+)\]$') {
      $section=$Matches[1].Trim()
      if($section -match '^Mod\d+$') {
        $current=@{}
        [void]$groups.Add($current)
      } else {$current=$null}
      continue
    }
    if($line -match '^([^=]+)=(.*)$') {
      $key=$Matches[1].Trim()
      $value=$Matches[2].Trim()
      if($section -ieq 'Updater') {$head[$key]=$value}
      elseif($section -match '^Mod\d+$' -and $null -ne $current) {$current[$key]=$value}
    }
  }
  if([string]$head['Schema'] -ne '2' -or [string]$head['Live'] -ne '1') {
    Fail 'Ответ каталога не содержит актуальный FHD_UpdateCheck Schema=2 Live=1.'
  }
  $folder=[string]$head['FolderId']
  if($folder -ne [string]$script:cfg.folderId) {
    Fail 'FHD_UpdateCheck подключён к другой папке Google Диска.'
  }
  if($groups.Count -eq 0) {Fail 'Ответ FHD_UpdateCheck не содержит записей ModN.'}
  [int]$advertised=0
  if([int]::TryParse([string]$head['Count'],[ref]$advertised) -and $advertised -ne $groups.Count) {
    Fail ('Неполный ответ каталога: ожидается '+$advertised+', получено '+$groups.Count+'.')
  }
  # The old static snapshot supplies friendly descriptions, not version IDs.
  $static=Get-Content -LiteralPath (Join-Path $script:base 'catalog\catalog-snapshot.json') -Raw -Encoding UTF8 | ConvertFrom-Json
  $out=New-Object System.Collections.ArrayList
  $seen=@{}
  foreach($group in $groups) {
    $modId=[string]$group['Id']
    $driveId=[string]$group['DriveFileId']
    if($modId -notmatch '^[A-Za-z][A-Za-z0-9_.-]+$' -or $driveId -notmatch '^[A-Za-z0-9_-]{10,}$') {continue}
    $rawExe=[string]$group['Exe']
    $variant=if($rawExe -match '^(?i:old|legacy|старый)$') {'old'} elseif($rawExe -match '^(?i:new|modern|новый)$') {'new'} else {'any'}
    if($variant -eq 'any' -and $modId -match '_(old|new)$') {$variant=$Matches[1].ToLowerInvariant()}
    $rawChannel=[string]$group['Channel']
    $channel=if($rawChannel -match '(?i:test|alpha|beta|preview|dev)') {'test'} else {'stable'}
    $baseId=$modId -replace '_(old|new)$',''
    $id=$baseId
    if($variant -ne 'any') {$id+='_'+$variant}
    if($channel -eq 'test') {$id+='__TEST'}
    if($seen.ContainsKey($id)) {Fail ('Каталог содержит повторяющийся ID: '+$id)}
    $seen[$id]=$true
    $reference=@($static.mods | Where-Object {$_.id -eq $id -or $_.id -eq $modId -or $_.id -eq $baseId} | Select-Object -First 1)
    $title=$baseId.Replace('_',' ')
    $description='Модификация Space Rangers HD из каталога FHD.'
    if($reference.Count -gt 0) {
      $title=[string]$reference[0].title
      $description=[string]$reference[0].description
    }
    if($variant -eq 'old' -and $title -notmatch '(?i)old|старый') {$title+=' (старый EXE)'}
    if($variant -eq 'new' -and $title -notmatch '(?i)new|новый') {$title+=' (новый EXE)'}
    if($baseId -eq 'FHD_UpdateCheck') {$title='FHD UpdateCheck'; $description='Обязательный внутриигровой мод проверки обновлений.'}
    if($baseId -eq 'FHD_AutoUpdater') {$title='FHD AutoUpdater'; $description='Обязательный внутриигровой автообновлятор.'}
    [long]$size=0
    [void][long]::TryParse([string]$group['Size'],[ref]$size)
    $version=[string]$group['Version']
    if(-not $version) {$version='не указана'}
    $fname=$baseId+'_'+$version+'.zip' # only metadata; actual archive type validated after download
    [void]$out.Add([pscustomobject]@{
      id=$id;title=$title;description=$description;fileId=$driveId;fileName=$fname
      size=$size;version=$version;channel=$channel;variant=$variant
      updater=($baseId -in @('FHD_AutoUpdater','FHD_UpdateCheck','FHD_AutoUpdate'))
      modified=[string]$group['ModifiedUTC']
    })
  }
  if($out.Count -eq 0) {Fail 'Онлайн-каталог не содержит пригодных для установки файлов.'}
  return [pscustomobject]@{schema=1;source='fhd-updatecheck-schema2-live';folderId=$folder;generated=[string]$head['SnapshotUTC'];mods=$out.ToArray()}
}
function Parse-LiveCatalog([string]$text) {
  if($text.TrimStart([char]0xFEFF).TrimStart().StartsWith('{')) {
    $data=$text | ConvertFrom-Json
    if([int]$data.schema -ne 1 -or -not $data.mods) {Fail 'Неверная JSON-схема онлайн-каталога.'}
    return $data
  }
  return Convert-UpdaterIni $text
}
function Read-Catalog {
  $script:online=$false
  $data=$null
  if($script:cfg.catalogUrl -and $script:cfg.catalogUrl.Trim()) {
    try {
      $text=Get-RemoteText $script:cfg.catalogUrl.Trim()
      $data=Parse-LiveCatalog $text
      $script:online=$true
    } catch {Log ('Catalog HTTPS failed: '+$_.Exception.Message)}
  }
  if(-not $data) {
    $data=Get-Content -LiteralPath (Join-Path $script:base 'catalog\catalog-snapshot.json') -Raw -Encoding UTF8 | ConvertFrom-Json
  }
  if($data.folderId -ne $script:cfg.folderId) {Fail 'Источник каталога не совпадает с выбранной папкой Google Диска.'}
  $script:catalogData=$data
  return @($data.mods)
}
function GameRoot([string]$path) {
  if(-not $path) { Fail 'Укажите папку установленной игры.' }
  $full=[IO.Path]::GetFullPath($path.Trim())
  if(-not (Test-Path -LiteralPath (Join-Path $full 'Rangers.exe') -PathType Leaf)) { Fail 'Не найден Rangers.exe. Укажите корневую папку Space Rangers HD.' }
  return $full
}
# SHA-256 is matched against an author-maintained list of two or more known EXE builds.
# Do not infer a game build from its filename, date, size or unverifiable heuristics.
function Detect-ExeVariant([string]$game) {
  $exe=Join-Path $game 'Rangers.exe'
  if(-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    return [pscustomobject]@{Variant='unknown';Hash='';Reason='Rangers.exe не найден'}
  }
  $hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToUpperInvariant()
  $known=Join-Path $script:base 'catalog\exe-hashes.json'
  if(-not (Test-Path -LiteralPath $known)) {Fail 'Отсутствует база catalog\exe-hashes.json'}
  $db=Get-Content -LiteralPath $known -Raw -Encoding UTF8 | ConvertFrom-Json
  if([int]$db.schema -ne 1) {Fail 'Неверная схема базы хешей Rangers.exe'}
  $matches=@($db.executables | Where-Object {
      $_.sha256 -and ([string]$_.sha256).ToUpperInvariant() -eq $hash -and $_.variant -in @('old','new')
  })
  if($matches.Count -gt 1 -and @($matches | Select-Object -ExpandProperty variant -Unique).Count -gt 1) {
    Fail 'Один SHA-256 ошибочно привязан к разным версиям EXE. Исправьте базу.'
  }
  if($matches.Count -eq 0) {
    return [pscustomobject]@{Variant='unknown';Hash=$hash;Reason='SHA-256 не найден среди эталонов'}
  }
  return [pscustomobject]@{Variant=[string]$matches[0].variant;Hash=$hash;Reason='SHA-256 совпал с эталоном'}
}
function Update-ExeStatus {
  $script:currentVariant='unknown'
  $script:currentHash=''
  $raw=$script:pathBox.Text.Trim()
  if(-not $raw -or -not (Test-Path -LiteralPath $raw -PathType Container)) {
    $script:exeStatus.Text='EXE: выберите папку игры (Rangers.exe)'
    Render-Items
    return
  }
  try {
    $detected=Detect-ExeVariant $raw
    $script:currentVariant=$detected.Variant
    $script:currentHash=$detected.Hash
    if($detected.Variant -eq 'old') {$version='СТАРЫЙ EXE'}
    elseif($detected.Variant -eq 'new') {$version='НОВЫЙ EXE'}
    else {$version='НЕИЗВЕСТНЫЙ EXE'}
    $prefix=if($detected.Hash){$detected.Hash.Substring(0,12)}else{'󢀔'}
    $script:exeStatus.Text=('EXE: '+$version+' | SHA-256 '+$prefix+'... | '+$detected.Reason)
    Log ('EXE detection variant='+$detected.Variant+' hash='+$detected.Hash)
  } catch {
    $script:exeStatus.Text='EXE: ошибка проверки 󢀔 '+$_.Exception.Message
    Log ('EXE detect failed: '+$_.Exception.Message)
  }
  Render-Items
}
function Get-Candidates([string]$game) {
  $list=New-Object System.Collections.ArrayList
  foreach($p in @(
    (Join-Path $game 'Rangers.exe'),
    (Join-Path $env:ProgramFiles '7-Zip\7z.exe'),
    (Join-Path ${env:ProgramFiles(x86)} '7-Zip\7z.exe'),
    (Join-Path $script:base 'tools\7z.exe')
  )) {if($p -and (Test-Path -LiteralPath $p -PathType Leaf)) {[void]$list.Add($p)}}
  $seven=Get-Command 7z.exe -ErrorAction SilentlyContinue
  if($seven) {[void]$list.Add($seven.Source)}
  return @($list | Where-Object { $_ -like '*7z.exe' })
}
function Download-DriveFile([string]$fileId,[string]$target,[scriptblock]$progressCallback) {
  if($fileId -notmatch '^[a-zA-Z0-9_-]{10,}$') { Fail ('Неверный ID Google Drive: '+$fileId) }
  $handler=New-Object System.Net.Http.HttpClientHandler
  $handler.AllowAutoRedirect=$true
  $handler.UseCookies=$true
  $client=New-Object System.Net.Http.HttpClient($handler)
  $client.Timeout=[TimeSpan]::FromHours(5)
  $url='https://drive.usercontent.google.com/download?id='+$fileId+'&export=download&confirm=t'
  try {
    for($step=0;$step -lt 5;$step++) {
      $response=$client.GetAsync($url,[System.Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
      try {
        if(-not $response.IsSuccessStatusCode) {Fail ('Google Drive HTTP '+[int]$response.StatusCode)}
        $ctype=''
        if($response.Content.Headers.ContentType) {$ctype=$response.Content.Headers.ContentType.MediaType}
        $disposition=$response.Content.Headers.ContentDisposition
        if($ctype -like 'text/html*' -or ($ctype -like 'text/plain*' -and -not $disposition)) {
          $html=$response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
          # Google Drive confirms large downloads with an HTML form.
          $form=[regex]::Match($html,'(?is)<form[^>]*action=["''](?<url>[^"'']+)["''][^>]*>(?<body>.*?)</form>')
          if($form.Success) {
            $url=[Net.WebUtility]::HtmlDecode($form.Groups['url'].Value)
            if($url.StartsWith('/')) {$url='https://drive.google.com'+$url}
            if(-not $url.StartsWith('https://')) {Fail 'Небезопасный адрес в подтверждении Google Drive.'}
            $fields=[regex]::Matches($form.Groups['body'].Value,'(?is)<input[^>]*name=["''](?<key>[^"'']+)["''][^>]*value=["''](?<val>[^"'']*)["'']')
            $pairs=New-Object System.Collections.Generic.List[string]
            foreach($f in $fields) {$pairs.Add([Uri]::EscapeDataString($f.Groups['key'].Value)+'='+[Uri]::EscapeDataString([Net.WebUtility]::HtmlDecode($f.Groups['val'].Value)))}
            if($pairs.Count -gt 0) {$url += $(if($url.Contains('?')) {'&'} else {'?'})+($pairs -join '&')}
            continue
          }
          $confirm=[regex]::Match($html,'confirm=([a-zA-Z0-9_-]+)')
          if($confirm.Success) {$url='https://drive.google.com/uc?export=download&id='+$fileId+'&confirm='+$confirm.Groups[1].Value;continue}
          Fail 'Google Диск не выдал архив: проверьте публичный доступ, лимит скачиваний и размер файла.'
        }
        $length=$response.Content.Headers.ContentLength
        $input=$response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
        $output=[IO.File]::Create($target)
        try {
          $buf=New-Object byte[] 1048576
          [long]$readBytes=0
          [int]$last=-1
          while(($count=$input.Read($buf,0,$buf.Length)) -gt 0) {
            $output.Write($buf,0,$count)
            $readBytes += $count
            if($length -gt 0) {
              $pct=[int][Math]::Min(100,[Math]::Floor(100*$readBytes/$length))
              if($pct -ne $last) {& $progressCallback $pct; $last=$pct}
            } else {& $progressCallback -1}
          }
          if($length -gt 0 -and $readBytes -ne $length) {Fail 'Скачан неполный архив (размер не совпал).'}
        } finally {$output.Dispose();$input.Dispose()}
        if((Get-Item -LiteralPath $target).Length -lt 20) {Fail 'Получен пустой или повреждённый файл.'}
        $signature=New-Object byte[] 7
        $stream=[IO.File]::OpenRead($target)
        try {[void]$stream.Read($signature,0,7)} finally {$stream.Dispose()}
        $zip=($signature[0] -eq 0x50 -and $signature[1] -eq 0x4B)
        $rar=([Text.Encoding]::ASCII.GetString($signature,0,4) -eq 'Rar!')
        $sz=($signature[0] -eq 0x37 -and $signature[1] -eq 0x7A)
        if(-not ($zip -or $rar -or $sz)) { Fail 'Ответ Google Диска не является ZIP/RAR/7z архивом.' }
        return
      } finally {$response.Dispose()}
    }
    Fail 'Не удалось подтвердить скачивание Google Drive.'
  } finally {$client.Dispose();$handler.Dispose()}
}
function Safe-Relative([string]$name) {
  $r=$name.Replace('/','\').TrimStart('\')
  if($name.StartsWith('/') -or $name.StartsWith('\') -or $r -match '(^|\\)\.\.(\\|$)' -or $r -match '^[a-zA-Z]:' -or $r.Contains(':')) {
    Fail ('Небезопасный путь внутри архива: '+$name)
  }
  return $r
}
function Extract-Archive([string]$archive,[string]$dir,[string]$game) {
  [IO.Directory]::CreateDirectory($dir) | Out-Null
  $stream=[IO.File]::OpenRead($archive); $sig=New-Object byte[] 7
  try {[void]$stream.Read($sig,0,7)} finally {$stream.Dispose()}
  if($sig[0] -eq 0x50 -and $sig[1] -eq 0x4B) {
    $zip=[IO.Compression.ZipFile]::OpenRead($archive)
    try {
      [long]$total=0
      foreach($entry in $zip.Entries) {
        $relative=Safe-Relative $entry.FullName
        if(-not $relative){continue}
        $total+=$entry.Length
        if($total -gt 30000000000) {Fail 'Распакованное содержимое превышает безопасный предел 30 ГБ.'}
        $dest=Join-Path $dir $relative
        if($entry.Name -eq '') {[IO.Directory]::CreateDirectory($dest)|Out-Null;continue}
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest))|Out-Null
        $i=$entry.Open();$o=[IO.File]::Create($dest)
        try {$i.CopyTo($o)} finally {$i.Dispose();$o.Dispose()}
      }
    } finally {$zip.Dispose()}
    return
  }
  $seven=Get-Candidates $game | Select-Object -First 1
  if($seven) {
    $args=@('x','-y','-bd','-spe','-o'+$dir,$archive)
    & $seven @args | Out-Null
    if($LASTEXITCODE -ne 0) {Fail 'Ошибка распаковки RAR/7z через 7-Zip.'}
  } elseif(Get-Command tar.exe -ErrorAction SilentlyContinue) {
    & tar.exe -xf $archive -C $dir
    if($LASTEXITCODE -ne 0) {Fail 'RAR не распакован. Установите бесплатный 7-Zip, затем повторите.'}
  } else {Fail 'Для настоящего RAR/7z требуется установленный 7-Zip (7z.exe). ZIP работает без него.'}
  foreach($file in (Get-ChildItem -LiteralPath $dir -Force -Recurse)) {
    if(($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {Fail 'Архив содержит недопустимую символическую ссылку.'}
  }
}
function Build-InstallMap([string]$stage) {
  $modules=Get-ChildItem -LiteralPath $stage -Recurse -File -Filter 'ModuleInfo.txt' | Where-Object {$_.FullName -notmatch '[\\/](SOURCE|QA|Backup)[\\/]'}
  if(-not $modules) {Fail 'В архиве нет ModuleInfo.txt: схема установки требует ручной проверки.'}
  $map=New-Object System.Collections.ArrayList
  $activated=New-Object System.Collections.ArrayList
  foreach($module in $modules) {
    $folder=$module.Directory.FullName
    if(($module.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {Fail 'Недопустимая символическая ссылка.'}
    $rel=$folder.Substring($stage.Length).TrimStart('\','/')
    $parts=@($rel -split '[\\/]')
    $modName=$module.Directory.Name
    if($modName -notmatch '^[a-zA-Z0-9_.-]+$') {Fail ('Недопустимое имя папки мода: '+$modName)}
    if($parts.Count -ge 3 -and $parts[0] -ieq 'Mods') {$destBase=Join-Path 'Mods' (Join-Path $parts[1] $parts[2]);$key=$parts[1]+'\'+$parts[2]}
    elseif($parts.Count -ge 4 -and $parts[1] -ieq 'Mods') {$destBase=Join-Path 'Mods' (Join-Path $parts[2] $parts[3]);$key=$parts[2]+'\'+$parts[3]}
    else {$destBase=Join-Path 'Mods\FHD_MODS' $modName;$key='FHD_MODS\'+$modName}
    if(-not ($key -match '^(FHD_MODS|OtherMods)\\[a-zA-Z0-9_.-]+$')) { Fail ('Неожиданный путь мода: '+$key) }
    if(-not ($activated -contains $key)) {[void]$activated.Add($key)}
    $files=Get-ChildItem -LiteralPath $folder -Recurse -File | Where-Object {$_.FullName -notmatch '[\\/](SOURCE|QA)[\\/]'}
    foreach($f in $files) {
      $tail=$f.FullName.Substring($folder.Length).TrimStart('\')
      $target=Safe-Relative (Join-Path $destBase $tail)
      [void]$map.Add([pscustomobject]@{Source=$f.FullName;Target=$target})
    }
  }
  return [pscustomobject]@{Files=@($map);Mods=@($activated)}
}
function Read-ModCfg([string]$file) {
  if(-not (Test-Path -LiteralPath $file)) {return [pscustomobject]@{Prefix=@();Mods=@();Suffix=@();Encoding=(New-Object System.Text.UTF8Encoding($false))}}
  $bytes=[IO.File]::ReadAllBytes($file)
  $enc=[Text.Encoding]::Default  # Preserve Windows ANSI files when there is no BOM.
  if($bytes.Length -gt 1 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {$enc=[Text.Encoding]::Unicode}
  elseif($bytes.Length -gt 1 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {$enc=[Text.Encoding]::BigEndianUnicode}
  elseif($bytes.Length -gt 2 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {$enc=New-Object System.Text.UTF8Encoding($true)}
  $lines=@($enc.GetString($bytes).TrimStart([char]0xFEFF) -split '\r?\n')
  $prefix=New-Object System.Collections.ArrayList
  $mods=New-Object System.Collections.ArrayList
  foreach($line in $lines) {
    if($line -match '^\s*CurrentMod\s*=\s*(.*)$') {foreach($v in ($matches[1] -split ',')) {if($v.Trim()){[void]$mods.Add($v.Trim())}}}
    elseif($line.Trim()) {[void]$prefix.Add($line)}
  }
  return [pscustomobject]@{Prefix=@($prefix);Mods=@($mods);Encoding=$enc}
}
function Write-ModCfg([string]$file,[string[]]$newMods) {
  $old=Read-ModCfg $file
  $all=New-Object System.Collections.ArrayList
  foreach($m in @($old.Mods)+@($newMods)) {if($m -and -not (@($all) -icontains $m)) {[void]$all.Add($m)}}
  $lines=@($old.Prefix)+@('CurrentMod='+($all -join ','))
  [IO.File]::WriteAllText($file,($lines -join "`r`n")+"`r`n",$old.Encoding)
}
function Install-Selected([object[]]$mods) {
  # Always refresh remote file IDs/version metadata immediately before downloading.
  # Never install from a stale bundled snapshot if the live service is offline.
  $fresh=@(Read-Catalog)
  if(-not $script:online) {Fail 'Онлайн-каталог не подключён или недоступен. Чтобы всегда получать ПОСЛЕДНИЕ версии, установка из устаревшего снимка запрещена.'}
  $freshSelection=New-Object System.Collections.ArrayList
  foreach($m in $mods) {
    $found=@($fresh | Where-Object {$_.id -eq $m.id})
    if($found.Count -ne 1) {Fail ('Выбранный мод больше не найден в актуальном каталоге: '+$m.id)}
    [void]$freshSelection.Add($found[0])
  }
  $script:items=$fresh
  $mods=@($freshSelection)
  $game=GameRoot $script:pathBox.Text
  # Re-read the hash immediately before installation. A modified EXE is never trusted.
  $build=Detect-ExeVariant $game
  $script:currentVariant=$build.Variant
  $script:currentHash=$build.Hash
  if(Get-Process -Name Rangers -ErrorAction SilentlyContinue) {Fail 'Перед установкой полностью закройте игру Rangers.exe.'}
  $auto=@($script:items | Where-Object {$_.updater -eq $true -or $_.id -in @('FHD_AutoUpdater','FHD_UpdateCheck','FHD_AutoUpdate')})
  if($auto.Count -eq 0) {Fail 'В Google Диске отсутствует обязательный FHD_UpdateCheck / FHD_AutoUpdater. Установка остановлена без изменения игры. Добавьте архив автообновлятора в папку-источник.'}
  if($mods.Count -eq 0) {Fail 'Отметьте хотя бы один мод.'}
  $all=New-Object System.Collections.ArrayList
  foreach($mod in $mods) {[void]$all.Add($mod)}
  foreach($a in $auto) {if(-not (@($all | ForEach-Object {$_.id}) -contains $a.id)) {[void]$all.Add($a)}}
  if(@($all | Where-Object {$_.channel -eq 'test'}).Count -gt 0 -and -not $script:allowTest.Checked) {Fail 'Выбрана тестовая сборка. Отметьте «Показывать тестовые сборки» для её установки.'}
  $conflicts=@($all | Where-Object {$_.variant -ne 'any' -and $_.variant -ne $script:currentVariant})
  if($conflicts.Count -gt 0) {Fail ('Несовместимый мод! Определённая по SHA-256 версия Rangers.exe: '+$build.Variant+'. Если версия неизвестна, доступны только универсальные моды. Автор может добавить эталон в catalog\exe-hashes.json.')}
  $work=Join-Path $env:TEMP ('FHD_Setup_'+[Guid]::NewGuid().ToString('N'))
  [IO.Directory]::CreateDirectory($work)|Out-Null
  $modsRoot=Join-Path $game 'Mods';[IO.Directory]::CreateDirectory($modsRoot)|Out-Null
  $backup=Join-Path $game ('FHD_Setup_Backups\'+(Get-Date -Format 'yyyyMMdd_HHmmss'))
  [IO.Directory]::CreateDirectory($backup)|Out-Null
  $ops=New-Object System.Collections.ArrayList
  $keys=New-Object System.Collections.ArrayList
  $done=New-Object System.Collections.ArrayList
  $commitStarted=$false
  $cfgChanged=$false
  try {
    $n=0
    foreach($mod in $all) {
      $n++
      Tick ('Скачивание: '+$mod.title+' ('+$n+'/'+$all.Count+')') 0
      $archive=Join-Path $work ('download_'+$n+'.bin')
      $cb={param($percent) if($percent -ge 0) {Tick ('Загрузка '+$mod.title+' 󢀔 '+$percent+'%') $percent} else {Tick ('Загрузка '+$mod.title+'...')}}
      Download-DriveFile $mod.fileId $archive $cb
      $dir=Join-Path $work ('unpack_'+$n)
      Tick ('Проверка архива: '+$mod.title) 0
      Extract-Archive $archive $dir $game
      $built=Build-InstallMap $dir
      if($built.Files.Count -eq 0) {Fail ('Пустой архив: '+$mod.title)}
      foreach($entry in $built.Files) {[void]$ops.Add($entry)}
      foreach($k in $built.Mods) {if(-not (@($keys) -icontains $k)) {[void]$keys.Add($k)}}
      Log ('Staged: '+$mod.id+' files='+$built.Files.Count+' mods='+($built.Mods -join ','))
    }
    $cfgFile=Join-Path $modsRoot 'ModCFG.txt'
    $commitStarted=$true
    if(Test-Path -LiteralPath $cfgFile) {Copy-Item -LiteralPath $cfgFile -Destination (Join-Path $backup 'ModCFG.txt') -Force}
    foreach($op in $ops) {
      $target=Join-Path $game $op.Target
      if(Test-Path -LiteralPath $target -PathType Leaf) {
        $dest=Join-Path (Join-Path $backup 'files') $op.Target
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest))|Out-Null
        Copy-Item -LiteralPath $target -Destination $dest -Force
      }
    }
    $index=0
    foreach($op in $ops) {
      $index++
      $target=Join-Path $game $op.Target
      [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
      [void]$done.Add($op)
      Copy-Item -LiteralPath $op.Source -Destination $target -Force
      if(($index % 40) -eq 0) {Tick ('Установка файлов: '+$index+'/'+$ops.Count) ([int](100*$index/$ops.Count))}
    }
    if($script:enableMods.Checked) {$cfgChanged=$true; Write-ModCfg $cfgFile ([string[]]@($keys))}
    $receipt=[pscustomobject]@{date=(Get-Date -Format o);mods=@($keys);files=$ops.Count;source=$script:catalogData.source;backup=$backup}
    $receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $backup 'install-receipt.json') -Encoding UTF8
    $script:cfg.lastGamePath=$game
    $script:cfg | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $script:settingsPath -Encoding UTF8
    Tick ('Готово! Установлено модов: '+$keys.Count) 100
    Log ('Success '+($keys -join ','))
    Alert ('Установка завершена. Модов: '+$keys.Count+"`r`nРезервная копия: "+$backup+"`r`nВключение при следующем запуске игры: "+$script:enableMods.Checked)
  } catch {
    Log ('FAILED: '+$_.Exception.Message)
    Tick 'Ошибка: восстановление резервных копий...' 0
    if($commitStarted) {
    foreach($op in $done) {
      $target=Join-Path $game $op.Target
      $old=Join-Path (Join-Path $backup 'files') $op.Target
      if(Test-Path -LiteralPath $old) {Copy-Item -LiteralPath $old -Destination $target -Force}
      elseif(Test-Path -LiteralPath $target) {Remove-Item -LiteralPath $target -Force}
    }
    if($cfgChanged) {
      $cfgFile=Join-Path $modsRoot 'ModCFG.txt'
      $origCfg=Join-Path $backup 'ModCFG.txt'
      if(Test-Path -LiteralPath $origCfg) {Copy-Item -LiteralPath $origCfg -Destination $cfgFile -Force}
      else {if(Test-Path -LiteralPath $cfgFile) {Remove-Item -LiteralPath $cfgFile -Force}}
    }
    }
    throw
  } finally {
    if(Test-Path -LiteralPath $work) {Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue}
  }
}

# ------------------------------ USER INTERFACE ------------------------------
$form=New-Object System.Windows.Forms.Form
$form.Text='FHD MODS INSTALLER 󢀢 Space Rangers HD'
$form.ClientSize=New-Object System.Drawing.Size(1020,735)
$form.MinimumSize=New-Object System.Drawing.Size(850,650)
$form.StartPosition='CenterScreen'
$form.BackColor=[System.Drawing.Color]::FromArgb(17,24,39)
$form.ForeColor=[System.Drawing.Color]::FromArgb(226,232,240)
$form.Font=New-Object System.Drawing.Font('Segoe UI',10)
$script:form=$form
function Create-Label($text,$x,$y,$w,$h,$size=10,$strong=$false) {
  $l=New-Object System.Windows.Forms.Label
  $l.Text=$text;$l.Left=$x;$l.Top=$y;$l.Width=$w;$l.Height=$h
  $style=if($strong){[System.Drawing.FontStyle]::Bold}else{[System.Drawing.FontStyle]::Regular}
  $l.Font=New-Object System.Drawing.Font('Segoe UI',$size,$style)
  $l.ForeColor=[System.Drawing.Color]::FromArgb(226,232,240)
  $form.Controls.Add($l)
  return $l
}
[void](Create-Label 'FHD MODS' 18 14 410 36 24 $true)
[void](Create-Label 'Установщик модификаций 󢀢 загрузка с Google Диска' 20 55 650 24 10 $false)
[void](Create-Label 'Папка игры:' 20 92 128 25 10 $true)
$script:pathBox=New-Object System.Windows.Forms.TextBox
$script:pathBox.SetBounds(150,89,678,29)
$script:pathBox.Text=$script:cfg.lastGamePath
$form.Controls.Add($script:pathBox)
$browse=New-Object System.Windows.Forms.Button
$browse.Text='Обзор...';$browse.SetBounds(840,88,140,30)
$form.Controls.Add($browse)
$browse.Add_Click({$d=New-Object System.Windows.Forms.FolderBrowserDialog;$d.Description='Выберите папку с Rangers.exe';if($d.ShowDialog() -eq 'OK'){$script:pathBox.Text=$d.SelectedPath;Update-ExeStatus}})
$script:exeStatus=Create-Label 'EXE: выберите папку игры' 20 121 760 21 9 $false
$onlineStatus=Create-Label '' 20 143 760 21 9 $false
$refresh=New-Object System.Windows.Forms.Button;$refresh.Text='Обновить каталог';$refresh.SetBounds(796,127,184,30);$form.Controls.Add($refresh)
$list=New-Object System.Windows.Forms.FlowLayoutPanel
$list.SetBounds(20,169,960,412);$list.AutoScroll=$true;$list.WrapContents=$true;$list.FlowDirection='TopDown'
$list.BackColor=[System.Drawing.Color]::FromArgb(24,35,55)
$list.BorderStyle='FixedSingle';$form.Controls.Add($list)
$script:list=$list
$script:allowTest=New-Object System.Windows.Forms.CheckBox
$script:allowTest.Text='Показывать тестовые/ALPHA';$script:allowTest.SetBounds(20,588,254,29);$form.Controls.Add($script:allowTest)
$script:enableMods=New-Object System.Windows.Forms.CheckBox
$script:enableMods.Text='Сразу включить моды в игре';$script:enableMods.Checked=$true;$script:enableMods.SetBounds(278,588,266,29);$form.Controls.Add($script:enableMods)
[void](Create-Label 'Версия EXE определяется автоматически по хешу файла игры' 553 592 420 25 8 $false)
$script:allowTest.Add_CheckedChanged({Render-Items})
$script:state=Create-Label 'Готов к проверке каталога.' 20 622 950 20 9 $false
$script:progress=New-Object System.Windows.Forms.ProgressBar;$script:progress.SetBounds(20,648,780,22);$form.Controls.Add($script:progress)
$install=New-Object System.Windows.Forms.Button;$install.Text='СКАЧАТЬ И УСТАНОВИТЬ';$install.SetBounds(809,624,174,48)
$install.BackColor=[System.Drawing.Color]::FromArgb(23,108,163);$install.ForeColor=[System.Drawing.Color]::White
$install.FlatStyle='Flat';$form.Controls.Add($install)
[void](Create-Label 'FHD UpdateCheck обязателен 󢀢 настройки существующих модов сохраняются 󢀢 журнал: fhd_setup.log' 20 683 950 30 9 $false)
function Render-Items {
  if(-not $script:list){return}
  $script:list.SuspendLayout()
  $script:list.Controls.Clear();$script:checkboxes=@{}
  foreach($mod in $script:items) {
    if($mod.channel -eq 'test' -and -not $script:allowTest.Checked){continue}
    if($mod.variant -ne 'any' -and $mod.variant -ne $script:currentVariant){continue}
    $panel=New-Object System.Windows.Forms.Panel
    $panel.Size=New-Object System.Drawing.Size(445,98);$panel.Margin=New-Object System.Windows.Forms.Padding(7)
    $panel.BackColor=[System.Drawing.Color]::FromArgb(33,48,68)
    $check=New-Object System.Windows.Forms.CheckBox;$check.SetBounds(5,30,19,28)
    $check.Checked=$false
    if($mod.updater -eq $true) {$check.Checked=$true;$check.Enabled=$false}
    $panel.Controls.Add($check);$script:checkboxes[$mod.id]=$check
    $imageKey=([string]$mod.id -replace '__TEST$','')
    $imagePath=Join-Path (Join-Path $script:base 'images') ($imageKey+'.png')
    if(-not (Test-Path -LiteralPath $imagePath)) {
      $imageKey=$imageKey -replace '_(old|new)$',''
      $imagePath=Join-Path (Join-Path $script:base 'images') ($imageKey+'.png')
    }
    if(-not (Test-Path -LiteralPath $imagePath)) {$imagePath=Join-Path $script:base 'images\default.png'}
    if(Test-Path -LiteralPath $imagePath){
      $pb=New-Object System.Windows.Forms.PictureBox;$pb.SetBounds(30,10,100,75);$pb.SizeMode='Zoom'
      $pb.Image=[System.Drawing.Image]::FromFile($imagePath);$panel.Controls.Add($pb)
    }
    $name=New-Object System.Windows.Forms.Label;$name.Text=[string]$mod.title
    $name.Font=New-Object System.Drawing.Font('Segoe UI',10,[System.Drawing.FontStyle]::Bold)
    $name.SetBounds(140,8,295,24);$name.ForeColor=[System.Drawing.Color]::White;$panel.Controls.Add($name)
    $desc=New-Object System.Windows.Forms.Label;$desc.Text=[string]$mod.description
    $desc.SetBounds(140,33,295,37);$desc.Font=New-Object System.Drawing.Font('Segoe UI',8.2)
    $desc.ForeColor=[System.Drawing.Color]::FromArgb(185,200,220);$panel.Controls.Add($desc)
    $meta=New-Object System.Windows.Forms.Label
    $meta.Text=('Версия: '+[string]$mod.version+'   󢀢   '+[Math]::Round(([double]$mod.size / 1048576),1)+' МБ   󢀢   '+[string]$mod.channel)
    $meta.SetBounds(140,72,295,20);$meta.Font=New-Object System.Drawing.Font('Segoe UI',8)
    $meta.ForeColor=[System.Drawing.Color]::FromArgb(115,195,245);$panel.Controls.Add($meta)
    $script:list.Controls.Add($panel)
  }
  $script:list.ResumeLayout()
}
function Refresh-Catalog {
  try {
    Tick 'Проверка каталога...' 0
    $script:items=@(Read-Catalog)
    Render-Items
    $up=@($script:items | Where-Object {$_.updater -eq $true -or $_.id -in @('FHD_AutoUpdater','FHD_UpdateCheck','FHD_AutoUpdate')}).Count
    $where=if($script:online){'ОНЛАЙН 󢀔 последние версии'}else{'ОФЛАЙН-КАТАЛОГ 󢀔 новинки появятся после подключения онлайн-списка'}
    $onlineStatus.Text=$where+'  |  Модов: '+$script:items.Count+'  |  AutoUpdater: '+$(if($up){'есть'}else{'нет'})
    Tick 'Выберите моды. Для установки нужен обязательный FHD UpdateCheck / AutoUpdater.' 0
  } catch {Log ('Refresh error: '+$_.Exception.Message);Alert $_.Exception.Message}
}
$refresh.Add_Click({Refresh-Catalog})
$script:pathBox.Add_Leave({Update-ExeStatus})
$install.Add_Click({
  $install.Enabled=$false
  try {
    $chosen=New-Object System.Collections.ArrayList
    foreach($mod in $script:items) {
      if($script:checkboxes.ContainsKey($mod.id) -and $script:checkboxes[$mod.id].Checked) {[void]$chosen.Add($mod)}
    }
    Install-Selected @($chosen)
  } catch {Log ('Install error: '+$_.Exception.ToString());Alert ('Установка остановлена: '+$_.Exception.Message)}
  finally {$install.Enabled=$true}
})
Refresh-Catalog
Update-ExeStatus
[void]$form.ShowDialog()
