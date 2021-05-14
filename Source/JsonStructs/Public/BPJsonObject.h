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



UCLASS(BlueprintType)
class JSONSTRUCTS_API UBPJsonObject : public UObject
{
	GENERATED_BODY()

public:

	void InitSubObjects();

	UBPJsonObject();

	UFUNCTION(BlueprintPure)
	TArray<FString> GetFieldNames();


	UFUNCTION(BlueprintPure)
		FString GetJsonStringField(const FString Name, bool SearchNested = false);

	UFUNCTION(BlueprintPure)
		float GetJsonNumberField(const FString Name, bool SearchNested = false);

	UFUNCTION(BlueprintPure)
		EBPJson GetFieldType(const FString Name);

	UFUNCTION(BlueprintCallable)
		void SetJsonStringField(const FString Name, const FString Value);


	UFUNCTION(BlueprintCallable)
		void SetJsonNumberField(const FString Name, const float Value);
	

	UFUNCTION(BlueprintCallable)
		static UBPJsonObject * GetStringAsJson(UPARAM(ref)FString &String, UObject * Outer);

	UFUNCTION(BlueprintCallable)
		FString GetAsString();

	TSharedPtr<FJsonObject>  InnerObj;

	bool IsArray = false;
	
	UPROPERTY(BlueprintReadWrite)
	TMap< FString , UBPJsonObjectValue* > Values;
};
