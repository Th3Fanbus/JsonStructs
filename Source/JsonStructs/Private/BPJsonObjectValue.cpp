#include "BPJsonObjectValue.h"
#include "BPJsonObject.h"

void UBPJsonObjectValue::InitSubObject(FString Name, UBPJsonObject * NewParent)
{
	FieldName = Name;
	Parent = NewParent;
	TSharedPtr<FJsonValue> Field = NewParent->InnerObj->TryGetField(FieldName);
	if (Field->Type == EJson::Object)
	{
		UBPJsonObject* Obj = NewObject<UBPJsonObject>(NewParent);
		ValueObject = Obj;
		Obj->InnerObj = Field->AsObject();
		Obj->InitSubObjects();
	}
	else if (Field->Type == EJson::Array)
	{

		UBPJsonObject* Obj = NewObject<UBPJsonObject>(NewParent);
		ValueObject = Obj;
		Obj->IsArray = true;
		Obj->InnerObj = TSharedPtr<FJsonObject>(new FJsonObject());
		int32 ind = 0;

		for (auto i : Field->AsArray())
		{
			if (i->Type == EJson::Object)
			{
				Obj->InnerObj->SetObjectField(FString::FromInt(ind), i->AsObject());
			}
			else if (i->Type == EJson::Boolean)
			{
				Obj->InnerObj->SetBoolField(FString::FromInt(ind), i->AsBool());
			}
			else if (i->Type == EJson::Number)
			{
				Obj->InnerObj->SetNumberField(FString::FromInt(ind), i->AsNumber());
			}
			else if (i->Type == EJson::String)
			{
				Obj->InnerObj->SetStringField(FString::FromInt(ind), i->AsString());
			}
			else if(i->Type == EJson::Array)
			{	
				Obj->InnerObj->SetArrayField(FString::FromInt(ind), i->AsArray());
			}
			ind++;
		}

		Obj->InitSubObjects();
	}
	else
	{
		ValueObject = Parent;
	}
}

