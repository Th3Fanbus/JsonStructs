#include "BPJsonObjectValue.h"
#include "BPJsonObject.h"

EBPJson UBPJsonObjectValue::GetFieldType()
{
	if (!Value)
		return EBPJson::BPJSON_Null;
	else
	{
		auto Field = Value;
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

FString UBPJsonObjectValue::AsString()
{
	if (!Value) 
		return "";
	if (Value->Type == EJson::Object) {
		if (Value->AsObject())
			if (Value->AsObject()->Values.Num() == 1)
			{
				for (auto i : Value->AsObject()->Values)
				{
					if (Value->Type == EJson::String)
						return i.Value->AsString();
					else
						break;
				}
			}
	}
	

	return Value.Get()->AsString();
};

float UBPJsonObjectValue::AsNumber()
{
	if (!Value) 
		return 0.f;
	if (Value->Type == EJson::Object) {
		if (Value->AsObject())
			if (Value->AsObject()->Values.Num() == 1)
			{
				for (auto i : Value->AsObject()->Values)
				{
					if (Value->Type == EJson::Number)
						return i.Value->AsNumber();
					else
						break;
				}
			}
	}
	else if (Value->Type == EJson::Number)
	{
		return Value.Get()->AsNumber();
	}
	else if (Value->Type == EJson::String)
	{
		FString var = Value.Get()->AsString();
		if (var.IsNumeric())
		{
			float num = FCString::Atof(*var);
			return num;
		}
	}
	
	return 0;
	
};

bool UBPJsonObjectValue::AsBoolean()
{
	if (!Value)
		return 0.f;
	if (Value->Type == EJson::Object) {
		if (Value->AsObject())
			if (Value->AsObject()->Values.Num() == 1)
			{
				for (auto i : Value->AsObject()->Values)
				{
					if (Value->Type == EJson::Boolean)
						return i.Value->AsBool();
					else
						break;
				}
			}
	}
	
	return Value.Get()->AsBool();
}

void UBPJsonObjectValue::SetValueFromNumber(int32 Number)
{
	if (Value->Type == EJson::Array) {

	}
	else if (Value->Type == EJson::String)
	{
		TSharedPtr<FJsonValueString> JsonObject = TSharedPtr<FJsonValueString>(new FJsonValueString(FString::FromInt(Number)));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Number)
	{
		TSharedPtr<FJsonValueNumber> JsonObject = TSharedPtr<FJsonValueNumber>(new FJsonValueNumber(Number));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Boolean)
	{
		TSharedPtr<FJsonValueBoolean> JsonObject = TSharedPtr<FJsonValueBoolean>(new FJsonValueBoolean(bool(Number)));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Object)
	{

	}
}

void UBPJsonObjectValue::SetValueFromString(FString String)
{
	if (Value->Type == EJson::Array) {

	}
	else if (Value->Type == EJson::String)
	{
		TSharedPtr<FJsonValueString> JsonObject = TSharedPtr<FJsonValueString>(new FJsonValueString(String));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Number)
	{
		if (!String.IsNumeric())
			return;

		int32 Property = int32(FCString::Atof(*String));

		TSharedPtr<FJsonValueNumber> JsonObject = TSharedPtr<FJsonValueNumber>(new FJsonValueNumber(Property));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Boolean)
	{
		if (!String.IsNumeric())
			return;

		bool Property = bool(FCString::Atof(*String));
		TSharedPtr<FJsonValueBoolean> JsonObject = TSharedPtr<FJsonValueBoolean>(new FJsonValueBoolean(Property));
		Value = JsonObject;
	}
	else if (Value->Type == EJson::Object)
	{

	}
}


TArray<UBPJsonObjectValue * > UBPJsonObjectValue::AsArray()
{
	TArray<UBPJsonObjectValue* > Out = {};
	if (Value->Type == EJson::Array) {
		TArray< TSharedPtr<FJsonValue> > Arr = Value->AsArray();
		for (auto i : Arr)
		{
			UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>();
			Obj->Value = i;
			Out.Add(Obj);
		}
	}
	return Out;
}
;

UBPJsonObject * UBPJsonObjectValue::AsObject()
{
	if (!Value)
		return nullptr;

	UBPJsonObject * Obj = NewObject<UBPJsonObject>(this);
	Obj->InnerObj = Value.Get()->AsObject();
	return Obj;

}

FString UBPJsonObjectValue::ConvertToString()
{
	if (!Value)
		return "";
	FString write;
	TSharedPtr<FJsonObject> JsonObject = TSharedPtr<FJsonObject>(new FJsonObject());
	JsonObject->SetField("ObjectValue:", Value);
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return write;
}

UBPJsonObjectValue* UBPJsonObjectValue::ConvertFromString(FString JsonString)
{
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*JsonString);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);
	UBPJsonObjectValue* Obj = NewObject<UBPJsonObjectValue>();
	if (result.IsValid())
	{

		TSharedRef< FJsonObject> Ref = result.ToSharedRef();
		if (Ref->HasField("ObjectValue"))
		{
			FString FieldName = "ObjectValue";
			Obj->Value = *result->Values.Find(FieldName);
			return Obj;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Valid Json but not of Type Value. Use UBPJsonObject instead."));
		}
	}

	return nullptr;
}