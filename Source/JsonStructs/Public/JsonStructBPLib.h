#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "BPJsonObject.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h" 
#include "JsonStructBPLib.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(JsonStructs_Log, Log, Log);


/**
 * 
 */
UCLASS()
class JSONSTRUCTS_API UJsonStructBPLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	static TSharedPtr<FJsonObject> SetupJsonObject(UClass* Class, UObject* Object);

	TArray<FString> CreateTableFromJSONString(const FString& InString);

	static void Log(FString LogString, int32 Level);

	static FString RemoveUStructGuid(FString String);
	static UClass* FindClassByName(FString ClassNameInput);

	static TSharedPtr<FJsonValue> Conv_FPropertyToJsonValue(FProperty* Prop, void* Ptr, bool IncludeObjects, TArray<UObject*>& RecursionArray, bool IncludeNonInstanced, TArray<FString> FilteredFields, bool Exclude);
	static TSharedPtr<FJsonObject> Conv_UStructToJsonObject(const UStruct* Struct, void* Ptr, bool IncludeObjects, TArray<UObject*>& RecursionArray, bool IncludeNonInstanced, TArray<FString> FilteredFields, bool Exclude);
	static void Conv_JsonObjectToUStruct(TSharedPtr<FJsonObject> json, UStruct* Struct, void* Ptr, UObject* Outer);
	static void Conv_JsonValueToFProperty(TSharedPtr<FJsonValue> json, FProperty* Prop, void* Ptr, UObject* Outer,bool DoLog = true);
	static void InternalGetStructAsJson(FStructProperty* Structure, void* StructurePtr, FString& String, bool RemoveGUID = false, bool IncludeObjects = false);

	static void InternalGetStructAsJsonForTable(FStructProperty* Structure, void* StructurePtr, FString& String, bool RemoveGUID, FString Name);
	static TSharedPtr<FJsonObject> ConvertUStructToJsonObjectWithName(UStruct* Struct, void* Ptr, bool RemoveGUID, FString Name);

	static FString JsonObjectToString(TSharedPtr<FJsonObject> JsonObject);

	static TSharedPtr<FJsonObject> CDOToJson(UClass* ObjectClass, UObject* Object, bool ObjectRecursive, bool DeepRecursion, bool SkipRoot, bool SkipTransient, bool OnlyEditable, TArray<FString> FilteredFields, bool Exclude);


public:

	// Formats a Json String resulting from a Struct , in a Format Data Tables can accept ( Needs Row Name) 
	UFUNCTION(BlueprintCallable, Category = "Editor| Json", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetJsonFromStructForTable(FString RowName, UPARAM(ref)FString &String, uint8 Structure);

	DECLARE_FUNCTION(execGetJsonFromStructForTable)
	{
		FString RowName;
		Stack.StepCompiledIn<FStrProperty>(&RowName);
		PARAM_PASSED_BY_REF(String, FStrProperty, FString);
		Stack.Step(Stack.Object, nullptr);

		FStructProperty* StructureProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

		void* StructurePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		InternalGetStructAsJsonForTable(StructureProperty , StructurePtr, String, false, RowName);
	}

	UFUNCTION(BlueprintCallable, Category = "JsonStructs")
		static bool FillDataTableFromJSONString(UDataTable * DataTable, const FString & InString);

	// Json String from Struct
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void StructFromJsonDirect(const FString& String , UStruct* Structure);
	// Get Struct from Json
	UFUNCTION(BlueprintCallable, Category = "JsonStructs", CustomThunk, meta = (CustomStructureParam = "Structure"))
		static void GetJsonFromStructDirect(UPARAM(ref)FString & String , UStruct * Structure);

	DECLARE_FUNCTION(execStructFromJsonDirect) {
		FString String;

		Stack.StepCompiledIn<FStrProperty>(&String);
		Stack.Step(Stack.Object, nullptr);

		FStructProperty* Struct = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*String);
		FJsonSerializer Serializer;
		TSharedPtr<FJsonObject> Result;
		Serializer.Deserialize(Reader, Result);
		Conv_JsonObjectToUStruct(Result, Struct->Struct, StructPtr, nullptr);
	}
	DECLARE_FUNCTION(execGetJsonFromStructDirect)
	{
		PARAM_PASSED_BY_REF(String, FStrProperty, FString);
		Stack.Step(Stack.Object, nullptr);

		FStructProperty* StructureProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

		void* StructurePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		InternalGetStructAsJson(StructureProperty, StructurePtr, String, false);
	}

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static FString ObjectToJsonString(UClass * Value ,bool ObjectRecursive = false, UObject * DefaultObject= nullptr,bool DeepRecursion = false, bool SkipRoot= false, bool SkipTransient= false,bool OnlyEditable = false);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static void ObjectToJsonObject(UClass * ObjectClass, bool ObjectRecursive, UObject * DefaultObject, UObject* Outer, bool DeepRecursion , bool SkipRoot , bool SkipTransient ,bool OnlyEditable, FBPJsonObject& Object);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static void ObjectToJsonObjectFiltered(TArray<FString> Fields, UClass* ObjectClass, UObject* DefaultObject, UObject* Outer, bool ObjectRecursive, bool Exclude, bool DeepRecursion, bool SkipRoot, bool SkipTransient, bool OnlyEditable, FBPJsonObject& Object);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static FString ObjectToJsonStringFiltered(TArray<FString> Fields, UClass * Value, UObject* DefaultObject, bool ObjectRecursive = false, bool Exclude = false, bool DeepRecursion = false, bool SkipRoot = false, bool SkipTransient = false,bool OnlyEditable = false);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static bool SetClassDefaultsFromJsonString(FString JsonString, UClass * BaseClass, UObject * DefaultObject = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static UClass* FailSafeClassFind(FString Path);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static AActor* SpawnActorWithName(UObject* WorldContext, UClass* C, FName Name);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
		static UClass* CreateNewClass(const FString& ClassName, const FString& PackageName, UClass* ParentClass);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "IsNative", CompactNodeTitle = "IsNative", BlueprintAutocast), Category = "Utilities")
		static bool IsNative(UClass* Class) { if (!Class) return false;  return Class->IsNative(); };

	UFUNCTION(BlueprintPure, meta = (DisplayName = "String to Transform", CompactNodeTitle = "->", BlueprintAutocast), Category = "Utilities")
		static FTransform Conv_StringToTransform(FString String);


	UFUNCTION(BlueprintPure, meta = (DisplayName = "UClass to UProperty FieldNames", CompactNodeTitle = "->", BlueprintAutocast, CustomThunk,CustomStructureParam = "Structure"), Category = "Utilities")
		static void Conv_UClassToPropertyFieldNames(UStruct* Structure, TArray<FString>& Array, bool Recurse);

};