EBPJson UBPJsonObjectValue::GetFieldType()
{
	if (!ValueObject)
		return EBPJson::BPJSON_Null;
	else
	{
		TSharedPtr<FJsonValue> Field;
		
		Field = Parent->InnerObj->TryGetField(FieldName);
		
		if (!Field)
		{
			return EBPJson::BPJSON_Null;
		}
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
			if (ValueObject->IsArray)
			{
				return EBPJson::BPJSON_Array;
			}
			else if(ValueObject != Parent)
			{
				if (ValueObject->GetFieldNames().Num() > 1)
				{
					return EBPJson::BPJSON_Object;
				}
				for (auto i : ValueObject->GetFieldNames())
				{
					return ValueObject->GetFieldType(i);
				}
			}
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
	if (!ValueObject)
		return "";
	if (!Parent->InnerObj->Values.Contains(FieldName))
		return "";
	auto val = *Parent->InnerObj->Values.Find(FieldName);
	if (val->Type == EJson::Object) {
		if (val->AsObject())
			if (val->AsObject()->Values.Num() == 1)
			{
				for (auto i : val->AsObject()->Values)
				{
					if (i.Value->Type == EJson::String)
						return i.Value->AsString();
					else
						break;
				}
			}
	}
	return val->AsString();
};

float UBPJsonObjectValue::AsNumber()
{
	if (!ValueObject)
		return 0.f;
	if (!Parent->InnerObj->Values.Contains(FieldName))
		return 0.f;
	auto val = *Parent->InnerObj->Values.Find(FieldName);

	if (val->Type == EJson::Object) {
		if (val->AsObject())
			if (val->AsObject()->Values.Num() == 1)
			{
				for (auto i : val->AsObject()->Values)
				{
					if (i.Value->Type == EJson::Number)
						return i.Value->AsNumber();
					else
						break;
				}
			}
	}
	else if (val->Type == EJson::Number)
	{
		return val->AsNumber();
	}
	else if (val->Type == EJson::String)
	{
		FString var = val->AsString();
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
	if (!ValueObject)
		return false;
	if (!Parent->InnerObj->Values.Contains(FieldName))
		return false;

	auto & val = *Parent->InnerObj->Values.Find(FieldName);

	if (val->Type == EJson::Object) {
		if (val->AsObject())
			if (val->AsObject()->Values.Num() == 1)
			{
				for (auto i : val->AsObject()->Values)
				{
					if (val->Type == EJson::Boolean)
						return i.Value->AsBool();
					else
						break;
				}
			}
	}
	
	return val->AsBool();
}

void UBPJsonObjectValue::SetValueFromNumber(int32 Number)
{
	if (!ValueObject)
		return;

	auto & val = *Parent->InnerObj->Values.Find(FieldName);

	if (val->Type == EJson::Array) {

	}
	else if (val->Type == EJson::String)
	{
		TSharedPtr<FJsonValueString> JsonObject = TSharedPtr<FJsonValueString>(new FJsonValueString(FString::FromInt(Number)));
		val = JsonObject;
	}
	else if (val->Type == EJson::Number)
	{
		TSharedPtr<FJsonValueNumber> JsonObject = TSharedPtr<FJsonValueNumber>(new FJsonValueNumber(Number));
		val = JsonObject;
	}
	else if (val->Type == EJson::Boolean)
	{
		TSharedPtr<FJsonValueBoolean> JsonObject = TSharedPtr<FJsonValueBoolean>(new FJsonValueBoolean(bool(Number)));
		val = JsonObject;
	}
	else if (val->Type == EJson::Object)
	{

	}
}

void UBPJsonObjectValue::SetValueFromString(FString String)
{
	if (!ValueObject)
		return;

	auto & val = *Parent->InnerObj->Values.Find(FieldName);

	if (val->Type == EJson::Array) {

	}
	else if (val->Type == EJson::String)
	{
		TSharedPtr<FJsonValueString> JsonObject = TSharedPtr<FJsonValueString>(new FJsonValueString(String));
		val = JsonObject;
	}
	else if (val->Type == EJson::Number)
	{
		if (!String.IsNumeric())
			return;

		int32 Property = int32(FCString::Atof(*String));

		TSharedPtr<FJsonValueNumber> JsonObject = TSharedPtr<FJsonValueNumber>(new FJsonValueNumber(Property));
		val = JsonObject;
	}
	else if (val->Type == EJson::Boolean)
	{
		if (!String.IsNumeric())
			return;

		bool Property = bool(FCString::Atof(*String));
		TSharedPtr<FJsonValueBoolean> JsonObject = TSharedPtr<FJsonValueBoolean>(new FJsonValueBoolean(Property));
		val = JsonObject;
	}
	else if (val->Type == EJson::Object)
	{

	}
}


TArray<UBPJsonObjectValue * > UBPJsonObjectValue::AsArray()
{
	TArray<UBPJsonObjectValue* > Out = {};
	if (ValueObject) {
		for (auto i : ValueObject->Values)
		{
			Out.Add(i.Value);
		}
	}
	return Out;
}
;

UBPJsonObject * UBPJsonObjectValue::AsObject()
{
	if (!ValueObject)
		return nullptr;

	if (Parent->Values.Contains(FieldName) && Parent != ValueObject)
	{
		return ValueObject;
	}
	else
		return nullptr;
}

FString UBPJsonObjectValue::ConvertToString()
{
	if (!ValueObject)
		return "";
	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(ValueObject->InnerObj.ToSharedRef(), JsonWriter);
	return write;
}

UBPJsonObjectValue* UBPJsonObjectValue::ConvertFromString(FString JsonString)
{
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*JsonString);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);
	UBPJsonObject* Obj = NewObject<UBPJsonObject>();
	if (result.IsValid())
	{
		Obj->InnerObj = result.ToSharedRef();
		Obj->InitSubObjects();
	}
	UBPJsonObjectValue* Objv = NewObject<UBPJsonObjectValue>();
	Objv->ValueObject = Obj;
	return Objv;
}