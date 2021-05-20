#include "JsonStructBPLib.h"
#include "BPJsonObject.h"
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

	if (UObjectRedirector* RenamedClassRedirect = FindObject<UObjectRedirector>(ANY_PACKAGE, ClassName, true))
		return CastChecked<UClass>(RenamedClassRedirect->DestinationObject);

	return nullptr;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::Conv_UStructToJsonObject(const UStruct* Struct, void* Ptr, bool IncludeObjects, TArray<UObject*>& RecursionArray,bool IncludeNonInstanced, TArray<FString> FilteredFields, bool Exclude= true) {
	auto Obj = MakeShared<FJsonObject>();
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
		TSharedPtr<FJsonValue> value = Conv_FPropertyToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(Ptr), IncludeObjects, RecursionArray, IncludeNonInstanced, FilteredFields, Exclude);
		if (value.IsValid() && value->Type != EJson::Null)
		{
			Obj->SetField(prop->GetName(), value);
		}
	}
	return Obj;
}

void UJsonStructBPLib::Conv_JsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* Ptr,UObject* Outer) {
	if (!json)
		return;
	for (const auto Field : json->Values) {
		const FString FieldName = Field.Key;
		const auto Prop = Struct->FindPropertyByName(*FieldName);
		if (Prop) {
			Conv_JsonValueToFProperty(Field.Value, Prop, Prop->ContainerPtrToValuePtr<void>(Ptr),Outer);
		}
	}
}


