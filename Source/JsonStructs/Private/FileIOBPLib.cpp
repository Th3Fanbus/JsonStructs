#include "FileIOBPLib.h"
#include "Algo/Reverse.h"

static bool GetFilePath(FString& OutFullPath, const FString& Path, bool Relative)
{
#if WITH_EDITOR
	OutFullPath = Relative ? (FPaths::ProjectDir() + Path) : Path;
	return true;
#else
	const FString AbsoluteRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString AbsolutePath = AbsoluteRootPath + TEXT("Mods/") + Path;
	if (AbsolutePath.Contains(TEXT(".."))) {
		UE_LOG(LogTemp, Error, TEXT("Absolute or escaping Paths are not allowed in Runtime"));
		return false;
	} else {
		OutFullPath = AbsolutePath;
		return true;
	}
#endif
}

void UFileIOBPLib::WriteStringToFile(FString Path, FString resultString, bool Relative)
{
	FString OutFullPath;
	if (GetFilePath(OutFullPath, Path, Relative)) {
		FFileHelper::SaveStringToFile(resultString, *OutFullPath);
	}
}

bool UFileIOBPLib::LoadStringFromFile(FString& String, FString Path, bool Relative)
{
	FString OutFullPath;
	if (GetFilePath(OutFullPath, Path, Relative)) {
		FFileHelper::LoadFileToString(String, *OutFullPath);
		return true;
	}
	return false;
}

bool UFileIOBPLib::GetDirectoriesInPath(const FString& FullPathOfBaseDir, TArray<FString>& DirsOut, const FString& NotContainsStr, bool Recursive, const FString& ContainsStr)
{
	FString Str;

	auto FilenamesVisitor = MakeDirectoryVisitor([&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (bIsDirectory) {
			Str = FPaths::GetCleanFilename(FilenameOrDirectory);

			bool ShouldAdd;

			//Using a Contains Filter?
			if (ContainsStr != "") {
				ShouldAdd = Str.Contains(ContainsStr); //Only if Directory Contains Str
			} else if (NotContainsStr != "") {
				ShouldAdd = !Str.Contains(NotContainsStr);
			} else {
				ShouldAdd = true; //Get ALL Directories!
			}

			if (ShouldAdd) {
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
			Str = FPaths::GetCleanFilename(FilenameOrDirectory);

			bool ShouldAdd;

			if (FileExt != "") {
				//Filter by Extension
				ShouldAdd = FPaths::GetExtension(Str).ToLower() == FileExt;
			} else {
				//Include All Filenames!
				ShouldAdd = true;
			}

			if (ShouldAdd) {
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