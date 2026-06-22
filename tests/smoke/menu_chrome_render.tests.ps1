#!/usr/bin/env pwsh
# Pester v5 — main-menu chrome regression check via --auto-screenshot.
#
# The "menu chrome disappears after ~1s" regression introduced by the
# modern-shadow Phase 2 (SelectVertexShader(VSShadow) routing in the
# IsShadow branch of ApplyPassState) only manifests after the engine
# accumulates real wall-clock frames against the menu intro mission's
# shadow-casting geometry.  triSimFrames advances simulation time but
# doesn't reproduce the GPU-state interaction; a 600-frame tri pump
# shows chrome unchanged in pixel-diff while a 2-second live launch
# shows the full chrome panel + items missing.
#
# Strategy: launch with --auto-screenshot 2s:<path>.  The engine runs
# the menu for 2 wall-clock seconds, captures its own framebuffer
# (clean PNG, no desktop window chrome), then exits.  We sample the
# right-column menu-panel region: in the working state it's near-black
# (panel background); in the regressed state the 3D backdrop bleeds
# through (sky/terrain colour ~150+ on B).
#
# Tolerance: max-channel < 120.  The working panel is a translucent
# dark overlay (~80 max channel).  The regression bleeds sky/horizon
# colour through (~150-200 max).  Threshold sits between with margin.

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../cli/TestHelpers.psm1" -Force
    Add-Type -AssemblyName System.Drawing

    $script:repoRoot  = Get-RepoRoot $PSScriptRoot
    $script:gameDir   = Get-GameDataDir -RepoRoot $script:repoRoot
    $script:gameExe   = Get-GameExe -RepoRoot $script:repoRoot -Preset $Preset -Name PoseidonGame
    $script:hasAddons = $script:gameDir -and (Test-HasAddons $script:gameDir)
    $script:Skip = if (-not $script:gameExe)        { "PoseidonGame not found" }
                   elseif (-not $script:hasAddons)   { "Game/Addons not found in $($script:gameDir)" }
                   elseif (-not ($IsWindows -or $env:OS -eq 'Windows_NT')) { "Windows-only (uses System.Drawing)" }
                   else { "" }
}

Describe "Main menu chrome renders after live boot" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "menu items panel is opaque (not bleeding 3D scene through)" {
        $shotPath = Join-Path ([System.IO.Path]::GetTempPath()) "menu-chrome-$(Get-Random).png"

        # --auto-screenshot 2s:<path> runs the menu for 2 wall-clock seconds,
        # captures, then exits. --window keeps GPU work off the desktop owner.
        & $script:gameExe -C $script:gameDir --window --no-splash --auto-screenshot "2s:$shotPath" 2>&1 | Out-Null
        $LASTEXITCODE | Should -Be 0 -Because "game must exit cleanly after auto-screenshot"
        Test-Path $shotPath | Should -Be $true -Because "auto-screenshot must produce the PNG"

        $img = [System.Drawing.Bitmap]::FromFile($shotPath)
        try {
            # Sample the right-column menu panel region.  In the working state
            # this region is the dark panel hosting the menu items; in the
            # regressed state the 3D backdrop (sky / horizon) bleeds through.
            $samples = @(
                @{ uX = 0.85; uY = 0.30; name = "panel right top" }
                @{ uX = 0.85; uY = 0.50; name = "panel right mid" }
                @{ uX = 0.85; uY = 0.70; name = "panel right bot" }
            )
            foreach ($s in $samples) {
                $x = [int]($img.Width * $s.uX)
                $y = [int]($img.Height * $s.uY)
                $px = $img.GetPixel($x, $y)
                $maxChan = [Math]::Max([Math]::Max($px.R, $px.G), $px.B)
                $maxChan | Should -BeLessThan 120 `
                    -Because "$($s.name) at uv=($($s.uX),$($s.uY))=($x,$y) should be the dark panel (~80 max channel) — got R=$($px.R) G=$($px.G) B=$($px.B); regression bleeds 3D scene through (~150+)"
            }
        } finally {
            $img.Dispose()
            Remove-Item $shotPath -ErrorAction SilentlyContinue
        }
    }
}
