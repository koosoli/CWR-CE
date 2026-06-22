#!/usr/bin/env pwsh
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
    $script:Skip = if (-not $script:gameExe)  { "PoseidonGame not found" }
                   elseif (-not $script:hasAddons) { "dist/Game/Addons not found" }
                   else { "" }
}

function script:Connect-Harness {
    param(
        [Parameter(Mandatory)][int]$Port,
        [int]$TimeoutMs = 30000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $client = [System.Net.Sockets.TcpClient]::new()
        try {
            $client.Connect('127.0.0.1', $Port)
            $stream = $client.GetStream()
            $stream.ReadTimeout = 10000
            $stream.WriteTimeout = 10000
            return @{
                Client = $client
                Reader = [System.IO.StreamReader]::new($stream)
                Writer = [System.IO.StreamWriter]::new($stream)
            }
        } catch {
            $client.Dispose()
            Start-Sleep -Milliseconds 200
        }
    }

    throw "Failed to connect to harness on port $Port"
}

function script:Read-HarnessMessage {
    param(
        [Parameter(Mandatory)][hashtable]$Connection,
        [int]$TimeoutMs = 10000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $Connection.Reader.ReadLine()
            if (-not [string]::IsNullOrWhiteSpace($line)) {
                return $line | ConvertFrom-Json
            }
        } catch [System.IO.IOException] {
            Start-Sleep -Milliseconds 50
        }
    }

    throw "Timed out waiting for harness message"
}

function script:Invoke-HarnessCommand {
    param(
        [Parameter(Mandatory)][hashtable]$Connection,
        [Parameter(Mandatory)][hashtable]$Command,
        [int]$TimeoutMs = 10000
    )

    $Connection.Writer.WriteLine(($Command | ConvertTo-Json -Compress))
    $Connection.Writer.Flush()

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $msg = Read-HarnessMessage -Connection $Connection -TimeoutMs 1000
        if ($null -ne $msg.ok) {
            if (-not $msg.ok) {
                throw "Harness command failed: $($msg.error)"
            }
            return $msg
        }
    }

    throw "Timed out waiting for harness response"
}

function script:Invoke-SqfEval {
    param(
        [Parameter(Mandatory)][hashtable]$Connection,
        [Parameter(Mandatory)][string]$Code
    )

    $resp = Invoke-HarnessCommand -Connection $Connection -Command @{ cmd = 'eval'; code = $Code }
    $result = [string]$resp.result
    if ($result -match '^"(.*)"$') {
        return $matches[1]
    }
    return $result
}

function script:Invoke-SqfExec {
    param(
        [Parameter(Mandatory)][hashtable]$Connection,
        [Parameter(Mandatory)][string]$Code
    )

    Invoke-HarnessCommand -Connection $Connection -Command @{ cmd = 'exec'; code = $Code } | Out-Null
}

function script:Wait-HarnessEvent {
    # -Predicate is a superset kept signature-compatible with the same script:-scoped
    # helper in tests/cli/server.tests.ps1. Pester runs all containers in one runspace,
    # so these two definitions can shadow each other; matching signatures keeps either
    # file's calls valid whichever one wins.
    param(
        [Parameter(Mandatory)][hashtable]$Connection,
        [Parameter(Mandatory)][string]$Name,
        [scriptblock]$Predicate = { param($msg) $true },
        [int]$TimeoutMs = 30000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $msg = Read-HarnessMessage -Connection $Connection -TimeoutMs 1000
        if ($msg.event -eq $Name -and (& $Predicate $msg)) {
            return $msg
        }
    }

    throw "Timed out waiting for harness event '$Name'"
}

function script:Wait-Until {
    param(
        [Parameter(Mandatory)][scriptblock]$Condition,
        [string]$Description = "condition",
        [int]$TimeoutMs = 10000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $result = & $Condition
        if ($result) {
            return $result
        }
        Start-Sleep -Milliseconds 100
    }

    throw "Timed out waiting for $Description"
}

