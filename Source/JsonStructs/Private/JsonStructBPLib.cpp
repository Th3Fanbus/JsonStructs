#include "JsonStructBPLib.h"
#include "BPJsonObjectValue.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "Internationalization/PackageLocalizationManager.h"
#include "Engine/AssetManager.h" 
#include "UObject/UObjectAllocator.h"



void UJsonStructBPLib::Log(FString LogString, int32 Level)
{
	if (Level == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *LogString);
	}
	else if (Level == 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *LogString);
	}
	else if (Level == 2)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *LogString);
	}

}


FString UJsonStructBPLib::RemoveUStructGuid(FString write)
{
	FString Replace = "";
	FString Cutoff;
	write.Split("_", &Replace, &Cutoff, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
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

TSharedPtr<FJsonObject> UJsonStructBPLib::convertUStructToJsonObject(const UStruct* Struct, void* ptrToStruct, bool includeObjects, TArray<UObject*>& RecursedObjects,bool IncludeNonInstanced, TArray<FString> FilteredFields, bool Exclude= true) {
	auto Obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		if (FilteredFields.Contains(prop->GetName()))
		{
			if(Exclude)
				continue;
		}
		else
		{
			if (!Exclude)
				continue;
		}
		TSharedPtr<FJsonValue> value = convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct), includeObjects, RecursedObjects, IncludeNonInstanced, FilteredFields, Exclude);
		if (value.IsValid() && value->Type != EJson::Null)
		{
			Obj->SetField(prop->GetName(), value);
		}
	}
	return Obj;
}

void UJsonStructBPLib::convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct,UObject* Outer) {
	if (!json)
		return;
	for (auto field : json->Values) {
		const FString FieldName = field.Key;
		auto prop = Struct->FindPropertyByName(*FieldName);
		if (prop) {
			convertJsonValueToFProperty(field.Value, prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct),Outer);
		}
	}
}


