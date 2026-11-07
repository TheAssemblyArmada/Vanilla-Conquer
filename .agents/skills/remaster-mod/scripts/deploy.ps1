param(
    [ValidateSet('TD', 'RA', 'Both')]
    [string]$Game = 'Both',
    [ValidateSet('RelWithDebInfo', 'Debug')]
    [string]$Config = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

function Assert-Pe32([string]$Path) {
    $fs = [System.IO.File]::OpenRead($Path)
    try {
        $br = New-Object System.IO.BinaryReader($fs)
        $fs.Position = 0x3C
        $pe = $br.ReadInt32()
        $fs.Position = $pe + 4
        $machine = $br.ReadUInt16()
    } finally {
        $fs.Dispose()
    }
    # 0x14c is IMAGE_FILE_MACHINE_I386. The Remastered Collection is a 32-bit process.
    if ($machine -ne 0x14c) {
        throw "Built DLL is not 32-bit ($Path, machine=0x$($machine.ToString('x'))). Delete build/remaster and deploy again so CMake picks the x86 toolchain."
    }
}

function Get-ModsRoot {
    $docs = [Environment]::GetFolderPath('MyDocuments')
    $candidates = @(
        (Join-Path $docs 'CnCRemastered\Mods'),
        (Join-Path $docs 'CnCRemastered\CnCRemastered\Mods')
    )
    foreach ($candidate in $candidates) {
        if ((Test-Path (Join-Path $candidate 'Tiberian_Dawn')) -or (Test-Path (Join-Path $candidate 'Red_Alert'))) {
            return $candidate
        }
    }
    throw "CnC Remastered mods folder not found under $docs. Launch Tiberian Dawn and Red Alert once so the game creates Mods\Tiberian_Dawn and Mods\Red_Alert."
}

$repo = (& git -C $PSScriptRoot rev-parse --show-toplevel).Trim()
if (-not $repo) {
    throw "Could not resolve the git root from $PSScriptRoot"
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install the Visual Studio 2022 C++ build tools."
}
$vsRoot = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvarsall.bat'
if (-not (Test-Path $vcvars)) {
    throw "vcvarsall.bat not found under $vsRoot"
}

$modsRoot = Get-ModsRoot
$td = if ($Game -eq 'RA') { 'OFF' } else { 'ON' }
$ra = if ($Game -eq 'TD') { 'OFF' } else { 'ON' }

Write-Host "Configuring $Game ($Config) with the x86 toolchain"
$configure = "call `"$vcvars`" x86 >nul && cd /d `"$repo`" && cmake -DBUILD_REMASTERTD=$td -DBUILD_REMASTERRA=$ra -DBUILD_VANILLATD=OFF -DBUILD_VANILLARA=OFF --preset remaster && cmake --build --preset remaster --config $Config"
cmd /c $configure
if ($LASTEXITCODE -ne 0) {
    throw "Remaster build failed with exit code $LASTEXITCODE"
}

$games = @()
if ($Game -ne 'RA') {
    $games += @{ Mod = 'Vanilla_TD'; Parent = 'Tiberian_Dawn'; Dll = 'TiberianDawn.dll' }
}
if ($Game -ne 'TD') {
    $games += @{ Mod = 'Vanilla_RA'; Parent = 'Red_Alert'; Dll = 'RedAlert.dll' }
}

foreach ($entry in $games) {
    $staged = Join-Path $repo "build\remaster\$($entry.Mod)"
    $stagedDll = Join-Path $staged "Data\$($entry.Dll)"
    if (-not (Test-Path $stagedDll)) {
        throw "Build did not produce $stagedDll"
    }
    Assert-Pe32 $stagedDll

    $pdbName = [System.IO.Path]::GetFileNameWithoutExtension($entry.Dll) + '.pdb'
    $pdb = Join-Path $repo "build\remaster\$Config\$pdbName"
    if (Test-Path $pdb) {
        Copy-Item $pdb (Join-Path $staged 'Data') -Force
    }

    $dest = Join-Path $modsRoot "$($entry.Parent)\$($entry.Mod)"
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    try {
        Copy-Item -Path (Join-Path $staged '*') -Destination $dest -Recurse -Force
    } catch {
        throw "Could not copy $($entry.Mod) to $dest. Close Command & Conquer Remastered and deploy again. $($_.Exception.Message)"
    }

    $deployed = Join-Path $dest "Data\$($entry.Dll)"
    $info = Get-Item $deployed
    Write-Host ("Deployed {0}: {1} ({2} bytes, {3:yyyy-MM-dd HH:mm:ss})" -f $entry.Mod, $deployed, $info.Length, $info.LastWriteTime)
}
