// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPJsonObject.h"
#include "BPJsonObjectLib.generated.h"

UCLASS()
class JSONSTRUCTS_API UBPJsonObjectLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static EBPJson FromEJson(EJson Value);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Set"))
	static void SetValueFromNumber(UPARAM(ref)FBPJsonObject& Object, const int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Set"))
	static void SetValueFromString(UPARAM(ref)FBPJsonObject& Object, const FString Value);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Set"))
	static void SetJsonStringField(UPARAM(ref)FBPJsonObject& Object, const FString Name, const FString Value);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Set"))
	static void SetJsonNumberField(UPARAM(ref)FBPJsonObject& Object, const FString Name, const float Value);


	UFUNCTION(BlueprintPure, Category = "Utilities", meta = (CompactNodeTitle = "Get"))
	static FBPJsonObject GetJsonField(UPARAM(ref)FBPJsonObject& Object, const FString Name);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Json String as BPJsonObject"), Category = "Utilities")
	static FBPJsonObject JsonStringToBPJsonObject(UPARAM(ref)FString& String);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "BPJsonObject as Json String"), Category = "Utilities")
	static FString BPJsonObjectToJsonString(const FBPJsonObject& Object);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject Fields", CompactNodeTitle = "Fields", BlueprintAutocast), Category = "Utilities")
	static TArray<FString> Conv_BPJsonObjectToStringArray(const FBPJsonObject& Obj);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject Type", CompactNodeTitle = "Type", BlueprintAutocast), Category = "Utilities")
	static EBPJson Conv_BPJsonObjectToBPJson(const FBPJsonObject& Obj);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject Values", CompactNodeTitle = "Values", BlueprintAutocast), Category = "Utilities")
	static TArray<FBPJsonObject> Conv_BPJsonObjectToBPJsonObjectArray(const FBPJsonObject& Object);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject As String", CompactNodeTitle = "Get", BlueprintAutocast), Category = "Utilities")
	static FString Conv_BPJsonObjectToString(const FBPJsonObject& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject As Float", CompactNodeTitle = "Get", BlueprintAutocast), Category = "Utilities")
	static float Conv_BPJsonObjectToFloat(const FBPJsonObject& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject As Int", CompactNodeTitle = "Get", BlueprintAutocast), Category = "Utilities")
	static int32 Conv_BPJsonObjectToInt(const FBPJsonObject& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get BPJsonObject As Boolean", CompactNodeTitle = "Get", BlueprintAutocast), Category = "Utilities")
	static bool Conv_BPJsonObjectToBool(const FBPJsonObject& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Float As BPJsonObject", CompactNodeTitle = "new", BlueprintAutocast), Category = "Utilities")
	static FBPJsonObject Conv_FloatToBPJsonObject(UPARAM(ref)float& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Int As BPJsonObject", CompactNodeTitle = "new", BlueprintAutocast), Category = "Utilities")
	static FBPJsonObject Conv_IntToBPJsonObject(UPARAM(ref)int32& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Boolean As BPJsonObject", CompactNodeTitle = "new", BlueprintAutocast), Category = "Utilities")
	static FBPJsonObject Conv_BoolToBPJsonObject(UPARAM(ref)bool& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Boolean As BPJsonObject", CompactNodeTitle = "new", BlueprintAutocast), Category = "Utilities")
	static FBPJsonObject Conv_StringToBPJsonObject(UPARAM(ref)FString& Value);

	UFUNCTION(BlueprintCallable, meta = (CompactNodeTitle = "Remove"), Category = "Utilities")
	static void RemoveFromParent(UPARAM(ref)FBPJsonObject& Value);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Remove"))
	static void RemoveField(UPARAM(ref)FBPJsonObject& Object, const FString Name);

	UFUNCTION(BlueprintCallable, Category = "Utilities", meta = (CompactNodeTitle = "Remove"))
	static void AddField(UPARAM(ref)FBPJsonObject& Object, const FString Name, UPARAM(ref)FBPJsonObject& FieldValue);
};
