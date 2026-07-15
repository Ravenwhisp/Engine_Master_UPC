rm export -Recurse
mkdir export

cp .\Engine_OUT\Engine.* .\export
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
cp .\Engine_Master_UPC\AssetRef.h .\export\EngineDependencies\
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