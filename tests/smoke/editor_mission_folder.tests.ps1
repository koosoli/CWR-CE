#!/usr/bin/env pwsh
# @tag gpu
#
# Combined Pester + Trident: drive the in-game mission editor through a Save,
# then assert the resulting mission.sqm landed in the env-configured user-content
# folder (POSEIDON_USER_CONTENT_DIR) rather than the roaming per-profile UserDir.
#
# This is the on-disk half of the editor save->list round-trip; the tri scenario
# (tests/integration/ui/editor/editor_mission_save_load.test.sqf) proves the
# editor LISTS the saved mission back (reads the same folder). Here we set
# POSEIDON_USER_CONTENT_DIR to an ephemeral dir, run that same scenario, and
# confirm <content>/missions/<name>.<world>/mission.sqm exists afterwards.
#
# tri honours an inherited POSEIDON_USER_DIR (so it does not spawn its own
# tempdir) and passes the rest of the environment through to the game, so
# POSEIDON_USER_CONTENT_DIR set here reaches the engine.
#
# Run:
#   Invoke-Pester -Container (New-PesterContainer
#       -Path tests/smoke/editor_mission_folder.tests.ps1
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

Describe "editor missions persist to the configured user-content folder" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "saving in the editor writes mission.sqm under POSEIDON_USER_CONTENT_DIR" {
        $eph = New-EphemeralGamePaths
        $content = Join-Path $eph.BaseDir "content"
        $oldContent = $env:POSEIDON_USER_CONTENT_DIR
        $env:POSEIDON_USER_CONTENT_DIR = $content
        try {
            # --retries 2 absorbs the pre-existing engine flake on long idle sim,
            # same as the other live-UI smoke tests. game-dir/data-dir are passed
            # explicitly so the wrapper is self-sufficient (doesn't depend on
            # CWR_*/OFPR_* env being pre-set by run-tests.ps1); the editor needs
            # the data dir to load an island landscape.
            $gameBinDir = Split-Path -Parent $script:gameExe
            $sqf = Join-Path $script:repoRoot "tests/integration/ui/editor/editor_mission_save_load.test.sqf"
            & $script:triExe test $sqf --game-dir $gameBinDir --data-dir $script:gameDir --retries 2 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "the editor save->list round-trip scenario must pass"

            $missionsDir = Join-Path $content "missions"
            Test-Path $missionsDir | Should -BeTrue -Because "the editor must create the missions folder under the content root"

            # Saved as <name>.<world>/ — world is whichever island the editor
            # defaulted to, so match on the name prefix.
            $saved = @(Get-ChildItem -Path $missionsDir -Filter "TriEditorRoundtrip.*" -Directory -ErrorAction SilentlyContinue)
            $saved.Count | Should -BeGreaterThan 0 -Because "the saved mission folder <name>.<world> must exist under the content root"
            Test-Path (Join-Path $saved[0].FullName "mission.sqm") | Should -BeTrue -Because "the editor must write mission.sqm into the saved mission folder"
        } finally {
            $env:POSEIDON_USER_CONTENT_DIR = $oldContent
            Remove-EphemeralGamePaths $eph
        }
    }
}
