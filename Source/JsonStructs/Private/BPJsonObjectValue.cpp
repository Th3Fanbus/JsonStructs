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
};

UBPJsonObject * UBPJsonObjectValue::AsObject()
{
	if (!Value)
		return nullptr;

	UBPJsonObject * Obj = NewObject<UBPJsonObject>(this);
	Obj->InnerObj = Value.Get()->AsObject();
	return Obj;

}