void UJsonStructBPLib::Conv_JsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* Prop, void* Ptr, UObject* Outer) {
	if (FStrProperty* strProp = Cast<FStrProperty>(Prop)) {
		const FString currentValue = strProp->GetPropertyValue(Ptr); 
		if (currentValue != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(currentValue).Append(" NewValue = ").Append(json->AsString()),0);
			strProp->SetPropertyValue(Ptr, json->AsString());
		}
	}
	else if (FTextProperty*  txtProp = Cast<FTextProperty>(Prop)) {
		const FText currentValue = txtProp->GetPropertyValue(Ptr);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			txtProp->SetPropertyValue(Ptr, FText::FromString(json->AsString()));
		}
	}
	else if (FNameProperty* nameProp = Cast<FNameProperty>(Prop)) {
		const FName currentValue = nameProp->GetPropertyValue(Ptr);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FNameProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			nameProp->SetPropertyValue(Ptr, *json->AsString());
		}

	}
	else if (FFloatProperty* fProp = Cast<FFloatProperty>(Prop)) {
		const float currentValue = fProp->GetPropertyValue(Ptr);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FFloatProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::SanitizeFloat(currentValue)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			fProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	}
	else if (FIntProperty* iProp = Cast<FIntProperty>(Prop)) {
		const int64 currentValue = iProp->GetPropertyValue(Ptr);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FIntProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(currentValue)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
			iProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	}
	else if (FBoolProperty* bProp = Cast<FBoolProperty>(Prop)) {
		const bool currentValue = bProp->GetPropertyValue(Ptr);
		if (currentValue != json->AsBool())
		{
			Log(FString("Overwrite FBoolProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(currentValue ? "true" : "false" ).Append(" NewValue = ").Append(json->AsBool() ? "true" : "false"), 0);
			bProp->SetPropertyValue(Ptr, json->AsBool());
		}
	}
	else if (FEnumProperty* eProp = Cast<FEnumProperty>(Prop)) {	
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
						if (NumProp && eProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(Ptr) != StringInt) {
							Log(FString("Overwrite FIntProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(eProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(StringInt), 0);
							NumProp->SetNumericPropertyValueFromString(Ptr, *StringInt);
						}
					}
				}
			}
		}	
	}
	else if (FByteProperty* byteProp = Cast<FByteProperty>(Prop)) {
		
		if (byteProp->IsEnum()) {
			if (json->Type == EJson::Object) {
				if (json->AsObject()->HasField("enumAsByte") && json->AsObject()->HasField("value")) {
					const TSharedPtr<FJsonValue> KeyValue = json->AsObject()->TryGetField("enumAsByte");
					if (FPackageName::IsValidObjectPath(*KeyValue->AsString())) {
						UEnum* EnumClass = LoadObject<UEnum>(nullptr, *KeyValue->AsString());
						if (EnumClass) {
							const TSharedPtr<FJsonValue> ValueObject = json->AsObject()->TryGetField("value");
							const TSharedPtr<FJsonValue> Byte = json->AsObject()->TryGetField("byte");
							auto ByteValue = byteProp->GetPropertyValue(Ptr);
							uint8 newValue = Byte->AsNumber();
							/*
							FString Value = *valuevalue->AsString();
							FName Name = FName(Value);
							int32 value = EnumClass->GetIndexByName(Name, EGetByNameFlags::CheckAuthoredName);
							*/
							if (newValue != ByteValue)
							{
								Log(FString("Overwrite FByteProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(ByteValue)).Append(" NewValue = ").Append(FString::FromInt(newValue)), 0);
								byteProp->SetPropertyValue(Ptr, newValue);

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
				if (NumProp && NumProp->GetSignedIntPropertyValue(Ptr) != json->AsNumber()) {
					Log(FString("Overwrite FByteProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(byteProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
					byteProp->SetNumericPropertyValueFromString(Ptr, *FString::FromInt(json->AsNumber()));
				}
			}
		}
		
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(Prop)) {
		if (ddProp->GetPropertyValue(Ptr) != json->AsNumber())
		{
			Log(FString("Overwrite FDoubleProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(ddProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			ddProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	}
	else if (FNumericProperty* numProp = Cast<FNumericProperty>(Prop)) {
		if (numProp->GetNumericPropertyValueToString(Ptr) != json->AsString())
		{
			Log(FString("Overwrite FNumericProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(numProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(json->AsString()), 0);
			numProp->SetNumericPropertyValueFromString(Ptr, *json->AsString());
		}
	}
	else if (FArrayProperty* aProp = Cast<FArrayProperty>(Prop)) {
		if (json->Type == EJson::Array)
		{
			FScriptArrayHelper helper(aProp, Ptr);
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

				Conv_JsonValueToFProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex), Outer);
			}
		}
	}
	else if (FMapProperty* mProp = Cast<FMapProperty>(Prop)) {
		if (json->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			FScriptMapHelper MapHelper(mProp, Ptr);
			TArray<uint32> Hashes;
			TArray<int32> HashesNew;
			
			for (int32 e = 0; e < MapHelper.Num(); e++)
			{
				Hashes.Add(mProp->KeyProp->GetValueTypeHash(MapHelper.GetKeyPtr(e)));
			}

			for (int i = 0; i < jsonArr.Num(); i++) {
				void* PropertyValue = FMemory::Malloc(mProp->KeyProp->GetSize());
				mProp->KeyProp->InitializeValue(PropertyValue);
				TSharedPtr<FJsonValue> KeyValue = jsonArr[i]->AsObject()->TryGetField("key");
				Conv_JsonValueToFProperty(KeyValue, mProp->KeyProp, PropertyValue, Outer);
				const uint32 Hash = mProp->KeyProp->GetValueTypeHash(PropertyValue); 
				if (!Hashes.Contains(Hash))
				{
					HashesNew.Add(i);
				}
				else
				{
					uint8* ValuePtr = MapHelper.GetValuePtr(Hashes.Find(Hash));
					TSharedPtr<FJsonValue> ObjectValue = jsonArr[i]->AsObject()->TryGetField("value");
					Conv_JsonValueToFProperty(ObjectValue, mProp->ValueProp, ValuePtr, Outer);

				}
				mProp->KeyProp->DestroyValue(PropertyValue);
				FMemory::Free(PropertyValue);
			}

			for (int32 i : HashesNew)
			{
				const int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				uint8* mapPtr = MapHelper.GetPairPtr(Index);
				TSharedPtr<FJsonValue> ObjectValue = jsonArr[i]->AsObject()->TryGetField("value");
				TSharedPtr<FJsonValue> KeyValue = jsonArr[i]->AsObject()->TryGetField("key");
				Conv_JsonValueToFProperty(KeyValue, mProp->KeyProp, mapPtr, Outer);
				Conv_JsonValueToFProperty(ObjectValue, mProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset, Outer);
			}

			MapHelper.Rehash();
		}
	}
	else if (FClassProperty* cProp = Cast<FClassProperty>(Prop)) {
		UClass * CastResult = nullptr;
		if (json->Type == EJson::String) {
			const FString ClassPath = json->AsString();
			if (ClassPath == "")
			{
				if (cProp->GetPropertyValue(Ptr) != nullptr)
				{
					Log(FString("Overwrite FClassProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(Ptr)->GetPathName()).Append(" NewValue = Nullpeter"), 0);
					cProp->SetPropertyValue(Ptr, CastResult);
				}
			}
			else
			{
				CastResult = LoadObject<UClass>(nullptr, *json->AsString());
				// Failsafe for script classes with BP Stubs
				if (!CastResult && json->AsString() != "") {
					CastResult = FSoftClassPath(json->AsString()).TryLoadClass<UClass>();
					if (!CastResult)
					{
						CastResult = FailSafeClassFind(json->AsString());
					}
				}
				if (CastResult && CastResult != cProp->GetPropertyValue(Ptr))
				{
					Log(FString("Overwrite FClassProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(Ptr)->GetPathName()).Append(" NewValue = ").Append(CastResult->GetPathName()), 0);
					cProp->SetPropertyValue(Ptr, CastResult);
				}
			}
		}
	}
	else if (FObjectProperty* uProp = Cast<FObjectProperty>(Prop)) {
	
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("object") && json->AsObject()->HasField("class") && json->AsObject()->HasField("objectFlags") && json->AsObject()->HasField("objectName")) {
				const FString KeyValue = json->AsObject()->TryGetField("class")->AsString();
				const FString NameValue = json->AsObject()->TryGetField("objectName")->AsString();
				const TSharedPtr<FJsonValue> ObjectValue = json->AsObject()->TryGetField("object");
				const TSharedPtr<FJsonValue> OuterValue = json->AsObject()->TryGetField("objectOuter");
				const EObjectFlags ObjectLoadFlags = static_cast<EObjectFlags>(json->AsObject()->GetIntegerField((TEXT("objectFlags"))));
				if (ObjectValue->Type == EJson::Object && KeyValue != "") {
					UClass* InnerBPClass = LoadObject<UClass>(nullptr, *KeyValue);
					if (InnerBPClass) {
						const TSharedPtr<FJsonObject> Obj = ObjectValue->AsObject();
						UObject* Value = uProp->GetPropertyValue(Ptr);
						if (Value && Value->GetClass() == InnerBPClass)
						{
							UClass* OuterLoaded = LoadObject<UClass>(nullptr, *OuterValue->AsString());
							if (OuterLoaded && OuterLoaded != Outer)
							{
								Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" using != Outer !").Append(KeyValue).Append(" actual Outer : ").Append(OuterLoaded->GetPathName()), 0);
								Conv_JsonObjectToUStruct(Obj, InnerBPClass, Value, OuterLoaded);
							}
							else
							{
								Conv_JsonObjectToUStruct(Obj, InnerBPClass, Value, Outer);
							}
						}
						else
						{
							UObject* Template = UObject::GetArchetypeFromRequiredInfo(InnerBPClass, Outer, *NameValue, ObjectLoadFlags);
							UObject* Constructed = StaticConstructObject_Internal(InnerBPClass, Outer, *NameValue, ObjectLoadFlags, EInternalObjectFlags::None, Template);
							Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Constructed new Obejct of Class").Append(KeyValue), 0);
							Conv_JsonObjectToUStruct(Obj, InnerBPClass, Constructed, Outer);
							uProp->SetObjectPropertyValue(Ptr, Constructed);
						}
					}
					else
					{
						Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Failed to load UClass for Object").Append(KeyValue), 2);
					}
				}
				else if (ObjectValue->Type == EJson::String) {

					UObject* Obj = uProp->GetPropertyValue(Ptr);
					if (ObjectValue->AsString() != "") {
						UObject* UObj = LoadObject<UObject>(nullptr, *ObjectValue->AsString());
						if (!UObj)
						{
							Log(FString("Skipped Overwrite Reason[Load Fail] FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
						}
						else
						{
							Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
							uProp->SetPropertyValue(Ptr, UObj);
						}
					}
					else if (Obj)
					{
						Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with nullpeter")), 0);
						uProp->SetPropertyValue(Ptr, nullptr);
					}
				}
			}
		}
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(Prop)) {
		Conv_JsonObjectToUStruct(json->AsObject(), sProp->Struct, Ptr, Outer);
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Prop))
	{
		FWeakObjectPtr wObj = WeakObjectProperty->GetPropertyValue(Ptr);

		
		if (json->AsString() != "")
		{
			UObject* UObj = FSoftObjectPath(json->AsString()).TryLoad();
			UObject* Obj = wObj.Get();

				Log(FString("Overwrite WeakObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
				WeakObjectProperty->SetPropertyValue(Ptr, UObj);
		}
		else
		{
			if (wObj.IsValid())
			{
				Log(FString("Overwrite FWeakObjectProperty: ").Append(Prop->GetName()).Append(" with nullpeter"), 0);
				WeakObjectProperty->SetPropertyValue(Ptr, nullptr);
			}
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Prop))
	{
		if (json->AsString() != "")
		{
			const FString PathString = json->AsString();
			FSoftObjectPtr* ObjectPtr = static_cast<FSoftObjectPtr*>(Ptr);
			*ObjectPtr = FSoftObjectPath(PathString);
		}
	}
	else if (auto* CastProp = CastField<FMulticastSparseDelegateProperty>(Prop))
	{
	//ExportTextItem
	}
	else
	{
	
	}
}


TSharedPtr<FJsonValue> UJsonStructBPLib::Conv_FPropertyToJsonValue(FProperty* Prop, void* Ptr,bool IncludeObjects ,TArray<UObject*>& RecursionArray,bool DeepRecursion, TArray<FString> FilteredFields, bool Exclude) {
	if (FStrProperty* strProp = Cast<FStrProperty>(Prop)) {
		return MakeShared<FJsonValueString>(strProp->GetPropertyValue(Ptr));
	}
	if (FTextProperty* txtProp = Cast<FTextProperty>(Prop)) {
		return MakeShared<FJsonValueString>(txtProp->GetPropertyValue(Ptr).ToString());
	}
	if (FNameProperty* nameProp = Cast<FNameProperty>(Prop)) {
		return MakeShared<FJsonValueString>(nameProp->GetPropertyValue(Ptr).ToString());
	}
	else if (FFloatProperty* fProp = Cast<FFloatProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(fProp->GetPropertyValue(Ptr));
	}
	else if (FIntProperty* iProp = Cast<FIntProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(iProp->GetPropertyValue(Ptr));
	}
	else if (FBoolProperty* bProp = Cast<FBoolProperty>(Prop)) {
		return MakeShared<FJsonValueBoolean>(bProp->GetPropertyValue(Ptr));
	}
	else if (FEnumProperty* eProp = Cast<FEnumProperty>(Prop)) {
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		const TSharedPtr<FJsonValue> key = MakeShared<FJsonValueString>(eProp->GetEnum()->GetPathName());
		const TSharedPtr<FJsonValue> value = MakeShared<FJsonValueString>(
			eProp->GetEnum()->GetNameByValue(eProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(Ptr)).
			       ToString());
		JsonObject->SetField("enum", key);
		JsonObject->SetField("value", value);
		return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
	}
	else if (FDoubleProperty* ddProp = Cast<FDoubleProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(ddProp->GetPropertyValue(Ptr));
	}
	else if (FByteProperty* byProp = Cast<FByteProperty>(Prop)) {
		if (byProp->IsEnum())
		{
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			TSharedPtr<FJsonValue> key = MakeShared<FJsonValueString>(byProp->Enum->GetPathName());
			TSharedPtr<FJsonValue> value = MakeShared<FJsonValueString>(
				byProp->Enum->GetNameByValue(byProp->GetSignedIntPropertyValue(Ptr)).ToString());
			TSharedPtr<FJsonValue> byte = MakeShared<FJsonValueNumber>(byProp->GetPropertyValue(Ptr));
			JsonObject->SetField("enumAsByte", key);
			JsonObject->SetField("value", value);
			JsonObject->SetField("byte", byte);
			return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
		}
		else
			return MakeShared<FJsonValueNumber>(byProp->GetSignedIntPropertyValue(Ptr));
	}
	else if (FNumericProperty* nProp = Cast<FNumericProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(nProp->GetUnsignedIntPropertyValue(Ptr));
	}
	else if (FArrayProperty* aProp = Cast<FArrayProperty>(Prop)) {
		auto& arr = aProp->GetPropertyValue(Ptr);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(Conv_FPropertyToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields,Exclude));
		}
		return MakeShared<FJsonValueArray>(jsonArr);
	}
	else if (FMapProperty* mProp = Cast<FMapProperty>(Prop)) {
		FScriptMapHelper arr(mProp, Ptr);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			auto ptr = arr.GetValuePtr(i);
			
			TSharedPtr<FJsonValue> key = Conv_FPropertyToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields,Exclude);
			TSharedPtr<FJsonValue> value = Conv_FPropertyToJsonValue(mProp->ValueProp, arr.GetValuePtr(i), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields,Exclude);
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = MakeShared<FJsonValueObject>(JsonObject);
			jsonArr.Add(Obj);
		}
		return MakeShared<FJsonValueArray>(jsonArr);
	}
	else if (FSetProperty* SetProperty = Cast<FSetProperty>(Prop)) {
		auto& arr = SetProperty->GetPropertyValue(Ptr);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(Conv_FPropertyToJsonValue(SetProperty->ElementProp, (void*)((size_t)arr.GetData(i, SetProperty->SetLayout)), IncludeObjects, RecursionArray, DeepRecursion,FilteredFields, Exclude));
		}
		return MakeShared<FJsonValueArray>(jsonArr);
	}
	else if (FClassProperty* cProp = Cast<FClassProperty>(Prop)) {
		if (cProp->GetPropertyValue(Ptr))
		{
			return MakeShared<FJsonValueString>(cProp->GetPropertyValue(Ptr)->GetPathName());
		}
		else
		{
			return MakeShared<FJsonValueString>("");
		}
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Prop))
	{
		if (WeakObjectProperty->GetPropertyValue(Ptr).IsValid())
		{
			return MakeShared<FJsonValueString>(
				FSoftObjectPath(WeakObjectProperty->GetPropertyValue(Ptr).Get()).ToString());
		}
		else
		{
			return MakeShared<FJsonValueString>("");
		}
	}
	// Check to see if this is a simple soft object property (eg. not an array of soft objects).
	else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Prop))
	{
		if (SoftObjectProperty->GetPropertyValue(Ptr).IsValid())
		{
			return MakeShared<FJsonValueString>(
				SoftObjectProperty->GetPropertyValue(Ptr).ToSoftObjectPath().ToString());
		}
		else
		{
			return MakeShared<FJsonValueString>("");
		}
	}
	else if (FObjectProperty* oProp = Cast<FObjectProperty>(Prop)) {
		if (Ptr && oProp->GetPropertyValue(Ptr))
		{
			UObject* ObjectValue = oProp->GetPropertyValue(Ptr);
			const UClass* BaseClass = ObjectValue->GetClass();
			const TSharedPtr<FJsonValue> key = MakeShared<FJsonValueString>(BaseClass->GetPathName());
			const TSharedPtr<FJsonValue> value = MakeShared<FJsonValueString>(ObjectValue->GetPathName());
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetField("class", key);


			// Everything that is streamable or AssetUserData ( aka Meshs and Textures etc ) will be skipped
			for (auto i : BaseClass->Interfaces)
			{
				if (i.Class == UInterface_AssetUserData::StaticClass() || i.Class == UStreamableRenderAsset::StaticClass())
				{
					JsonObject->SetField("object", value);
					const TSharedPtr<FJsonValueObject> Obj = MakeShared<FJsonValueObject>(JsonObject);
					return TSharedPtr<FJsonValue>(Obj);
				}
			}

			// We do want include Objects
			// Object is defaulting to Instanced or we Include Non Instnaced
			// Property Value must not be nullpeter
			// Property Object wasnt already serialized or is RootComponent based
			const bool Condition = IncludeObjects
				&& ((BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_EditInlineNew)) || DeepRecursion)
				&& (!RecursionArray.Contains(ObjectValue)) && RecursionArray.Num() < 500;

			// skip this if we dont need it 
			if (Condition) {
				TSharedPtr<FJsonObject> ChildObj = MakeShared<FJsonObject>();
				RecursionArray.Add(ObjectValue);
				USceneComponent* Scene = Cast<USceneComponent>(ObjectValue);
				if (Scene && Scene->GetAttachmentRoot() == ObjectValue)
				{
					TArray<USceneComponent*> arr = Scene->GetAttachChildren();
					for (auto i : arr)
					{
						RecursionArray.Add(i);
						ChildObj->Values.Add(i->GetName(),
						                     MakeShared<FJsonValueObject>(Conv_UStructToJsonObject(i->GetClass(), i,
							                     IncludeObjects, RecursionArray, DeepRecursion, FilteredFields,
							                     Exclude)));
					}
				}
				TSharedPtr<FJsonObject> Obj = Conv_UStructToJsonObject(BaseClass, ObjectValue, IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude);
				JsonObject->SetField("object", MakeShared<FJsonValueObject>(Obj));
				JsonObject->SetField("objectFlags", MakeShared<FJsonValueNumber>(int32(ObjectValue->GetFlags())));
				JsonObject->SetField("objectName", MakeShared<FJsonValueString>(ObjectValue->GetName()));
				JsonObject->SetField("objectOuter",
				                     MakeShared<FJsonValueString>(ObjectValue->GetOutermost()->GetFName().ToString()));
				JsonObject->SetField("Children:", MakeShared<FJsonValueObject>(ChildObj));
				return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
			}
			else
			{
				JsonObject->SetField("object", value);
				return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
			}
		}
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(Prop)) {
		return MakeShared<FJsonValueObject>(Conv_UStructToJsonObject(sProp->Struct, Ptr, IncludeObjects,
		                                                               RecursionArray, DeepRecursion, FilteredFields,
		                                                               Exclude));
	}
	else if (!DeepRecursion)
	{
		return MakeShared<FJsonValueNull>();
	}
	else if (FMulticastSparseDelegateProperty* CastProp = Cast<FMulticastSparseDelegateProperty>(Prop))
	{
		if (CastProp->SignatureFunction)
		{
			return MakeShared<FJsonValueString>(CastProp->SignatureFunction->GetName());
		}
	}
	else if (FMulticastInlineDelegateProperty* mCastProp = Cast<FMulticastInlineDelegateProperty>(Prop))
	{
		if (mCastProp->SignatureFunction)
		{
			return MakeShared<FJsonValueString>(mCastProp->SignatureFunction->GetName());
		}
	}
	else if (FDelegateProperty* Delegate = Cast<FDelegateProperty>(Prop)) {
		if (Delegate->SignatureFunction)
		{
			return MakeShared<FJsonValueString>(Delegate->SignatureFunction->GetName());
		}
	}
	else if (FInterfaceProperty* InterfaceProperty = Cast<FInterfaceProperty>(Prop)) {
		if (InterfaceProperty->InterfaceClass)
		{
			return MakeShared<FJsonValueString>(InterfaceProperty->InterfaceClass->GetPathName());
		}
		else
		{
			return MakeShared<FJsonValueString>("");

		}
	}
	else if (FFieldPathProperty* FieldPath = Cast<FFieldPathProperty>(Prop)) {
	
		return MakeShared<FJsonValueString>(FieldPath->PropertyClass->GetName());
	}
	else
	{
		// Log Debug
	}
	return MakeShared<FJsonValueNull>();

}

void UJsonStructBPLib::InternalGetStructAsJson(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,bool IncludeObjects)
{
	TArray<UObject*> Array;
	const TSharedPtr<FJsonObject> JsonObject = Conv_UStructToJsonObject(Structure->Struct, StructurePtr, IncludeObjects, Array,false, TArray<FString>(), true);
	String = JsonObjectToString(JsonObject);
}

void UJsonStructBPLib::InternalGetStructAsJsonForTable(FStructProperty *Structure, void* StructurePtr, FString &String, bool RemoveGUID,FString Name )
{
	const TSharedPtr<FJsonObject> JsonObject = ConvertUStructToJsonObjectWithName(Structure->Struct, StructurePtr, RemoveGUID,Name);
	String = JsonObjectToString(JsonObject);
}

TSharedPtr<FJsonObject> UJsonStructBPLib::ConvertUStructToJsonObjectWithName(UStruct * Struct, void * ptrToStruct,bool RemoveGUID,  FString Name)
{
	TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
	obj->SetStringField("Name", Name);
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(prop->GetName()) : prop->GetName();
		TArray<UObject* > IteratedObjects;
		obj->SetField(PropName, Conv_FPropertyToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct), RemoveGUID,IteratedObjects,false,TArray<FString>(), true));
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

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetField("LibClass", MakeShared<FJsonValueString>(
		                     Class->IsNative() ? Class->GetPathName() : Class->GetSuperClass()->GetPathName()));
	if (Cast<AActor>(Object) && Object != Class->GetDefaultObject())
	{
		JsonObject->SetField("LibTransform",
		                     MakeShared<FJsonValueString>(Cast<AActor>(Object)->GetTransform().ToString()));
	}
	JsonObject->SetField("LibOuter", MakeShared<FJsonValueString>(Object->GetPathName()));
	return JsonObject;
}

FString UJsonStructBPLib::ObjectToJsonString(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, bool DeepRecursion , bool SkipRoot, bool SkipTransient,bool OnlyEditable)
{
	if (!ObjectClass)
		return"";

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("LibValue", MakeShared<FJsonValueObject>(CDOToJson(
			                     ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient,
			                     OnlyEditable, TArray<FString>(), true)));
		return JsonObjectToString(JsonObject);
	}
	else
		return "";
}


FString UJsonStructBPLib::JsonObjectToString(TSharedPtr<FJsonObject> JsonObject)
{
	FString write;
	const TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return write;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::CDOToJson(UClass * ObjectClass, UObject* Object, bool ObjectRecursive, bool DeepRecursion, bool SkipRoot,bool SkipTransient,bool OnlyEditable, TArray<FString> FilteredFields, bool Exclude)
{
	TArray<UObject* > RecursionArray;
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

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
			Obj->SetField(RootComponent->GetName(), Conv_FPropertyToJsonValue(RootComponent, RootComponent->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, RecursionArray, DeepRecursion, FilteredFields, Exclude));
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
			TSharedPtr<FJsonValue> value = Conv_FPropertyToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, RecursionArray, DeepRecursion, FilteredFields, Exclude);
			if (value.IsValid() && value->Type != EJson::Null)
			{
				Obj->SetField(prop->GetName(), value);
			}
		}
	}

	return Obj;
}