void UJsonStructBPLib::convertJsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* prop, void* ptrToProp, UObject* Outer) {
	if (FStrProperty* strProp = Cast<FStrProperty>(prop)) {
		const FString currentValue = strProp->GetPropertyValue(ptrToProp); 
		if (currentValue != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue).Append(" NewValue = ").Append(json->AsString()),0);
			strProp->SetPropertyValue(ptrToProp, json->AsString());
		}
	}
	else if (FTextProperty*  txtProp = Cast<FTextProperty>(prop)) {
		const FText currentValue = txtProp->GetPropertyValue(ptrToProp);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			txtProp->SetPropertyValue(ptrToProp, FText::FromString(json->AsString()));
		}
	}
	else if (FNameProperty* nameProp = Cast<FNameProperty>(prop)) {
		const FName currentValue = nameProp->GetPropertyValue(ptrToProp);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FNameProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			nameProp->SetPropertyValue(ptrToProp, *json->AsString());
		}

	}
	else if (FFloatProperty* fProp = Cast<FFloatProperty>(prop)) {
		const float currentValue = fProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FFloatProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(FString::SanitizeFloat(currentValue)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			fProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (FIntProperty* iProp = Cast<FIntProperty>(prop)) {
		const int64 currentValue = iProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FIntProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(currentValue)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
			iProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (FBoolProperty* bProp = Cast<FBoolProperty>(prop)) {
		const bool currentValue = bProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsBool())
		{
			Log(FString("Overwrite FBoolProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue ? "true" : "false" ).Append(" NewValue = ").Append(json->AsBool() ? "true" : "false"), 0);
			bProp->SetPropertyValue(ptrToProp, json->AsBool());
		}
	}
	else if (FEnumProperty* eProp = Cast<FEnumProperty>(prop)) {	
		if (json->Type == EJson::Object)
		{
			if (json->AsObject()->HasField("enum") && json->AsObject()->HasField("value"))
			{
				const FString KeyValue = json->AsObject()->TryGetField("enum")->AsString();
				if (FPackageName::IsValidObjectPath(*KeyValue)) {
					UEnum* EnumClass = LoadObject<UEnum>(NULL, *KeyValue);
					if (EnumClass) {
						FString Value = *json->AsObject()->TryGetField("value")->AsString();
						FNumericProperty* NumProp = Cast<FNumericProperty>(eProp->GetUnderlyingProperty());
						int32 intValue = EnumClass->GetIndexByName(FName(Value),EGetByNameFlags::CheckAuthoredName);
						FString StringInt = FString::FromInt(intValue);
						if (NumProp && eProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(ptrToProp) != StringInt) {
							Log(FString("Overwrite FIntProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(eProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(ptrToProp)).Append(" NewValue = ").Append(StringInt), 0);
							NumProp->SetNumericPropertyValueFromString(ptrToProp, *StringInt);
						}
					}
				}
			}
		}	
	}
	else if (FByteProperty* byteProp = Cast<FByteProperty>(prop)) {
		
		if (byteProp->IsEnum()) {
			if (json->Type == EJson::Object) {
				if (json->AsObject()->HasField("enumAsByte") && json->AsObject()->HasField("value")) {
					const TSharedPtr<FJsonValue> keyvalue = json->AsObject()->TryGetField("enumAsByte");
					if (FPackageName::IsValidObjectPath(*keyvalue->AsString())) {
						UEnum* EnumClass = LoadObject<UEnum>(NULL, *keyvalue->AsString());
						if (EnumClass) {
							const TSharedPtr<FJsonValue> valuevalue = json->AsObject()->TryGetField("value");
							const TSharedPtr<FJsonValue> byte = json->AsObject()->TryGetField("byte");
							auto byteValue = byteProp->GetPropertyValue(ptrToProp);
							uint8 newValue = byte->AsNumber();
							/*
							FString Value = *valuevalue->AsString();
							FName Name = FName(Value);
							int32 value = EnumClass->GetIndexByName(Name, EGetByNameFlags::CheckAuthoredName);
							*/
							if (newValue != byteValue)
							{
								Log(FString("Overwrite FByteProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(byteValue)).Append(" NewValue = ").Append(FString::FromInt(newValue)), 0);
								byteProp->SetPropertyValue(ptrToProp, newValue);

							}
						}
					}
				}
			}
		}
		else
		{
			if (json->Type == EJson::Number) {
				FNumericProperty* NumProp = Cast<FNumericProperty>(byteProp);
				if (NumProp && NumProp->GetSignedIntPropertyValue(ptrToProp) != json->AsNumber()) {
					Log(FString("Overwrite FByteProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(byteProp->GetNumericPropertyValueToString(ptrToProp)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
					byteProp->SetNumericPropertyValueFromString(ptrToProp, *FString::FromInt(json->AsNumber()));
				}
			}
		}
		
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(prop)) {
		if (ddProp->GetPropertyValue(ptrToProp) != json->AsNumber())
		{
			Log(FString("Overwrite FDoubleProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(ddProp->GetNumericPropertyValueToString(ptrToProp)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			ddProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (FNumericProperty* numProp = Cast<FNumericProperty>(prop)) {
		if (numProp->GetNumericPropertyValueToString(ptrToProp) != json->AsString())
		{
			Log(FString("Overwrite FNumericProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(numProp->GetNumericPropertyValueToString(ptrToProp)).Append(" NewValue = ").Append(json->AsString()), 0);
			numProp->SetNumericPropertyValueFromString(ptrToProp, *json->AsString());
		}
	}
	else if (FArrayProperty* aProp = Cast<FArrayProperty>(prop)) {
		if (json->Type == EJson::Array)
		{
			FScriptArrayHelper helper(aProp, ptrToProp);
			const TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			// in order to compare values we want to keep values when overwriting children 
			if (helper.Num() > jsonArr.Num())
			{
				helper.Resize(jsonArr.Num());
			}
			for (int i = 0; i < jsonArr.Num(); i++) {
				int64 valueIndex = i;
				// only add values when size is smaller then requested ptr Slot
				if (helper.Num() <= i)
					valueIndex = helper.AddValue();

				convertJsonValueToFProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex), Outer);
			}
		}
	}
	else if (FMapProperty* mProp = Cast<FMapProperty>(prop)) {
		if (json->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			FScriptMapHelper MapHelper(mProp, ptrToProp);
			TArray<uint32> Hashes;
			TArray<int32> HashesNew;
			
			for (int32 e = 0; e < MapHelper.Num(); e++)
			{
				Hashes.Add(mProp->KeyProp->GetValueTypeHash(MapHelper.GetKeyPtr(e)));
			}

			for (int i = 0; i < jsonArr.Num(); i++) {
				void* PropertyValue = FMemory::Malloc(mProp->KeyProp->GetSize());
				mProp->KeyProp->InitializeValue(PropertyValue);
				TSharedPtr<FJsonValue> keyvalue = jsonArr[i]->AsObject()->TryGetField("key");
				convertJsonValueToFProperty(keyvalue, mProp->KeyProp, PropertyValue, Outer);
				const uint32 Hash = mProp->KeyProp->GetValueTypeHash(PropertyValue); 
				if (!Hashes.Contains(Hash))
				{
					HashesNew.Add(i);
				}
				else
				{
					uint8* valueptr = MapHelper.GetValuePtr(Hashes.Find(Hash));
					TSharedPtr<FJsonValue> valuevalue = jsonArr[i]->AsObject()->TryGetField("value");
					convertJsonValueToFProperty(valuevalue, mProp->ValueProp, valueptr, Outer);

				}
				mProp->KeyProp->DestroyValue(PropertyValue);
				FMemory::Free(PropertyValue);
			}

			for (int32 i : HashesNew)
			{
				const int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				uint8* mapPtr = MapHelper.GetPairPtr(Index);
				TSharedPtr<FJsonValue> valuevalue = jsonArr[i]->AsObject()->TryGetField("value");
				TSharedPtr<FJsonValue> keyvalue = jsonArr[i]->AsObject()->TryGetField("key");
				convertJsonValueToFProperty(keyvalue, mProp->KeyProp, mapPtr, Outer);
				convertJsonValueToFProperty(valuevalue, mProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset, Outer);
			}

			MapHelper.Rehash();
		}
	}
	else if (FClassProperty* cProp = Cast<FClassProperty>(prop)) {
		UClass * CastResult = nullptr;
		if (json->Type == EJson::String) {
			const FString ClassPath = json->AsString();
			if (ClassPath == "")
			{
				if (cProp->GetPropertyValue(ptrToProp) != nullptr)
				{
					Log(FString("Overwrite FClassProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(ptrToProp)->GetPathName()).Append(" NewValue = Nullpeter"), 0);
					cProp->SetPropertyValue(ptrToProp, CastResult);
				}
			}
			else
			{
				CastResult = LoadObject<UClass>(NULL, *json->AsString());
				// Failsafe for script classes with BP Stubs
				if (!CastResult && json->AsString() != "") {
					CastResult = FSoftClassPath(json->AsString()).TryLoadClass<UClass>();
					if (!CastResult)
					{
						CastResult = FailSafeClassFind(json->AsString());
					}
				}
				if (CastResult && CastResult != cProp->GetPropertyValue(ptrToProp))
				{
					Log(FString("Overwrite FClassProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(ptrToProp)->GetPathName()).Append(" NewValue = ").Append(CastResult->GetPathName()), 0);
					cProp->SetPropertyValue(ptrToProp, CastResult);
				}
			}
		}
	}
	else if (FObjectProperty* uProp = Cast<FObjectProperty>(prop)) {
	
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("object") && json->AsObject()->HasField("class") && json->AsObject()->HasField("objectFlags") && json->AsObject()->HasField("objectName")) {
				const FString keyvalue = json->AsObject()->TryGetField("class")->AsString();
				const FString Namevalue = json->AsObject()->TryGetField("objectName")->AsString();
				const TSharedPtr<FJsonValue> ObjectValue = json->AsObject()->TryGetField("object");
				const TSharedPtr<FJsonValue> OuterValue = json->AsObject()->TryGetField("objectOuter");
				const EObjectFlags ObjectLoadFlags = (EObjectFlags)json->AsObject()->GetIntegerField((TEXT("objectFlags")));
				if (ObjectValue->Type == EJson::Object && keyvalue != "") {
					UClass* InnerBPClass = LoadObject<UClass>(NULL, *keyvalue);
					if (InnerBPClass) {
						const TSharedPtr<FJsonObject> Obj = ObjectValue->AsObject();
						UObject* Value = uProp->GetPropertyValue(ptrToProp);
						if (Value && Value->GetClass() == InnerBPClass)
						{
							UClass* OuterLoaded = LoadObject<UClass>(NULL, *OuterValue->AsString());
							if (OuterLoaded && OuterLoaded != Outer)
							{
								Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" using != Outer !").Append(keyvalue).Append(" actual Outer : ").Append(OuterLoaded->GetPathName()), 0);
								convertJsonObjectToUStruct(Obj, InnerBPClass, Value, OuterLoaded);
							}
							else
							{
								convertJsonObjectToUStruct(Obj, InnerBPClass, Value, Outer);
							}
						}
						else
						{
							UObject* Template = UObject::GetArchetypeFromRequiredInfo(InnerBPClass, Outer, *Namevalue, ObjectLoadFlags);
							UObject* Constructed = StaticConstructObject_Internal(InnerBPClass, Outer, *Namevalue, ObjectLoadFlags, EInternalObjectFlags::None, Template);
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Constructed new Obejct of Class").Append(keyvalue), 0);
							convertJsonObjectToUStruct(Obj, InnerBPClass, Constructed, Outer);
							uProp->SetObjectPropertyValue(ptrToProp, Constructed);
						}
					}
					else
					{
						Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Failed to load UClass for Object").Append(keyvalue), 2);
					}
				}
				else if (ObjectValue->Type == EJson::String) {

					UObject* Obj = uProp->GetPropertyValue(ptrToProp);
					if (ObjectValue->AsString() != "") {
						UObject* uObj = LoadObject<UObject>(NULL, *ObjectValue->AsString());
						if (!uObj)
						{
							Log(FString("Skipped Overwrite Reason[Load Fail] FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((uObj ? uObj->GetPathName() : FString("nullpeter")))), 0);
						}
						else
						{
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((uObj ? uObj->GetPathName() : FString("nullpeter")))), 0);
							uProp->SetPropertyValue(ptrToProp, uObj);
						}
					}
					else if (Obj)
					{
						Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with nullpeter")), 0);
						uProp->SetPropertyValue(ptrToProp, nullptr);
					}
				}
			}
		}
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(prop)) {
		convertJsonObjectToUStruct(json->AsObject(), sProp->Struct, ptrToProp, Outer);
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(prop))
	{
		FWeakObjectPtr wObj = WeakObjectProperty->GetPropertyValue(ptrToProp);

		
		if (json->AsString() != "")
		{
			UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
			UObject* Obj = wObj.Get();

				Log(FString("Overwrite WeakObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((uObj ? uObj->GetPathName() : FString("nullpeter")))), 0);
				WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
		}
		else
		{
			if (wObj.IsValid())
			{
				Log(FString("Overwrite FWeakObjectProperty: ").Append(prop->GetName()).Append(" with nullpeter"), 0);
				WeakObjectProperty->SetPropertyValue(ptrToProp, nullptr);
			}
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		if (json->AsString() != "")
		{
			const FString PathString = json->AsString();
			FSoftObjectPtr* ObjectPtr = static_cast<FSoftObjectPtr*>(ptrToProp);
			*ObjectPtr = FSoftObjectPath(PathString);
		}
	}
	else if (auto* CastProp = CastField<FMulticastSparseDelegateProperty>(prop))
	{
	//ExportTextItem
	}
	else
	{
	
	}
}


TSharedPtr<FJsonValue> UJsonStructBPLib::convertUPropToJsonValue(FProperty* prop, void* ptrToProp,bool includeObjects ,TArray<UObject*>& RecursedObjects,bool DeepRecursion, TArray<FString> FilteredFields, bool Exclude) {
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
		const TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetPathName()));
		const TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(eProp->GetEnum()->GetNameByValue(eProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ptrToProp)).ToString()));
		JsonObject->SetField("enum", key);
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
			TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(byProp->Enum->GetNameByValue(byProp->GetSignedIntPropertyValue(ptrToProp)).ToString()));
			TSharedPtr<FJsonValue> byte = TSharedPtr<FJsonValue>(new FJsonValueNumber(byProp->GetPropertyValue(ptrToProp)));
			JsonObject->SetField("enumAsByte", key);
			JsonObject->SetField("value", value);
			JsonObject->SetField("byte", byte);
			return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
		}
		else
			return TSharedPtr<FJsonValue>(new FJsonValueNumber(byProp->GetSignedIntPropertyValue(ptrToProp)));
	}
	else if (FNumericProperty* nProp = Cast<FNumericProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(nProp->GetUnsignedIntPropertyValue(ptrToProp)));
	}
	else if (FArrayProperty* aProp = Cast<FArrayProperty>(prop)) {
		auto& arr = aProp->GetPropertyValue(ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(convertUPropToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize), includeObjects, RecursedObjects, DeepRecursion, FilteredFields,Exclude));
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FMapProperty* mProp = Cast<FMapProperty>(prop)) {
		FScriptMapHelper arr(mProp, ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			auto ptr = arr.GetValuePtr(i);
			
			TSharedPtr<FJsonValue> key = convertUPropToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i), includeObjects, RecursedObjects, DeepRecursion, FilteredFields,Exclude);
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(mProp->ValueProp, arr.GetValuePtr(i), includeObjects, RecursedObjects, DeepRecursion, FilteredFields,Exclude);
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
			jsonArr.Add(Obj);
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FSetProperty* SetProperty = Cast<FSetProperty>(prop)) {
		auto& arr = SetProperty->GetPropertyValue(ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(convertUPropToJsonValue(SetProperty->ElementProp, (void*)((size_t)arr.GetData(i, SetProperty->SetLayout)), includeObjects, RecursedObjects, DeepRecursion,FilteredFields, Exclude));
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FClassProperty* cProp = Cast<FClassProperty>(prop)) {
		if (cProp->GetPropertyValue(ptrToProp))
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
		else
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(""));
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(prop))
	{
		if (SoftObjectProperty->GetPropertyValue(ptrToProp).IsValid())
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(SoftObjectProperty->GetPropertyValue(ptrToProp).ToSoftObjectPath().ToString()));
		}
		else
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(""));
		}
	}
	else if (FObjectProperty* oProp = Cast<FObjectProperty>(prop)) {
		if (ptrToProp && oProp->GetPropertyValue(ptrToProp))
		{
			UObject* ObjectValue = oProp->GetPropertyValue(ptrToProp);
			const UClass* BaseClass = ObjectValue->GetClass();
			const TSharedPtr<FJsonValue> key = TSharedPtr<FJsonValue>(new FJsonValueString(BaseClass->GetPathName()));
			const TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueString(ObjectValue->GetPathName()));
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("class", key);


			// Everything that is streamable or AssetUserData ( aka Meshs and Textures etc ) will be skipped
			for (auto i : BaseClass->Interfaces)
			{
				if (i.Class == UInterface_AssetUserData::StaticClass() || i.Class == UStreamableRenderAsset::StaticClass())
				{
					JsonObject->SetField("object", value);
					const TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
					return TSharedPtr<FJsonValue>(Obj);
				}
			}

			// We do want include Objects
			// Object is defaulting to Instanced or we Include Non Instnaced
			// Property Value must not be nullpeter
			// Property Object wasnt already serialized or is RootComponent based
			const bool Condition = includeObjects
				&& ((BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_EditInlineNew)) || DeepRecursion)
				&& (!RecursedObjects.Contains(ObjectValue)) && RecursedObjects.Num() < 500;

			// skip this if we dont need it 
			if (Condition) {
				TSharedPtr<FJsonObject> ChildObj = TSharedPtr<FJsonObject>(new FJsonObject());
				RecursedObjects.Add(ObjectValue);
				USceneComponent* Scene = Cast<USceneComponent>(ObjectValue);
				if (Scene && Scene->GetAttachmentRoot() == ObjectValue)
				{
					TArray<USceneComponent*> arr = Scene->GetAttachChildren();
					for (auto i : arr)
					{
						RecursedObjects.Add(i);
						ChildObj->Values.Add(i->GetName(), TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(i->GetClass(), i, includeObjects, RecursedObjects, DeepRecursion, FilteredFields, Exclude))));
					}
				}
				TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(BaseClass, ObjectValue, includeObjects, RecursedObjects, DeepRecursion, FilteredFields, Exclude);
				JsonObject->SetField("object", TSharedPtr<FJsonValue>(new FJsonValueObject(Obj)));
				JsonObject->SetField("objectFlags", TSharedPtr<FJsonValue>(new FJsonValueNumber(int32(ObjectValue->GetFlags()))));
				JsonObject->SetField("objectName", TSharedPtr<FJsonValue>(new FJsonValueString(ObjectValue->GetName())));
				JsonObject->SetField("objectOuter", TSharedPtr<FJsonValue>(new FJsonValueString(ObjectValue->GetOutermost()->GetFName().ToString())));
				JsonObject->SetField("Children:", TSharedPtr<FJsonValue>(new FJsonValueObject(ChildObj)));
				return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
			}
			else
			{
				JsonObject->SetField("object", value);
				return TSharedPtr<FJsonValue>(TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject)));
			}
		}
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(sProp->Struct, ptrToProp, includeObjects, RecursedObjects, DeepRecursion, FilteredFields, Exclude)));
	}
	else if (!DeepRecursion)
	{
		return TSharedPtr<FJsonValue>(new FJsonValueNull());
	}
	else if (FMulticastSparseDelegateProperty* CastProp = Cast<FMulticastSparseDelegateProperty>(prop))
	{
		if (CastProp->SignatureFunction)
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(CastProp->SignatureFunction->GetName()));
		}
	}
	else if (FMulticastInlineDelegateProperty* mCastProp = Cast<FMulticastInlineDelegateProperty>(prop))
	{
		if (mCastProp->SignatureFunction)
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(mCastProp->SignatureFunction->GetName()));
		}
	}
	else if (FDelegateProperty* Delegate = Cast<FDelegateProperty>(prop)) {
		if (Delegate->SignatureFunction)
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(Delegate->SignatureFunction->GetName()));
		}
	}
	else if (FInterfaceProperty* InterfaceProperty = Cast<FInterfaceProperty>(prop)) {
		if (InterfaceProperty->InterfaceClass)
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(InterfaceProperty->InterfaceClass->GetPathName()));
		}
		else
		{
			return TSharedPtr<FJsonValue>(new FJsonValueString(""));

		}
	}
	else if (FFieldPathProperty* FieldPath = Cast<FFieldPathProperty>(prop)) {
	
		return TSharedPtr<FJsonValue>(new FJsonValueString(FieldPath->PropertyClass->GetName()));
	}
	else
	{
		// Log Debug
	}
	return TSharedPtr<FJsonValue>(new FJsonValueNull());

}

