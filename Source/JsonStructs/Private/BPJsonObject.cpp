#include "BPJsonObject.h"

FBPJsonObject::FBPJsonObject() : InnerObj(nullptr), FieldName(""), JsonType(EBPJson::BPJSON_Null)
{
}

FBPJsonObject::FBPJsonObject(const EBPJson JsonTypeIn, const TSharedPtr<FJsonObject> JsonObject, const FString FieldNameIn) :
	InnerObj(JsonObject), FieldName(FieldNameIn), JsonType(JsonTypeIn)
{
}

FString FBPJsonObject::GetJsonStringField(const FString Name, const bool SearchNested) const
{
	if (!InnerObj)
		return "";

	FString Out = "";
	if (InnerObj->Values.Contains(Name)) {
		const TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(Name);
		return Val->AsString();
	} else if (SearchNested) {
		if (InnerObj->Values.Num() == 0)
			return Out;

		for (const auto& i : InnerObj->Values) {
			const TSharedPtr<FJsonValue> Val = i.Value;
			if (Val->Type == EJson::Object) {
				const TSharedPtr<FJsonObject>& Object = Val->AsObject();
				if (Object->HasField(Name)) {
					return Object->GetStringField(Name);
				}
			} else if (Val->Type == EJson::Array) {
				const TArray<TSharedPtr<FJsonValue>>& Array = Val->AsArray();
				for (const TSharedPtr<FJsonValue>& Element : Array) {
					if (Element->Type == EJson::Object) {
						if (Element->AsObject()->HasField(Name)) {
							return Element->AsObject()->GetStringField(Name);
						}
					}
				}
			}
		}
	}
	return Out;
}

float FBPJsonObject::GetJsonNumberField(const FString Name, const bool SearchNested) const
{
	if (!InnerObj.IsValid())
		return 0.f;

	if (InnerObj->Values.Contains(Name)) {
		const TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(Name);
		return Val->AsNumber();
	} else if (SearchNested) {
		if (InnerObj->Values.Num() == 0)
			return 0.f;

		for (const auto& Value : InnerObj->Values) {
			if (!Value.Value.IsValid())
				continue;

			const TSharedPtr<FJsonValue> Val = Value.Value;
			if (Val->Type == EJson::Object) {
				if (Val->AsObject()->HasField(Name)) {
					const TSharedPtr<FJsonValue> NestedObjectField = Val->AsObject()->TryGetField(Name);
					if (NestedObjectField->Type == EJson::Number)
						return NestedObjectField->AsNumber();
				}
			} else if (Val->Type == EJson::Array) {
				auto& NestedArray = Val->AsArray();
				for (auto& Element : NestedArray) {
					if (Element->AsObject()->HasField(Name)) {
						const TSharedPtr<FJsonValue> NestedObjectField = Element->AsObject()->TryGetField(Name);
						if (NestedObjectField->Type == EJson::Number)
							return NestedObjectField->AsNumber();
					}
				}
			}
		}
	}
	return 0.f;
}

void FBPJsonObject::SetJsonStringField(const FString Name, const FString Value) const
{
	if (!InnerObj)
		return;
	if (InnerObj->Values.Contains(Name)) {
		auto val = *InnerObj->Values.Find(Name);
		if (val->Type == EJson::String) {
			const TSharedPtr<FJsonValueString> JsonObject = MakeShared<FJsonValueString>(Value);
			val = JsonObject;
		} else if (val->Type == EJson::Number) {
			if (!Value.IsNumeric())
				return;

			const int32 Property = static_cast<int32>(FCString::Atof(*Value));

			const TSharedPtr<FJsonValueNumber> JsonObject = MakeShared<FJsonValueNumber>(Property);
			val = JsonObject;
		} else if (val->Type == EJson::Boolean) {
			if (!Value.IsNumeric())
				return;

			bool Property = static_cast<bool>(FCString::Atof(*Value));
			const TSharedPtr<FJsonValueBoolean> JsonObject = MakeShared<FJsonValueBoolean>(Property);
			val = JsonObject;
		}
	}
}

