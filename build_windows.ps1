# Build Devotion on Windows via MSYS2 MINGW64.
# Important: mingw64 must be on the PATH.
# Optional: copy the built PK3 into the local test install (test\devotion\).
param(
    [switch]$Deploy,
    [switch]$NoBuild,
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot

function Deploy-TestPk3 {
    $BuildDir = Join-Path $RepoRoot "build"
    $ModDir = Join-Path $RepoRoot "test\devotion"

    if (-not (Test-Path $ModDir)) {
        Write-Error "Test install not found at $ModDir"
    }

    $pk3 = Get-ChildItem -Path $BuildDir -Filter "devotion-*.pk3" -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $pk3) {
        Write-Error "No devotion-*.pk3 in $BuildDir - run build first (omit -NoBuild)."
    }

    Get-ChildItem -Path $ModDir -Filter "devotion-*.pk3" -File | ForEach-Object {
        if ($_.FullName -ne (Join-Path $ModDir $pk3.Name)) {
            Remove-Item -LiteralPath $_.FullName -Force
            Write-Host "Removed old $($_.Name)"
        }
    }

    $dest = Join-Path $ModDir $pk3.Name
    Copy-Item -LiteralPath $pk3.FullName -Destination $dest -Force
    Write-Host ('Deployed {0} to {1}' -f $pk3.Name, $dest)
}

Push-Location $RepoRoot
try {
    if (-not $NoBuild) {
        $quietFlag = if ($Quiet) { "QUIET=1" } else { "" }
        $makeArgs = if ($quietFlag) {
            "make clean $quietFlag;make $quietFlag"
        } else {
            "make clean;make"
        }
        # Compiler warnings go to stderr; merge streams without NativeCommandError noise.
        $prevEap = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            msys2_shell.cmd -mingw64 -defterm -no-start -here -c $makeArgs 2>&1 |
                ForEach-Object {
                    if ($_ -is [System.Management.Automation.ErrorRecord]) {
                        $_.ToString()
                    } else {
                        $_
                    }
                }
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        finally {
            $ErrorActionPreference = $prevEap
        }
        if ($Quiet) {
            Write-Host "Build succeeded."
        }
    }

    if ($Deploy) {
        Deploy-TestPk3
    }
}
finally {
    Pop-Location
}