void UJsonStructBPLib::InternalGetStructAsJson(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,bool includeObjects)
{
	TArray<UObject*> Array;
	TSharedPtr<FJsonObject> JsonObject = convertUStructToJsonObject(Structure->Struct, StructurePtr, includeObjects, Array,false, TArray<FString>(), true);
	String = JsonObjectToString(JsonObject);
}

void UJsonStructBPLib::InternalGetStructAsJsonForTable(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,FString Name )
{
	TSharedPtr<FJsonObject> JsonObject = ConvertUStructToJsonObjectWithName(Structure->Struct, StructurePtr, RemoveGUID,Name);
	String = JsonObjectToString(JsonObject);
}

TSharedPtr<FJsonObject> UJsonStructBPLib::ConvertUStructToJsonObjectWithName(UStruct * Struct, void * ptrToStruct,bool RemoveGUID,  FString Name)
{
	TSharedPtr<FJsonObject> obj = TSharedPtr<FJsonObject>(new FJsonObject());
	obj->SetStringField("Name", Name);
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(prop->GetName()) : prop->GetName();
		TArray<UObject* > RecursedObjects;
		obj->SetField(PropName, convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct), RemoveGUID,RecursedObjects,false,TArray<FString>(), true));
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

TSharedPtr<FJsonObject> UJsonStructBPLib::SetupJsonObject(UClass* Class, UObject* Object)
{
	if (!Class)
		return nullptr;

	TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
	JsonObject->SetField("LibClass", TSharedPtr<FJsonValue>(new FJsonValueString(Class->IsNative()? Class->GetPathName(): Class->GetSuperClass()->GetPathName())));
	if (Cast<AActor>(Object) && Object != Class->GetDefaultObject())
	{
		JsonObject->SetField("LibTransform", TSharedPtr<FJsonValue>(new FJsonValueString(Cast<AActor>(Object)->GetTransform().ToString())));
	}
	JsonObject->SetField("LibOuter", TSharedPtr<FJsonValue>(new FJsonValueString(Object->GetPathName())));
	return JsonObject;
}