void UJsonStructBPLib::ObjectToJsonObject(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, UObject* Outer, bool DeepRecursion,bool SkipRoot, bool SkipTransient,bool OnlyEditable,FBPJsonObject& JObject)
{
	if (!ObjectClass)
		return;

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("LibValue", MakeShared<FJsonValueObject>(CDOToJson(
			                     ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient,
			                     OnlyEditable, TArray<FString>(), true)));
		JObject = FBPJsonObject(EBPJson::BPJSON_Object, JsonObject, "");

	}
	return;
}


void UJsonStructBPLib::ObjectToJsonObjectFiltered(TArray<FString> Fields, UClass* ObjectClass, UObject* DefaultObject, UObject* Outer, bool ObjectRecursive, bool Exclude, bool DeepRecursion, bool SkipRoot, bool SkipTransient, bool OnlyEditable, FBPJsonObject& JObject)
{
	if (!ObjectClass)
		return;

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		const TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueObject>(
			CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable,
			          Fields, Exclude).ToSharedRef());
		JsonObject->SetField("LibValue", Value);	
		JObject = FBPJsonObject(EBPJson::BPJSON_Object, JsonObject, "");
	}
	return;
}

FString UJsonStructBPLib::ObjectToJsonStringFiltered(TArray<FString> Fields, UClass * ObjectClass,  UObject* DefaultObject, bool ObjectRecursive, bool Exclude, bool DeepRecursion, bool SkipRoot , bool SkipTransient,bool OnlyEditable)
{
	if (!ObjectClass)
		return"";

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		const TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueObject>(
			CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable,
			          Fields, Exclude).ToSharedRef());
		JsonObject->SetField("LibValue", Value);
		return JsonObjectToString(JsonObject);
	}
	else
		return "";
}




