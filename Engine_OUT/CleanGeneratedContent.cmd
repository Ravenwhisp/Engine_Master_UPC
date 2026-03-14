@echo off
echo Limpiando proyecto...

REM Borrar carpeta Library completa
if exist "Library" (
echo Eliminando carpeta Library...
rmdir /s /q "Library"
)

echo Limpieza completada.
pause
