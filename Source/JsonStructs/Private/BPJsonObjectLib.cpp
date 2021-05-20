// Copyright Coffee Stain Studios. All Rights Reserved.


#include "BPJsonObjectLib.h"
#include "BPJsonObject.h"

EBPJson UBPJsonObjectLib::FromEJson(const EJson Value)
{
	switch (Value)
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

void UBPJsonObjectLib::SetValueFromNumber(FBPJsonObject& Object, const int32 Value)
{
	Object.SetValueFromNumber(Value);
}

void UBPJsonObjectLib::SetValueFromString(FBPJsonObject& Object, const FString Value)
{
	Object.SetValueFromString(Value);
}

void UBPJsonObjectLib::SetJsonStringField(FBPJsonObject& Object, const FString Name, const FString Value)
{
	Object.SetJsonStringField(Name,Value);
}

void UBPJsonObjectLib::SetJsonNumberField(FBPJsonObject& Object, const FString Name, const float Value)
{
	Object.SetJsonNumberField(Name,Value);
}

FBPJsonObject UBPJsonObjectLib::JsonStringToBPJsonObject(FString & String)
{
	if (String == "")
		return FBPJsonObject();
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*String);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> Result;
	Serializer.Deserialize(Reader, Result);

	return FBPJsonObject(EBPJson::BPJSON_Object, Result, "");
}

FString UBPJsonObjectLib::BPJsonObjectToJsonString(const FBPJsonObject & Object)
{
	if (!Object.InnerObj)
		return "";
	FString Write;
	const TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&Write); //Our Writer Factory
	FJsonSerializer::Serialize(Object.InnerObj.ToSharedRef(), JsonWriter);
	return Write;
}


TArray<FBPJsonObject> UBPJsonObjectLib::Conv_BPJsonObjectToBPJsonObjectArray(const FBPJsonObject & Object)
{
	TArray<FBPJsonObject> ValueFields;
	if (Object.JsonType == BPJSON_Object || Object.JsonType == BPJSON_Array)
	{
		if (Object.InnerObj.IsValid() && Object.FieldName == "")
		{
			for (auto Field : Object.InnerObj->Values)
			{
				const TSharedPtr<FJsonValue> InnerField = Object.InnerObj->TryGetField(Field.Key);
				if (InnerField->Type != EJson::Null && InnerField->Type != EJson::None)
				{
					ValueFields.Add(FBPJsonObject(FromEJson(InnerField->Type), Object.InnerObj, Field.Key));
				}
			}
		}
		else
		{
			if (Object.InnerObj->Values.Contains(Object.FieldName))
			{
				if ((*Object.InnerObj->Values.Find(Object.FieldName))->Type == EJson::Object)
				{
					const TSharedPtr<FJsonValue > Val = *Object.InnerObj->Values.Find(Object.FieldName);
					TSharedPtr<FJsonObject> ObjectValue = Val->AsObject();
					if (ObjectValue.IsValid())
					{
						for (auto Field : ObjectValue->Values)
						{
							const TSharedPtr<FJsonValue> InnerField = ObjectValue->TryGetField(Field.Key);
							if (InnerField->Type != EJson::Null && InnerField->Type != EJson::None)
							{
								ValueFields.Add(FBPJsonObject(FromEJson(InnerField->Type), ObjectValue, Field.Key));
							}
						}
					}
				}
				else if (Object.JsonType == BPJSON_Array)
				{
					const TSharedPtr<FJsonValue > Val = *Object.InnerObj->Values.Find(Object.FieldName);
					if (Val->Type == EJson::Array)
					{
						auto ObjectArray = Val->AsArray();
						TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
						int32 Ind = 0;
						for (auto Field : ObjectArray)
						{
							if (Field->Type != EJson::Null && Field->Type != EJson::None)
							{
								JsonObject->Values.Add(FString::FromInt(Ind), Field);
								ValueFields.Add(FBPJsonObject(FromEJson(Field->Type), JsonObject, FString::FromInt(Ind)));
							}
							Ind = Ind + 1;
						}

					}
				}
			}
		}
	}
	return ValueFields;
}

