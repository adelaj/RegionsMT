# Script should be run as administrator
# script should be run from the 'Build' directory

cd RegionsMT/.MSVC/RegionsMT
rm -Force src
cmd /c mklink /d src ..\..\src