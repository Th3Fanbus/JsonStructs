#include "JsonStructBPLib.h"
#include "BPJsonObjectValue.h"
#include "UObject/Class.h"
#include "Engine/UserDefinedStruct.h"
#include "UObject/UObjectAllocator.h"




FString UJsonStructBPLib::RemoveUStructGuid(FString write)
{
	FString VariableName = write;
	FString Replace = "";
	FString Cutoff;
	VariableName.Split("_", &Replace, &Cutoff, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (Cutoff == "")
		return write;
	
	return Replace;
}


UClass* UJsonStructBPLib::FindClassByName(FString ClassNameInput) {
	// prevent crash from wrong user Input
	if (ClassNameInput.Len() == 0)
		return nullptr;

	const TCHAR* ClassName = *ClassNameInput;
	UObject* ClassPackage = ANY_PACKAGE;

	if (UClass* Result = FindObject<UClass>(ANY_PACKAGE, ClassName, false))
		return Result;

	if (UObjectRedirector* RenamedClassRedirector = FindObject<UObjectRedirector>(ANY_PACKAGE, ClassName, true))
		return CastChecked<UClass>(RenamedClassRedirector->DestinationObject);

	return nullptr;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct, bool includeObjects, TArray<UObject*>& RecursedObjects) {
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		obj->SetField(prop->GetName(), convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct),includeObjects, RecursedObjects));
	}
	return obj;
}

void UJsonStructBPLib::convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct,UObject* Outer) {
	if (!json)
		return;
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto field : json->Values) {
		FString FieldName = field.Key;
		auto prop = Struct->FindPropertyByName(*FieldName);
		if (prop) {
			convertJsonValueToFProperty(field.Value, prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct),Outer);
		}
	}
}


