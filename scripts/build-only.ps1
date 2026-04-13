$ErrorActionPreference = "Stop"

Write-Host "==> Configure with CMake"
cmake -S . -B build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Build (Release)"
cmake --build build --config Release -j
exit $LASTEXITCODE
