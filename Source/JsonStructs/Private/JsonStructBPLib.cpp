#include "JsonStructBPLib.h"
#include "BPJsonObjectValue.h"
#include "UObject/Class.h"
#include "UObject/UObjectAllocator.h"



void UJsonStructBPLib::Log(FString LogString, int32 Level)
{
	if (Level == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *LogString);
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

TSharedPtr<FJsonObject> UJsonStructBPLib::convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct, bool includeObjects, TArray<UObject*>& RecursedObjects,bool IncludeNonInstanced) {
	auto Obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		TSharedPtr<FJsonValue> value = convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct), includeObjects, RecursedObjects, IncludeNonInstanced);
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
		FString currentValue = strProp->GetPropertyValue(ptrToProp); 
		if (currentValue != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue).Append(" NewValue = ").Append(json->AsString()),0);
			strProp->SetPropertyValue(ptrToProp, json->AsString());
		}
	}
	else if (FTextProperty*  txtProp = Cast<FTextProperty>(prop)) {
		FText currentValue = txtProp->GetPropertyValue(ptrToProp);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FStrProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			txtProp->SetPropertyValue(ptrToProp, FText::FromString(json->AsString()));
		}
	}
	else if (FNameProperty* nameProp = Cast<FNameProperty>(prop)) {
		FName currentValue = nameProp->GetPropertyValue(ptrToProp);
		if (currentValue.ToString() != json->AsString())
		{
			Log(FString("Overwrite FNameProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			nameProp->SetPropertyValue(ptrToProp, *json->AsString());
		}

	}
	else if (FFloatProperty* fProp = Cast<FFloatProperty>(prop)) {
		float currentValue = fProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FFloatProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(FString::SanitizeFloat(currentValue)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			fProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (FIntProperty* iProp = Cast<FIntProperty>(prop)) {
		int64 currentValue = iProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsNumber())
		{
			Log(FString("Overwrite FIntProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(currentValue)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
			iProp->SetPropertyValue(ptrToProp, json->AsNumber());
		}
	}
	else if (auto bProp = Cast<FBoolProperty>(prop)) {
		bool currentValue = bProp->GetPropertyValue(ptrToProp);
		if (currentValue != json->AsBool())
		{
			Log(FString("Overwrite FBoolProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(currentValue ? "true" : "false" ).Append(" NewValue = ").Append(json->AsBool() ? "true" : "false"), 0);
			bProp->SetPropertyValue(ptrToProp, json->AsBool());
		}
	}
	else if (auto eProp = Cast<FEnumProperty>(prop)) {
		
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
	else if (auto byteProp = Cast<FByteProperty>(prop)) {
		
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
	else if (auto numProp = Cast<FNumericProperty>(prop)) {
		if (numProp->GetNumericPropertyValueToString(ptrToProp) != json->AsString())
		{
			Log(FString("Overwrite FNumericProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(numProp->GetNumericPropertyValueToString(ptrToProp)).Append(" NewValue = ").Append(json->AsString()), 0);
			numProp->SetNumericPropertyValueFromString(ptrToProp, *json->AsString());
		}
	}
	else if (auto aProp = Cast<FArrayProperty>(prop)) {
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

			convertJsonValueToFProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex),Outer);
		}
	}
	else if (auto mProp = Cast<FMapProperty>(prop)) {
		if (json->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			FScriptMapHelper MapHelper(mProp, ptrToProp);
			// this is stupid .. but somehow we need to ideally copy this somewhere or deserialize our new Map Keys
			// seperately first so we can check for values to overwrite..
			// this currently just empties the Map and writes new to it , which can be fine but not when wanting to Patch a Maps Elements Properties Filtered
			// wanted is basically : as long as there is a valid Key, the Object should remain and be converted 
			// what its doing atm : clears Map fully and writes new Values
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
					else {
						// Log error
						Log(FString("Map Property expected 'key' and 'value' Fields for Property").Append(*prop->GetName()),2);
					}
				}
			}
			MapHelper.Rehash();
		}
	}
	else if (auto cProp = Cast<FClassProperty>(prop)) {
		UClass * CastResult = nullptr;
		if (json->Type == EJson::String) {
			if (FPackageName::IsValidObjectPath(*json->AsString())) {
				CastResult = LoadObject<UClass>(NULL, *json->AsString());
				// Failsafe for script classes with BP Stubs
				if (!CastResult) {
					CastResult = FSoftClassPath(json->AsString()).TryLoadClass<UClass>();
					if (!CastResult)
					{
						FString Left; FString JsonString = json->AsString(); FString Right;
						JsonString.Split("/", &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
						Right.Split(".", &Left, &JsonString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
						if (JsonString != "") {
							CastResult = FindClassByName(Right);
						}
						else
						{
							CastResult = FindClassByName(JsonString);
						}
					}
					
				}
			}


			if (CastResult && cProp->GetPropertyValue(ptrToProp) && CastResult != cProp->GetPropertyValue(ptrToProp))
			{
				Log(FString("Overwrite FClassProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(ptrToProp)->GetPathName()).Append(" NewValue = ").Append(CastResult->GetPathName()), 0);
				cProp->SetPropertyValue(ptrToProp, CastResult);
			}
			else if (CastResult && !cProp->GetPropertyValue(ptrToProp))
			{
				Log(FString("Overwrite FClassProperty: ").Append(prop->GetName()).Append(" OldValue = nullpeter NewValue = ").Append(CastResult->GetPathName()), 0);
				cProp->SetPropertyValue(ptrToProp, CastResult);
			}
			else if (!CastResult && cProp->GetPropertyValue(ptrToProp))
			{
				Log(FString("Overwrite FClassProperty: ").Append(prop->GetName()).Append(" OldValue = ").Append(cProp->GetPropertyValue(ptrToProp)->GetPathName()).Append(" NewValue = nullpeter "), 0);
				cProp->SetPropertyValue(ptrToProp, CastResult);
			}

		}
	}
	else if (auto uProp = Cast<FObjectProperty>(prop)) {
	
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("object") && json->AsObject()->HasField("class") && json->AsObject()->HasField("objectFlags") && json->AsObject()->HasField("objectName")) {
				const TSharedPtr<FJsonValue> keyvalue = json->AsObject()->TryGetField("class");
				const TSharedPtr<FJsonValue> valuevalue = json->AsObject()->TryGetField("object");
				const TSharedPtr<FJsonValue> Namevalue = json->AsObject()->TryGetField("objectName");
				const TSharedPtr<FJsonValue> Outervalue = json->AsObject()->TryGetField("objectOuter");
				const EObjectFlags ObjectLoadFlags = (EObjectFlags)json->AsObject()->GetIntegerField((TEXT("objectFlags")));
				if (valuevalue->Type == EJson::Object) {
					if (FPackageName::IsValidObjectPath(*keyvalue->AsString())) {
						UClass* InnerBPClass = LoadObject<UClass>(NULL, *keyvalue->AsString());
						if (InnerBPClass) {

							if (uProp->GetPropertyValue(ptrToProp) && uProp->GetPropertyValue(ptrToProp)->GetClass() == InnerBPClass)
							{
								//Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append("Iterating SubObject : ").Append(uProp->GetPropertyValue(ptrToProp)->GetName()), 0);
								convertJsonObjectToUStruct(valuevalue->AsObject(), InnerBPClass, uProp->GetPropertyValue(ptrToProp), Outer);
							}
							else
							{
								UClass* OuterLoaded = LoadObject<UClass>(NULL, *Outervalue->AsString());
								UObject* Template = UObject::GetArchetypeFromRequiredInfo(InnerBPClass, Outer ? Outer : OuterLoaded, *Namevalue->AsString(), ObjectLoadFlags);
								UObject* Constructed = StaticConstructObject_Internal(InnerBPClass, Outer ? Outer : OuterLoaded ? OuterLoaded : InnerBPClass, *Namevalue->AsString(), ObjectLoadFlags, EInternalObjectFlags::None, Template);
								Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Constructed new Obejct of Class").Append(keyvalue->AsString()), 0);
								convertJsonObjectToUStruct(valuevalue->AsObject(), InnerBPClass, Constructed, Outer);
								uProp->SetObjectPropertyValue(ptrToProp, Constructed);
							}
						}
						else
						{
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Failed to load UClass for Object").Append(keyvalue->AsString()), 2);
						}
					}
				}
				else if (valuevalue->Type == EJson::String) {

					UObject* Obj = uProp->GetPropertyValue(ptrToProp);
					if (valuevalue->AsString() != "") {
						UObject* uObj = LoadObject<UObject>(NULL, *valuevalue->AsString());
						if (uObj && Obj && uObj != Obj)
						{
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with: ").Append(uObj->GetPathName())), 0);
							uProp->SetPropertyValue(ptrToProp, uObj);
						}
						else if (Obj && !uObj)
						{
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with nullpeter")), 0);
							uProp->SetPropertyValue(ptrToProp, uObj);
						}
						else if (uObj && !Obj)
						{
							Log(FString("Overwrite FObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append("nullpeter").Append("with : ").Append(uObj->GetPathName()), 0);
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
	else if (auto sProp = Cast<FStructProperty>(prop)) {
		convertJsonObjectToUStruct(json->AsObject(), sProp->Struct, ptrToProp, Outer);
	}
	else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(prop))
	{
		FWeakObjectPtr wObj = WeakObjectProperty->GetPropertyValue(ptrToProp);

		
		if (json->AsString() != "")
		{
			UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
			UObject* Obj = wObj.Get();

			if (uObj && Obj && uObj != Obj)
			{
				Log(FString("Overwrite FWeakObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with: ").Append(uObj->GetPathName())), 0);
				WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
			}
			else if (Obj && !uObj)
			{
				Log(FString("Overwrite FWeakObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with nullpeter")), 0);
				WeakObjectProperty->SetPropertyValue(ptrToProp, nullptr);
			}
			else if (uObj && !Obj)
			{
				Log(FString("Overwrite FWeakObjectProperty: ").Append(prop->GetName()).Append(" Value : ").Append("nullpeter").Append("with : ").Append(uObj->GetPathName()), 0);
				WeakObjectProperty->SetPropertyValue(ptrToProp, uObj);
			}
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

	}
	else
	{
	
	}
}


TSharedPtr<FJsonValue> UJsonStructBPLib::convertUPropToJsonValue(FProperty* prop, void* ptrToProp,bool includeObjects ,TArray<UObject*>& RecursedObjects,bool IncludeNonInstanced) {
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
			jsonArr.Add(convertUPropToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize),includeObjects, RecursedObjects, IncludeNonInstanced));
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
			UClass * BaseClass = ObjectValue->GetClass();

			
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
				&& (BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced) || IncludeNonInstanced) 
				&& (!RecursedObjects.Contains(ObjectValue));

			// skip this if we dont need it 
			if (Condition) {
				TSharedPtr<FJsonObject> ChildObj = TSharedPtr<FJsonObject>(new FJsonObject());
				RecursedObjects.Add(ObjectValue);
				const USceneComponent * Scene = Cast<USceneComponent>(ObjectValue);
				if (Scene && Scene->GetAttachmentRoot() == ObjectValue)
				{
					TArray<USceneComponent*> arr = Scene->GetAttachChildren();
					TArray<TSharedPtr<FJsonValue>> Children;
					TArray<UObject * > Temp;
					Temp.Add(ObjectValue);
					for (auto i : arr)
						ChildObj->Values.Add(i->GetName(), TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(i->GetClass(), i, includeObjects, Temp, IncludeNonInstanced))));
				}
				TSharedPtr<FJsonObject> Obj = convertUStructToJsonObject(BaseClass, ObjectValue, includeObjects, RecursedObjects, IncludeNonInstanced);
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
	else if (FMapProperty* mProp = Cast<FMapProperty>(prop)) {
		FScriptMapHelper arr(mProp, ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			TSharedPtr<FJsonValue> key = convertUPropToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i), includeObjects, RecursedObjects, IncludeNonInstanced);
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(mProp->ValueProp, arr.GetValuePtr(i), includeObjects, RecursedObjects, IncludeNonInstanced);
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
			jsonArr.Add(Obj);
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FStructProperty* sProp = Cast<FStructProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(sProp->Struct, ptrToProp, includeObjects, RecursedObjects, IncludeNonInstanced)));
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
	else if (FSetProperty* SetProperty = Cast<FSetProperty>(prop)) {
		auto& arr = SetProperty->GetPropertyValue(ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(convertUPropToJsonValue(SetProperty->ElementProp, (void*)((size_t)arr.GetData(i,SetProperty->SetLayout)), includeObjects, RecursedObjects, IncludeNonInstanced));
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (FFieldPathProperty* FieldPath = Cast<FFieldPathProperty>(prop)) {
	
		return TSharedPtr<FJsonValue>(new FJsonValueString(FieldPath->PropertyClass->GetName()));
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
	TArray<UObject*> Array;
	TSharedPtr<FJsonObject> JsonObject = convertUStructToJsonObject(Structure->Struct, StructurePtr, includeObjects, Array,false);
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
		obj->SetField(PropName, convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct), RemoveGUID,RecursedObjects,false));
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

FString UJsonStructBPLib::ClassDefaultsToJsonString(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, bool DeepRecursion )
{
	if (!ObjectClass)
		return"";

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		FString PathName = "";
		if (ObjectClass->IsNative())
		{
			PathName = ObjectClass->GetPathName();
		}
		else
		{
			PathName = ObjectClass->GetSuperClass()->GetPathName();
		}
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		JsonObject->SetField("LibClass", TSharedPtr<FJsonValue>(new FJsonValueString(PathName)));
		JsonObject->SetField("LibValue", TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion))));
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

TSharedPtr<FJsonObject> UJsonStructBPLib::CDOToJson(UClass * ObjectClass, UObject* Object, bool ObjectRecursive, bool DeepRecursion)
{
	TArray<UObject* > ResursionArray;
	FProperty * RootComponent = nullptr;
	for (auto prop = TFieldIterator<FProperty>(ObjectClass); prop; ++prop) {
		if (prop->GetName() == "RootComponent")
		{
			RootComponent = *prop;
			break;
		}
	}
	TSharedPtr<FJsonObject> Obj = TSharedPtr<FJsonObject>(new FJsonObject());

	if (RootComponent)
	{
		Obj->SetField(RootComponent->GetName(), convertUPropToJsonValue(RootComponent, RootComponent->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, ResursionArray, DeepRecursion));
	}

	for (auto prop = TFieldIterator<FProperty>(ObjectClass); prop; ++prop) {
		if (prop->GetName() != "RootComponent")
		{
			TSharedPtr<FJsonValue> value = convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, ResursionArray, DeepRecursion);
			if (value.IsValid() && value->Type != EJson::Null)
			{
				Obj->SetField(prop->GetName(), value);
			}
		}
	}
	return Obj;
}

UBPJsonObject * UJsonStructBPLib::ClassDefaultsToJsonObject(UClass *  ObjectClass, bool ObjectRecursive, UObject * DefaultObject, UObject* Outer, bool DeepRecursion)
{
	if (!ObjectClass)
		return nullptr;

	UObject * Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		FString PathName = "";
		if (ObjectClass->IsNative())
		{
			PathName = ObjectClass->GetPathName();
		}
		else
		{
			PathName = ObjectClass->GetSuperClass()->GetPathName();
		}
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		JsonObject->SetField("LibClass", TSharedPtr<FJsonValue>(new FJsonValueString(PathName)));
		JsonObject->SetField("LibValue", TSharedPtr<FJsonValue>(new FJsonValueObject(CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion))));
		UBPJsonObject * JObj = NewObject<UBPJsonObject>(Outer);
		JObj->InnerObj = JsonObject;
		JObj->InitSubObjects();
		return JObj;
	}
	return nullptr;
}

FString UJsonStructBPLib::CDOFieldsToJsonString(TArray<FString> Fields, UClass * ObjectClass, bool ObjectRecursive, bool Exclude, bool DeepRecursion)
{
	if (!ObjectClass)
		return"";

	UObject * Object = ObjectClass->GetDefaultObject();
	if (ObjectClass != NULL) {
		FString PathName = "";
		if (ObjectClass->IsNative())
		{
			PathName = ObjectClass->GetPathName();
		}
		else
		{
			PathName = ObjectClass->GetSuperClass()->GetPathName();
		}
		TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
		JsonObject->SetField("LibClass", TSharedPtr<FJsonValue>(new FJsonValueString(PathName)));
		TSharedRef<FJsonObject> Ref = CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion).ToSharedRef();
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
		TSharedPtr<FJsonValue> value = TSharedPtr<FJsonValue>(new FJsonValueObject(Ref));
		JsonObject->SetField("LibValue", value);
		return JsonObjectToString(JsonObject);
	}
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
					UE_LOG(LogTemp, Warning, TEXT("FailSafe Class Loading %s"), *String);
					LoadedObject = FindClassByName(Right);
				}
			}
			if (!BaseClass)
			{
				BaseClass = LoadedObject;
			}
			else if (LoadedObject && LoadedObject != BaseClass && !BaseClass->IsChildOf(LoadedObject) && !DefaultObject)
			{
				UE_LOG(LogTemp, Warning, TEXT("Class Mismatch  !! %s provided Class : %s"), *String, *BaseClass->GetName());
				return false;
			}
		}
	}
	
	//  by now we either have a class or we failed, safe to exit here incase
	if (!BaseClass)
		return false; 

	// some safe checks , class must not be native if we arent acting on a Package, we only supply Object when its a Package
	if (!BaseClass->ClassConstructor && BaseClass->IsNative() && !DefaultObject)
	{
		return false;
	}

	UObject * Object = nullptr;
	
	// if we are acting on Asset DefaultObject is the Asset and we use this, otherwise Class CDO 
	if (DefaultObject)
	{
		Object = DefaultObject;
	}
	else
	{
		BaseClass->GetDefaultObject();
	}
	
	for (auto field : result->Values) {
		const FString FieldName = field.Key;

		auto prop = BaseClass->FindPropertyByName(*FieldName);
		if (prop) {
			convertJsonValueToFProperty(field.Value, prop, prop->ContainerPtrToValuePtr<void>(Object), Object);
		}
	}
	return true;
}


FString UJsonStructBPLib::Conv_BPJsonObjectValueToString(UBPJsonObjectValue * Value) { return Value->AsString(); }

float UJsonStructBPLib::Conv_BPJsonObjectValueToFloat(UBPJsonObjectValue * Value) { return Value->AsNumber(); }

int32 UJsonStructBPLib::Conv_BPJsonObjectValueToInt(UBPJsonObjectValue * Value) { return int32(Value->AsNumber()); }

bool UJsonStructBPLib::Conv_BPJsonObjectValueToBool(UBPJsonObjectValue * Value) { return Value->AsBoolean(); }

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
