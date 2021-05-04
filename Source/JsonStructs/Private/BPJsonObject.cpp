#include "BPJsonObject.h"

TArray<FString> UBPJsonObject::GetFieldNames()
{
	TArray<FString> Out;
	if (!InnerObj)
		return Out;
	for (auto i : InnerObj->Values)
	{
		Out.Add(i.Key);
	}
	return Out;
}

FString UBPJsonObject::GetJsonStringField(const FString Name, bool SearchNested)
{
	if (!InnerObj)
		return "";
	FString Out = "";
	if (InnerObj->HasField(Name))
	{
		auto val = InnerObj->TryGetField(Name);
		if (val)
		{
			val->TryGetString(Out);
		}
	}
	else if (SearchNested)
	{
		if (!InnerObj->Values.begin())
			return Out;
		UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>(this);
		Obj->Value = InnerObj->Values.begin()->Value;
		if (InnerObj->Values.Num() == 1)
		{
			if (Obj->Value->Type == EJson::String)
			{
				return Obj->AsString();
			}
		}
	}
	return Out;
}

float UBPJsonObject::GetJsonNumberField(const FString Name, bool SearchNested)
{
	if (!InnerObj)
		return 0.f;
	float Out = 0.f;
	if (InnerObj->HasField(Name))
	{
		auto val = InnerObj->TryGetField(Name);
		if (val)
		{
			val->TryGetNumber(Out);
		}
	}
	else if (SearchNested)
	{
		if (!InnerObj->Values.begin())
			return Out;
		UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>(this);
		Obj->Value = InnerObj->Values.begin()->Value;
		if (InnerObj->Values.Num() == 1)
		{
			if (Obj->Value->Type == EJson::Number)
			{
				return Obj->Value.Get()->AsNumber();
			}
			else if (Obj->Value->Type == EJson::String)
			{
				return Obj->AsNumber();
			}
		}

	}
	return Out;
}

TArray<UBPJsonObjectValue*> UBPJsonObject::GetJsonArrayField(const FString Name, bool SearchNested)
{
	TArray<UBPJsonObjectValue*> _Out;
	if (!InnerObj)
		return _Out;
	const TArray< TSharedPtr<FJsonValue> >* Out;
	if (InnerObj->HasField(Name))
	{
		auto val = InnerObj->TryGetField(Name);
		if (val)
		{
			if (!val->TryGetArray(Out))
				return _Out;
			int32 Ind = 0;
			for (TSharedPtr<FJsonValue> i : *Out)
			{
				UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>(this);
				Obj->Value = i;
				_Out.Add(Obj);
			}
		}
	}
	else if (SearchNested)
	{
		if (!InnerObj->Values.begin())
			return _Out;
		if (InnerObj->Values.Num() == 1)
		{
			if (InnerObj->Values.begin()->Value->Type == EJson::Array)
			{
				Out = &InnerObj->Values.begin()->Value.Get()->AsArray();
				for (TSharedPtr<FJsonValue> i : *Out)
				{
					UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>(this);
					Obj->Value = i;
					_Out.Add(Obj);
				}
			}
		}

	}
	return _Out;
}

EBPJson UBPJsonObject::GetFieldType(const FString Name)
{
	if (!InnerObj)
		return EBPJson::BPJSON_Null;
	else
	{
		auto Field = InnerObj->TryGetField(Name);
		if (!Field)
			return EBPJson::BPJSON_Null;
		switch (Field->Type)
		{
		case EJson::Array:
		{
			return EBPJson::BPJSON_Array;
			break;
		}
		case EJson::Boolean:
		{
			return EBPJson::BPJSON_Boolean;
			break;
		}
		case EJson::None:
		{
			return EBPJson::BPJSON_None;
			break;
		}
		case EJson::Null:
		{
			return EBPJson::BPJSON_Null;
			break;
		}
		case EJson::Number:
		{
			return EBPJson::BPJSON_Number;
			break;
		}
		case EJson::Object:
		{
			return EBPJson::BPJSON_Object;
			break;
		}
		case EJson::String:
		{
			return EBPJson::BPJSON_String;
			break;
		}
		default:
			return EBPJson::BPJSON_Null;
			break;
		}
	}

};

void UBPJsonObject::SetJsonStringField(const FString Name, const FString Value)
{
	if (!InnerObj)
		return;
	InnerObj->SetStringField(Name, Value);
}
void UBPJsonObject::SetJsonNumberField(const FString Name, const float Value)
{
	if (!InnerObj)
		return;
	InnerObj->SetNumberField(Name, Value);
}

UBPJsonObject * UBPJsonObject::GetStringAsJson(UPARAM(ref)FString & String, UObject * Outer)
{
	if (String == "")
		return nullptr;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);

	UBPJsonObject * Obj = NewObject<UBPJsonObject>(Outer);
	if(!Obj)
		return nullptr;
	else
	{
		Obj->InnerObj = result;
		return Obj;
	}
}

FString UBPJsonObject::GetAsString()
{
	if (!InnerObj)
		return "";
	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(InnerObj.ToSharedRef(), JsonWriter);
	return write;
}
