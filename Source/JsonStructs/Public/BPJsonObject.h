#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BPJsonObjectValue.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h" 
#include "BPJsonObject.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum EBPJson
{
	BPJSON_None,
	BPJSON_Null,
	BPJSON_String,
	BPJSON_Number,
	BPJSON_Boolean,
	BPJSON_Array,
	BPJSON_Object
};



USTRUCT(BlueprintType)
struct JSONSTRUCTS_API FBPJsonObject
{
	GENERATED_BODY()

public:
	FBPJsonObject(); ;
	FBPJsonObject(EBPJson JsonTypeIn, TSharedPtr<FJsonObject> JsonObject, FString FieldNameIn);
	
	FString GetJsonStringField(const FString Name, bool SearchNested = false) const;
	float GetJsonNumberField(const FString Name, bool SearchNested = false) const;

	TSharedPtr<FJsonObject>  InnerObj;
	FString FieldName;
	EBPJson JsonType;

	FString AsString() const;
	float AsNumber() const;
	bool AsBoolean() const;

	void SetValueFromNumber(int32 Number) const;
	void SetValueFromString(FString String) const;

	void SetJsonStringField(const FString Name, const FString Value) const;
	void SetJsonNumberField(const FString Name, const float Value) const;
	
};
