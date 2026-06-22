#!/usr/bin/env pwsh
# Pester smoke tests for the audio.cfg eager-write boot dance.
#
# Each `It` runs PoseidonGame in --check mode against an ephemeral
# POSEIDON_USER_DIR so we can observe the file the engine writes
# without polluting the developer's real config.  Run:
#   Invoke-Pester -Container (New-PesterContainer -Path tests/smoke/audio_config.tests.ps1 -Data @{ Preset = 'win-x64-clang-rwdi' }) -Output Detailed

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../cli/TestHelpers.psm1" -Force

    $script:repoRoot = Get-RepoRoot $PSScriptRoot
    $script:gameDir  = Get-GameDataDir -RepoRoot $script:repoRoot
    $script:gameExe  = Get-GameExe -RepoRoot $script:repoRoot -Preset $Preset -Name PoseidonGame
    $script:hasAddons = $script:gameDir -and (Test-HasAddons $script:gameDir)
    $script:Skip = if (-not $script:gameExe) { "PoseidonGame not found" }
                   elseif (-not $script:hasAddons) { "dist/Game/Addons not found" }
                   else { "" }

    function Read-AudioCfg {
        param([string]$Path)
        if (-not (Test-Path $Path)) { return $null }
        $text = Get-Content $Path -Raw
        $kv = @{}
        foreach ($line in ($text -split "`r?`n")) {
            if ($line -match '^\s*(\w+)\s*=\s*(.*?);?\s*$') {
                $key = $matches[1]
                $val = $matches[2].Trim()
                if ($val -match '^"(.*)"$') { $val = $matches[1] }
                $kv[$key] = $val
            }
        }
        return $kv
    }
}

Describe "audio.cfg boot dance" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "creates audio.cfg with defaults when file is missing (eager-write)" {
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"
            Test-Path $audioCfg | Should -BeFalse -Because "ephemeral dir starts empty"

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            Test-Path $audioCfg | Should -BeTrue -Because "missing audio.cfg should be eager-written on boot"

            $kv = Read-AudioCfg $audioCfg
            $kv['musicVolume']   | Should -Be '80'
            $kv['effectsVolume'] | Should -Be '80'
            $kv['speechVolume']  | Should -Be '80'
            $kv['eaxEnabled']    | Should -Be '0' -Because "EAX defaults off"
            $kv.ContainsKey('outputDevice') | Should -BeTrue
            $kv['outputDevice']  | Should -BeNullOrEmpty -Because "system default"
            $kv.ContainsKey('inputDevice') | Should -BeTrue
            $kv['inputDevice']   | Should -BeNullOrEmpty -Because "system default"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "preserves a valid pre-existing audio.cfg verbatim across boot" {
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"
            # Volumes within range, devices empty (system default = always valid)
            @"
musicVolume=42;
effectsVolume=33;
speechVolume=99;
eaxEnabled=1;
outputDevice="";
inputDevice="";
"@ | Set-Content -Path $audioCfg -Encoding ASCII -NoNewline

            $before = (Get-FileHash $audioCfg).Hash

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $after = (Get-FileHash $audioCfg).Hash
            $after | Should -Be $before -Because "boot must not rewrite a valid audio.cfg"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "does NOT persist normalization on boot — invalid device stays in file" {
        # Key invariant: a temporarily unplugged device must keep its
        # remembered name in the file so the user gets it back when
        # they reconnect.  Runtime uses the default (validated by the
        # AudioConfig::Normalize unit tests) but the on-disk record is
        # untouched until AudioPage::Unmount writes a fresh save.
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"
            $phantom  = "Phantom Device that does not exist on this machine"
            @"
musicVolume=50;
effectsVolume=50;
speechVolume=50;
eaxEnabled=0;
outputDevice="$phantom";
inputDevice="";
"@ | Set-Content -Path $audioCfg -Encoding ASCII -NoNewline

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $logs = Get-JsonlLogs $output
            $normLog = Find-LogMessage $logs 'normalized invalid fields'
            $normLog | Should -Not -BeNullOrEmpty -Because "boot should log that normalization happened"

            $kv = Read-AudioCfg $audioCfg
            $kv['outputDevice'] | Should -Be $phantom -Because "phantom device should remain in file pending user action"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "tolerates an empty audio.cfg without crashing" {
        # An empty-but-existing file is the lower-friction edge case —
        # user opened it, accidentally cleared content, saved.  Boot
        # must not crash.  ParamFile treats this as "no keys" and
        # AudioConfig keeps in-memory defaults; on Save (next time the
        # user touches Audio settings) it would persist defaults back.
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"
            New-Item -ItemType File -Path $audioCfg -Force | Out-Null

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