void UJsonStructBPLib::convertJsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* prop, void* ptrToProp, UObject* Outer) {
	if (auto strProp = Cast<FStrProperty>(prop)) {
		strProp->SetPropertyValue(ptrToProp, json->AsString());
	}
	else if (auto txtProp = Cast<FTextProperty>(prop)) {
		txtProp->SetPropertyValue(ptrToProp, FText::FromString(json->AsString()));
	}
	else if (auto nameProp = Cast<FNameProperty>(prop)) {
		nameProp->SetPropertyValue(ptrToProp, *json->AsString());
	}
	else if (auto fProp = Cast<FFloatProperty>(prop)) {
		fProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto iProp = Cast<FIntProperty>(prop)) {
		iProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto bProp = Cast<FBoolProperty>(prop)) {
		bProp->SetPropertyValue(ptrToProp, json->AsBool());
	}
	else if (auto eProp = Cast<FEnumProperty>(prop)) {

		if (json->Type == EJson::Object)
		{
			if (json->AsObject()->HasField("enum") && json->AsObject()->HasField("value"))
			{
				const FString KeyValue = json->AsObject()->TryGetField("enum")->AsString();
				if (FPackageName::IsValidObjectPath(*KeyValue)) {
					const UEnum* EnumClass = LoadObject<UEnum>(NULL, *KeyValue);
					if (EnumClass) {
						const FString Value = *json->AsObject()->TryGetField("value")->AsString();
						FNumericProperty* NumProp = Cast<FNumericProperty>(eProp->GetUnderlyingProperty());
						if (NumProp) {
							NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *FString::FromInt(int32(EnumClass->GetIndexByName(FName(Value), EGetByNameFlags::CheckAuthoredName))));
						}
					}
				}
			}
		}
		else if(json->Type == EJson::String)
		{
			FNumericProperty * NumProp = Cast<FNumericProperty>(eProp->GetUnderlyingProperty());
			if (NumProp) {
				NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *json->AsString());
			}
		}
	}
	else if (auto byteProp = Cast<FByteProperty>(prop)) {
		if (byteProp->IsEnum()) {
			if (json->Type == EJson::Object) {
				if (json->AsObject()->HasField("enum") && json->AsObject()->HasField("value")) {
					const FString keyvalue = json->AsObject()->TryGetField("enum")->AsString();
					if (FPackageName::IsValidObjectPath(keyvalue)) {
						UEnum* EnumClass = LoadObject<UEnum>(NULL, *keyvalue);
						if (EnumClass) {
							FNumericProperty* NumProp = Cast<FNumericProperty>(byteProp);
							NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *FString::FromInt(EnumClass->GetIndexByName(FName(*json->AsObject()->TryGetField("value")->AsString()), EGetByNameFlags::CheckAuthoredName)));
						}
					}
				}
			}
			else if(json->Type == EJson::String) {
				byteProp->SetNumericPropertyValueFromString(byteProp->ContainerPtrToValuePtr<void>(ptrToProp), *json->AsString());
			}
		}
		else
		{
			if (json->Type == EJson::Number) {
				byteProp->SetPropertyValue(ptrToProp, json->AsNumber());
			}
			else if (json->Type == EJson::String) {
				byteProp->SetNumericPropertyValueFromString(byteProp->ContainerPtrToValuePtr<void>(ptrToProp), *json->AsString());
			}
		}
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(prop)) {
		ddProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto numProp = Cast<FNumericProperty>(prop)) {
		numProp->SetNumericPropertyValueFromString(ptrToProp, *json->AsString());
	}
	else if (auto aProp = Cast<FArrayProperty>(prop)) {
		const TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
		FScriptArrayHelper helper(aProp, ptrToProp);
		helper.EmptyValues();	
		for (int i = 0; i < jsonArr.Num(); i++) {
			int64 valueIndex = helper.AddValue();
			convertJsonValueToFProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex),Outer);
		}
	}
	else if (auto mProp = Cast<FMapProperty>(prop)) {
		if (json->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			FScriptMapHelper MapHelper(mProp, ptrToProp);
			MapHelper.EmptyValues();
			for (int i = 0; i < jsonArr.Num(); i++) {
				const int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				uint8* mapPtr = MapHelper.GetPairPtr(Index);
				if (jsonArr[i]->AsObject()) {
					// Map Property
					if (jsonArr[i]->AsObject()->HasField("key") && jsonArr[i]->AsObject()->HasField("value")) {
						TSharedPtr<FJsonValue> keyvalue = jsonArr[i]->AsObject()->TryGetField("key");
						TSharedPtr<FJsonValue> valuevalue = jsonArr[i]->AsObject()->TryGetField("value");
						convertJsonValueToFProperty(keyvalue, mProp->KeyProp, mapPtr, Outer);
						convertJsonValueToFProperty(valuevalue, mProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset, Outer);
					}
				}
			}
			MapHelper.Rehash();
		}
	}
	else if (auto cProp = Cast<FClassProperty>(prop)) {
		if (json->Type == EJson::String) {
			if (FPackageName::IsValidObjectPath(*json->AsString())) {
				UObject* LoadedObject = LoadObject<UClass>(NULL, *json->AsString());
				// Failsafe for script classes with BP Stubs
				if (!LoadedObject) {
					FString Left; FString JsonString = json->AsString(); FString Right;
					JsonString.Split("/", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					Right.Split(".", &Left, &JsonString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					if (JsonString != "") {
						LoadedObject = FindClassByName(JsonString.Append("_C"));
						if(LoadedObject)
							cProp->SetPropertyValue(ptrToProp, LoadedObject);
						else
						{
							// same reverse
							LoadedObject = FindClassByName(JsonString);
							if (LoadedObject)
								cProp->SetPropertyValue(ptrToProp, LoadedObject);
							else
								UE_LOG(LogTemp, Warning, TEXT("FailSafe Class Loading failed"), *json->AsString());

						}
					}
				}
				else
					cProp->SetPropertyValue(ptrToProp, LoadedObject);
			}
		}
	}
	else if (auto uProp = Cast<FObjectProperty>(prop)) {
	
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("object") && json->AsObject()->HasField("class")) {
				const TSharedPtr<FJsonValue> Key = json->AsObject()->TryGetField("class");
				const TSharedPtr<FJsonValue> InnerValue = json->AsObject()->TryGetField("object");
				if (InnerValue->Type == EJson::Object && json->AsObject()->HasField("objectFlags") && json->AsObject()->HasField("objectName") && json->AsObject()->HasField("objectFlags") && json->AsObject()->HasField("objectOuter")) {
					if (FPackageName::IsValidObjectPath(*Key->AsString())) {

						FString ObjectName = json->AsObject()->TryGetField("objectName")->AsString();
						const TSharedPtr<FJsonValue> OuterValue = json->AsObject()->TryGetField("objectOuter");
						const EObjectFlags ObjectLoadFlags = (EObjectFlags)json->AsObject()->GetIntegerField((TEXT("objectFlags")));
						UClass* BaseClass = LoadObject<UClass>(NULL, *Key->AsString());
						UClass* OuterLoaded = LoadObject<UClass>(NULL, *OuterValue->AsString());
						UObject* ExistingProp = uProp->GetPropertyValue(ptrToProp);
						if (BaseClass && (!BaseClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_Intrinsic) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced))) {

							UObject* Template = UObject::GetArchetypeFromRequiredInfo(BaseClass, Outer ? Outer : OuterLoaded, *ObjectName, ObjectLoadFlags);
							UObject* Constructed = StaticConstructObject_Internal(BaseClass, Outer ? Outer : OuterLoaded ? OuterLoaded : BaseClass, *ObjectName, ObjectLoadFlags, EInternalObjectFlags::None, Template);
							convertJsonObjectToUStruct(InnerValue->AsObject(), BaseClass, Constructed, Outer ? Outer : OuterLoaded);
							uProp->SetObjectPropertyValue(ptrToProp, Constructed);
							
						}
					}
				}
				else if (InnerValue->Type == EJson::String) {
					if (InnerValue->AsString() != "") {
						UObject* uObj = LoadObject<UObject>(NULL, *InnerValue->AsString());
						uProp->SetPropertyValue(ptrToProp, uObj);
					}
				}
			}
		}
	}
	else if (auto sProp = Cast<FStructProperty>(prop)) {
		convertJsonObjectToUStruct(json->AsObject(), sProp->Struct, ptrToProp, Outer);
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(prop))
	{
		if (json->Type == EJson::String)
		{
			if (json->AsString() != "")
			{
				//UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
				//WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
			}
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		if (json->Type == EJson::String)
		{
			if (json->AsString() != "")
			{
				//UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
				//WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
			}
		}
	}
	else if (auto* CastProp = CastField<FMulticastSparseDelegateProperty>(prop))
	{

	}
	else
	{
	
	}
}


