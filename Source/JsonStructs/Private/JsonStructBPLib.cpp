#include "JsonStructBPLib.h"
#include "BPJsonObjectValue.h"


TSharedPtr<FJsonObject> UJsonStructBPLib::convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct) {
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<UProperty>(Struct); prop; ++prop) {
		obj->SetField(prop->GetName(), convertUPropToJsonValue(*prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct)));
	}
	return obj;
}

void UJsonStructBPLib::convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct) {
	if (!json)
		return;
	auto obj = TSharedPtr<FJsonObject>(new FJsonObject());
	for (auto prop = TFieldIterator<UProperty>(Struct); prop; ++prop) {
		FString FieldName;
		FieldName = prop->GetName();
		auto field = json->TryGetField(FieldName);
		if (!field.IsValid()) continue;
		convertJsonValueToUProperty(field, *prop, prop->ContainerPtrToValuePtr<void>(ptrToStruct));
	}
}

void UJsonStructBPLib::convertJsonValueToUProperty(TSharedPtr<FJsonValue> json, UProperty* prop, void* ptrToProp) {
	if (auto strProp = Cast<UStrProperty>(prop)) {
		strProp->SetPropertyValue(ptrToProp, json->AsString());
	}
	else if (auto txtProp = Cast<UTextProperty>(prop)) {
		txtProp->SetPropertyValue(ptrToProp, FText::FromString(json->AsString()));
	}
	else if (auto nameProp = Cast<UNameProperty>(prop)) {
		nameProp->SetPropertyValue(ptrToProp, *json->AsString());
	}
	else if (auto fProp = Cast<UFloatProperty>(prop)) {
		fProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto iProp = Cast<UIntProperty>(prop)) {
		iProp->SetPropertyValue(ptrToProp, json->AsNumber());
	}
	else if (auto bProp = Cast<UBoolProperty>(prop)) {
		bProp->SetPropertyValue(ptrToProp, json->AsBool());
	}
	else if (auto eProp = Cast<UEnumProperty>(prop)) {
		UByteProperty* ByteProp = Cast<UByteProperty>(eProp->GetUnderlyingProperty());
		if (ByteProp)
		{
			ByteProp->SetIntPropertyValue(eProp->ContainerPtrToValuePtr<void>(ptrToProp), int64(json->AsNumber()));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Enum Property failed to Cast the underlyingProperty to a ByteProperty; Property Name : "), *eProp->GetName());
		}
	}
	else if (auto aProp = Cast<UArrayProperty>(prop)) {
		FScriptArrayHelper helper(aProp, ptrToProp);
		helper.EmptyValues();
		TArray<TSharedPtr<FJsonValue>> jsonArr = json->AsArray();
		for (int i = 0; i < jsonArr.Num(); i++) {
			int64 valueIndex = helper.AddValue();
			convertJsonValueToUProperty(jsonArr[i], aProp->Inner, helper.GetRawPtr(valueIndex));
		}
	}
	else if (auto mProp = Cast<UMapProperty>(prop)) {
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
					convertJsonValueToUProperty(keyvalue, mProp->KeyProp, mapPtr);
					convertJsonValueToUProperty(valuevalue, mProp->ValueProp, mapPtr + MapHelper.MapLayout.ValueOffset);
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
	else if (auto cProp = Cast<UClassProperty>(prop)) {
		UObject* LoadedObject = FSoftObjectPath(json->AsString()).TryLoad();
		UClass * CastResult = Cast<UClass>(LoadedObject);
		cProp->SetPropertyValue(ptrToProp, CastResult);
	}
	else if (auto uProp = Cast<UObjectProperty>(prop)) {
		UObject* uObj = FSoftObjectPath(json->AsString()).TryLoad();
		uProp->SetPropertyValue(ptrToProp, uObj);
	}
	else if (auto sProp = Cast<UStructProperty>(prop)) {
		convertJsonObjectToUStruct(json->AsObject(), sProp->Struct, ptrToProp);
	}
}

TSharedPtr<FJsonValue> UJsonStructBPLib::convertUPropToJsonValue(UProperty* prop, void* ptrToProp) {
	if (auto strProp = Cast<UStrProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(strProp->GetPropertyValue(ptrToProp)));
	}
	if (auto txtProp = Cast<UTextProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(txtProp->GetPropertyValue(ptrToProp).ToString()));
	}
	if (auto nameProp = Cast<UNameProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(nameProp->GetPropertyValue(ptrToProp).ToString()));
	}
	else if (auto fProp = Cast<UFloatProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(fProp->GetPropertyValue(ptrToProp)));
	}
	else if (auto iProp = Cast<UIntProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(iProp->GetPropertyValue(ptrToProp)));
	}
	else if (auto bProp = Cast<UBoolProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueBoolean(bProp->GetPropertyValue(ptrToProp)));
	}
	else if (auto eProp = Cast<UEnumProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(eProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ptrToProp)));
	}
	else if (auto nProp = Cast<UNumericProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueNumber(nProp->GetUnsignedIntPropertyValue(ptrToProp)));
	}
	else if (auto aProp = Cast<UArrayProperty>(prop)) {
		auto& arr = aProp->GetPropertyValue(ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			jsonArr.Add(convertUPropToJsonValue(aProp->Inner, (void*)((size_t)arr.GetData() + i * aProp->Inner->ElementSize)));
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (auto cProp = Cast<UClassProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(cProp->GetPropertyValue(ptrToProp)->GetPathName()));
	}
	else if (auto oProp = Cast<UObjectProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueString(oProp->GetPropertyValue(ptrToProp)->GetPathName()));
	}
	else if (auto mProp = Cast<UMapProperty>(prop)) {
		FScriptMapHelper arr(mProp, ptrToProp);
		TArray<TSharedPtr<FJsonValue>> jsonArr;
		for (int i = 0; i < arr.Num(); i++) {
			TSharedPtr<FJsonValue> key = (convertUPropToJsonValue(mProp->KeyProp, arr.GetKeyPtr(i)));
			TSharedPtr<FJsonValue> value = (convertUPropToJsonValue(mProp->ValueProp, arr.GetValuePtr(i)));
			TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
			JsonObject->SetField("key", key);
			JsonObject->SetField("value", value);
			TSharedPtr<FJsonValueObject> Obj = TSharedPtr<FJsonValueObject>(new FJsonValueObject(JsonObject));
			jsonArr.Add(Obj);
		}
		return TSharedPtr<FJsonValue>(new FJsonValueArray(jsonArr));
	}
	else if (auto sProp = Cast<UStructProperty>(prop)) {
		return TSharedPtr<FJsonValue>(new FJsonValueObject(convertUStructToJsonObject(sProp->Struct, ptrToProp)));
	}
	else return TSharedPtr<FJsonValue>(new FJsonValueNull());
}

void UJsonStructBPLib::InternalGetStructAsJson(UStructProperty *Structure, void* StructurePtr, FString &String)
{
	TSharedPtr<FJsonObject> JsonObject;
	JsonObject = convertUStructToJsonObject(Structure->Struct, StructurePtr);

	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	String = write;
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

FString UJsonStructBPLib::Conv_BPJsonObjectValueToString(UBPJsonObjectValue * Value)
{
	return Value->AsString();
}

float UJsonStructBPLib::Conv_BPJsonObjectValueToFloat(UBPJsonObjectValue * Value)
{
	return Value->AsNumber();
}

int32 UJsonStructBPLib::Conv_BPJsonObjectValueToInt(UBPJsonObjectValue * Value)
{
	return int32(Value->AsNumber());
}

bool UJsonStructBPLib::Conv_BPJsonObjectValueToBool(UBPJsonObjectValue * Value)
{
	return Value->AsBoolean();
}

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

UBPJsonObject * UJsonStructBPLib::Conv_UBPJsonObjectValueToBPJsonObject(UBPJsonObjectValue * Value)
{
	if (!Value)
		return nullptr;

	return Value->AsObject();
}
