#include "FileIOBPLib.h"
#include "Algo/Reverse.h"

void UFileIOBPLib::WriteStringToFile(FString Path, FString resultString, bool Relative)
{
#if WITH_EDITOR 
	FFileHelper::SaveStringToFile(resultString, Relative ? *(FPaths::ProjectDir() + Path) : *Path);
#else
	const FString AbsoluteRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString AbsolutePath = AbsoluteRootPath + TEXT("Mods/") + Path;
	if (!AbsolutePath.Contains(TEXT(".."))) {
		FFileHelper::SaveStringToFile(resultString, *AbsolutePath);
	} else {
		UE_LOG(LogTemp, Error, TEXT("Absolute or escaping Paths are not allowed in Runtime"));
	}
#endif
}

bool UFileIOBPLib::LoadStringFromFile(FString& String, FString Path, bool Relative)
{
#if WITH_EDITOR 
	return FFileHelper::LoadFileToString(String, Relative ? *(FPaths::ProjectDir() + Path) : *Path);
#else
	const FString AbsoluteRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(Path);
	if (AbsolutePath.StartsWith(AbsoluteRootPath)) {
		return FFileHelper::LoadFileToString(String, *AbsolutePath);
	} else {
		return false;
		UE_LOG(LogTemp, Error, TEXT("Absolute or escaping Paths are not allowed in Runtime"));
	}
#endif
}

bool UFileIOBPLib::GetDirectoriesInPath(const FString& FullPathOfBaseDir, TArray<FString>& DirsOut, const FString& NotContainsStr, bool Recursive, const FString& ContainsStr)
{
	FString Str;

	auto FilenamesVisitor = MakeDirectoryVisitor([&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (bIsDirectory) {
			//Using a Contains Filter?
			if (ContainsStr != "") {
				Str = FPaths::GetCleanFilename(FilenameOrDirectory);
				//Only if Directory Contains Str
				if (Str.Contains(ContainsStr)) {
					if (Recursive) {
						DirsOut.Push(FilenameOrDirectory); //need whole path for recursive
					} else {
						DirsOut.Push(Str);
					}
				}
			} else if (NotContainsStr != "") {
				if (!Str.Contains(NotContainsStr)) {
					if (Recursive) {
						DirsOut.Push(FilenameOrDirectory); //need whole path for recursive
					} else {
						DirsOut.Push(Str);
					}
				}
			} else {
				//Get ALL Directories!

				//Just the Directory
				Str = FPaths::GetCleanFilename(FilenameOrDirectory);
				if (Recursive) {
					DirsOut.Push(FilenameOrDirectory); //need whole path for recursive
				} else {
					DirsOut.Push(Str);
				}
			}
		}
		return true;
	});

	if (Recursive) {
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FullPathOfBaseDir, FilenamesVisitor);
	} else {
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*FullPathOfBaseDir, FilenamesVisitor);
	}
}

void UFileIOBPLib::String_Sort(UPARAM(ref) TArray<FString>& Array_To_Sort, bool Descending, bool FilterToUnique, TArray<FString>& Sorted_Array)
{
	// TODO: What happens if FilterToUnique is false?
	if (FilterToUnique) {
		for (const auto& i : Array_To_Sort) {
			Sorted_Array.AddUnique(i);
		}
	}
	Sorted_Array.Sort(); // Sort array using built in function (sorts A-Z)

	if (Descending == true) {
		Algo::Reverse(Sorted_Array); // array is now Z-A
	}
}

bool UFileIOBPLib::GetFilesInPath(const FString& FullPathOfBaseDir, TArray<FString>& FilenamesOut, bool Recursive, const FString& FilterByExtension)
{
	//Format File Extension, remove the "." if present
	const FString FileExt = FilterByExtension.Replace(TEXT("."), TEXT("")).ToLower();

	FString Str;
	auto FilenamesVisitor = MakeDirectoryVisitor([&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		//Files
		if (!bIsDirectory) {
			//Filter by Extension
			if (FileExt != "") {
				Str = FPaths::GetCleanFilename(FilenameOrDirectory);

				//Filter by Extension
				if (FPaths::GetExtension(Str).ToLower() == FileExt) {
					if (Recursive) {
						FilenamesOut.Push(FilenameOrDirectory); //need whole path for recursive
					} else {
						FilenamesOut.Push(Str);
					}
				}
			} else {
				//Include All Filenames!

				//Just the Directory
				Str = FPaths::GetCleanFilename(FilenameOrDirectory);
				if (Recursive) {
					FilenamesOut.Push(FilenameOrDirectory); //need whole path for recursive
				} else {
					FilenamesOut.Push(Str);
				}
			}
		}
		return true;
	});

	if (Recursive) {
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FullPathOfBaseDir, FilenamesVisitor);
	} else {
		return FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*FullPathOfBaseDir, FilenamesVisitor);
	}
}

void UFileIOBPLib::GetAllScriptClassFolders(TArray<FString>& Folders, FString StartsWith)
{
	for (TObjectIterator<UClass> It; It; ++It) {
		FString Name = It->GetPathName();
		Name = Name.Replace(*FString("."), *FString("/"), ESearchCase::IgnoreCase);
		if (!It->IsNative())
			continue;

		FString Left;
		FString Right;
		FString G;
		if (Name.Split(StartsWith, &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromStart)) {
			const bool bCondition = Right.Split("/", &Left, &G, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			if (!bCondition && Right != "") {
				if (!Folders.Contains(Right)) {
					Folders.Add(Right);
				}
			} else if (bCondition) {
				if (!Folders.Contains(Left)) {
					Folders.Add(Left);
				}
			}
		}
	}
}

void UFileIOBPLib::GetAllScriptClasses(TArray<UClass*>& Classes)
{
	for (TObjectIterator<UClass> It; It; ++It) {
		if (!It->IsNative())
			continue;

		if (!Classes.Contains(*It)) {
			Classes.Add(*It);
		}
	}
}

void UFileIOBPLib::GetAllScriptClassesInPath(TArray<UClass*>& Classes, FString Path)
{
	TArray<FString> UniqueFolders;

	for (TObjectIterator<UClass> It; It; ++It) {
		if (!It->IsNative())
			continue;
		FString Left;
		FString Right;
		FString Name = It->GetPathName();
		if (Name.Split(Path, &Left, &Right)) {
			const bool bCondition = Right.Split(".", &Left, &Right);
			if (bCondition && !UniqueFolders.Contains(Right) && (Left == "")) {
				UniqueFolders.Add(Right);
				Classes.Add(*It);
			}
		}
	}
}

void UFileIOBPLib::GetDerivedClassesFiltered(UClass* ClassIn, TArray<UClass*>& DerivedClasses, TArray<UClass*> Filter, bool Recursive)
{
	GetDerivedClasses(ClassIn, DerivedClasses, Recursive);
	for (UClass* i : Filter) {
		DerivedClasses.Remove(i);
	}
}