TSharedPtr<FJsonValue> UJsonStructBPLib::convertUPropToJsonValue(FProperty* prop, void* ptrToProp,bool includeObjects,TArray<UObject *>& RecursedObjects) {
	if (FStrProperty* strProp = Cast<FStrProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(strProp->GetPropertyValue(ptrToProp)));
	}
	if (FTextProperty* txtProp = Cast<FTextProperty>(prop)) {
			return TSharedPtr<FJsonValue>(new FJsonValueString(txtProp->GetPropertyValue(ptrToProp).ToString()));
	}
	if (FNameProperty* nameProp = Cast<FNameProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(nameProp->GetPropertyValue(ptrToProp).ToString()));
	}
	else if (FFloatProperty* fProp = Cast<FFloatProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(fProp->GetPropertyValue(ptrToProp)));
	}
	else if (FIntProperty* iProp = Cast<FIntProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(iProp->GetPropertyValue(ptrToProp)));
	}
	else if (FBoolProperty* bProp = Cast<FBoolProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueBoolean(bProp->GetPropertyValue(ptrToProp)));
	}
	else if (FEnumProperty* eProp = Cast<FEnumProperty>(prop)) {
		if (eProp->GetEnum() && eProp->IsValidLowLevel())
		{
			if (eProp->GetEnum()->GetClass() && eProp->GetEnum()->GetClass()->IsValidLowLevel())
			{
				UE_LOG(LogTemp, Error, TEXT("CppType : %s"), *eProp->GetEnum()->CppType);
				TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
				TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetPathName()));
				JsonObject->SetField("enum", key);
				TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetNameByValue(eProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ptrToProp)).ToString()));
				JsonObject->SetField("value", value);
				return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
			}
		}
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(ddProp->GetPropertyValue(ptrToProp)));
	}
	else if (FByteProperty* byProp = Cast<FByteProperty>(prop)) {
		if (byProp->IsEnum())
		{
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(byProp->Enum->GetPathName()));
			JsonObject->SetField("enum", key);
			TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(byProp->Enum->GetNameByValue(byProp->GetSignedIntPropertyValue(ptrToProp)).ToString()));
			JsonObject->SetField("value", value);
			return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
		}
		else
			return TSharedPtr<FJsonValue>(new FJsonValueNumber(byProp->GetUnsignedIntPropertyValue(ptrToProp)));
	}
	else if (FNumericProperty* nProp = Cast<FNumericProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(nProp->GetUnsignedIntPropertyValue(ptrToProp)));
	}
	else if (FArrayProperty* aProp = Cast<FArrayProperty>(prop)) {
		auto& arr = aProp->GetPropertyValue(ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(convertUPropToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize),includeObjects, RecursedObjects));
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FClassProperty* cProp = Cast<FClassProperty>(prop)) {
		if (cProp->GetPropertyValue(ptrToProp)->IsValidLowLevel())
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(cProp->GetPropertyValue(ptrToProp)->GetPathName()));
		}
		else 
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(""));

		}
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(prop))
	{
		//if (WeakObjectProperty->GetPropertyValue(ptrToProp).IsValid())
		//{
			//return TSharedPtr<FJsonValue>(new FJsonValueString(FSoftObjectPath(WeakObjectProperty->GetPropertyValue(ptrToProp).Get()).ToString()));

		//}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		//if (SoftObjectProperty->GetPropertyValue(ptrToProp).IsValid())
		//{
		//	return TSharedPtr<FJsonValue>(new FJsonValueString(SoftObjectProperty->GetPropertyValue(ptrToProp).ToSoftObjectPath().ToString()));

		//}
	}
	else if (FObjectProperty* oProp = Cast<FObjectProperty>(prop)) {
		if (oProp->GetPropertyValue(ptrToProp)->IsValidLowLevel())
		{
			UClass * BaseClass = oProp->GetPropertyValue(ptrToProp)->GetClass();

			
			TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(BaseClass->GetPathName()));
			TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(oProp->GetPropertyValue(ptrToProp)->GetPathName()));
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("class", key);
			UObject* ObjectValue = oProp->GetPropertyValue(ptrToProp);
			JsonObject->SetField("objectFlags", TSharedPtr<FJsonValue>(new FJsonValueNumber(int32(ObjectValue->GetFlags()))));
			JsonObject->SetField("objectName", TSharedPtr<FJsonValue>(new FJsonValueString(ObjectValue->GetName())));

			// Everything that is streamable or AssetUserData ( aka Meshs and Textures etc ) will never be serialized
			for (auto i : BaseClass->Interfaces)
			{
				if (i.Class == UInterface_AssetUserData::StaticClass() || i.Class == UStreamableRenderAsset::StaticClass())
				{
					JsonObject->SetField("object", value);
					JsonObject->SetField("type", TSharedPtr<FJsonValue>(new FJsonValueString("Streamable or Interface")));
					TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
					return TSharedPtr<FJsonValue>(Obj);
				}
			}

			// skip this if we dont need it 
			if (includeObjects && !RecursedObjects.Contains(ObjectValue) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_EditInlineNew)) {
				UE_LOG(LogTemp, Error, TEXT("SubObject : %s"), *BaseClass->GetName());
				RecursedObjects.Add(ObjectValue);
				TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(BaseClass, ObjectValue, includeObjects, RecursedObjects);
				JsonObject->SetField("object", TSharedPtr<FJsonValue>(new FJsonValueObject(Obj)));
				JsonObject->SetField("objectOuter", TSharedPtr<FJsonValue>(new FJsonValueString(ObjectValue->GetOutermost()->GetFName().ToString())));

				return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
				
			}
			else
			{
				JsonObject->SetField("object", value);
				return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
			}
		}
		else
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(""));
		}
	}
	else if (FMapProperty* mProp = Cast<FMapProperty>(prop)) {
		FScriptMapHelper arr(mProp, ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			TSharedPtr<FJsonValue> key = convertUPropToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i),includeObjects, RecursedObjects);
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(mProp->ValueProp, arr.GetValuePtr(i), includeObjects, RecursedObjects);
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
			jsonArr.Add(Obj);
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(sProp->Struct, ptrToProp, includeObjects, RecursedObjects)));
	}
	else if (FMulticastSparseDelegateProperty* CastProp = CastField<FMulticastSparseDelegateProperty>(prop))
	{
	}
	else if (FMulticastInlineDelegateProperty* mCastProp = CastField<FMulticastInlineDelegateProperty>(prop))
	{
	}
	else
	{
		// Log error
		UE_LOG(LogTemp, Error, TEXT("Debug what is this? %s, Type: %s "), *prop->GetName(), *prop->GetClass()->GetName());
	}
	return TSharedPtr<FJsonValue>(new FJsonValueNull());

}