FString UJsonStructBPLib::ObjectToJsonString(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, bool DeepRecursion , bool SkipRoot, bool SkipTransient,bool OnlyEditable)
{
	if (!ObjectClass)
		return"";

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("LibValue", TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable, TArray<FString>(), true))));
		return JsonObjectToString(JsonObject);
	}
	else
		return "";
}


FString UJsonStructBPLib::JsonObjectToString(TSharedPtr<FJsonObject> JsonObject)
{
	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return write;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::CDOToJson(UClass * ObjectClass, UObject* Object, bool ObjectRecursive, bool DeepRecursion, bool SkipRoot,bool SkipTransient,bool OnlyEditable, TArray<FString> FilteredFields, bool Exclude)
{
	TArray<UObject* > ResursionArray;
	TSharedPtr<FJsonObject> Obj = TSharedPtr<FJsonObject>(new FJsonObject());

	if (!SkipRoot)
	{
		FProperty* RootComponent = nullptr;
		for (auto prop = TFieldIterator<FProperty>(ObjectClass); prop; ++prop) {
			if (prop->GetName() == "RootComponent")
			{
				RootComponent = *prop;
				break;
			}
		}
		if (RootComponent)
		{
			Obj->SetField(RootComponent->GetName(), convertUPropToJsonValue(RootComponent, RootComponent->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, ResursionArray, DeepRecursion, FilteredFields, Exclude));
		}

	}

	for (auto prop = TFieldIterator<FProperty>(ObjectClass); prop; ++prop) {
		if (FilteredFields.Contains(prop->GetName()))
		{
			if (Exclude)
				continue;
		}
		else
		{
			if (!Exclude)
				continue;
		}
		const bool bEditable = prop->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit);
		const bool bLikelyOnlyReadable = prop->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient | EPropertyFlags::CPF_Net | CPF_EditConst | CPF_GlobalConfig | CPF_Config);
		const bool bRootComponent = prop->GetName() == "RootComponent";
		if (bRootComponent || (bLikelyOnlyReadable  && SkipTransient) || (!bEditable && OnlyEditable))
		{
			continue;
		}
		else
		{
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, ResursionArray, DeepRecursion, FilteredFields, Exclude);
			if (value.IsValid() && value->Type != EJson::Null)
			{
				Obj->SetField(prop->GetName(), value);
			}
		}
	}

	return Obj;
}