TArray<FString> UBPJsonObjectLib::Conv_BPJsonObjectToStringArray(const FBPJsonObject & Obj)
{
	TArray<FString> Fields;
	if (Obj.JsonType == BPJSON_Object)
	{
		if (Obj.InnerObj.IsValid() && Obj.FieldName == "")
		{
			for (auto Field : Obj.InnerObj->Values)
			{
				Fields.Add(Field.Key);
			}
		}
		else
		{
			const TSharedPtr<FJsonValue> InnerField = Obj.InnerObj->TryGetField(Obj.FieldName);
			if(InnerField->Type == EJson::Object)
			{
				for (auto Field : InnerField->AsObject()->Values)
				{
					Fields.Add(Field.Key);
				}
			}
		}
	}
	else if (Obj.JsonType == BPJSON_Array)
	{
		if (Obj.InnerObj.IsValid() && Obj.FieldName != "")
		{
			const TSharedPtr<FJsonValue> InnerField = Obj.InnerObj->TryGetField(Obj.FieldName);
			int32 Ind = 0;
			for (auto Field : InnerField->AsArray())
			{
				Fields.Add(FString::FromInt(Ind));
				Ind = Ind +1 ;
			}
		}
	}
	return Fields;
}

EBPJson UBPJsonObjectLib::Conv_BPJsonObjectToBPJson(const FBPJsonObject & Obj)
{
	if (!Obj.InnerObj.IsValid())
		return EBPJson::BPJSON_Null;
	else if (Obj.FieldName != "")
	{
		const TSharedPtr<FJsonValue> Field = Obj.InnerObj->TryGetField(Obj.FieldName);
		if (Field.IsValid())
			return FromEJson(Field->Type);
		else
			return EBPJson::BPJSON_Null;
	}
	else
	{
		return EBPJson::BPJSON_Object;
	}
};

FString UBPJsonObjectLib::Conv_BPJsonObjectToString(const FBPJsonObject & Value)
{
	return Value.AsString();
}

float UBPJsonObjectLib::Conv_BPJsonObjectToFloat(const FBPJsonObject & Value)
{
	return Value.AsNumber();
}

int32 UBPJsonObjectLib::Conv_BPJsonObjectToInt(const FBPJsonObject & Value)
{
	return static_cast<int32>(Value.AsNumber());
}

bool UBPJsonObjectLib::Conv_BPJsonObjectToBool(const FBPJsonObject & Value)
{
	if (Value.JsonType == EBPJson::BPJSON_Boolean)
		return Value.AsBoolean();
	else
		return false;
}


FBPJsonObject UBPJsonObjectLib::Conv_FloatToBPJsonObject( float & Value)
{
	const TSharedPtr<FJsonValue> Val = MakeShared<FJsonValueNumber>(Value);
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->Values.Add("",Val);
	return FBPJsonObject(EBPJson::BPJSON_Number, Obj, "");
}

FBPJsonObject UBPJsonObjectLib::Conv_IntToBPJsonObject(int32 & Value)
{
	const TSharedPtr<FJsonValue>  Val = MakeShared<FJsonValueNumber>(Value);
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->Values.Add("",Val);
	return FBPJsonObject(EBPJson::BPJSON_Number, Obj, "");
}

FBPJsonObject UBPJsonObjectLib::Conv_BoolToBPJsonObject(bool & Value)
{
	const TSharedPtr<FJsonValue> Val = MakeShared<FJsonValueBoolean>(Value);
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->Values.Add("",Val);
	return FBPJsonObject(EBPJson::BPJSON_Boolean, Obj, "");
}

FBPJsonObject UBPJsonObjectLib::Conv_StringToBPJsonObject(FString& Value)
{
	const TSharedPtr<FJsonValue> Val = MakeShared<FJsonValueString>(Value);
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->Values.Add("",Val);
	return FBPJsonObject(EBPJson::BPJSON_Boolean, Obj, "");
}

void UBPJsonObjectLib::RemoveFromParent(FBPJsonObject& Value)
{
	if (Value.FieldName != "" && Value.InnerObj.IsValid())
	{
		if(Value.InnerObj->HasField(Value.FieldName))
		{
			Value.InnerObj->RemoveField(Value.FieldName);
		}
	}
}
