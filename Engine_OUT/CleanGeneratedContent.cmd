@echo off
echo Limpiando proyecto...

REM Borrar carpeta Library completa
if exist "Library" (
echo Eliminando carpeta Library...
rmdir /s /q "Library"
)

REM Borrar archivos .metadata dentro de Assets recursivamente
if exist "Assets" (
echo Eliminando archivos .metadata en Assets...
for /r "Assets" %%f in (*.metadata) do (
del /q "%%f"
)
)

echo Limpieza completada.
pause