UBPJsonObject * UJsonStructBPLib::ObjectToJsonObject(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, UObject* Outer, bool DeepRecursion,bool SkipRoot, bool SkipTransient,bool OnlyEditable)
{
	if (!ObjectClass)
		return nullptr;

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("LibValue", TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable, TArray<FString>(), true))));
		UBPJsonObject * JObj = NewObject<UBPJsonObject>(Outer);
		JObj->InnerObj = JsonObject;
		JObj->InitSubObjects();
		return JObj;
	}
	return nullptr;
}


UBPJsonObject* UJsonStructBPLib::ObjectToJsonObjectFiltered(TArray<FString> Fields, UClass* ObjectClass, UObject* DefaultObject, UObject* Outer, bool ObjectRecursive, bool Exclude, bool DeepRecursion, bool SkipRoot, bool SkipTransient, bool OnlyEditable)
{
	if (!ObjectClass)
		return nullptr;

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable, Fields, Exclude).ToSharedRef()));
		JsonObject->SetField("LibValue", value);	
		UBPJsonObject* JObj = NewObject<UBPJsonObject>(Outer);
		JObj->InnerObj = JsonObject;
		JObj->InitSubObjects();
		return JObj;
	}
	return nullptr;
}

FString UJsonStructBPLib::ObjectToJsonStringFiltered(TArray<FString> Fields, UClass * ObjectClass,  UObject* DefaultObject, bool ObjectRecursive, bool Exclude, bool DeepRecursion, bool SkipRoot , bool SkipTransient,bool OnlyEditable)
{
	if (!ObjectClass)
		return"";

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable, Fields, Exclude).ToSharedRef()));
		JsonObject->SetField("LibValue", value);
		return JsonObjectToString(JsonObject);
	}
	else
		return "";
}




