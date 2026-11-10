Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Compilando Spet@term (C++20 / Direct2D / Win32 / WIC)    " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan

$compiler = "clang++"
$src = "src/main.cpp"
$res = "app.res"
$out = "spetaterm.exe"

# Stop any running instances to avoid LNK1104 file lock
taskkill /f /im spetaterm.exe 2>$null | Out-Null
Start-Sleep -Milliseconds 100

# Compile Windows resources (Application Icon: Spet Terminal)
if (Test-Path "src/app.rc") {
    & llvm-rc /fo $res src/app.rc
}

$cflags = @(
    "-std=c++20",
    "-O2",
    "-DUNICODE",
    "-D_UNICODE",
    "-DWIN32_LEAN_AND_MEAN",
    "-DNOMINMAX",
    "-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
    "-mwindows",
    "-Xlinker", "/subsystem:windows",
    "-Xlinker", "/entry:wWinMainCRTStartup",
    "-ld2d1",
    "-ldwrite",
    "-lwindowscodecs",
    "-lole32",
    "-luuid",
    "-luser32",
    "-lgdi32",
    "-lshell32",
    "-lshlwapi",
    "-lpsapi",
    "-ldwmapi",
    "-lcomdlg32",
    "-ladvapi32",
    "-ld3d11",
    "-ldxgi",
    "-ldcomp",
    "-lpdh",
    "-lmfplat",
    "-lmfreadwrite",
    "-lmfuuid",
    "-ld3dcompiler",
    "-lbcrypt",
    "-lrstrtmgr",
    "-lwinmm"
)

$cmdInputs = @($src)
if (Test-Path $res) {
    $cmdInputs += $res
}

$cmd = "$compiler $($cmdInputs -join ' ') -o $out $($cflags -join ' ')"
Write-Host "Executando: $cmd" -ForegroundColor Gray

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $compiler @cmdInputs -o $out @cflags
$exitCode = $LASTEXITCODE
$sw.Stop()

if ($exitCode -eq 0 -and (Test-Path $out)) {
    $bytes = (Get-Item $out).Length
    $mb = [math]::Round($bytes / 1MB, 2)
    Write-Host "`n[SUCESSO] '$out' gerado em $($sw.ElapsedMilliseconds) ms ($mb MB)!" -ForegroundColor Green
    Write-Host "Zero DLLs externas. 100% nativo do Windows." -ForegroundColor Cyan
} else {
    Write-Host "`n[ERRO] Falha na compilação. Código de saída: $exitCode" -ForegroundColor Red
    exit 1
}
