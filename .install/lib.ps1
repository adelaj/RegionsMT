$msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\amd64\MSBuild.exe"
$lib="gsl"
$arch="Win32","x64"
$conf="Debug","Release"
Foreach ($i in $lib)
{
    Foreach ($j in $arch) 
    {
        if (Test-Path $i-$j) { rm $i-$j -Force -Recurse }
        New-Item -ItemType Directory -Force -Path $i-$j
        cd $i-$j
        cmake -D CMAKE_C_FLAGS_INIT="/GL" -D CMAKE_STATIC_LINKER_FLAGS_INIT="/LTCG" -D CMAKE_GENERATOR_PLATFORM=$j -G "Visual Studio 15" ../$i
        Foreach ($k in $conf) { & $msbuild "${i}.sln" /t:$i /p:Configuration=$k }
        cd ..
    }
}
