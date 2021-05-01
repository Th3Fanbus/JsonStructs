#include "JsonStructBPLib.h"
#include "BPJsonObjectValue.h"
#include "UObject/Class.h"
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

TSharedPtr<FJsonObject> UJsonStructBPLib::convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct, bool RemoveGUID, bool includeObjects) {
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(prop->GetName()) : prop->GetName();
		obj->SetField(PropName, convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct),RemoveGUID,includeObjects));
	}
	return obj;
}

void UJsonStructBPLib::convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct,UObject* Outer) {
	if (!json)
		return;
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto field : json->Values)
	{
		FString FieldName = field.Key;
		auto prop = Struct->FindPropertyByName(*FieldName);
		if (prop)
		{
			convertJsonValueToFProperty(field.Value, prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct));
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
		if (json->AsObject())
		{
			if (json->AsObject()->HasField("enum") && json->AsObject()->HasField("value"))
			{

				TSharedPtr<FJsonValue> keyvalue = json->AsObject()->TryGetField("enum");
				if (FSoftObjectPath(keyvalue->AsString()).IsValid())
				{
					UObject* LoadedObject = FSoftObjectPath(keyvalue->AsString()).TryLoad();
					UEnum * CastResult = Cast<UEnum>(LoadedObject);
					TSharedPtr<FJsonValue> valuevalue = json->AsObject()->TryGetField("value");
					FString Value = *valuevalue->AsString();
					FName Name = FName(Value);
					int32 value = CastResult->GetIndexByName(Name, EGetByNameFlags::CheckAuthoredName);
					FNumericProperty * NumProp = Cast<FNumericProperty>(eProp->GetUnderlyingProperty());
					if (NumProp)
					{
						NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *FString::FromInt(value));
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Enum Property failed to Cast the underlyingProperty to a EnumProperty; Property Name : "), *eProp->GetName());
			}
		}
		else
		{
			FNumericProperty * NumProp = Cast<FNumericProperty>(eProp->GetUnderlyingProperty());
			if (NumProp)
			{
				NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *json->AsString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Enum Property failed to Cast the underlyingProperty to a FNumericProperty; Property Name : "), *eProp->GetName());
			}
		}
	}
	else if (auto byteProp = Cast<FByteProperty>(prop)) {
		if (byteProp->IsEnum())
		{
			if (json->AsObject())
			{
				if (json->AsObject()->HasField("enum") && json->AsObject()->HasField("value"))
				{

					TSharedPtr<FJsonValue> keyvalue = json->AsObject()->TryGetField("enum");
					if (FSoftObjectPath(keyvalue->AsString()).IsValid())
					{
						UObject* LoadedObject = FSoftObjectPath(keyvalue->AsString()).TryLoad();
						UEnum* CastResult = Cast<UEnum>(LoadedObject);
						TSharedPtr<FJsonValue> valuevalue = json->AsObject()->TryGetField("value");
						FString Value = *valuevalue->AsString();
						FName Name = FName(Value);
						int32 value = CastResult->GetIndexByName(Name, EGetByNameFlags::CheckAuthoredName);
						FNumericProperty* NumProp = Cast<FNumericProperty>(byteProp);
						if (NumProp)
						{
							NumProp->SetNumericPropertyValueFromString(NumProp->ContainerPtrToValuePtr<void>(ptrToProp), *FString::FromInt(value));
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Enum Property failed to Cast the underlyingProperty to a EnumProperty; Property Name : "), *byteProp->GetName());
				}
			}
			else
			{
				byteProp->SetNumericPropertyValueFromString(byteProp->ContainerPtrToValuePtr<void>(ptrToProp), *json->AsString());
			}
		}
		else
		{
			byteProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(prop)) {
		ddProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto numProp = Cast<FNumericProperty>(prop)) {
		numProp->SetNumericPropertyValueFromString(ptrToProp, *json->AsString());
	}
	else if (auto aProp = Cast<FArrayProperty>(prop)) {
		FScriptArrayHelper helper(aProp, ptrToProp);
		helper.EmptyValues();
		TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
		for (int i = 0; i < jsonArr.Num(); i++) {
			int64 valueIndex = helper.AddValue();
			convertJsonValueToFProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex),Outer);
		}
	}
	else if (auto mProp = Cast<FMapProperty>(prop)) {
		TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
		FScriptMapHelper MapHelper(mProp, ptrToProp);
		for (int i = 0; i < jsonArr.Num(); i++) {
			const int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
			uint8* mapPtr = MapHelper.GetPairPtr(Index);

			if (jsonArr[i]->AsObject())
			{
				// Map Property
				if (jsonArr[i]->AsObject()->HasField("key") && jsonArr[i]->AsObject()->HasField("value"))
				{
					TSharedPtr<FJsonValue> keyvalue = jsonArr[i]->AsObject()->TryGetField("key");
					TSharedPtr<FJsonValue> valuevalue = jsonArr[i]->AsObject()->TryGetField("value");
					convertJsonValueToFProperty(keyvalue, mProp->KeyProp, mapPtr,Outer);
					convertJsonValueToFProperty(valuevalue, mProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset, Outer);
				}
				else
				{
					// Log error
					UE_LOG(LogTemp, Error, TEXT("Expected Fields Key & Value in Jsob Object for Property of Type Map"));

				}
			}

		}
		MapHelper.Rehash();

	}
	else if (auto cProp = Cast<FClassProperty>(prop)) {
		UObject* LoadedObject = nullptr; UClass * CastResult = nullptr;
		if (FPackageName::IsValidObjectPath(*json->AsString()))
		{
			LoadedObject = LoadObject<UClass>(NULL, *json->AsString());
			// Failsafe for script classes with BP Stubs
			if (!LoadedObject)
			{
				FString JsonString = json->AsString();
				FString Left;
				FString Right;
				JsonString.Split("/", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				Right.Split(".", &Left, &JsonString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				JsonString.Split("_C", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if (JsonString != "")
				{
					UE_LOG(LogTemp, Warning, TEXT("FailSafe Class Loading %s") , *json->AsString());
					LoadedObject = FindClassByName(Right);
				}
			}
		}
		
		cProp->SetPropertyValue(ptrToProp, LoadedObject);
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(prop))
	{
		FWeakObjectPtr WeakObject = WeakObjectProperty->GetPropertyValue(ptrToProp);
		if (json->AsString() != "")
		{
			UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
			WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		FSoftObjectPtr SoftObject = SoftObjectProperty->GetPropertyValue(ptrToProp);
		if (json->AsString() != "")
		{
			FSoftObjectPtr* ObjectPtr = static_cast<FSoftObjectPtr*>(ptrToProp);
			*ObjectPtr = FSoftObjectPath(json->AsString());
		}
	}
	else if (auto uProp = Cast<FObjectProperty>(prop)) {
	
		if (json->Type == EJson::Object)
		{
			if (json->AsObject()->HasField("object") && json->AsObject()->HasField("class"))
			{
				TSharedPtr<FJsonValue> keyvalue = json->AsObject()->TryGetField("class");
				TSharedPtr<FJsonValue> valuevalue = json->AsObject()->TryGetField("object");

				if (valuevalue->Type == EJson::Object)
				{
					UObject* LoadedObject = FSoftClassPath(keyvalue->AsString()).TryLoad();
					FString PathName = LoadedObject->GetPathName();
					if (!PathName.EndsWith("_C")) {
						PathName.Append("_C");
					}
					UClass* InnerBPClass = LoadObject<UClass>(NULL, *PathName);
					UObject * Constructed = NewObject<UObject>(Outer->GetOutermost(), InnerBPClass, *InnerBPClass->GetName(), EObjectFlags::RF_NoFlags);

					convertJsonObjectToUStruct(valuevalue->AsObject(), InnerBPClass, Constructed);
					uProp->SetObjectPropertyValue(ptrToProp, Constructed);
				}
				else if (valuevalue->Type == EJson::String)
				{
					if (valuevalue->AsString() != "")
					{
						UObject* uObj = FSoftObjectPath(valuevalue->AsString()).TryLoad();
						uProp->SetPropertyValue(ptrToProp, uObj);
					}
				}
			}
		}
		else if (json->Type == EJson::String)
		{
			if (json->AsString() != "")
			{
				UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
				uProp->SetPropertyValue(ptrToProp, uObj);
			}
		}	
	}
	else if (auto* CastProp = CastField<FMulticastSparseDelegateProperty>(prop))
	{
		const FMulticastScriptDelegate* Value = CastProp->GetMulticastDelegate(ptrToProp);
	}
	else if (auto sProp = Cast<FStructProperty>(prop)) {
		convertJsonObjectToUStruct(json->AsObject(), sProp->Struct, ptrToProp, Outer);
	}
	else
	{
		// Log error
		UE_LOG(LogTemp, Error, TEXT("Debug what is this? %s, Type: %s "), *prop->GetName(), *prop->GetClass()->GetName());
	}
}


TSharedPtr<FJsonValue> UJsonStructBPLib::convertUPropToJsonValue(FProperty* prop, void* ptrToProp, bool RemoveGUID,bool includeObjects) {
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
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetPathName()));
		JsonObject->SetField("enum", key);
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetNameByValue(eProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ptrToProp)).ToString()));
		JsonObject->SetField("value", value);
		return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
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
			jsonArr.Add(convertUPropToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize), RemoveGUID,includeObjects));
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
		if (WeakObjectProperty->GetPropertyValue(ptrToProp).IsValid())
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(FSoftObjectPath(WeakObjectProperty->GetPropertyValue(ptrToProp).Get()).ToString()));

		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		if (SoftObjectProperty->GetPropertyValue(ptrToProp).IsValid())
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(SoftObjectProperty->GetPropertyValue(ptrToProp).ToSoftObjectPath().ToString()));

		}
	}
	else if (FObjectProperty* oProp = Cast<FObjectProperty>(prop)) {
		if (oProp->GetPropertyValue(ptrToProp)->IsValidLowLevel())
		{
			UClass * BaseClass = oProp->GetPropertyValue(ptrToProp)->GetClass();

			
			TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(BaseClass->GetPathName()));
			TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(FSoftObjectPath(oProp->GetPropertyValue(ptrToProp)).ToString()));
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("class", key);

			for (auto i : BaseClass->Interfaces)
			{
				if (i.Class == UInterface_AssetUserData::StaticClass() || i.Class == UStreamableRenderAsset::StaticClass())
				{
					JsonObject->SetField("object", value);
					TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
					return TSharedPtr<FJsonValue>(Obj);
				}
			}

			if (includeObjects && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced)) {
				TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(BaseClass, oProp->GetPropertyValue(ptrToProp), false, includeObjects);
				JsonObject->SetField("object", TSharedPtr<FJsonValue>(new FJsonValueObject(Obj)));
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
			TSharedPtr<FJsonValue> key = convertUPropToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i),RemoveGUID, includeObjects);
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(mProp->ValueProp, arr.GetValuePtr(i), RemoveGUID, includeObjects);
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
			jsonArr.Add(Obj);
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(sProp->Struct, ptrToProp, RemoveGUID, includeObjects)));
	}
	else if (FMulticastSparseDelegateProperty* CastProp = CastField<FMulticastSparseDelegateProperty>(prop))
	{
		return TSharedPtr<FJsonValue>(new FJsonValueString("Not Implemented"));

	}
	else if (FMulticastInlineDelegateProperty* mCastProp = CastField<FMulticastInlineDelegateProperty>(prop))
	{
		return TSharedPtr<FJsonValue>(new FJsonValueString("Not Implemented"));
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
	JsonObject = convertUStructToJsonObject(Structure->Struct, StructurePtr, RemoveGUID, includeObjects);

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
	obj->SetStringField("Name", Name);
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(prop->GetName()) : prop->GetName();
		obj->SetField(PropName, convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct)));
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

FString UJsonStructBPLib::ClassDefaultsToJsonString(UClass *  ObjectClass, bool Filtered, bool ObjectRecursive)
{
	if (!ObjectClass)
		return"";

	UObject * Object = ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(ObjectClass, ObjectClass->GetDefaultObject(), false, ObjectRecursive);
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

	if (!GIsUCCMakeStandaloneHeaderGenerator && !BaseClass->ClassConstructor && BaseClass->IsNative())
	{
		return nullptr;
	}

	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);
	if (!result.IsValid())
		return nullptr;
	UObject * Object = BaseClass->GetDefaultObject();
	for (auto prop = TFieldIterator<FProperty>(BaseClass); prop; ++prop) {
		FString FieldName;
		FieldName = prop->GetName();
		auto field = result->TryGetField(FieldName);
		if (!field.IsValid()) continue;
		convertJsonValueToFProperty(field, *prop, prop->ContainerPtrToValuePtr<void>(Object), BaseClass);
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
