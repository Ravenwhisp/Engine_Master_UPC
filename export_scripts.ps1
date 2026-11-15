param(
    [Alias('Mode')]
    [ValidateSet('Auto', 'Release', 'GameRelease', 'Editor')]
    [string]$Configuration = 'Auto'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$output = Join-Path $root 'Engine_OUT'
$export = Join-Path $root 'export'
$buildMarker = Join-Path $output 'last_build_configuration.txt'
if (-not (Test-Path -LiteralPath $buildMarker)) {
    throw 'No build marker found. Build Release or GameRelease with the updated engine project before exporting.'
}
$lastBuild = (Get-Content -LiteralPath $buildMarker -TotalCount 1).Trim()
if ($lastBuild -notin @('Release', 'GameRelease')) { throw "Unsupported last build configuration: $lastBuild" }
if ($Configuration -eq 'Auto') { $Configuration = $lastBuild }
if ($Configuration -eq 'Editor') { $Configuration = 'Release' }
if ($Configuration -ne $lastBuild) {
    throw "Last build was $lastBuild, but $Configuration was requested. Build $Configuration before exporting."
}
$executableName = if ($Configuration -eq 'GameRelease') { 'BoundByDeath.exe' } else { 'Engine.exe' }
$importBaseName = if ($Configuration -eq 'GameRelease') { 'Engine_GameRelease' } else { 'Engine_Release' }
$executable = Join-Path $output $executableName
$runtimeDlls = @('WinPixEventRuntime.dll', 'SDL3.dll', 'avcodec-63.dll', 'avdevice-63.dll', 'avfilter-12.dll', 'avformat-63.dll', 'avutil-61.dll', 'swresample-7.dll', 'swscale-10.dll')
foreach ($required in @($executableName, "$importBaseName.lib", "$importBaseName.exp") + $runtimeDlls) {
    if (-not (Test-Path -LiteralPath (Join-Path $output $required))) {
        throw "Missing $required in Engine_OUT. Build $Configuration and restore its runtime dependencies before exporting."
    }
}
if (-not (Get-ChildItem -LiteralPath $output -Filter '*.cso' -File | Select-Object -First 1)) {
    throw 'No compiled shaders found in Engine_OUT. Build the shaders before exporting.'
}
Write-Output "Exporting $executableName"

Push-Location -LiteralPath $root
try {
if (Test-Path -LiteralPath $export) { Remove-Item -LiteralPath $export -Recurse -Force }
New-Item -ItemType Directory -Path $export | Out-Null

Copy-Item -LiteralPath $executable -Destination $export
# Gameplay scripts still link against the engine's import library.
Copy-Item -LiteralPath (Join-Path $output "$importBaseName.lib") -Destination (Join-Path $export 'Engine.lib')
Copy-Item -LiteralPath (Join-Path $output "$importBaseName.exp") -Destination (Join-Path $export 'Engine.exp')
$symbols = Join-Path $output ($executableName -replace '\.exe$', '.pdb')
if (Test-Path -LiteralPath $symbols) { Copy-Item -LiteralPath $symbols -Destination $export }
cp .\Engine_OUT\*.cso .\export
foreach ($dll in $runtimeDlls) {
    Copy-Item -LiteralPath (Join-Path $output $dll) -Destination $export
}

mkdir .\export\EngineDependencies

cp .\Engine_Master_UPC\Component.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ComponentType.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\EngineAPI.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\EngineAPI.inl .\export\EngineDependencies\
cp .\Engine_Master_UPC\GameObject.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Keyboard.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Layer.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Script.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ScriptAPI.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ComponentRef.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\FieldInfo.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Tag.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Transform.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\UID.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\IDebugDrawable.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\KeyCode.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ScriptMethodInfo.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ScriptMethodList.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\StateMachineScript.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\UISlider.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\IPointerEventHandler.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\PointerEventData.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\UIFill.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\FieldMacros.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\FieldHandler.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\FieldHandlerRegistry.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\NavMeshTypes.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ISerializable.h .\export\EngineDependencies\

cp .\Engine_Master_UPC\PrefabInstance.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\MD5Fwd.h .\export\EngineDependencies\

cp .\Engine_Master_UPC\FieldTypeTraits.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\Transform2D.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\UIRect.h .\export\EngineDependencies\

cp .\Engine_Master_UPC\Asset.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\AssetId.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\AssetReference.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\AssetType.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\DataContainer.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\DataContainerAPI.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\GenericTypeFactory.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\IArchive.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\IFieldContainer.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\ImportSettings.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\MD5.h .\export\EngineDependencies\
cp .\Engine_Master_UPC\UISheet.h .\export\EngineDependencies\
} finally {
    Pop-Location
}
