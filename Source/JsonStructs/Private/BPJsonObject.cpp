#include "BPJsonObject.h"


UBPJsonObject::UBPJsonObject()
{
	Values = {};
};

void UBPJsonObject::InitSubObjects()
{
	if (InnerObj.IsValid())
	{
		for (auto Field : InnerObj->Values)
		{
			UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>(this);
			Values.Add(Field.Key, Obj);
			Obj->InitSubObject(Field.Key, this);

		}
	}
};
TArray<FString> UBPJsonObject::GetFieldNames()
{
	TArray<FString> Out;
	if (!InnerObj)
		return Out;
	
	for (auto i : Values)
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
	if (Values.Contains(Name))
	{
		UBPJsonObjectValue* val = *Values.Find(Name);
		if (val)
		{
			return val->AsString();
		}
	}
	else if (SearchNested)
	{
		if (Values.Num() == 0)
			return Out;
		
		for (auto i : InnerObj->Values)
		{
			auto val = i.Value;
			if (val->Type == EJson::Object)
			{
				const TSharedPtr<FJsonObject>& Object = val->AsObject();
				if (Object->HasField(Name))
				{
					return Object->GetStringField(Name);
				}
			}
			else if (val->Type == EJson::Array)
			{
				const TArray < TSharedPtr<FJsonValue>>& Array = val->AsArray();
				for (auto Element : Array)
				{
					if (Element->Type == EJson::Object)
					{
						if (Element->AsObject()->HasField(Name))
						{
							return Element->AsObject()->GetStringField(Name);
						}
					}
				}
				
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

	if (Values.Contains(Name))
	{
		auto  val = *Values.Find(Name);
		if (val)
		{
			return val->AsNumber();
		}
	}
	else if (SearchNested)
	{

		if (Values.Num() == 0)
			return Out;

		for (auto Value : InnerObj->Values)
		{
			auto val = Value.Value;
			if (val->Type == EJson::Object)
			{
				if (val->AsObject()->HasField(Name))
				{
					auto NestedObjectField = val->AsObject()->TryGetField(Name);
					if(NestedObjectField->Type == EJson::Number)
						return NestedObjectField->AsNumber();
				}
			}
			else if (val->Type == EJson::Array)
			{
				auto NestedArray = val->AsArray();
				for (auto NestedArrayElement : NestedArray)
				{
					if (val->AsObject()->HasField(Name))
					{
						auto NestedObjectField = val->AsObject()->TryGetField(Name);
						if (NestedObjectField->Type == EJson::Number)
							return NestedObjectField->AsNumber();
					}
				}

			}
		}

	}
	return Out;
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
	if (Values.Contains(Name))
	{
		UBPJsonObjectValue* value = *Values.Find(Name);
		value->SetValueFromString(Value);
	}
}
void UBPJsonObject::SetJsonNumberField(const FString Name, const float Value)
{
	if (!InnerObj)
		return;
	if (Values.Contains(Name))
	{
		UBPJsonObjectValue* value = *Values.Find(Name);
		value->SetValueFromNumber(Value);
	}
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
		Obj->InitSubObjects();
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
