#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileIOBPLib.generated.h"

UCLASS()
class JSONSTRUCTS_API UFileIOBPLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	template <class FunctorType>
	class PlatformFileFunctor : public IPlatformFile::FDirectoryVisitor	//GenericPlatformFile.h
	{
	public:
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			return Functor(FilenameOrDirectory, bIsDirectory);
		}

		PlatformFileFunctor(FunctorType&& FunctorInstance)
			: Functor(MoveTemp(FunctorInstance))
		{
		}

	private:
		FunctorType Functor;
	};

	template <class Functor>
	static PlatformFileFunctor<Functor> MakeDirectoryVisitor(Functor&& FunctorInstance)
	{
		return PlatformFileFunctor<Functor>(MoveTemp(FunctorInstance));
	}

public:
	UFUNCTION(BlueprintCallable, Category = "File IO")
	static bool GetFilesInPath(const FString& FullPathOfBaseDir, TArray<FString>& FilenamesOut, bool Recursive = false, const FString& FilterByExtension = "");

	UFUNCTION(BlueprintCallable, Category = "File IO")
	static bool GetDirectoriesInPath(const FString& FullPathOfBaseDir, TArray<FString>& DirsOut, const FString& NotContainsStr = "", bool Recursive = false, const FString& ContainsStr = "");

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void String_Sort(UPARAM(ref)TArray<FString>& Array_To_Sort, bool Descending, bool FilterToUnique, TArray<FString>& Sorted_Array);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void WriteStringToFile(FString FilePath, FString String, bool Relative = true);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static bool LoadStringFromFile(FString& String, FString FilePath, bool Relative = true);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void GetDerivedClassesFiltered(UClass* ClassIn, TArray<UClass*>& DerivedClasses, TArray<UClass*> Filter, bool Recursive);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void GetAllScriptClassFolders(TArray<FString>& Folders, FString StartsWith);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void GetAllScriptClassesInPath(TArray<UClass*>& Classes, FString Path);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	static void GetAllScriptClasses(TArray<UClass*>& Classes);
};
