#include "JsonStructBPLib.h"
#include "BPJsonObject.h"
#include "UObject/Class.h"
#include "UObject/CoreRedirects.h"
#include "Internationalization/PackageLocalizationManager.h"
#include "Engine/AssetManager.h" 
#include "UObject/UObjectAllocator.h"
#include "..\Public\JsonStructBPLib.h"

DEFINE_LOG_CATEGORY(JsonStructs_Log);

void UJsonStructBPLib::Log(FString LogString, int32 Level)
{
	if (Level == 0) {
		UE_LOG(JsonStructs_Log, Display, TEXT("%s"), *LogString);
	} else if (Level == 1) {
		UE_LOG(JsonStructs_Log, Warning, TEXT("%s"), *LogString);
	} else if (Level == 2) {
		UE_LOG(JsonStructs_Log, Error, TEXT("%s"), *LogString);
	}
}

FString UJsonStructBPLib::RemoveUStructGuid(FString String)
{
	FString Replace = "";
	FString Cutoff;
	String.Split("_", &Replace, &Cutoff, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (Cutoff == "")
		return String;

	return Replace;
}

UClass* UJsonStructBPLib::FindClassByName(const FString ClassNameInput)
{
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

TSharedPtr<FJsonObject> UJsonStructBPLib::Conv_UStructToJsonObject(const UStruct* Struct, void* Ptr, bool IncludeObjects, TArray<UObject*>& RecursionArray, bool IncludeNonInstanced, TArray<FString> FilteredFields, bool Exclude = true)
{
	auto Obj = MakeShared<FJsonObject>();
	for (auto prop = TFieldIterator<FProperty>(Struct); prop; ++prop) {
		if (FilteredFields.Contains(prop->GetName())) {
			if (Exclude)
				continue;
		} else {
			if (!Exclude)
				continue;
		}
		TSharedPtr<FJsonValue> value = Conv_FPropertyToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(Ptr), IncludeObjects, RecursionArray, IncludeNonInstanced, FilteredFields, Exclude);
		if (value.IsValid() && value->Type != EJson::Null) {
			Obj->SetField(prop->GetName(), value);
		}
	}
	return Obj;
}

void UJsonStructBPLib::Conv_JsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* Ptr, UObject* Outer)
{
	if (!json)
		return;
	for (const auto& Field : json->Values) {
		const FString FieldName = Field.Key;
		const auto Prop = Struct->FindPropertyByName(*FieldName);
		if (Prop) {
			Conv_JsonValueToFProperty(Field.Value, Prop, Prop->ContainerPtrToValuePtr<void>(Ptr), Outer);
		}
	}
}