// Function Arch wrote we borrow here
// changed UClass to UDynamicClass and added MountPoint Registration incase of this being on one
UClass* UJsonStructBPLib::CreateNewClass(const FString& ClassName, const FString& PackageName, UClass * ParentClass, const FString& MountPoint)
{
	if (ClassName != "" && FPackageName::DoesPackageNameContainInvalidCharacters(ClassName) && PackageName != "" && FPackageName::DoesPackageNameContainInvalidCharacters(PackageName))
	{
		FString Left; FString Right;
		if (!PackageName.Split("/", &Left, &Right))
		{
			Log("PackageName must start with a / ", 2);
			return nullptr;
		}
		if (!Right.Split("/", &Left, &Right))
		{
			Log("PackageName must have a SubFolder", 2);
			return nullptr;
		}
		if (Left == "" || Left.Contains("/") || FPackageName::DoesPackageNameContainInvalidCharacters(Left))
		{
			Log("Failed to Parse MountPoint", 2);
			return nullptr;
		}
		FString ClassMountPoint = Left;

		if (FPackageName::GetPackageMountPoint(ClassMountPoint) == FName())
		{
			FPackageName::RegisterMountPoint(ClassMountPoint, FPaths::ProjectModsDir() + ClassMountPoint);
			FPackageLocalizationManager::Get().ConditionalUpdateCache();
		}
	}
	else
		return nullptr;

	const EClassFlags ParamsClassFlags = CLASS_Native | CLASS_MatchedSerializers;

	

	//Code below is taken from GetPrivateStaticClassBody
	//Allocate memory from ObjectAllocator for class object and call class constructor directly
	UClass* ConstructedClassObject = (UClass*)GUObjectAllocator.AllocateUObject(sizeof(UDynamicClass), alignof(UDynamicClass), true);
	::new (ConstructedClassObject)UDynamicClass(
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
		ParentClass->ClassAddReferencedObjects, nullptr);

	

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

	ConstructedClassObject->DeferredRegister(UDynamicClass::StaticClass(), *PackageName, *ClassName);

	//Mark class as Constructed and perform linking
	ConstructedClassObject->ClassFlags |= (EClassFlags)(ParamsClassFlags | CLASS_Constructed);
	ConstructedClassObject->AssembleReferenceTokenStream(true);
	ConstructedClassObject->StaticLink();

	//Make sure default class object is initialized
	ConstructedClassObject->GetDefaultObject();
	
	return ConstructedClassObject;

}


