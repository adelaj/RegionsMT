param ([boolean]$CLR = $False, [string]$SRC = ".")
$lib = Join-Path "$SRC" "gsl"
if ($CLR -And (Test-Path $lib)) { rm $lib -Force -Recurse }
if (-Not (Test-Path $lib)) { git clone --depth 1 "https://github.com/ampl/gsl" $lib }