# NOTE: keep this signature/body identical to the copy in tests/cli/server.tests.ps1 —
# both are script:-scoped, so under the full Pester run (run-tests.ps1 loads every file)
# the last-defined one shadows the other in the shared runspace. -UserDir is OPTIONAL so
# either copy satisfies both callers (HARNESS_PORT= in stdout is the primary signal).
function script:Wait-HarnessPort {
    param(
        [Parameter(Mandatory)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory)][string]$StdoutPath,
        [string]$UserDir = '',
        [int]$TimeoutMs = 60000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        if (Test-Path $StdoutPath) {
            $raw = Get-Content $StdoutPath -Raw -ErrorAction SilentlyContinue
            if ($raw -match 'HARNESS_PORT=(\d+)') {
                return [int]$matches[1]
            }
        }

        if ($UserDir) {
            $latestLog = Get-ChildItem $UserDir -Filter '*.log' -File -ErrorAction SilentlyContinue |
                         Sort-Object LastWriteTime -Descending |
                         Select-Object -First 1
            if ($latestLog) {
                $logRaw = Get-Content $latestLog.FullName -Raw -ErrorAction SilentlyContinue
                if ($logRaw -match '\[Harness\] Listening on localhost:(\d+)') {
                    return [int]$matches[1]
                }
            }
        }

        if ($Process.HasExited) {
            throw "Game exited before announcing a harness port (stdout: $StdoutPath)"
        }
        Start-Sleep -Milliseconds 250
    }

    throw "Timed out waiting for harness port (stdout: $StdoutPath)"
}

function script:Start-HarnessGame {
    # Launch with --harness 0 so the OS assigns a free port, read the announced
    # HARNESS_PORT from stdout, then connect. A fixed random --harness port can
    # land on one still in TIME_WAIT from a recent test under the long sequential
    # suite, leaving the game unable to bind the harness ("Failed to connect").
    param(
        [Parameter(Mandatory)][hashtable]$Eph,
        [Parameter(Mandatory)][string[]]$GameArgs
    )

    $stdout = Join-Path $Eph.TempDir ("harness-" + [System.IO.Path]::GetRandomFileName() + ".out")
    $allArgs = @('--harness', '0') + $GameArgs
    $proc = Start-Process -FilePath $script:gameExe -ArgumentList $allArgs -PassThru -NoNewWindow -RedirectStandardOutput $stdout
    $port = Wait-HarnessPort -Process $proc -StdoutPath $stdout
    $conn = Connect-Harness -Port $port
    return @{ Proc = $proc; Conn = $conn; Port = $port }
}

