param ([boolean]$CLR = $False)
$lib = "pthread-win32"
if ($CLR -And (Test-Path $lib)) { rm $lib -Force -Recurse }
if (-Not (Test-Path $lib)) { git clone "https://github.com/GerHobbelt/pthread-win32" $lib }
if (-Not (Test-Path (Join-Path $lib "CMakeLists.txt"))) { Copy-Item (Join-Path (Split-Path -parent $PSCommandPath) "CMakeLists.txt") $lib/ }