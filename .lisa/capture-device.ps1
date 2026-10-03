param(
    [string]$AdbPath,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
if (-not $AdbPath) {
    $kernelAdbCommand = Get-Command adb -ErrorAction SilentlyContinue
    if ($kernelAdbCommand) { $AdbPath = $kernelAdbCommand.Source }
    else { $AdbPath = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe' }
}
if (-not (Test-Path -LiteralPath $AdbPath)) { throw 'ADB bulunamadı. -AdbPath ile platform-tools/adb.exe yolunu belirtin.' }
$kernelDeviceLines = & $AdbPath devices
if ($LASTEXITCODE -ne 0) { throw 'ADB cihaz listesi okunamadı.' }
$kernelDevices = @($kernelDeviceLines | Where-Object { $_ -match '^\S+\s+device$' })
if ($kernelDevices.Count -ne 1) { throw 'Tam olarak bir yetkili ADB cihazı bağlı olmalı.' }
$kernelDeviceSerial = ($kernelDevices[0] -split '\s+')[0]
function Read-KernelDevice {
    param([string[]]$Arguments)
    $kernelCommandOutput = & $AdbPath -s $kernelDeviceSerial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ADB okuma başarısız: $($Arguments[0])" }
    return ($kernelCommandOutput | Out-String).TrimEnd()
}
$kernelDeviceName = Read-KernelDevice -Arguments @('shell', 'getprop ro.product.device')
if ($kernelDeviceName -ne 'lisa') { throw 'Bu kayıt betiği lisa için hazırlanmıştır.' }
if (-not $OutputDirectory) {
    $kernelCaptureTime = [DateTimeOffset]::UtcNow.ToOffset([TimeSpan]::FromHours(3)).ToString('yyyyMMdd-HHmmss')
    $OutputDirectory = Join-Path $PSScriptRoot "private\captures\$kernelCaptureTime"
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$kernelReadCommands = [ordered]@{
    'kernel.txt' = 'uname -a; cat /proc/version; cat /proc/sys/kernel/tainted; getenforce'
    'rom.txt' = 'getprop ro.product.model; getprop ro.product.device; getprop ro.build.fingerprint; getprop ro.vendor.build.fingerprint; getprop ro.build.version.release; getprop ro.build.version.sdk'
    'runtime.txt' = 'cat /proc/modules; cat /proc/swaps; cat /sys/block/zram0/comp_algorithm'
    'dmesg.txt' = 'dmesg'
    'crash.txt' = 'logcat -b crash -d -t 500'
    'webview.txt' = 'dumpsys webviewupdate'
}
foreach ($kernelReadName in $kernelReadCommands.Keys) {
    try {
        Read-KernelDevice -Arguments @('shell', $kernelReadCommands[$kernelReadName]) |
            Set-Content -LiteralPath (Join-Path $OutputDirectory $kernelReadName) -Encoding utf8
    } catch {
        $_.Exception.Message | Set-Content -LiteralPath (Join-Path $OutputDirectory "$kernelReadName.error") -Encoding utf8
    }
}
$kernelConfigGzipPath = Join-Path $OutputDirectory 'config.gz'
& $AdbPath -s $kernelDeviceSerial pull /proc/config.gz $kernelConfigGzipPath
if ($LASTEXITCODE -eq 0) {
    $kernelConfigInput = [System.IO.File]::OpenRead($kernelConfigGzipPath)
    $kernelConfigDecoder = [System.IO.Compression.GZipStream]::new($kernelConfigInput, [System.IO.Compression.CompressionMode]::Decompress)
    $kernelConfigOutput = [System.IO.File]::Create((Join-Path $OutputDirectory 'kernel.config'))
    try { $kernelConfigDecoder.CopyTo($kernelConfigOutput) }
    finally { $kernelConfigOutput.Dispose(); $kernelConfigDecoder.Dispose(); $kernelConfigInput.Dispose() }
}
Write-Output "Cihaz değiştirilmeden kayıt alındı: $OutputDirectory"
Write-Output 'Ham kayıtlar kişisel bilgi içerebilir; private klasörü Git tarafından dışlanır.'