// Tries to spawn an Actor with exact name to World
AActor* UJsonStructBPLib::SpawnActorWithName(UObject * WorldContext, UClass* C, FName Name)
{
	if (!WorldContext || !WorldContext->GetWorld() || !C)
		return nullptr;

	FActorSpawnParameters spawnParams;
	spawnParams.Name = Name;
	return WorldContext->GetWorld()->SpawnActor<AActor>(C, spawnParams);
}

UClass *  UJsonStructBPLib::FailSafeClassFind(FString String)
{

	// Path to Parent class might be changed
	// so we search Ambiguously
	FString ClassName; FString Path; FString Right;
	FPaths::Split(String, Path, Right, ClassName);
	if (ClassName != "")
	{
		Log(FString("FailSafe Class #1 Loading ").Append(String).Append("--  Searching : ").Append(ClassName), 1);
		UClass * out = FindClassByName(ClassName);
		if (out)
			return out;

		if (!ClassName.EndsWith("_C"))
		{
			Log(FString("FailSafe Class #2 Loading ").Append(String).Append("--  Searching : ").Append(ClassName.Append("_C")), 1);
			return FindClassByName(ClassName.Append("_C"));
		}
	}
	return nullptr;
}
bool  UJsonStructBPLib::SetClassDefaultsFromJsonString(FString StringIn, UClass *  BaseClass, UObject * DefaultObject)
{
	if (StringIn == "")
		return false;

	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*StringIn);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);
	if (!result.IsValid())
		return false;

	if (result->HasField("LibClass") && result->HasField("LibValue"))
	{
		const FString String = result->GetStringField("LibClass");
		TSharedPtr<FJsonObject> Obj = result->GetObjectField("LibValue");
		result = Obj;
		if (FPackageName::IsValidObjectPath(String))
		{
			UClass * LoadedObject = LoadObject<UClass>(NULL, *String);
			if (!LoadedObject)
			{
				LoadedObject = FailSafeClassFind(String);
			}
			// if the classinput was invalid we can fallback to Loaded Class
			if (!BaseClass && LoadedObject)
			{
				BaseClass = LoadedObject;
			}
			// if LoadedClass is valid and is not equal to the input and not a Child of it eiher , and we dont provide DefaultObject 
			// then something weird is going on; DefaultObject provided from Blueprint can only be valid for 
			// Assets or Actors in which case all this does not matter and BaseClass should be valid or retrieveable anyway
			else if (LoadedObject && LoadedObject != BaseClass && !BaseClass->IsChildOf(LoadedObject) && !DefaultObject)
			{
				Log(FString("Class Mismatch  !!  ").Append(String).Append(" provided Class :  ").Append(*BaseClass->GetName()), 2);
				return false;
			}
		}
	}
	
	//  by now we either have a class or we failed, safe to exit here incase
	// some safe checks; maybe this should not be excluded by DefaultObject valid, this line previously had IsNative() as fail condition which should have been ignored only for valid Object provided
	if ((!BaseClass || !BaseClass->ClassConstructor)  && !DefaultObject)
		return false;

	// The Container we Modify
	UObject * Object = nullptr;
	// if we provide a Object we might want to modify Asset or Instance of an Object
	if (DefaultObject)
		Object = DefaultObject;
	else
		// if not then we modify CDO of resolved Class
		Object = BaseClass->GetDefaultObject();
	
	for (auto field : result->Values) {
		if (FProperty* prop = BaseClass->FindPropertyByName(*field.Key)) {
			convertJsonValueToFProperty(field.Value, prop, prop->ContainerPtrToValuePtr<void>(Object), Object);
		}
	}
	return true;
}


FString UJsonStructBPLib::Conv_BPJsonObjectValueToString(UBPJsonObjectValue * Value) { return Value->AsString(); }