void UJsonStructBPLib::InternalGetStructAsJson(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,bool includeObjects)
{
	TSharedPtr<FJsonObject> JsonObject;
	TArray<UObject* > RecursedObjects;
	JsonObject = convertUStructToJsonObject(Structure->Struct, StructurePtr, includeObjects, RecursedObjects);

	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	String = write;
}

void UJsonStructBPLib::InternalGetStructAsJsonForTable(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,FString Name )
{
	TSharedPtr<FJsonObject> JsonObject;
	JsonObject = ConvertUStructToJsonObjectWithName(Structure->Struct, StructurePtr, RemoveGUID,Name);

	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	String = write;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::ConvertUStructToJsonObjectWithName(UStruct * Struct, void * ptrToStruct,bool RemoveGUID,  FString Name)
{
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	TArray<UObject*> Rec;
	obj->SetStringField("Name", Name);
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(prop->GetName()) : prop->GetName();
		obj->SetField(PropName, convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct),false, Rec));
	}
	return obj;
}

bool UJsonStructBPLib::FillDataTableFromJSONString(UDataTable* DataTable, const FString& InString)
{
	if (!DataTable)
	{
		UE_LOG(LogDataTable, Error, TEXT("Can't fill an invalid DataTable."));
		return false;
	}

	bool bResult = true;
	if (InString.Len() == 0)
	{
		DataTable->EmptyTable();
	}
	else
	{
		TArray<FString> Errors = DataTable->CreateTableFromJSONString(InString);
		if (Errors.Num())
		{
			for (const FString& Error : Errors)
			{
				UE_LOG(LogDataTable, Warning, TEXT("%s"), *Error);
			}
		}
		bResult = Errors.Num() == 0;
	}
	return bResult;
}

