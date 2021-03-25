#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JsonStructBPLib.generated.h"

/**
 * 
 */
UCLASS()
class JSONSTRUCTS_API UJsonStructBPLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TSharedPtr<FJsonValue> convertUPropToJsonValue(UProperty* prop, void* ptrToProp);
	static TSharedPtr<FJsonObject> convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct);
	static void convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct);
	static void convertJsonValueToUProperty(TSharedPtr<FJsonValue> json, UProperty* prop, void* ptrToProp);
	static void InternalGetStructAsJson(UStructProperty *Structure, void * StructurePtr, FString &String);


	// Json String from Struct
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetJsonAsStruct(const FString& String, UPARAM(ref) UProperty*& Structure);
	// Get Struct from Json
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetStructAsJson(UPARAM(ref)FString &String, UProperty *Structure);

	DECLARE_FUNCTION(execGetJsonAsStruct) {
		FString String;
		Stack.StepCompiledIn<UStrProperty>(&String);
		Stack.Step(Stack.Object, NULL);

		UStructProperty* Struct = Cast<UStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
		FJsonSerializer Serializer;
		TSharedPtr<FJsonObject> result;
		Serializer.Deserialize(reader, result);
		convertJsonObjectToUStruct(result, Struct->Struct, StructPtr);
	}
	DECLARE_FUNCTION(execGetStructAsJson)
	{
		PARAM_PASSED_BY_REF(String, UStrProperty, FString);
		Stack.Step(Stack.Object, NULL);

		UStructProperty* StructureProperty = Cast<UStructProperty>(Stack.MostRecentProperty);

		void* StructurePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		InternalGetStructAsJson(StructureProperty, StructurePtr, String);
	}
};
