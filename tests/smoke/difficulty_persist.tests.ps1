#!/usr/bin/env pwsh
# Combined Pester + Trident: drive the live game through a Difficulty
# settings change and assert the resulting difficulty.cfg on disk.

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

Describe "difficulty.cfg persistence through live UI flow" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "Difficulty page toggle round-trips into difficulty.cfg" {
        $eph = New-EphemeralGamePaths
        try {
            $cfgPath = Join-Path $eph.UserDir "difficulty.cfg"
            $sqf = Join-Path $script:repoRoot "tests/integration/ui/options/game/new_difficulty_persist.test.sqf"
            $reloadSqf = Join-Path $script:repoRoot "tests/integration/ui/options/game/new_difficulty_reloaded_persist.sqf"

            $gameBinDir = Split-Path -Parent $script:gameExe
            & $script:triExe test $sqf --game-dir $gameBinDir --data-dir $script:gameDir --retries 3 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "tri scenario must pass"

            Test-Path $cfgPath | Should -BeTrue -Because "Difficulty page should write difficulty.cfg on close"
            $kv = Read-CfgFile $cfgPath
            $kv['diffFriendlyTag[]'] | Should -Be '0,0' -Because "Friendly Tag OFF should persist for active Cadet difficulty while Veteran stays at its original default"
            $kv['diffEnemyTag[]'] | Should -Be '1,0' -Because "Enemy Tag ON should persist for active Cadet difficulty while Veteran stays at its original default"

            & $script:triExe test $reloadSqf --game-dir $gameBinDir --data-dir $script:gameDir --retries 3 2>&1 | Out-Null
            $LASTEXITCODE | Should -Be 0 -Because "fresh game process should reload the saved active difficulty values"
        } finally {
            Remove-EphemeralGamePaths $eph
        }
    }
}
