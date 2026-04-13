$ErrorActionPreference = "Stop"

Write-Host "==> Configure with CMake"
cmake -S . -B build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Build (Release)"
cmake --build build --config Release -j
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exePath = ".\build\Release\csp.exe"
if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found: $exePath"
    exit 1
}

Write-Host "==> Run executable"
& $exePath
exit $LASTEXITCODE