FString UJsonStructBPLib::ClassDefaultsToJsonString(UClass *  ObjectClass, bool ObjectRecursive)
{
	if (!ObjectClass)
		return"";


	UObject * Object = ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TArray<UObject* > RecursedObjects;
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(ObjectClass, ObjectClass->GetDefaultObject(), ObjectRecursive,RecursedObjects);
		FString PathName = "";
		if (ObjectClass->IsNative())
		{
			PathName = ObjectClass->GetPathName();
		}
		else
			PathName = ObjectClass->GetSuperClass()->GetPathName();

		TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(PathName));
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueObject(Obj));
		JsonObject->SetField("LibClass", key);
		JsonObject->SetField("LibValue", value);
		FString write;
		TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
		return write;
	}
	else
		return "";
}

FString UJsonStructBPLib::CDOFieldsToJsonString(TArray<FString> Fields, UClass * ObjectClass, bool ObjectRecursive, bool Exclude)
{
	if (!ObjectClass)
		return"";

	UObject * Object = ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TArray<UObject*> RecursedObjects;
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(ObjectClass, ObjectClass->GetDefaultObject(), ObjectRecursive, RecursedObjects);
		TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(ObjectClass->GetSuperClass()->GetPathName()));
		JsonObject->SetField("Class", key);
		TSharedRef<FJsonObject> Ref = Obj.ToSharedRef();
		TMap<FString, TSharedPtr<FJsonValue>> ValueCopy = Ref->Values;
		for (auto i : ValueCopy)
		{
			FString FieldName = i.Key;
			if (!Exclude)
			{
				if (!Fields.Contains(FieldName))
				{
					Ref->Values.Remove(i.Key);
				}
			}
			else
			{
				if (Fields.Contains(FieldName))
				{
					Ref->Values.Remove(i.Key);
				}
			}
			
		}
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueObject(Obj));
		JsonObject->SetField("value", value);
		FString write;
		TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
		FJsonSerializer::Serialize(Obj.ToSharedRef(), JsonWriter);
		return write;
	}
	else
		return "";
}




FString UJsonStructBPLib::GetPackage(UClass* Class)
{
	if (Class->GetOuterUPackage())
		return Class->GetOuterUPackage()->GetName();
	else if (GetTransientPackage())
		return GetTransientPackage()->GetName();
	else
		return "";
}


