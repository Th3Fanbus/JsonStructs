#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h" 
#include "BPJsonObjectValue.generated.h"

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

class UBPJsonObject;

UCLASS(BlueprintType)
class JSONSTRUCTS_API UBPJsonObjectValue : public UObject
{
	GENERATED_BODY()


public:
	UBPJsonObjectValue() { 
		ValueObject = nullptr;
		Parent = nullptr;
		FieldName = "";
	};

	void InitSubObject(FString Name, UBPJsonObject* Parent);

	FString FieldName;

	UPROPERTY(BlueprintReadOnly)
	UBPJsonObject* ValueObject; 
	UPROPERTY(BlueprintReadOnly)
	UBPJsonObject* Parent;

	UFUNCTION(BlueprintPure)
		EBPJson GetFieldType();
	UFUNCTION(BlueprintPure)
		FString AsString();
	UFUNCTION(BlueprintPure)
		float AsNumber();
	UFUNCTION(BlueprintPure)
		bool AsBoolean();
	UFUNCTION(BlueprintPure)
		TArray<UBPJsonObjectValue* > AsArray();

	UFUNCTION(BlueprintCallable)
	void SetValueFromNumber(int32 Number);
	UFUNCTION(BlueprintCallable)
	void SetValueFromString(FString String);


	UFUNCTION(BlueprintPure)
		UBPJsonObject * AsObject();


	UFUNCTION(BlueprintCallable)
		FString ConvertToString();

	UFUNCTION(BlueprintCallable)
	static UBPJsonObjectValue* ConvertFromString(FString JsonString);
};