// Function Arch wrote we borrow here
// changed UClass to UDynamicClass and added MountPoint Registration in case of this being on one
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
		const FString ClassMountPoint = Left;

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

	const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*StringIn);
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
			// if the Class Input was invalid we can fallback to Loaded Class
			if (!BaseClass && LoadedObject)
			{
				BaseClass = LoadedObject;
			}
			// if LoadedClass is valid and is not equal to the input and not a Child of it either , and we dont provide DefaultObject 
			// then something weird is going on; DefaultObject provided from Blueprint can only be valid for 
			// Assets or Actors in which case all this does not matter and BaseClass should be valid or retrieve able anyway
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
	
	for (auto Field : result->Values) {
		if (FProperty* prop = BaseClass->FindPropertyByName(*Field.Key)) {
			Conv_JsonValueToFProperty(Field.Value, prop, prop->ContainerPtrToValuePtr<void>(Object), Object);
		}
	}
	return true;
}




FTransform  UJsonStructBPLib::Conv_StringToTransform(FString String)
{
	FTransform T = FTransform();
	T.InitFromString(*String);
	return T;
}


void UJsonStructBPLib::Conv_UClassToPropertyFieldNames(UStruct* Structure, TArray<FString>& Array , const bool Recurse) {
	if (Array.Num() > 500)
		return;
	for (auto Prop = TFieldIterator<FProperty>(Structure); Prop; ++Prop) {
		if (Recurse)
		{
			if (Cast<FObjectProperty>(*Prop) && Cast<UClass>(Structure))
			{
				UObject* ObjectValue = Cast<FObjectProperty>(*Prop)->GetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Structure)->GetDefaultObject()));
				if (ObjectValue && ObjectValue->GetClass())
				{
					Conv_UClassToPropertyFieldNames(ObjectValue->GetClass(), Array,Recurse);
				}
			}
			else if (Cast<FClassProperty>(*Prop) && Cast<UClass>(Structure))
			{
				UObject* ObjectValue = Cast<FClassProperty>(*Prop)->GetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Structure)->GetDefaultObject()));

				if (ObjectValue && ObjectValue->GetClass())
				{
					Conv_UClassToPropertyFieldNames(ObjectValue->GetClass(), Array, Recurse);
				}
			}
			
			else if (Cast<FStructProperty>(*Prop))
			{
				Conv_UClassToPropertyFieldNames(Cast<FStructProperty>(*Prop)->Struct, Array, Recurse);
			}
			


		}
		if(!Array.Contains(Prop->GetName()))
			Array.Add(Prop->GetName());
	}
}