Describe "Display mode apply/revert" {
    BeforeEach {
        if ($script:Skip -ne "") { Set-ItResult -Skipped -Because $script:Skip }
    }

    It "applies Exclusive 800x600 and Revert restores the original windowed size" {
        $eph = New-EphemeralGamePaths
        $conn = $null
        $proc = $null
        try {
            $game = Start-HarnessGame -Eph $eph -GameArgs @('-C', $script:gameDir, '--window', '--no-splash', '--log-level', 'off')
            $proc = $game.Proc
            $conn = $game.Conn
            Wait-HarnessEvent -Connection $conn -Name 'ready' | Out-Null

            Invoke-SqfExec -Connection $conn -Code 'triSetLanguage "English";'
            $startMode = Invoke-SqfEval -Connection $conn -Code 'triGetWindowMode'
            $startSize = Invoke-SqfEval -Connection $conn -Code 'triGetWindowSize'
            $startMode | Should -Be 'windowed'

            Invoke-SqfEval -Connection $conn -Code 'triClickText "OPTIONS"' | Should -Be 'OK'
            Wait-Until -Description 'Options shell' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triAssertDisplay 9099') -eq 'OK') { return $true }
                $null
            } | Out-Null

            Invoke-SqfEval -Connection $conn -Code 'triClick 1102' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            Wait-Until -Description 'Display page' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triControlText 510') -eq 'Window Mode') { return $true }
                $null
            } | Out-Null

            (Invoke-SqfEval -Connection $conn -Code 'triControlText 511') | Should -Be 'Window'

            Invoke-SqfEval -Connection $conn -Code 'triClick 518' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 2' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 511') | Should -Match 'Exclusive'

            for ($i = 0; $i -lt 80; $i++) {
                $resolution = Invoke-SqfEval -Connection $conn -Code 'triControlText 521'
                if ($resolution -eq '800x600') {
                    break
                }
                Invoke-SqfEval -Connection $conn -Code 'triClick 529' | Should -Be 'true'
                Invoke-SqfEval -Connection $conn -Code 'triSimFrames 1' | Out-Null
            }
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 521') | Should -Be '800x600'

            Invoke-SqfEval -Connection $conn -Code 'triClick 563' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triAssertText "Keep these display settings?"') | Should -Be 'OK'
            (Invoke-SqfEval -Connection $conn -Code 'triGetWindowMode') | Should -Be 'fullscreen'
            (Invoke-SqfEval -Connection $conn -Code 'triGetWindowSize') | Should -Be '800x600'
            $requestedMode = Invoke-SqfEval -Connection $conn -Code 'triGetRequestedFullscreenMode'
            $currentMode = Invoke-SqfEval -Connection $conn -Code 'triGetCurrentDisplayMode'
            $desktopMode = Invoke-SqfEval -Connection $conn -Code 'triGetDesktopDisplayMode'
            $requestedMode | Should -Match '^800x600@[0-9]+$'
            (($currentMode -eq $requestedMode) -or ($currentMode -eq $desktopMode)) | Should -BeTrue

            Invoke-SqfEval -Connection $conn -Code 'triClick 9202' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 10' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triGetWindowMode') | Should -Be 'windowed'
            (Invoke-SqfEval -Connection $conn -Code 'triGetWindowSize') | Should -Be $startSize
        } finally {
            if ($conn) {
                try {
                    Invoke-HarnessCommand -Connection $conn -Command @{ cmd = 'exit' } -TimeoutMs 3000 | Out-Null
                } catch {}
                try { $conn.Writer.Dispose() } catch {}
                try { $conn.Reader.Dispose() } catch {}
                try { $conn.Client.Dispose() } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                try {
                    $proc.WaitForExit(5000) | Out-Null
                } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                Stop-Process -Id $proc.Id -Force
            }
            Remove-EphemeralGamePaths $eph
        }
    }

    It "enables aspect handling rows and fills a 16:9 viewport edge to edge" {
        $eph = New-EphemeralGamePaths
        $conn = $null
        $proc = $null
        try {
            $game = Start-HarnessGame -Eph $eph -GameArgs @('-C', $script:gameDir, '--window', '--width', '1280', '--height', '720', '--no-splash', '--log-level', 'off')
            $proc = $game.Proc
            $conn = $game.Conn
            Wait-HarnessEvent -Connection $conn -Name 'ready' | Out-Null

            Invoke-SqfExec -Connection $conn -Code 'triSetLanguage "English";'

            Invoke-SqfEval -Connection $conn -Code 'triClickText "OPTIONS"' | Should -Be 'OK'
            Wait-Until -Description 'Options shell' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triAssertDisplay 9099') -eq 'OK') { return $true }
                $null
            } | Out-Null

            Invoke-SqfEval -Connection $conn -Code 'triClick 1102' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            Wait-Until -Description 'Display page' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triControlText 540') -eq 'Aspect Handling') { return $true }
                $null
            } | Out-Null

            # Aspect handling is selectable. Adaptive starts with the 21:9 HUD
            # width limit; Original stretches the HUD and disables limit changes.
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 541') | Should -Be 'Adaptive'
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 551') | Should -Be '21:9'
            Invoke-SqfEval -Connection $conn -Code 'triClick 549' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 2' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 541') | Should -Be 'Original (stretched HUD)'
            Invoke-SqfEval -Connection $conn -Code 'triClick 559' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 2' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 551') | Should -Be '21:9'

            # 16:9 uses the full viewport, undistorted: leftFOV=1.3333,
            # topFOV=0.75, UI region 0..1 (no pillarbox).  Broken state (a
            # persisted Classic value, or editable rows) pillarboxes the UI to
            # a centered 4:3 strip -> uiX=[0.1250..0.8750].
            (Invoke-SqfEval -Connection $conn -Code 'triGetAspectSettings') | Should -Be '1.3333,0.7500,0.0000,0.0000,1.0000,1.0000'
        } finally {
            if ($conn) {
                try {
                    Invoke-HarnessCommand -Connection $conn -Command @{ cmd = 'exit' } -TimeoutMs 3000 | Out-Null
                } catch {}
                try { $conn.Writer.Dispose() } catch {}
                try { $conn.Reader.Dispose() } catch {}
                try { $conn.Client.Dispose() } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                try {
                    $proc.WaitForExit(5000) | Out-Null
                } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                Stop-Process -Id $proc.Id -Force
            }
            Remove-EphemeralGamePaths $eph
        }
    }

    It "reselecting Native updates the requested fullscreen mode on a normal startup path" {
        $eph = New-EphemeralGamePaths
        $conn = $null
        $proc = $null
        try {
            $game = Start-HarnessGame -Eph $eph -GameArgs @('-C', $script:gameDir, '--no-splash', '--log-level', 'off')
            $proc = $game.Proc
            $conn = $game.Conn
            Wait-HarnessEvent -Connection $conn -Name 'ready' | Out-Null

            Invoke-SqfExec -Connection $conn -Code 'triSetLanguage "English";'
            $desktopMode = Invoke-SqfEval -Connection $conn -Code 'triGetDesktopDisplayMode'
            $desktopSize = $desktopMode -replace '@.*$', ''

            Invoke-SqfEval -Connection $conn -Code 'triClickText "OPTIONS"' | Should -Be 'OK'
            Wait-Until -Description 'Options shell' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triAssertDisplay 9099') -eq 'OK') { return $true }
                $null
            } | Out-Null

            Invoke-SqfEval -Connection $conn -Code 'triClick 1102' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            Wait-Until -Description 'Display page' -Condition {
                if ((Invoke-SqfEval -Connection $conn -Code 'triControlText 510') -eq 'Window Mode') { return $true }
                $null
            } | Out-Null

            $nativeLabel = Invoke-SqfEval -Connection $conn -Code 'triControlText 521'
            for ($i = 0; $i -lt 3; $i++) {
                if ((Invoke-SqfEval -Connection $conn -Code 'triControlText 511') -match 'Exclusive') { break }
                Invoke-SqfEval -Connection $conn -Code 'triClick 519' | Should -Be 'true'
                Invoke-SqfEval -Connection $conn -Code 'triSimFrames 2' | Out-Null
            }
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 511') | Should -Match 'Exclusive'

            $pickedSteps = 0
            for ($i = 1; $i -lt 80; $i++) {
                Invoke-SqfEval -Connection $conn -Code 'triClick 529' | Should -Be 'true'
                Invoke-SqfEval -Connection $conn -Code 'triSimFrames 1' | Out-Null
                $candidate = Invoke-SqfEval -Connection $conn -Code 'triControlText 521'
                if ($candidate -ne $nativeLabel) {
                    $pickedSteps = $i
                    break
                }
            }
            $pickedSteps | Should -BeGreaterThan 0

            Invoke-SqfEval -Connection $conn -Code 'triClick 563' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triAssertText "Keep these display settings?"') | Should -Be 'OK'
            Invoke-SqfEval -Connection $conn -Code 'triClick 9201' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 10' | Out-Null

            for ($i = 0; $i -lt $pickedSteps; $i++) {
                Invoke-SqfEval -Connection $conn -Code 'triClick 528' | Should -Be 'true'
                Invoke-SqfEval -Connection $conn -Code 'triSimFrames 1' | Out-Null
            }
            (Invoke-SqfEval -Connection $conn -Code 'triControlText 521') | Should -Be $nativeLabel

            Invoke-SqfEval -Connection $conn -Code 'triClick 563' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 5' | Out-Null
            (Invoke-SqfEval -Connection $conn -Code 'triAssertText "Keep these display settings?"') | Should -Be 'OK'
            Invoke-SqfEval -Connection $conn -Code 'triClick 9201' | Should -Be 'true'
            Invoke-SqfEval -Connection $conn -Code 'triSimFrames 15' | Out-Null

            # Broken state before the fix on normal startup:
            # current display mode returned to desktop, but requested fullscreen mode stayed on the previous non-native mode.
            (Invoke-SqfEval -Connection $conn -Code 'triGetCurrentDisplayMode') | Should -Be $desktopMode
            (Invoke-SqfEval -Connection $conn -Code 'triGetRequestedFullscreenMode') | Should -Be $desktopMode
            (Invoke-SqfEval -Connection $conn -Code 'triGetWindowSize') | Should -Be $desktopSize
        } finally {
            if ($conn) {
                try {
                    Invoke-HarnessCommand -Connection $conn -Command @{ cmd = 'exit' } -TimeoutMs 3000 | Out-Null
                } catch {}
                try { $conn.Writer.Dispose() } catch {}
                try { $conn.Reader.Dispose() } catch {}
                try { $conn.Client.Dispose() } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                try {
                    $proc.WaitForExit(5000) | Out-Null
                } catch {}
            }
            if ($proc -and -not $proc.HasExited) {
                Stop-Process -Id $proc.Id -Force
            }
            Remove-EphemeralGamePaths $eph
        }
    }
}
