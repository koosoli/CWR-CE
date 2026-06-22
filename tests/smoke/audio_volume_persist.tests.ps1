#!/usr/bin/env pwsh
# Combined Pester + Trident: drive the live game through an Audio
# settings change and assert the resulting audio.cfg on disk.
#
# Pester sets up an ephemeral POSEIDON_USER_DIR.  Trident respects
# an inherited POSEIDON_USER_DIR (instead of creating its own
# tempdir) when the parent process supplies one — see
# engine/Trident/src/scenarios/integration.rs run_sqf_test.  The tri
# scenario opens Audio, calls triSetVolume to write runtime values,
# Esc-closes Audio so AudioPage::Unmount fires SaveConfig, and exits.
# Pester then reads the file and asserts the new values.
#
# Run:
#   Invoke-Pester -Container (New-PesterContainer
#       -Path tests/smoke/audio_volume_persist.tests.ps1
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

    function Read-AudioCfg {
        param([string]$Path)
        if (-not (Test-Path $Path)) { return $null }
        $kv = @{}
        foreach ($line in (Get-Content $Path -Raw) -split "`r?`n") {
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

Describe "audio.cfg persistence through live UI flow" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "volume changes from triSetVolume + Esc round-trip into audio.cfg" {
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"

            # Run tri with the inherited POSEIDON_USER_DIR.  tri honours
            # the parent's value (when set) instead of substituting its
            # own tempdir, so the file the game writes is observable here.
            $sqf = Join-Path $script:repoRoot "tests/integration/ui/options/audio/new_audio_volume_persist.test.sqf"
            # --retries 3 to absorb the pre-existing engine flake on
            # the options screen (access violation during long idle
            # sim — documented in production_options_capture.test.sqf
            # comments; same crash on master, not introduced here).
            & $script:triExe test $sqf --game-dir (Split-Path $script:gameExe -Parent) --data-dir $script:gameDir --retries 3 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "tri scenario must pass"

            Test-Path $audioCfg | Should -BeTrue -Because "Audio Unmount → SaveConfig should write the file"
            $kv = Read-AudioCfg $audioCfg
            $kv['musicVolume']   | Should -Be '33' -Because "triSetVolume music 33 set runtime; Unmount persisted it"
            $kv['effectsVolume'] | Should -Be '44'
            $kv['speechVolume']  | Should -Be '55'
            $kv['eaxEnabled']    | Should -Be '0' -Because "EAX not toggled by the test"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }

    It "subsequent boot reads the persisted values back into the runtime" {
        $eph = New-EphemeralGamePaths
        try {
            $audioCfg = Join-Path $eph.UserDir "audio.cfg"

            # Round-trip: write a known cfg, --check boot, and read the
            # file back to confirm boot did not stomp it (covered also
            # by audio_config.tests.ps1, but here we verify it survives
            # the same boot path tri uses).
            @"
musicVolume=22;
effectsVolume=33;
speechVolume=44;
eaxEnabled=1;
outputDevice="";
inputDevice="";
"@ | Set-Content -Path $audioCfg -Encoding ASCII -NoNewline

            $output = Invoke-GuiExe $script:gameExe -C $script:gameDir --window --check --log-format jsonl --render dummy
            $LASTEXITCODE | Should -Be 0

            $kv = Read-AudioCfg $audioCfg
            $kv['musicVolume']   | Should -Be '22'
            $kv['effectsVolume'] | Should -Be '33'
            $kv['speechVolume']  | Should -Be '44'
            $kv['eaxEnabled']    | Should -Be '1'
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
