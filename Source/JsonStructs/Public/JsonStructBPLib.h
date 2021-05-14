#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "BPJsonObject.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h" 
#include "JsonStructBPLib.generated.h"



/**
 * 
 */
UCLASS()
class JSONSTRUCTS_API UJsonStructBPLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void Log(FString LogString, int32 Level);


	static FString RemoveUStructGuid(FString String);
	static UClass * FindClassByName(FString ClassNameInput);

	static TSharedPtr<FJsonValue> convertUPropToJsonValue(FProperty* prop, void* ptrToProp, bool includeObjects, TArray<UObject*>& RecursedObjects, bool IncludeNonInstanced);
	static TSharedPtr<FJsonObject> convertUStructToJsonObject(UStruct* Struct, void* ptrToStruct, bool includeObjects, TArray<UObject*>& RecursedObjects, bool IncludeNonInstanced);
	static void convertJsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* ptrToStruct, UObject* Outer);
	static void convertJsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* prop, void* ptrToProp, UObject* Outer);
	static void InternalGetStructAsJson(FStructProperty *Structure, void * StructurePtr, FString &String, bool RemoveGUID = false, bool includeObjects = false);

	static void InternalGetStructAsJsonForTable(FStructProperty * Structure, void * StructurePtr, FString & String, bool RemoveGUID, FString Name);
	static TSharedPtr<FJsonObject> ConvertUStructToJsonObjectWithName(UStruct * Struct, void * ptrToStruct,bool RemoveGUID, FString Name);
	// Formats a Json String resulting from a Struct , in a Format Data Tables can accept ( Needs Row Name) 
	
	UFUNCTION(BlueprintCallable, Category = "Editor| Json", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetJsonFromStructForTable(FString RowName, UPARAM(ref)FString &String, uint8 Structure);

	DECLARE_FUNCTION(execGetJsonFromStructForTable)
	{
		FString RowName;
		Stack.StepCompiledIn<FStrProperty>(&RowName);
		PARAM_PASSED_BY_REF(String, FStrProperty, FString);
		Stack.Step(Stack.Object, NULL);

		FStructProperty* StructureProperty = Cast<FStructProperty>(Stack.MostRecentProperty);

		void* StructurePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		InternalGetStructAsJsonForTable(StructureProperty, StructurePtr, String, false, RowName);
	}

	UFUNCTION(BlueprintCallable, Category = "JsonStructs")
		static bool FillDataTableFromJSONString(UDataTable * DataTable, const FString & InString);

	TArray<FString> CreateTableFromJSONString(const FString& InString);

	// Json String from Struct
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetStructFromJson(const FString& String ,UPARAM(ref) bool Structure);
	// Get Struct from Json
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetJsonFromStruct(FString &String, UStruct * Structure);

	DECLARE_FUNCTION(execGetStructFromJson) {
		FString String;

		Stack.StepCompiledIn<FStrProperty>(&String);
		Stack.Step(Stack.Object, NULL);

		FStructProperty* Struct = Cast<FStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
		FJsonSerializer Serializer;
		TSharedPtr<FJsonObject> result;
		Serializer.Deserialize(reader, result);
		convertJsonObjectToUStruct(result, Struct->Struct, StructPtr,nullptr);
	}
	DECLARE_FUNCTION(execGetJsonFromStruct)
	{

		PARAM_PASSED_BY_REF(String, FStrProperty, FString);
		Stack.Step(Stack.Object, NULL);

		FStructProperty* StructureProperty = Cast<FStructProperty>(Stack.MostRecentProperty);

		void* StructurePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		InternalGetStructAsJson(StructureProperty, StructurePtr, String, false);
		UE_LOG(LogTemp, Error, TEXT("%s"),*String);
	}

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static FString ClassDefaultsToJsonString(UClass * Value ,bool ObjectRecursive = false, UObject * DefaultObject= nullptr,bool DeepRecursion = false, bool SkipRoot= false, bool SkipTransient= false,bool OnlyEditable = false);

	static FString JsonObjectToString(TSharedPtr<FJsonObject> JsonObject);


	static TSharedPtr<FJsonObject> CDOToJson(UClass* ObjectClass, UObject* Object, bool ObjectRecursive, bool DeepRecursion, bool SkipRoot, bool SkipTransient, bool OnlyEditable);
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static UBPJsonObject * ClassDefaultsToJsonObject(UClass * ObjectClass, bool ObjectRecursive, UObject * DefaultObject, UObject* Outer, bool DeepRecursion = false, bool SkipRoot = false, bool SkipTransient = false,bool OnlyEditable = false);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static FString CDOFieldsToJsonString(TArray<FString> Fields, UClass * Value, bool ObjectRecursive = false, bool Exclude = false, bool DeepRecursion = false, bool SkipRoot = false, bool SkipTransient = false,bool OnlyEditable = false);

		
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static bool SetClassDefaultsFromJsonString(FString JsonString, UClass * BaseClass, UObject * DefaultObject = nullptr);
	
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static UClass* FailSafeClassFind(FString Path);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static UClass* CreateNewClass(const FString& ClassName, const FString& PackageName, UClass* ParentClass, const FString& MountPoint);



	UFUNCTION(BlueprintPure, meta = (DisplayName = "A JsonValue String Conversion", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static FString Conv_BPJsonObjectValueToString(UBPJsonObjectValue * Value);
	
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Json Value to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static float Conv_BPJsonObjectValueToFloat(UBPJsonObjectValue * Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Json Value to Float", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static int32 Conv_BPJsonObjectValueToInt(UBPJsonObjectValue * Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Json Value to Bool", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static bool Conv_BPJsonObjectValueToBool(UBPJsonObjectValue * Value);


	UFUNCTION(BlueprintPure, meta = (DisplayName = "A JsonValue String Conversion", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static UBPJsonObjectValue * Conv_StringToBPJsonObjectValue(FString & Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Float to JsonValue", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static UBPJsonObjectValue * Conv_FloatToBPJsonObjectValue(float & Value);
	
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Int to JsonValue", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static UBPJsonObjectValue * Conv_IntToBPJsonObjectValue(int32 & Value);
	
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Bool to JsonValue", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static UBPJsonObjectValue * Conv_BoolToBPJsonObjectValue(bool & Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "JsonObject to JsonObjectValues", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static TArray<UBPJsonObjectValue *>  Conv_BPJsonObjectToBPJsonObjectValue(UBPJsonObject * Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "JsonObjectValue to JsonObject", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static UBPJsonObject * Conv_UBPJsonObjectValueToBPJsonObject(UBPJsonObjectValue * Value);


	UFUNCTION(BlueprintPure, meta = (DisplayName = "IsNative", CompactNodeTitle = "IsNative", BlueprintAutocast), Category = "Utilities")
		static bool IsNative(UClass* Class) { if (!Class) return false;  return Class->IsNative(); };

};
