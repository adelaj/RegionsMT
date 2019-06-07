# Run sample:
#   lib.ps1 -ARCH "Win32 x64" -CFG "Debug Release"

param ([string]$ARCH = "x64", [string]$CFG = "Release", [boolean]$CLR = $False)
# $vsroot = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
# $msbuild = Join-Path $vsroot "MSBuild\Current\Bin\MSBuild.exe"
$lib = "gsl", "pthread-win32"
${cmake-flags-gsl} = ""
${cmake-flags-pthread-win32} = ""
foreach ($i in $lib)
{
    $path = Join-Path (Split-Path -parent $PSCommandPath) $i
    $prologue = Join-Path $path prologue.ps1
    echo $prologue
    if (Test-Path $prologue) { & $prologue -CLR ${CLR} }
    foreach ($j in $ARCH.Split(" ")) 
    {
        if ($CLR -And (Test-Path $i-$j)) { rm $i-$j -Force -Recurse }
        if (-Not (Test-Path $i-$j)) { New-Item -ItemType Directory -Force -Path $i-$j }
        cd $i-$j
        ${lib-flags} = (Get-Variable -Name ("cmake-flags-" + $i)).Value.Trim()
        cmake $(if (${lib-flags}) { ${lib-flags} + " " })-D CMAKE_C_FLAGS_INIT="/GL" -D CMAKE_STATIC_LINKER_FLAGS_INIT="/LTCG" -D CMAKE_GENERATOR_PLATFORM="$j" $(join-path .. $i)
        foreach ($k in $CFG.Split(" ")) 
        {     
            # & $msbuild "$i.sln" /t:$i /p:Configuration=$k /p:CharacterSet=Unicode 
            cmake --build . --target $i --config $k --verbose -- /p:CharacterSet=Unicode
        }
        cd ..
    }
    $epilogue = Join-Path $path epilogue.ps1
    if (Test-Path $epilogue) { & $epilogue -CLR "${CLR}" }
}