#!/usr/bin/env pwsh
# Pester smoke tests for the display.cfg eager-write boot dance.
#
# Mirrors audio_config.tests.ps1 1:1 — same four It blocks, same
# ephemeral-POSEIDON_USER_DIR pattern, same invariants (eager-write
# defaults, byte-identical preservation of valid file, normalize-but-
# don't-persist, empty-file tolerance).  Run:
#   Invoke-Pester -Container (New-PesterContainer -Path tests/smoke/display_config.tests.ps1 -Data @{ Preset = 'win-x64-clang-rwdi' }) -Output Detailed

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../cli/TestHelpers.psm1" -Force

    $script:repoRoot  = Get-RepoRoot $PSScriptRoot
    $script:gameDir   = Get-GameDataDir -RepoRoot $script:repoRoot
    $script:gameExe   = Get-GameExe -RepoRoot $script:repoRoot -Preset $Preset -Name PoseidonGame
    $script:hasAddons = $script:gameDir -and (Test-HasAddons $script:gameDir)
    $script:Skip = if (-not $script:gameExe) { "PoseidonGame not found" }
                   elseif (-not $script:hasAddons) { "dist/Game/Addons not found" }
                   else { "" }
}

Describe "display.cfg boot dance" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "creates display.cfg with defaults when file is missing (eager-write)" {
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"
            Test-Path $cfgPath | Should -BeFalse -Because "ephemeral dir starts empty"

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            Test-Path $cfgPath | Should -BeTrue -Because "missing display.cfg should be eager-written on boot"

            $kv = Read-CfgFile $cfgPath
            $kv['monitor']          | Should -Be '0'  -Because "primary monitor"
            $kv['windowMode']       | Should -Be '1'  -Because "Borderless = 1 (DisplayConfig::WindowMode)"
            $kv['resolutionWidth']  | Should -Be '0'  -Because "native sentinel"
            $kv['resolutionHeight'] | Should -Be '0'  -Because "native sentinel"
            $kv['refreshRate']      | Should -Be '0'  -Because "system default"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "preserves a valid pre-existing display.cfg verbatim across boot" {
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"
            # Borderless on monitor 0 with native resolution + system
            # refresh — guaranteed-valid on every machine.
            @"
monitor=0;
windowMode=1;
resolutionWidth=0;
resolutionHeight=0;
refreshRate=0;
"@ | Set-Content -Path $cfgPath -Encoding ASCII -NoNewline

            $before = (Get-FileHash $cfgPath).Hash

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $after = (Get-FileHash $cfgPath).Hash
            $after | Should -Be $before -Because "boot must not rewrite a valid display.cfg"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "does NOT persist normalization on boot — invalid monitor stays in file" {
        # Key invariant: a temporarily disconnected external monitor
        # must keep its remembered index in the file so the user gets
        # it back when they plug it in.  Runtime falls back to 0
        # (validated by DisplayConfig::Normalize unit tests) but the
        # on-disk record is untouched until the user explicitly saves
        # via Apply + Keep on the Display screen.
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"
            @"
monitor=99;
windowMode=1;
resolutionWidth=0;
resolutionHeight=0;
refreshRate=0;
"@ | Set-Content -Path $cfgPath -Encoding ASCII -NoNewline

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $logs = Get-JsonlLogs $output
            $normLog = Find-LogMessage $logs 'normalized invalid fields'
            $normLog | Should -Not -BeNullOrEmpty -Because "boot should log that normalization happened"

            $kv = Read-CfgFile $cfgPath
            $kv['monitor'] | Should -Be '99' -Because "phantom monitor index should remain in file pending user action"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "tolerates an empty display.cfg without crashing" {
        # An empty-but-existing file is a likely accidental state —
        # user opened it, accidentally cleared content, saved.  Boot
        # must not crash.  ParamFile treats this as "no keys" and
        # DisplayConfig keeps in-memory defaults.
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"
            New-Item -ItemType File -Path $cfgPath -Force | Out-Null

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