// Function Arch wrote we borrow here
UClass* UJsonStructBPLib::CreateNewClass(const FString& ClassName, const FString& PackageName, UClass * ParentClass)
{
	const EClassFlags ParamsClassFlags = CLASS_Native | CLASS_MatchedSerializers;

	//Code below is taken from GetPrivateStaticClassBody
	//Allocate memory from ObjectAllocator for class object and call class constructor directly
	UClass* ConstructedClassObject = (UClass*)GUObjectAllocator.AllocateUObject(sizeof(UClass), alignof(UClass), true);
	::new (ConstructedClassObject)UClass(
		EC_StaticConstructor,
		*ClassName,
		ParentClass->GetStructureSize(),
		ParentClass->GetMinAlignment(),
		CLASS_Intrinsic,
		CASTCLASS_None,
		UObject::StaticConfigName(),
		EObjectFlags(RF_Public | RF_Standalone | RF_Transient | RF_MarkAsNative | RF_MarkAsRootSet),
		ParentClass->ClassConstructor,
		ParentClass->ClassVTableHelperCtorCaller,
		ParentClass->ClassAddReferencedObjects);


	//Set super structure and ClassWithin (they are required prior to registering)
	FCppClassTypeInfoStatic TypeInfoStatic = { false };
	ConstructedClassObject->SetSuperStruct(ParentClass);
	ConstructedClassObject->ClassWithin = UObject::StaticClass();
	ConstructedClassObject->SetCppTypeInfoStatic(&TypeInfoStatic);
#if WITH_EDITOR
	//Field with cpp type info only exists in editor, in shipping SetCppTypeInfoStatic is empty
	ConstructedClassObject->SetCppTypeInfoStatic(&TypeInfoStatic);
#endif
	//Register pending object, apply class flags, set static type info and link it
	ConstructedClassObject->RegisterDependencies();
	ConstructedClassObject->DeferredRegister(UClass::StaticClass(), *PackageName, *ClassName);

	//Mark class as Constructed and perform linking
	ConstructedClassObject->ClassFlags |= (EClassFlags)(ParamsClassFlags | CLASS_Constructed);
	ConstructedClassObject->AssembleReferenceTokenStream(true);
	ConstructedClassObject->StaticLink();

	//Make sure default class object is initialized
	ConstructedClassObject->GetDefaultObject();
	return ConstructedClassObject;
}