void FBPJsonObject::SetJsonNumberField(const FString Name, const float Value) const
{
	if (!InnerObj)
		return;
	if (InnerObj->Values.Contains(Name)) {
		TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(Name);
		if (Val->Type == EJson::String) {
			Val = MakeShared<FJsonValueString>(FString::FromInt(Value));
		} else if (Val->Type == EJson::Number) {
			Val = MakeShared<FJsonValueNumber>(Value);
		} else if (Val->Type == EJson::Boolean) {
			Val = MakeShared<FJsonValueBoolean>(static_cast<bool>(Value));
		}
	}
}

FString FBPJsonObject::AsString() const
{
	if (!InnerObj.IsValid())
		return "";
	if (!InnerObj->Values.Contains(FieldName))
		return "";

	const TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(FieldName);
	if (Val->Type == EJson::Object) {
		if (Val->AsObject())
			if (Val->AsObject()->Values.Num() == 1) {
				for (auto& i : Val->AsObject()->Values) {
					if (i.Value->Type == EJson::String)
						return i.Value->AsString();
					else
						break;
				}
			}
	}
	return Val->AsString();
};

float FBPJsonObject::AsNumber() const
{
	if (!InnerObj.IsValid())
		return 0.f;
	if (!InnerObj->Values.Contains(FieldName))
		return 0.f;
	const TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(FieldName);

	if (Val->Type == EJson::Object) {
		if (Val->AsObject()) {
			if (Val->AsObject()->Values.Num() == 1) {
				for (auto& i : Val->AsObject()->Values) {
					if (i.Value->Type == EJson::Number)
						return i.Value->AsNumber();
					else
						break;
				}
			}
		}
	} else if (Val->Type == EJson::Number) {
		return Val->AsNumber();
	} else if (Val->Type == EJson::String) {
		const FString Var = Val->AsString();
		if (Var.IsNumeric()) {
			const float Num = FCString::Atof(*Var);
			return Num;
		}
	}

	return 0;
};

bool FBPJsonObject::AsBoolean() const
{
	if (!InnerObj.IsValid())
		return false;
	if (!InnerObj->Values.Contains(FieldName))
		return false;

	const TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(FieldName);

	if (Val->Type == EJson::Object) {
		if (Val->AsObject()) {
			if (Val->AsObject()->Values.Num() == 1) {
				for (const auto& i : Val->AsObject()->Values) {
					if (Val->Type == EJson::Boolean)
						return i.Value->AsBool();
					else
						break;
				}
			}
		}
	} else if (Val->Type == EJson::Boolean) {
		return Val->AsBool();
	}

	return false;
}

void FBPJsonObject::SetValueFromNumber(int32 Number) const
{
	if (!InnerObj.IsValid())
		return;

	TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(FieldName);

	if (Val->Type == EJson::String) {
		Val = MakeShared<FJsonValueString>(FString::FromInt(Number));
	} else if (Val->Type == EJson::Number) {
		Val = MakeShared<FJsonValueNumber>(Number);
	} else if (Val->Type == EJson::Boolean) {
		Val = MakeShared<FJsonValueBoolean>(static_cast<bool>(Number));
	}
}

void FBPJsonObject::SetValueFromString(FString String) const
{
	if (!InnerObj.IsValid())
		return;

	TSharedPtr<FJsonValue> Val = *InnerObj->Values.Find(FieldName);

	if (Val->Type == EJson::String) {
		Val = MakeShared<FJsonValueString>(String);
	} else if (Val->Type == EJson::Number) {
		if (!String.IsNumeric())
			return;
		Val = MakeShared<FJsonValueNumber>(static_cast<int32>(FCString::Atof(*String)));
	} else if (Val->Type == EJson::Boolean) {
		if (!String.IsNumeric())
			return;
		Val = MakeShared<FJsonValueBoolean>(static_cast<bool>(FCString::Atof(*String)));
	}
}
