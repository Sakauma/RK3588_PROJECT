param(
    [string]$HostName = "192.168.1.100",
    [string]$User = "root",
    [string]$RemoteDir = "/root/rk3588_v7"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Target = "$User@$HostName"

function Run-Remote {
    param([string]$Command)
    ssh -o ConnectTimeout=8 $Target $Command
}

Write-Host "Checking SSH connection to $Target ..."
Run-Remote "echo SSH_OK && uname -a"

Write-Host "Preparing remote directory $RemoteDir ..."
Run-Remote "mkdir -p '$RemoteDir'"

Write-Host "Copying SDK files to RK3588 (overwrites existing files) ..."
scp -r `
    "$Root\README.md" `
    "$Root\docs" `
    "$Root\RK3588API" `
    "$Root\WinSim" `
    "$Root\cvgDrv.ko" `
    "$Root\libRK3588.so" `
    "$Root\libRK3588.so.1" `
    "$Root\libRK3588.so.1.0.0" `
    "$Root\rk3588.ini" `
    "$Root\rk3588.ini.example" `
    "${Target}:$RemoteDir/"

Write-Host "Installing cvgDrv.ko and checking PCIe visibility ..."
$remoteScript = @"
set -e
cd '$RemoteDir'
if ! lsmod | grep -q '^cvgDrv'; then
  insmod ./cvgDrv.ko
fi
echo '--- lsmod ---'
lsmod | grep cvgDrv
echo '--- module file ---'
modinfo ./cvgDrv.ko 2>/dev/null || true
echo '--- PCIe devices ---'
lspci -nn || true
echo '--- driver dmesg ---'
dmesg | tail -n 80 | grep -Ei 'cvg|pcie|pci|rk3588' || true
echo '--- device nodes ---'
ls -l /dev | grep -Ei 'cvg|rk|pcie' || true
"@
Run-Remote $remoteScript

Write-Host "Trying to rebuild RK3588Test on the board ..."
$buildScript = @"
set -e
cd '$RemoteDir/WinSim'
if command -v cmake >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
  rm -rf build_deploy
  cmake -S . -B build_deploy -DRK3588API_INCLUDE_DIR='$RemoteDir/RK3588API' -DRK3588_LIB_PATH='$RemoteDir/libRK3588.so'
  cmake --build build_deploy -j`$(nproc 2>/dev/null || echo 2)
  cp build_deploy/RK3588Test '$RemoteDir/RK3588Test'
  chmod +x '$RemoteDir/RK3588Test'
  echo 'Build OK: '$RemoteDir'/RK3588Test'
else
  echo 'cmake/make not found on RK3588; copied sources and runtime files only.'
fi
"@
Run-Remote $buildScript

Write-Host "Deployment finished."
