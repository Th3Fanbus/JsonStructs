// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
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
	UBPJsonObjectValue() {};
	TSharedPtr<FJsonValue> Value;
	UFUNCTION(BlueprintPure)
		EBPJson GetFieldType();
	UFUNCTION(BlueprintPure)
		FString AsString();
	UFUNCTION(BlueprintPure)
		float AsNumber();
	UFUNCTION(BlueprintPure)
		bool AsBoolean();

	UFUNCTION(BlueprintPure)
		UBPJsonObject * AsObject();
};
