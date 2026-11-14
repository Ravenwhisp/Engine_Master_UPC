param(
    [Alias('Mode')]
    [ValidateSet('Auto', 'Release', 'GameRelease', 'Editor')]
    [string]$Configuration = 'Auto'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$output = Join-Path $root 'Engine_OUT'
$export = Join-Path $root 'export'
$engineExe = Join-Path $output 'Engine.exe'
$gameExe = Join-Path $output 'BoundByDeath.exe'
if ($Configuration -eq 'Auto') {
    $available = @($engineExe, $gameExe) | Where-Object { Test-Path -LiteralPath $_ } |
        ForEach-Object { Get-Item -LiteralPath $_ } | Sort-Object LastWriteTime -Descending
    if ($available.Count -eq 0) { throw 'No engine executable found. Build Release or GameRelease before exporting.' }
    if ($available.Count -gt 1 -and $available[0].LastWriteTime -eq $available[1].LastWriteTime) {
        throw 'Both executables have the same timestamp. Specify -Configuration Release or GameRelease.'
    }
    $executableName = $available[0].Name
} else {
    $executableName = if ($Configuration -eq 'GameRelease') { 'BoundByDeath.exe' } else { 'Engine.exe' }
}
$executable = Join-Path $output $executableName
if (-not (Test-Path -LiteralPath $executable)) {
    throw "Missing $executableName. Build the $Configuration engine configuration before exporting."
}
Write-Output "Exporting $executableName"

Push-Location -LiteralPath $root
try {
if (Test-Path -LiteralPath $export) { Remove-Item -LiteralPath $export -Recurse -Force }
New-Item -ItemType Directory -Path $export | Out-Null

Copy-Item -LiteralPath $executable -Destination $export
# Gameplay scripts still link against the engine's import library.
Copy-Item -LiteralPath (Join-Path $output 'Engine.lib') -Destination $export
Copy-Item -LiteralPath (Join-Path $output 'Engine.exp') -Destination $export
$symbols = Join-Path $output ($executableName -replace '\.exe$', '.pdb')
if (Test-Path -LiteralPath $symbols) { Copy-Item -LiteralPath $symbols -Destination $export }
cp .\Engine_OUT\*.cso .\export
cp .\Engine_OUT\WinPixEventRuntime.dll .\export
cp .\Engine_OUT\SDL3.dll .\export

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