float UJsonStructBPLib::Conv_BPJsonObjectValueToFloat(UBPJsonObjectValue * Value) { return Value->AsNumber(); }

int32 UJsonStructBPLib::Conv_BPJsonObjectValueToInt(UBPJsonObjectValue * Value) { return int32(Value->AsNumber()); }

bool UJsonStructBPLib::Conv_BPJsonObjectValueToBool(UBPJsonObjectValue* Value) { if (Value->GetFieldType() == EBPJson::BPJSON_Boolean) return Value->AsBoolean(); else Log("Invalid Type used as Bool ; Debug me", 0); return 0; }

UBPJsonObjectValue * UJsonStructBPLib::Conv_StringToBPJsonObjectValue(FString & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueString(Value));
	TSharedPtr<FJsonObject> objs = TSharedPtr<FJsonObject>(new FJsonObject());

	UBPJsonObjectValue* out = NewObject<UBPJsonObjectValue>();
	UBPJsonObject* obj = NewObject<UBPJsonObject>();
	obj->InnerObj = objs;
	objs->SetField("ValueObject", val);
	out->ValueObject = obj;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_FloatToBPJsonObjectValue(float & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueNumber(Value));
	TSharedPtr<FJsonObject> objs = TSharedPtr<FJsonObject>(new FJsonObject());

	UBPJsonObjectValue* out = NewObject<UBPJsonObjectValue>();
	UBPJsonObject* obj = NewObject<UBPJsonObject>();
	obj->InnerObj = objs;
	objs->SetField("ValueObject", val);
	out->ValueObject = obj;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_IntToBPJsonObjectValue(int32 & Value)
{
	TSharedPtr<FJsonValue>  val = TSharedPtr<FJsonValue>(new FJsonValueNumber(Value));
	TSharedPtr<FJsonObject> objs = TSharedPtr<FJsonObject>(new FJsonObject());

	UBPJsonObjectValue* out = NewObject<UBPJsonObjectValue>();
	UBPJsonObject* obj = NewObject<UBPJsonObject>();
	obj->InnerObj = objs;
	objs->SetField("ValueObject", val);
	out->ValueObject = obj;
	return out;
}

UBPJsonObjectValue * UJsonStructBPLib::Conv_BoolToBPJsonObjectValue(bool & Value)
{
	TSharedPtr<FJsonValue> val = TSharedPtr<FJsonValue>(new FJsonValueBoolean(Value));
	TSharedPtr<FJsonObject> objs = TSharedPtr<FJsonObject>(new FJsonObject());

	UBPJsonObjectValue * out = NewObject<UBPJsonObjectValue>();
	UBPJsonObject * obj = NewObject<UBPJsonObject>();
	obj->InnerObj = objs;
	objs->SetField("ValueObject", val);
	out->ValueObject = obj;
	return out;
}

FTransform  UJsonStructBPLib::Conv_StringToTransform(FString String)
{
	FTransform T = FTransform();
	T.InitFromString(*String);
	return T;
}

TArray<UBPJsonObjectValue *> UJsonStructBPLib::Conv_BPJsonObjectToBPJsonObjectValue(UBPJsonObject * Value)
{
	TArray<UBPJsonObjectValue *> out;
	if (!Value)
		return out;
	if (!Value->InnerObj)
		return out;
	
	for (auto  i : Value->Values)
	{
		out.Add(i.Value);
	}
	return out;
}

UBPJsonObject * UJsonStructBPLib::Conv_UBPJsonObjectValueToBPJsonObject(UBPJsonObjectValue * Value) { if (!Value) return nullptr; return Value->AsObject(); }

void UJsonStructBPLib::Conv_UClassToUProeprtyFieldNames(UStruct* Class, TArray<FString>& Array , bool Recurse) {
	if (Array.Num() > 500)
		return;
	for (auto prop = TFieldIterator<FProperty>(Class); prop; ++prop) {
		if (Recurse)
		{
			if (Cast<FObjectProperty>(*prop) && Cast<UClass>(Class))
			{
				UObject* ObjectValue = Cast<FObjectProperty>(*prop)->GetPropertyValue(prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Class)->GetDefaultObject()));
				if (ObjectValue && ObjectValue->GetClass())
				{
					Conv_UClassToUProeprtyFieldNames(ObjectValue->GetClass(), Array,Recurse);
				}
			}
			else if (Cast<FClassProperty>(*prop) && Cast<UClass>(Class))
			{
				UObject* ObjectValue = Cast<FClassProperty>(*prop)->GetPropertyValue(prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Class)->GetDefaultObject()));

				if (ObjectValue && ObjectValue->GetClass())
				{
					Conv_UClassToUProeprtyFieldNames(ObjectValue->GetClass(), Array, Recurse);
				}
			}
			
			else if (Cast<FStructProperty>(*prop))
			{
				Conv_UClassToUProeprtyFieldNames(Cast<FStructProperty>(*prop)->Struct, Array, Recurse);
			}
			


		}
		if(!Array.Contains(prop->GetName()))
			Array.Add(prop->GetName());
	}
}

