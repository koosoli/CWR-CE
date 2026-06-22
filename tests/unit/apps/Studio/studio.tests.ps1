#!/usr/bin/env pwsh
# @tag gpu
# PoseidonStudio.Tests.ps1 - Pester tests for Studio CLI functionality
# Run with: Invoke-Pester -Container (New-PesterContainer -Path PoseidonStudio.Tests.ps1 -Data @{ Preset = 'linux-x64-clang' }) -Output Detailed

param(
    [Parameter(Mandatory)]
    [string]$Preset
)

BeforeAll {
    Import-Module "$PSScriptRoot/../../../cli/TestHelpers.psm1" -Force
    $ProjectRoot = Get-RepoRoot $PSScriptRoot

    $ExePath = Get-GameExe -RepoRoot $ProjectRoot -Preset $Preset -Name PoseidonStudio -SubDir studio

    if (-not $ExePath) {
        throw "PoseidonStudio not found. Build it first."
    }

    $TempDir = if ($IsWindows -or $env:OS -eq 'Windows_NT') { $env:TEMP } else { "/tmp" }

    # Test fixtures
    $FixturesPath = Join-Path $ProjectRoot "tests" "fixtures"
    $GameDir = Get-GameDataDir -RepoRoot $ProjectRoot

    $env:SDL_VIDEODRIVER = "offscreen"
}

Describe "PoseidonStudio CLI" {
    Context "Help and Version" {
        It "Shows help when --help is used" {
            $output = & $ExePath --help
            ($output -join "`n") | Should -Match "PoseidonStudio"
            ($output -join "`n") | Should -Match "content"
            ($output -join "`n") | Should -Match "headless"
            $LASTEXITCODE | Should -Be 0
        }

        It "Shows --open option in help" {
            $output = & $ExePath --help
            ($output -join "`n") | Should -Match "open"
        }
    }

    Context "Headless Mode" {
        It "Runs in headless mode without errors" {
            & $ExePath --headless 2>&1
            $LASTEXITCODE | Should -Be 0
        }

        It "Runs headless with fixture directory" {
            & $ExePath --headless -C $FixturesPath 2>&1
            $LASTEXITCODE | Should -Be 0
        }

        It "Runs headless with current directory (no -C)" {
            & $ExePath --headless 2>&1
            $LASTEXITCODE | Should -Be 0
        }
    }

    Context "Screenshot" {
        It "Captures screenshot in headless mode" {
            $screenshotFile = Join-Path $TempDir "studio_test_screenshot.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            $output = & $ExePath --headless -C $FixturesPath --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist
            (Get-Item $screenshotFile).Length | Should -BeGreaterThan 0

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }

        It "Screenshot with game dir shows files" {
            if (-not (Test-Path $GameDir)) {
                Set-ItResult -Skipped -Because "Game directory not found at $GameDir"
                return
            }

            $screenshotFile = Join-Path $TempDir "studio_game_screenshot.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            $output = & $ExePath --headless -C $GameDir --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }
    }

    Context "Content Directory" {
        It "Accepts -C flag for content directory" {
            & $ExePath --headless -C $FixturesPath 2>&1
            $LASTEXITCODE | Should -Be 0
        }

        It "Accepts --content flag for content directory" {
            & $ExePath --headless --content $FixturesPath 2>&1
            $LASTEXITCODE | Should -Be 0
        }

        It "Handles non-existent content directory gracefully" {
            & $ExePath --headless -C "/nonexistent/path/here" 2>&1
            $LASTEXITCODE | Should -Be 0
        }
    }

    Context "Open File" {
        It "Accepts --open flag" {
            $paaFile = Join-Path $FixturesPath "texture" "paa" "synthetic_dxt1.paa"
            if (-not (Test-Path $paaFile)) {
                Set-ItResult -Skipped -Because "PAA fixture not found"
                return
            }

            & $ExePath --headless -C $FixturesPath --open $paaFile 2>&1
            $LASTEXITCODE | Should -Be 0
        }

        It "Opens PAA texture and captures screenshot" {
            $paaFile = Join-Path $FixturesPath "texture" "paa" "synthetic_dxt1.paa"
            if (-not (Test-Path $paaFile)) {
                Set-ItResult -Skipped -Because "PAA fixture not found"
                return
            }

            $screenshotFile = Join-Path $TempDir "studio_paa_preview.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            & $ExePath --headless -C $FixturesPath --open $paaFile --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }

        It "Opens P3D model from PBO" {
            if (-not (Test-Path $GameDir)) {
                Set-ItResult -Skipped -Because "Game directory not found"
                return
            }

            $screenshotFile = Join-Path $TempDir "studio_p3d_preview.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            & $ExePath --headless -C $GameDir --open "AddOns/6G30.pbo/6g30.p3d" --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }
        It "Opens FXY font file" {
            $fxyFile = Join-Path $FixturesPath "font" "legacy.fxy"
            if (-not (Test-Path $fxyFile)) {
                Set-ItResult -Skipped -Because "Font fixture not found"
                return
            }

            $screenshotFile = Join-Path $TempDir "studio_font_preview.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            & $ExePath --headless -C $FixturesPath --open $fxyFile --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }

        It "Opens FXY font from PBO" {
            $fontPbo = Join-Path $FixturesPath "studio" "Addons" "font_fixture.pbo"
            if (-not (Test-Path $fontPbo)) {
                Set-ItResult -Skipped -Because "Font fixture PBO not found"
            }
            $screenshotFile = Join-Path $TempDir "studio_font_pbo_preview.bmp"
            if (Test-Path $screenshotFile) { Remove-Item $screenshotFile }

            & $ExePath --headless -C $FixturesPath --open "studio/Addons/font_fixture.pbo/font/legacy.fxy" --screenshot $screenshotFile 2>&1
            $LASTEXITCODE | Should -Be 0
            $screenshotFile | Should -Exist

            Remove-Item $screenshotFile -ErrorAction SilentlyContinue
        }
    }
}