UClass *  UJsonStructBPLib::SetClassDefaultsFromJsonString(FString String, UClass *  BaseClass)
{
	if (String == "")
		return nullptr;

	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);
	if (!result.IsValid())
		return nullptr;

	if (result->HasField("LibClass") && result->HasField("LibValue"))
	{
		FString String = result->GetStringField("LibClass");
		TSharedPtr<FJsonObject> Obj = result->GetObjectField("LibValue");
		result = Obj;
		if (FPackageName::IsValidObjectPath(String))
		{
			UClass * LoadedObject = LoadObject<UClass>(NULL, *String);
			// Failsafe for script classes with BP Stubs
			if (!LoadedObject)
			{
				FString JsonString = String;
				FString Left;
				FString Right;
				JsonString.Split("/", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				Right.Split(".", &Left, &JsonString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				JsonString.Split("_C", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if (JsonString != "")
				{
					UE_LOG(LogTemp, Warning, TEXT("SetClassDefaultsFromJsonString :: FailSafe Class Loading %s"), *String);
					LoadedObject = FindClassByName(Right);
					if (LoadedObject)
					{
						if (!BaseClass)
						{
							BaseClass = LoadedObject;
						}
						else if (LoadedObject != BaseClass)
						{
							UE_LOG(LogTemp, Warning, TEXT("Class Mismatch  !! %s"), *String);
							
						}
					}
				}
			}
		}
	}
	
	if (!BaseClass)
		return nullptr; 

	if (!GIsUCCMakeStandaloneHeaderGenerator && !BaseClass->ClassConstructor && BaseClass->IsNative())
	{
		return nullptr;
	}

	UObject * Object = BaseClass->GetDefaultObject();
	UObject* OuterClass = BaseClass->ClassGeneratedBy;
	if (!OuterClass)
		OuterClass = Object;
	for (auto prop = TFieldIterator<FProperty>(BaseClass); prop; ++prop) {
		FString FieldName;
		FieldName = prop->GetName();
		auto field = result->TryGetField(FieldName);
		if (!field.IsValid()) continue;
		convertJsonValueToFProperty(field, *prop, prop->ContainerPtrToValuePtr<void>(Object), OuterClass);
	}
	return BaseClass;
}

void UJsonStructBPLib::WriteStringToFile(FString Path, FString resultString, bool Relative) { FFileHelper::SaveStringToFile(resultString, Relative ? *(FPaths::ProjectDir() + Path) : *Path); }

bool UJsonStructBPLib::LoadStringFromFile(FString & String, FString Path, bool Relative){ return FFileHelper::LoadFileToString(String, Relative ? *(FPaths::ProjectDir() + Path) : *Path);}

bool UJsonStructBPLib::FindFilesInPath(const FString & FullPathOfBaseDir, TArray<FString>& FilenamesOut, bool Recursive, const FString & FilterByExtension)
{
	//Format File Extension, remove the "." if present
	const FString FileExt = FilterByExtension.Replace(TEXT("."), TEXT("")).ToLower();

	FString Str;
	auto FilenamesVisitor = MakeDirectoryVisitor(
		[&](const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			//Files
			if (!bIsDirectory)
			{
				//Filter by Extension
				if (FileExt != "")
				{
					Str = FPaths::GetCleanFilename(FilenameOrDirectory);
					//Filter by Extension
					if (FPaths::GetExtension(Str).ToLower() == FileExt)
					{
						if (Recursive)
							FilenamesOut.Push(FilenameOrDirectory); //need whole path for recursive
						else
							FilenamesOut.Push(Str);
						
					}
				}
				//Include All Filenames!
				else
				{
					//Just the Directory
					Str = FPaths::GetCleanFilename(FilenameOrDirectory);
					if (Recursive)
						FilenamesOut.Push(FilenameOrDirectory); //need whole path for recursive
					else
						FilenamesOut.Push(Str);
				}
			}
			return true;
		}
	);
	if (Recursive)
	{
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FullPathOfBaseDir, FilenamesVisitor);
	}
	else
	{
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*FullPathOfBaseDir, FilenamesVisitor);
	}
}

FString UJsonStructBPLib::Conv_BPJsonObjectValueToString(UBPJsonObjectValue * Value) { return Value->AsString(); }

float UJsonStructBPLib::Conv_BPJsonObjectValueToFloat(UBPJsonObjectValue * Value) { return Value->AsNumber(); }

int32 UJsonStructBPLib::Conv_BPJsonObjectValueToInt(UBPJsonObjectValue * Value) { return int32(Value->AsNumber()); }

bool UJsonStructBPLib::Conv_BPJsonObjectValueToBool(UBPJsonObjectValue * Value) { return Value->AsBoolean(); }

UBPJsonObjectValue * UJsonStructBPLib::Conv_StringToBPJsonObjectValue(FString & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueString(Value));
	UBPJsonObjectValue * out = NewObject<UBPJsonObjectValue>();
	out->Value = val;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_FloatToBPJsonObjectValue(float & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueNumber(Value));
	UBPJsonObjectValue * out = NewObject<UBPJsonObjectValue>();
	out->Value = val;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_IntToBPJsonObjectValue(int32 & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueNumber(Value));
	UBPJsonObjectValue * out = NewObject<UBPJsonObjectValue>();
	out->Value = val;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_BoolToBPJsonObjectValue(bool & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueBoolean(Value));
	UBPJsonObjectValue * out = NewObject<UBPJsonObjectValue>();
	out->Value = val;
	return out;
}

TArray<UBPJsonObjectValue *> UJsonStructBPLib::Conv_BPJsonObjectToBPJsonObjectValue(UBPJsonObject * Value)
{
	TArray<UBPJsonObjectValue *> out;
	if (!Value)
		return out;
	if (!Value->InnerObj)
		return out;
	for (auto  i : Value->InnerObj->Values)
	{
		UBPJsonObjectValue * Obj = NewObject<UBPJsonObjectValue>();
		Obj->Value = i.Value;
		out.Add(Obj);
	}
	return out;
}

UBPJsonObject * UJsonStructBPLib::Conv_UBPJsonObjectValueToBPJsonObject(UBPJsonObjectValue * Value) { if (!Value) return nullptr; return Value->AsObject(); }
