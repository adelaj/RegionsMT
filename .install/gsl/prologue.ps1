param ([boolean]$CLR = $False)
$lib = "gsl"
if ($CLR -And (Test-Path $lib)) { rm $lib -Force -Recurse }
if (-Not (Test-Path $lib)) { git clone "https://github.com/ampl/gsl" $lib }