void UJsonStructBPLib::Conv_JsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* Prop, void* Ptr, UObject* Outer, bool DoLog)
{
	if (FStrProperty* StrProp = CastField<FStrProperty>(Prop)) {
		const FString currentValue = StrProp->GetPropertyValue(Ptr);
		if (currentValue != json->AsString()) {
			if (DoLog)
				Log(FString("Overwrite FStrProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(currentValue).Append(" NewValue = ").Append(json->AsString()), 0);
			StrProp->SetPropertyValue(Ptr, json->AsString());
		}
	} else if (FTextProperty* TxtProp = CastField<FTextProperty>(Prop)) {
		const FText CurrentValue = TxtProp->GetPropertyValue(Ptr);
		if (CurrentValue.ToString() != json->AsString()) {
			if (DoLog)
				Log(FString("Overwrite FStrProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(CurrentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			TxtProp->SetPropertyValue(Ptr, FText::FromString(json->AsString()));
		}
	} else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop)) {
		const FName CurrentValue = NameProp->GetPropertyValue(Ptr);
		if (CurrentValue.ToString() != json->AsString()) {
			if (DoLog)
				Log(FString("Overwrite FNameProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(CurrentValue.ToString()).Append(" NewValue = ").Append(json->AsString()), 0);
			NameProp->SetPropertyValue(Ptr, *json->AsString());
		}
	} else if (FFloatProperty* FProp = CastField<FFloatProperty>(Prop)) {
		const float CurrentValue = FProp->GetPropertyValue(Ptr);
		if (CurrentValue != json->AsNumber()) {
			if (DoLog)
				Log(FString("Overwrite FFloatProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::SanitizeFloat(CurrentValue)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			FProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	} else if (FIntProperty* IProp = CastField<FIntProperty>(Prop)) {
		const int64 CurrentValue = IProp->GetPropertyValue(Ptr);
		if (CurrentValue != json->AsNumber()) {
			if (DoLog)
				Log(FString("Overwrite FIntProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(CurrentValue)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
			IProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	} else if (FBoolProperty* BProp = CastField<FBoolProperty>(Prop)) {
		const bool bCurrentValue = BProp->GetPropertyValue(Ptr);
		if (bCurrentValue != json->AsBool()) {
			if (DoLog)
				Log(FString("Overwrite FBoolProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(bCurrentValue ? "true" : "false").Append(" NewValue = ").Append(json->AsBool() ? "true" : "false"), 0);
			BProp->SetPropertyValue(Ptr, json->AsBool());
		}
	} else if (FEnumProperty* EProp = CastField<FEnumProperty>(Prop)) {
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("JS_Enum") && json->AsObject()->HasField("JS_Value")) {
				const FString KeyValue = json->AsObject()->TryGetField("JS_Enum")->AsString();
				if (FPackageName::IsValidObjectPath(*KeyValue)) {
					UEnum* EnumClass = LoadObject<UEnum>(nullptr, *KeyValue);
					if (EnumClass) {
						FString Value = *json->AsObject()->TryGetField("JS_Value")->AsString();
						FNumericProperty* NumProp = CastField<FNumericProperty>(EProp->GetUnderlyingProperty());
						int32 intValue = EnumClass->GetIndexByName(FName(Value), EGetByNameFlags::CheckAuthoredName);
						FString StringInt = FString::FromInt(intValue);
						if (NumProp && EProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(Ptr) != StringInt) {
							if (DoLog)
								Log(FString("Overwrite FIntProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(EProp->GetUnderlyingProperty()->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(StringInt), 0);
							NumProp->SetNumericPropertyValueFromString(Ptr, *StringInt);
						}
					}
				}
			}
		}
	} else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop)) {
		if (ByteProp->IsEnum()) {
			if (json->Type == EJson::Object) {
				if (json->AsObject()->HasField("JS_EnumAsByte") && json->AsObject()->HasField("JS_Value")) {
					const TSharedPtr<FJsonValue> KeyValue = json->AsObject()->TryGetField("JS_EnumAsByte");
					if (FPackageName::IsValidObjectPath(*KeyValue->AsString())) {
						UEnum* EnumClass = LoadObject<UEnum>(nullptr, *KeyValue->AsString());
						if (EnumClass) {
							const TSharedPtr<FJsonValue> ValueObject = json->AsObject()->TryGetField("JS_Value");
							const TSharedPtr<FJsonValue> Byte = json->AsObject()->TryGetField("JS_Byte");
							auto ByteValue = ByteProp->GetPropertyValue(Ptr);
							uint8 NewValue = Byte->AsNumber();
							if (NewValue != ByteValue) {
								if (DoLog)
									Log(FString("Overwrite FByteProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(FString::FromInt(ByteValue)).Append(" NewValue = ").Append(FString::FromInt(NewValue)), 0);
								ByteProp->SetPropertyValue(Ptr, NewValue);

							}
						}
					}
				}
			}
		} else {
			if (json->Type == EJson::Number) {
				FNumericProperty* NumProp = CastField<FNumericProperty>(ByteProp);
				if (NumProp && NumProp->GetSignedIntPropertyValue(Ptr) != json->AsNumber()) {
					if (DoLog)
						Log(FString("Overwrite FByteProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(ByteProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(FString::FromInt(json->AsNumber())), 0);
					ByteProp->SetNumericPropertyValueFromString(Ptr, *FString::FromInt(json->AsNumber()));
				}
			}
		}
	} else if (FDoubleProperty* DdProp = CastField<FDoubleProperty>(Prop)) {
		if (DdProp->GetPropertyValue(Ptr) != json->AsNumber()) {
			if (DoLog)
				Log(FString("Overwrite FDoubleProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(DdProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(FString::SanitizeFloat(json->AsNumber())), 0);
			DdProp->SetPropertyValue(Ptr, json->AsNumber());
		}
	} else if (FNumericProperty* NumProp = CastField<FNumericProperty>(Prop)) {
		if (NumProp->GetNumericPropertyValueToString(Ptr) != json->AsString()) {
			if (DoLog)
				Log(FString("Overwrite FNumericProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(NumProp->GetNumericPropertyValueToString(Ptr)).Append(" NewValue = ").Append(json->AsString()), 0);
			NumProp->SetNumericPropertyValueFromString(Ptr, *json->AsString());
		}
	} else if (FArrayProperty* AProp = CastField<FArrayProperty>(Prop)) {
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("JS_Values")) {
				const TArray<TSharedPtr<FJsonValue>> Values = json->AsObject()->TryGetField("JS_Values")->AsArray();
				bool Replace = false; bool Remove = false; bool Unique = false;
				if (json->AsObject()->HasField("JS_Replace"))
					Replace = json->AsObject()->TryGetField("JS_Replace")->AsBool();

				if (json->AsObject()->HasField("JS_Remove"))
					Remove = json->AsObject()->TryGetField("JS_Remove")->AsBool();

				if (json->AsObject()->HasField("JS_UniqueElements"))
					Unique = json->AsObject()->TryGetField("JS_UniqueElements")->AsBool();

				FScriptArrayHelper Helper(AProp, Ptr);
				// in order to compare values we want to keep values when overwriting children 

				if (Replace && !Remove) {
					if (Helper.Num() > Values.Num()) {
						Helper.Resize(Values.Num());
					}
					for (int i = 0; i < Values.Num(); i++) {
						int64 valueIndex = i;
						// only add values when size is smaller then requested ptr Slot
						if (Helper.Num() <= i)
							valueIndex = Helper.AddValue();

						Conv_JsonValueToFProperty(Values[i], AProp->Inner, Helper.GetRawPtr(valueIndex), Outer);
					}
				} else if (!Replace) {
					TArray<uint32> Hashes;

					// in case of the Property not being Hash-able we Serialize the property and compare it with the Input
					// this has quite a lot of issues with Inclusive Edits but those Limitations have to be dealt with for now 
					// Examples are Structs, for a correct removal or Unique adding an exact Match needs to be found
					TArray<FString> StringHashes;
					bool UseStringHash = false;
					// if 
					if ((Unique || Remove) && AProp->Inner->HasAnyPropertyFlags(CPF_HasGetValueTypeHash)) {
						for (int32 e = 0; e < Helper.Num(); e++) {
							Hashes.Add(AProp->GetValueTypeHash(Helper.GetRawPtr(e)));
						}
					} else if (Unique || Remove) {
						for (int32 e = 0; e < Helper.Num(); e++) {
							TArray<UObject*> Recurse;
							TSharedPtr<FJsonValue> Compare = Conv_FPropertyToJsonValue(AProp->Inner, Helper.GetRawPtr(e), true, Recurse, false, TArray<FString>(), true);
							TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
							JsonObject->SetField("JS_Compare", Compare);
							FString out = JsonObjectToString(JsonObject);
							StringHashes.Add(out);
						}
						// cant be Hashes so we cant Remove or Add Uniquely 
						UseStringHash = true;
					}
					for (int i = 0; i < Values.Num(); i++) {
						// Unique Add , must not contain Hash Value for Add
						if (Unique && !Remove) {
							if (!UseStringHash) {
								void* PropertyValue = FMemory::Malloc(AProp->Inner->GetSize());
								AProp->Inner->InitializeValue(PropertyValue);
								Conv_JsonValueToFProperty(Values[i], AProp->Inner, PropertyValue, Outer, false);
								const uint32 Hash = AProp->Inner->GetValueTypeHash(PropertyValue);
								if (!Hashes.Contains(Hash)) {
									Conv_JsonValueToFProperty(Values[i], AProp->Inner, Helper.GetRawPtr(Helper.AddValue()), Outer);
								}
								AProp->Inner->DestroyValue(PropertyValue);
								FMemory::Free(PropertyValue);
							} else {
								TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
								JsonObject->SetField("JS_Compare", Values[i]);
								FString out = JsonObjectToString(JsonObject);
								if (!StringHashes.Contains(out)) {
									Conv_JsonValueToFProperty(Values[i], AProp->Inner, Helper.GetRawPtr(Helper.AddValue()), Outer);
								}
							}
						} else if (Remove) {
							// try removing Elements if Hash exists
							if (!UseStringHash) {
								void* PropertyValue = FMemory::Malloc(AProp->Inner->GetSize());
								AProp->Inner->InitializeValue(PropertyValue);
								Conv_JsonValueToFProperty(Values[i], AProp->Inner, PropertyValue, Outer, false);
								const uint32 Hash = AProp->Inner->GetValueTypeHash(PropertyValue);
								if (Hashes.Contains(Hash)) {
									Helper.RemoveValues((Hashes.Find(Hash)), 1);
								}
								AProp->Inner->DestroyValue(PropertyValue);
								FMemory::Free(PropertyValue);
							} else {
								TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
								JsonObject->SetField("JS_Compare", Values[i]);
								FString out = JsonObjectToString(JsonObject);
								if (StringHashes.Contains(out)) {
									Helper.RemoveValues((StringHashes.Find(out)), 1);
								}
							}
						} else {
							// just adding we dont care about anything
							Conv_JsonValueToFProperty(Values[i], AProp->Inner, Helper.GetRawPtr(Helper.AddValue()), Outer);
						}
					}
				}
			}
		}
	} else if (FMapProperty* MProp = CastField<FMapProperty>(Prop)) {
		if (json->Type == EJson::Array) {
			TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
			FScriptMapHelper MapHelper(MProp, Ptr);
			TArray<uint32> MapHashes = {};
			TArray<int32> HashesNew = {};

			for (int32 e = 0; e < MapHelper.Num(); e++) {
				uint32 Hash = MProp->KeyProp->GetValueTypeHash(MapHelper.GetKeyPtr(e));
				MapHashes.Add(Hash);
			}

			for (int i = 0; i < jsonArr.Num(); i++) {
				void* PropertyValue = FMemory::Malloc(MProp->KeyProp->GetSize());
				MProp->KeyProp->InitializeValue(PropertyValue);
				TSharedPtr<FJsonValue> KeyValue = jsonArr[i]->AsObject()->TryGetField("JS_Key");
				Conv_JsonValueToFProperty(KeyValue, MProp->KeyProp, PropertyValue, Outer, false);
				const uint32 Hash = MProp->KeyProp->GetValueTypeHash(PropertyValue);
				if (!MapHashes.Contains(Hash)) {
					HashesNew.Add(i);
				} else {
					uint8* ValuePtr = MapHelper.GetValuePtr(MapHashes.Find(Hash));
					TSharedPtr<FJsonValue> ObjectValue = jsonArr[i]->AsObject()->TryGetField("JS_Value");
					Conv_JsonValueToFProperty(ObjectValue, MProp->ValueProp, ValuePtr, Outer);

				}
				MProp->KeyProp->DestroyValue(PropertyValue);
				FMemory::Free(PropertyValue);
			}

			for (int32 i : HashesNew) {
				const int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				uint8* mapPtr = MapHelper.GetPairPtr(Index);
				TSharedPtr<FJsonValue> ObjectValue = jsonArr[i]->AsObject()->TryGetField("JS_Value");
				TSharedPtr<FJsonValue> KeyValue = jsonArr[i]->AsObject()->TryGetField("JS_Key");
				Conv_JsonValueToFProperty(KeyValue, MProp->KeyProp, mapPtr, Outer);
				Conv_JsonValueToFProperty(ObjectValue, MProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset, Outer);
			}
			MapHashes.Empty();
			HashesNew.Empty();
			MapHelper.Rehash();
		}
	} else if (FClassProperty* CProp = CastField<FClassProperty>(Prop)) {
		UClass* CastResult = nullptr;
		if (json->Type == EJson::String) {
			const FString ClassPath = json->AsString();
			if (ClassPath == "") {
				if (CProp->GetPropertyValue(Ptr) != nullptr) {
					if (DoLog)
						Log(FString("Overwrite FClassProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(CProp->GetPropertyValue(Ptr)->GetPathName()).Append(" NewValue = Nullpeter"), 0);
					CProp->SetPropertyValue(Ptr, CastResult);
				}
			} else {
				CastResult = LoadObject<UClass>(nullptr, *json->AsString());
				// Failsafe for script classes with BP Stubs
				if (!CastResult && json->AsString() != "") {
					CastResult = FSoftClassPath(json->AsString()).TryLoadClass<UClass>();
					if (!CastResult) {
						CastResult = FailSafeClassFind(json->AsString());
					}
				}
				if (CastResult && CastResult != CProp->GetPropertyValue(Ptr)) {
					if (DoLog)
						Log(FString("Overwrite FClassProperty: ").Append(Prop->GetName()).Append(" OldValue = ").Append(CProp->GetPropertyValue(Ptr)->GetPathName()).Append(" NewValue = ").Append(CastResult->GetPathName()), 0);
					CProp->SetPropertyValue(Ptr, CastResult);
				}
			}
		}
	} else if (FObjectProperty* UProp = CastField<FObjectProperty>(Prop)) {
		if (json->Type == EJson::Object) {
			if (json->AsObject()->HasField("JS_Object") && json->AsObject()->HasField("JS_Class") && json->AsObject()->HasField("JS_ObjectFlags") && json->AsObject()->HasField("JS_ObjectName")) {
				const FString KeyValue = json->AsObject()->TryGetField("JS_Class")->AsString();
				const FString NameValue = json->AsObject()->TryGetField("JS_ObjectName")->AsString();
				const TSharedPtr<FJsonValue> ObjectValue = json->AsObject()->TryGetField("JS_Object");
				const TSharedPtr<FJsonValue> OuterValue = json->AsObject()->TryGetField("JS_ObjectOuter");
				const EObjectFlags ObjectLoadFlags = static_cast<EObjectFlags>(json->AsObject()->GetIntegerField((TEXT("JS_ObjectFlags"))));
				if (ObjectValue->Type == EJson::Object && KeyValue != "") {
					UClass* InnerBPClass = LoadObject<UClass>(nullptr, *KeyValue);
					if (InnerBPClass) {
						const TSharedPtr<FJsonObject> Obj = ObjectValue->AsObject();
						UObject* Value = UProp->GetPropertyValue(Ptr);
						if (Value && Value->GetClass() == InnerBPClass) {
							UClass* OuterLoaded = LoadObject<UClass>(nullptr, *OuterValue->AsString());
							if (OuterLoaded && OuterLoaded != Outer) {
								if (DoLog)
									Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" using != Outer !").Append(KeyValue).Append(" actual Outer : ").Append(OuterLoaded->GetPathName()), 0);
								Conv_JsonObjectToUStruct(Obj, InnerBPClass, Value, OuterLoaded);
							} else {
								Conv_JsonObjectToUStruct(Obj, InnerBPClass, Value, Outer);
							}
						} else {
							UObject* Template = UObject::GetArchetypeFromRequiredInfo(InnerBPClass, Outer, *NameValue, ObjectLoadFlags);
							auto params = FStaticConstructObjectParameters(InnerBPClass);
							params.Outer = Outer;
							params.Name = *NameValue;
							params.SetFlags = ObjectLoadFlags;
							params.InternalSetFlags = EInternalObjectFlags::None;
							params.Template = Template;
							UObject* Constructed = StaticConstructObject_Internal(params);
							if (DoLog)
								Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Constructed new Object of Class").Append(KeyValue), 0);
							Conv_JsonObjectToUStruct(Obj, InnerBPClass, Constructed, Outer);
							UProp->SetObjectPropertyValue(Ptr, Constructed);
						}
					} else {
						if (DoLog)
							Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Failed to load UClass for Object").Append(KeyValue), 2);
					}
				} else if (ObjectValue->Type == EJson::String) {
					UObject* Obj = UProp->GetPropertyValue(Ptr);
					if (ObjectValue->AsString() != "") {
						UObject* UObj = LoadObject<UObject>(nullptr, *ObjectValue->AsString());
						if (!UObj) {
							if (DoLog)
								Log(FString("Skipped Overwrite Reason[Load Fail] FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
						} else {
							if (DoLog)
								Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
							UProp->SetPropertyValue(Ptr, UObj);
						}
					} else if (Obj) {
						if (DoLog)
							Log(FString("Overwrite FObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append(Obj->GetPathName().Append(" with nullpeter")), 0);
						UProp->SetPropertyValue(Ptr, nullptr);
					}
				}
			}
		}
	} else if (FStructProperty* SProp = CastField<FStructProperty>(Prop)) {
		Conv_JsonObjectToUStruct(json->AsObject(), SProp->Struct, Ptr, Outer);
	} else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Prop)) {
		FWeakObjectPtr WObj = WeakObjectProperty->GetPropertyValue(Ptr);
		if (json->AsString() != "") {
			UObject* UObj = FSoftObjectPath(json->AsString()).TryLoad();
			UObject* Obj = WObj.Get();

			if (DoLog)
				Log(FString("Overwrite WeakObjectProperty: ").Append(Prop->GetName()).Append(" Value : ").Append((Obj ? Obj->GetPathName() : FString("nullpeter")).Append(" with: ").Append((UObj ? UObj->GetPathName() : FString("nullpeter")))), 0);
			WeakObjectProperty->SetPropertyValue(Ptr, UObj);
		} else {
			if (WObj.IsValid()) {
				if (DoLog)
					Log(FString("Overwrite FWeakObjectProperty: ").Append(Prop->GetName()).Append(" with nullpeter"), 0);
				WeakObjectProperty->SetPropertyValue(Ptr, nullptr);
			}
		}
	} else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Prop)) {
		// Check to see if this is a simple soft object property (eg. not an array of soft objects).
		if (json->AsString() != "") {
			const FString PathString = json->AsString();
			FSoftObjectPtr* ObjectPtr = static_cast<FSoftObjectPtr*>(Ptr);
			*ObjectPtr = FSoftObjectPath(PathString);
		}
	} else if (auto* CastProp = CastField<FMulticastSparseDelegateProperty>(Prop)) {
		//ExportTextItem
	} else {
	}
}

TSharedPtr<FJsonValue> UJsonStructBPLib::Conv_FPropertyToJsonValue(FProperty* Prop, void* Ptr, bool IncludeObjects, TArray<UObject*>& RecursionArray, bool DeepRecursion, TArray<FString> FilteredFields, bool Exclude)
{
	if (FStrProperty* StrProp = CastField<FStrProperty>(Prop)) {
		return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(Ptr));
	} else if (FTextProperty* TxtProp = CastField<FTextProperty>(Prop)) {
		return MakeShared<FJsonValueString>(TxtProp->GetPropertyValue(Ptr).ToString());
	} else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop)) {
		return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(Ptr).ToString());
	} else if (FFloatProperty* FProp = CastField<FFloatProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(FProp->GetPropertyValue(Ptr));
	} else if (FIntProperty* IProp = CastField<FIntProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(IProp->GetPropertyValue(Ptr));
	} else if (FBoolProperty* BProp = CastField<FBoolProperty>(Prop)) {
		return MakeShared<FJsonValueBoolean>(BProp->GetPropertyValue(Ptr));
	} else if (FEnumProperty* EProp = CastField<FEnumProperty>(Prop)) {
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		const TSharedPtr<FJsonValue> key = MakeShared<FJsonValueString>(EProp->GetEnum()->GetPathName());
		const TSharedPtr<FJsonValue> value = MakeShared<FJsonValueString>(
			EProp->GetEnum()->GetNameByValue(EProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(Ptr)).
			ToString());
		JsonObject->SetField("JS_Enum", key);
		JsonObject->SetField("JS_Value", value);
		return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
	} else if (FDoubleProperty* DdProp = CastField<FDoubleProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(DdProp->GetPropertyValue(Ptr));
	} else if (FByteProperty* ByProp = CastField<FByteProperty>(Prop)) {
		if (ByProp->IsEnum()) {
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			TSharedPtr<FJsonValue> Key = MakeShared<FJsonValueString>(ByProp->Enum->GetPathName());
			TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueString>(
				ByProp->Enum->GetNameByValue(ByProp->GetSignedIntPropertyValue(Ptr)).ToString());
			TSharedPtr<FJsonValue> Byte = MakeShared<FJsonValueNumber>(ByProp->GetPropertyValue(Ptr));
			JsonObject->SetField("JS_EnumAsByte", Key);
			JsonObject->SetField("JS_Value", Value);
			JsonObject->SetField("JS_Byte", Byte);
			return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
		} else {
			return MakeShared<FJsonValueNumber>(ByProp->GetSignedIntPropertyValue(Ptr));
		}
	} else if (FNumericProperty* NProp = CastField<FNumericProperty>(Prop)) {
		return MakeShared<FJsonValueNumber>(NProp->GetUnsignedIntPropertyValue(Ptr));
	} else if (FArrayProperty* AProp = CastField<FArrayProperty>(Prop)) {
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetField("JS_InnerProperty", MakeShared<FJsonValueString>(AProp->Inner->GetName()));
		//JsonObject->SetField("JS_Replace",  MakeShared<FJsonValueBoolean>(true));
		//JsonObject->SetField("JS_Remove",  MakeShared<FJsonValueBoolean>(false));
		//JsonObject->SetField("JS_UniqueElements",  MakeShared<FJsonValueBoolean>(false));

		auto& arr = AProp->GetPropertyValue(Ptr);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(Conv_FPropertyToJsonValue(AProp->Inner, (void*)((size_t)arr.GetData() + i * AProp->Inner->ElementSize), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude));
		}
		JsonObject->SetField("JS_Values", MakeShared<FJsonValueArray>(jsonArr));
		return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
	} else if (FMapProperty* mProp = CastField<FMapProperty>(Prop)) {
		FScriptMapHelper Arr(mProp, Ptr);
		TArray<TSharedPtr<FJsonValue>> JSONArr;
		for (int i = 0; i < Arr.Num(); i++) {
			TSharedPtr<FJsonValue> Key = Conv_FPropertyToJsonValue(mProp->KeyProp, Arr.GetKeyPtr(i), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude);
			TSharedPtr<FJsonValue> Value = Conv_FPropertyToJsonValue(mProp->ValueProp, Arr.GetValuePtr(i), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude);
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetField("JS_Key", Key);
			JsonObject->SetField("JS_Value", Value);
			TSharedPtr<FJsonValueObject> Obj = MakeShared<FJsonValueObject>(JsonObject);
			JSONArr.Add(Obj);
		}
		return MakeShared<FJsonValueArray>(JSONArr);
	} else if (FSetProperty* SetProperty = CastField<FSetProperty>(Prop)) {
		auto& Arr = SetProperty->GetPropertyValue(Ptr);
		TArray<TSharedPtr<FJsonValue>> JSONArr;
		for (int i = 0; i < Arr.Num(); i++) {
			JSONArr.Add(Conv_FPropertyToJsonValue(SetProperty->ElementProp, (void*)((size_t)Arr.GetData(i, SetProperty->SetLayout)), IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude));
		}
		return MakeShared<FJsonValueArray>(JSONArr);
	} else if (FClassProperty* CProp = CastField<FClassProperty>(Prop)) {
		if (CProp->GetPropertyValue(Ptr)) {
			return MakeShared<FJsonValueString>(CProp->GetPropertyValue(Ptr)->GetPathName());
		} else {
			return MakeShared<FJsonValueString>("");
		}
	} else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Prop)) {
		if (WeakObjectProperty->GetPropertyValue(Ptr).IsValid()) {
			return MakeShared<FJsonValueString>(
				FSoftObjectPath(WeakObjectProperty->GetPropertyValue(Ptr).Get()).ToString());
		} else {
			return MakeShared<FJsonValueString>("");
		}
	} else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Prop)) {
		// Check to see if this is a simple soft object property (eg. not an array of soft objects).
		if (SoftObjectProperty->GetPropertyValue(Ptr).IsValid()) {
			return MakeShared<FJsonValueString>(
				SoftObjectProperty->GetPropertyValue(Ptr).ToSoftObjectPath().ToString());
		} else {
			return MakeShared<FJsonValueString>("");
		}
	} else if (FObjectProperty* OProp = CastField<FObjectProperty>(Prop)) {
		UObject* ObjectValue = OProp->GetPropertyValue(Ptr);
		if (ObjectValue) {
			const UClass* BaseClass = ObjectValue->GetClass();
			const TSharedPtr<FJsonValue> Key = MakeShared<FJsonValueString>(BaseClass->GetPathName());
			const TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueString>(ObjectValue->GetPathName());
			TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetField("JS_Class", Key);

			// Everything that is streamable or AssetUserData ( aka Mesh's and Textures etc ) will be skipped
			for (auto& i : BaseClass->Interfaces) {
				if (i.Class == UInterface_AssetUserData::StaticClass() || i.Class == UStreamableRenderAsset::StaticClass()) {
					JsonObject->SetField("JS_Object", Value);
					const TSharedPtr<FJsonValueObject> Obj = MakeShared<FJsonValueObject>(JsonObject);
					return TSharedPtr<FJsonValue>(Obj);
				}
			}

			// We do want include Objects
			// Object is defaulting to Instanced or we Include Non Instanced
			// Property Value must not be nullpeter
			// Property Object wasn't already serialized or is RootComponent based
			const bool Condition = IncludeObjects
				&& ((BaseClass->HasAnyClassFlags(EClassFlags::CLASS_DefaultToInstanced) && BaseClass->HasAnyClassFlags(EClassFlags::CLASS_EditInlineNew)) || DeepRecursion)
				&& (!RecursionArray.Contains(ObjectValue)) && RecursionArray.Num() < 500;

			// skip this if we dont need it 
			if (Condition) {
				TSharedPtr<FJsonObject> ChildObj = MakeShared<FJsonObject>();
				RecursionArray.Add(ObjectValue);
				USceneComponent* Scene = Cast<USceneComponent>(ObjectValue);
				if (Scene && Scene->GetAttachmentRoot() == ObjectValue) {
					TArray<USceneComponent*> arr = Scene->GetAttachChildren();
					for (auto i : arr) {
						RecursionArray.Add(i);
						ChildObj->Values.Add(i->GetName(),
											 MakeShared<FJsonValueObject>(Conv_UStructToJsonObject(i->GetClass(), i,
																								   IncludeObjects, RecursionArray, DeepRecursion, FilteredFields,
																								   Exclude)));
					}
				}
				TSharedPtr<FJsonObject> Obj = Conv_UStructToJsonObject(BaseClass, ObjectValue, IncludeObjects, RecursionArray, DeepRecursion, FilteredFields, Exclude);
				JsonObject->SetField("JS_Object", MakeShared<FJsonValueObject>(Obj));
				JsonObject->SetField("JS_ObjectFlags", MakeShared<FJsonValueNumber>(static_cast<int32>(ObjectValue->GetFlags())));
				JsonObject->SetField("JS_ObjectName", MakeShared<FJsonValueString>(ObjectValue->GetName()));
				JsonObject->SetField("JS_ObjectOuter",
									 MakeShared<FJsonValueString>(ObjectValue->GetOutermost()->GetFName().ToString()));
				JsonObject->SetField("JS_Children:", MakeShared<FJsonValueObject>(ChildObj));
				return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
			} else {
				JsonObject->SetField("JS_Object", Value);
				return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(JsonObject));
			}
		}
	} else if (FStructProperty* SProp = CastField<FStructProperty>(Prop)) {
		return MakeShared<FJsonValueObject>(Conv_UStructToJsonObject(SProp->Struct, Ptr, IncludeObjects,
																	 RecursionArray, DeepRecursion, FilteredFields,
																	 Exclude));
	} else if (!DeepRecursion) {
		return MakeShared<FJsonValueNull>();
	} else if (FMulticastSparseDelegateProperty* CastProp = CastField<FMulticastSparseDelegateProperty>(Prop)) {
		if (CastProp->SignatureFunction) {
			return MakeShared<FJsonValueString>(CastProp->SignatureFunction->GetName());
		}
	} else if (FMulticastInlineDelegateProperty* mCastProp = CastField<FMulticastInlineDelegateProperty>(Prop)) {
		if (mCastProp->SignatureFunction) {
			return MakeShared<FJsonValueString>(mCastProp->SignatureFunction->GetName());
		}
	} else if (FDelegateProperty* Delegate = CastField<FDelegateProperty>(Prop)) {
		if (Delegate->SignatureFunction) {
			return MakeShared<FJsonValueString>(Delegate->SignatureFunction->GetName());
		}
	} else if (FInterfaceProperty* InterfaceProperty = CastField<FInterfaceProperty>(Prop)) {
		if (InterfaceProperty->InterfaceClass) {
			return MakeShared<FJsonValueString>(InterfaceProperty->InterfaceClass->GetPathName());
		} else {
			return MakeShared<FJsonValueString>("");
		}
	} else if (FFieldPathProperty* FieldPath = CastField<FFieldPathProperty>(Prop)) {
		return MakeShared<FJsonValueString>(FieldPath->PropertyClass->GetName());
	} else {
		// Log Debug
	}
	return MakeShared<FJsonValueNull>();
}

void UJsonStructBPLib::InternalGetStructAsJson(FStructProperty* Structure, void* StructurePtr, FString& String, bool RemoveGUID, const bool IncludeObjects)
{
	TArray<UObject*> Array;
	const TSharedPtr<FJsonObject> JsonObject = Conv_UStructToJsonObject(Structure->Struct, StructurePtr, IncludeObjects, Array, false, TArray<FString>(), true);
	String = JsonObjectToString(JsonObject);
}

void UJsonStructBPLib::InternalGetStructAsJsonForTable(FStructProperty* Structure, void* StructurePtr, FString& String, const bool RemoveGUID, const FString Name)
{
	const TSharedPtr<FJsonObject> JsonObject = ConvertUStructToJsonObjectWithName(Structure->Struct, StructurePtr, RemoveGUID, Name);
	String = JsonObjectToString(JsonObject);
}

TSharedPtr<FJsonObject> UJsonStructBPLib::ConvertUStructToJsonObjectWithName(UStruct* Struct, void* Ptr, const bool RemoveGUID, const FString Name)
{
	TSharedPtr<FJsonObject> OBJ = MakeShared<FJsonObject>();
	OBJ->SetStringField("Name", Name);
	for (auto Prop = TFieldIterator<FProperty>(Struct); Prop; ++Prop) {
		FString PropName = RemoveGUID ? RemoveUStructGuid(Prop->GetName()) : Prop->GetName();
		TArray<UObject*> IteratedObjects;
		OBJ->SetField(PropName, Conv_FPropertyToJsonValue(*Prop, Prop->ContainerPtrToValuePtr<void>(Ptr), RemoveGUID, IteratedObjects, false, TArray<FString>(), true));
	}
	return OBJ;
}

bool UJsonStructBPLib::FillDataTableFromJSONString(UDataTable* DataTable, const FString& InString)
{
	if (!DataTable) {
		UE_LOG(LogDataTable, Error, TEXT("Can't fill an invalid DataTable."));
		return false;
	}

	bool bResult = true;
	if (InString.Len() == 0) {
		DataTable->EmptyTable();
	} else {
		TArray<FString> Errors = DataTable->CreateTableFromJSONString(InString);
		if (Errors.Num()) {
			for (const FString& Error : Errors) {
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
	JsonObject->SetField("JS_LibClass", MakeShared<FJsonValueString>(
		Class->IsNative() ? Class->GetPathName() : Class->GetSuperClass()->GetPathName()));
	if (Cast<AActor>(Object) && Object != Class->GetDefaultObject()) {
		JsonObject->SetField("JS_LibTransform",
							 MakeShared<FJsonValueString>(Cast<AActor>(Object)->GetTransform().ToString()));
		JsonObject->SetField("JS_LibActorClass",
							 MakeShared<FJsonValueString>(Cast<AActor>(Object)->GetClass()->GetPathName()));
	}
	JsonObject->SetField("JS_LibOuter", MakeShared<FJsonValueString>(Object->GetPathName()));
	return JsonObject;
}

FString UJsonStructBPLib::ObjectToJsonString(UClass* ObjectClass, const bool ObjectRecursive, UObject* DefaultObject, const bool DeepRecursion, const bool SkipRoot, const bool SkipTransient, const bool OnlyEditable)
{
	if (!ObjectClass)
		return"";

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("JS_LibValue", MakeShared<FJsonValueObject>(CDOToJson(
			ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient,
			OnlyEditable, TArray<FString>(), true)));
		return JsonObjectToString(JsonObject);
	} else {
		return "";
	}
}

FString UJsonStructBPLib::JsonObjectToString(TSharedPtr<FJsonObject> JsonObject)
{
	FString Write;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return Write;
}

TSharedPtr<FJsonObject> UJsonStructBPLib::CDOToJson(UClass* ObjectClass, UObject* Object, const bool ObjectRecursive, const bool DeepRecursion, const bool SkipRoot, const bool SkipTransient, const bool OnlyEditable, const TArray<FString> FilteredFields, const bool Exclude)
{
	TArray<UObject*> RecursionArray;
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

	if (!SkipRoot) {
		FProperty* RootComponent = nullptr;
		for (auto Prop = TFieldIterator<FProperty>(ObjectClass); Prop; ++Prop) {
			if (Prop->GetName() == "RootComponent") {
				RootComponent = *Prop;
				break;
			}
		}
		if (RootComponent) {
			Obj->SetField(RootComponent->GetName(), Conv_FPropertyToJsonValue(RootComponent, RootComponent->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, RecursionArray, DeepRecursion, FilteredFields, Exclude));
		}
	}

	for (auto Prop = TFieldIterator<FProperty>(ObjectClass); Prop; ++Prop) {
		if (FilteredFields.Contains(Prop->GetName())) {
			if (Exclude)
				continue;
		} else {
			if (!Exclude)
				continue;
		}
		const bool bEditable = Prop->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit);
		const bool bLikelyOnlyReadable = Prop->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient | EPropertyFlags::CPF_Net | CPF_EditConst | CPF_GlobalConfig | CPF_Config);
		const bool bRootComponent = Prop->GetName() == "RootComponent";
		if (bRootComponent || (bLikelyOnlyReadable && SkipTransient) || (!bEditable && OnlyEditable)) {
			continue;
		} else {
			TSharedPtr<FJsonValue> Value = Conv_FPropertyToJsonValue(*Prop, Prop->ContainerPtrToValuePtr<void>(Object), ObjectRecursive, RecursionArray, DeepRecursion, FilteredFields, Exclude);
			if (Value.IsValid() && Value->Type != EJson::Null) {
				Obj->SetField(Prop->GetName(), Value);
			}
		}
	}
	return Obj;
}

void UJsonStructBPLib::ObjectToJsonObject(UClass* ObjectClass, const bool ObjectRecursive, UObject* DefaultObject, UObject* Outer, const bool DeepRecursion, const bool SkipRoot, const bool SkipTransient, const bool OnlyEditable, FBPJsonObject& JObject)
{
	if (!ObjectClass)
		return;

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		JsonObject->SetField("JS_LibValue", MakeShared<FJsonValueObject>(CDOToJson(
			ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient,
			OnlyEditable, TArray<FString>(), true)));
		JObject = FBPJsonObject(EBPJson::BPJSON_Object, JsonObject, "");

	}
	return;
}

void UJsonStructBPLib::ObjectToJsonObjectFiltered(const TArray<FString> Fields, UClass* ObjectClass, UObject* DefaultObject, UObject* Outer, const bool ObjectRecursive, const bool Exclude, const bool DeepRecursion, const bool SkipRoot, const bool SkipTransient, const bool OnlyEditable, FBPJsonObject& JObject)
{
	if (!ObjectClass)
		return;

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		const TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueObject>(
			CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable,
					  Fields, Exclude).ToSharedRef());
		JsonObject->SetField("JS_LibValue", Value);
		JObject = FBPJsonObject(EBPJson::BPJSON_Object, JsonObject, "");
	}
	return;
}

FString UJsonStructBPLib::ObjectToJsonStringFiltered(TArray<FString> Fields, UClass* ObjectClass, UObject* DefaultObject, const bool ObjectRecursive, const bool Exclude, const bool DeepRecursion, const bool SkipRoot, const bool SkipTransient, const bool OnlyEditable)
{
	if (!ObjectClass)
		return "";

	UObject* Object = DefaultObject ? DefaultObject : ObjectClass->GetDefaultObject();
	if (ObjectClass != nullptr) {
		TSharedPtr<FJsonObject> JsonObject = SetupJsonObject(ObjectClass, Object);
		const TSharedPtr<FJsonValue> Value = MakeShared<FJsonValueObject>(
			CDOToJson(ObjectClass, Object, ObjectRecursive, DeepRecursion, SkipRoot, SkipTransient, OnlyEditable,
					  Fields, Exclude).ToSharedRef());
		JsonObject->SetField("JS_LibValue", Value);
		return JsonObjectToString(JsonObject);
	} else {
		return "";
	}
}

// Function Arch wrote we borrow here
// changed UClass to UDynamicClass and added MountPoint Registration in case of this being on one
//UClass* UJsonStructBPLib::CreateNewClass(const FString& ClassName, const FString& PackageName, UClass * ParentClass)
//{
	/*
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
	*/
	/*return nullptr;*/
//}

// Tries to spawn an Actor with exact name to World
AActor* UJsonStructBPLib::SpawnActorWithName(UObject* WorldContext, UClass* C, const FName Name)
{
	if (!WorldContext || !WorldContext->GetWorld() || !C)
		return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = Name;
	return WorldContext->GetWorld()->SpawnActor<AActor>(C, SpawnParams);
}

UClass* UJsonStructBPLib::FailSafeClassFind(FString String)
{
	// Path to Parent class might be changed
	// so we search Ambiguously
	FString ClassName; FString Path; FString Right;
	FPaths::Split(String, Path, Right, ClassName);
	if (ClassName != "") {
		Log(FString("FailSafe Class #1 Loading ").Append(String).Append("--  Searching : ").Append(ClassName), 1);
		UClass* Out = FindClassByName(ClassName);
		if (Out)
			return Out;

		if (!ClassName.EndsWith("_C")) {
			Log(FString("FailSafe Class #2 Loading ").Append(String).Append("--  Searching : ").Append(ClassName.Append("_C")), 1);
			return FindClassByName(ClassName.Append("_C"));
		}
	}
	return nullptr;
}

bool UJsonStructBPLib::SetClassDefaultsFromJsonString(const FString JsonString, UClass* BaseClass, UObject* DefaultObject)
{
	if (JsonString == "")
		return false;

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*JsonString);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> Result;
	Serializer.Deserialize(Reader, Result);
	if (!Result.IsValid())
		return false;

	if (Result->HasField("JS_LibClass") && Result->HasField("JS_LibValue")) {
		const FString String = Result->GetStringField("JS_LibClass");
		const TSharedPtr<FJsonObject> Obj = Result->GetObjectField("JS_LibValue");
		Result = Obj;
		if (FPackageName::IsValidObjectPath(String)) {
			UClass* LoadedObject = LoadObject<UClass>(nullptr, *String);
			if (!LoadedObject) {
				LoadedObject = FailSafeClassFind(String);
			}
			// if the Class Input was invalid we can fallback to Loaded Class
			if (!BaseClass && LoadedObject) {
				BaseClass = LoadedObject;
			} else if (LoadedObject && LoadedObject != BaseClass && !BaseClass->IsChildOf(LoadedObject) && !DefaultObject) {
				// if LoadedClass is valid and is not equal to the input and not a Child of it either, and we dont provide DefaultObject
				// then something weird is going on; DefaultObject provided from Blueprint can only be valid for
				// Assets or Actors in which case all this does not matter and BaseClass should be valid or retrieve able anyway
				Log(FString("Class Mismatch  !!  ").Append(String).Append(" provided Class :  ").Append(*BaseClass->GetName()), 2);
				return false;
			}
		}
	}

	//  by now we either have a class or we failed, safe to exit here in case
	// some safe checks; maybe this should not be excluded by DefaultObject valid, this line previously had IsNative() as fail condition which should have been ignored only for valid Object provided
	if ((!BaseClass || !BaseClass->ClassConstructor) && !DefaultObject)
		return false;

	// The Container we Modify
	UObject* Object = nullptr;
	// if we provide a Object we might want to modify Asset or Instance of an Object
	if (DefaultObject) {
		Object = DefaultObject;
	} else {
		// if not then we modify CDO of resolved Class
		Object = BaseClass->GetDefaultObject();
	}

	for (auto Field : Result->Values) {
		if (FProperty* Prop = BaseClass->FindPropertyByName(*Field.Key)) {
			Conv_JsonValueToFProperty(Field.Value, Prop, Prop->ContainerPtrToValuePtr<void>(Object), Object);
		}
	}
	return true;
}

FTransform  UJsonStructBPLib::Conv_StringToTransform(const FString String)
{
	FTransform T = FTransform();
	T.InitFromString(*String);
	return T;
}

void UJsonStructBPLib::Conv_UClassToPropertyFieldNames(UStruct* Structure, TArray<FString>& Array, const bool Recurse)
{
	if (Array.Num() > 500)
		return;

	for (auto Prop = TFieldIterator<FProperty>(Structure); Prop; ++Prop) {
		if (Recurse) {
			if (CastField<FObjectProperty>(*Prop) && Cast<UClass>(Structure)) {
				UObject* ObjectValue = CastField<FObjectProperty>(*Prop)->GetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Structure)->GetDefaultObject()));
				if (ObjectValue && ObjectValue->GetClass()) {
					Conv_UClassToPropertyFieldNames(ObjectValue->GetClass(), Array, Recurse);
				}
			} else if (CastField<FClassProperty>(*Prop) && Cast<UClass>(Structure)) {
				UObject* ObjectValue = CastField<FClassProperty>(*Prop)->GetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Cast<UClass>(Structure)->GetDefaultObject()));

				if (ObjectValue && ObjectValue->GetClass()) {
					Conv_UClassToPropertyFieldNames(ObjectValue->GetClass(), Array, Recurse);
				}
			} else if (CastField<FStructProperty>(*Prop)) {
				Conv_UClassToPropertyFieldNames(CastField<FStructProperty>(*Prop)->Struct, Array, Recurse);
			}
		}
		if (!Array.Contains(Prop->GetName()))
			Array.Add(Prop->GetName());
	}
}
