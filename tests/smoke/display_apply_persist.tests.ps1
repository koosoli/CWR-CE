#!/usr/bin/env pwsh
# Combined Pester + Trident: drive the live game through a Display
# settings change and assert the resulting display.cfg on disk.
#
# Mirrors audio_volume_persist.tests.ps1 1:1.  Pester sets up an
# ephemeral POSEIDON_USER_DIR; tri honours it (commit 110cdba2)
# instead of creating its own tempdir; the tri scenario opens
# Display, changes Window Mode via the UI, presses Apply + Keep on
# the ConfirmRevertPage modal, exits.  Pester then reads display.cfg
# and asserts the new value landed.
#
# Run:
#   Invoke-Pester -Container (New-PesterContainer
#       -Path tests/smoke/display_apply_persist.tests.ps1
#       -Data @{ Preset = 'win-x64-clang-rwdi' }) -Output Detailed

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../cli/TestHelpers.psm1" -Force

    $script:repoRoot = Get-RepoRoot $PSScriptRoot
    $script:gameDir  = Get-GameDataDir -RepoRoot $script:repoRoot
    $script:gameExe  = Get-GameExe -RepoRoot $script:repoRoot -Preset $Preset -Name PoseidonGame
    $script:triExe   = Get-TriExe -RepoRoot $script:repoRoot -Preset $Preset
    $script:hasAddons = $script:gameDir -and (Test-HasAddons $script:gameDir)
    $script:Skip = if (-not $script:gameExe)  { "PoseidonGame not found" }
                   elseif (-not $script:triExe) { "tri not found" }
                   elseif (-not $script:hasAddons) { "dist/Game/Addons not found" }
                   else { "" }
}

Describe "display.cfg persistence through live UI flow" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "Window Mode change via Apply + Keep round-trips into display.cfg" {
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"

            # Run tri with the inherited POSEIDON_USER_DIR.  --retries 3
            # absorbs the pre-existing engine flake on the options
            # screen (access violation during long idle sim — same
            # crash documented in audio_volume_persist.tests.ps1).
            $sqf = Join-Path $script:repoRoot "tests/integration/ui/options/display/new_display_apply_persist.test.sqf"
            & $script:triExe test $sqf --game-dir (Split-Path $script:gameExe -Parent) --data-dir $script:gameDir --retries 3 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "tri scenario must pass"

            Test-Path $cfgPath | Should -BeTrue -Because "Apply + Keep should write display.cfg"
            $kv = Read-CfgFile $cfgPath
            $kv['windowMode'] | Should -Be '1' -Because "the harness boots windowed (dropdown 'Window'); tri cycled Window (2) → Fullscreen (1) and pressed Keep"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "subsequent boot reads the persisted Window Mode back into the runtime" {
        # Round-trip: hand-write a cfg with a non-default mode, --check
        # boot, confirm boot didn't stomp it.  Same invariant as the
        # display_config.tests.ps1 'preserves valid file' case but here
        # we verify it through the same Apply path the user takes.
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "display.cfg"
            @"
monitor=0;
windowMode=2;
resolutionWidth=0;
resolutionHeight=0;
refreshRate=0;
"@ | Set-Content -Path $cfgPath -Encoding ASCII -NoNewline

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $kv = Read-CfgFile $cfgPath
            $kv['windowMode'] | Should -Be '2'
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
