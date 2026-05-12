@echo off
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\ReposVS\Engine_Master_UPC\GameScripts\GameScripts.vcxproj" "/p:Configuration=Debug" "/p:Platform=x64" "/p:SolutionDir=D:\ReposVS\Engine_Master_UPC\Engine_Master_UPC\\"
exit /b %ERRORLEVEL%
