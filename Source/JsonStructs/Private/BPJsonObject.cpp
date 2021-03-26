#include "BPJsonObject.h"

UBPJsonObject * UBPJsonObject::GetStringAsJson(UPARAM(ref)FString & String, UObject * Outer)
{
	if (String == "")
		return nullptr;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(*String);
	FJsonSerializer Serializer;
	TSharedPtr<FJsonObject> result;
	Serializer.Deserialize(reader, result);

	UBPJsonObject * Obj = NewObject<UBPJsonObject>(Outer);
	if(!Obj)
		return nullptr;
	else
	{
		Obj->InnerObj = result;
		return Obj;
	}
}

FString UBPJsonObject::GetAsString()
{
	if (!InnerObj)
		return "";
	FString write;
	TSharedRef<TJsonWriter<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>> JsonWriter = TJsonWriterFactory<wchar_t, TPrettyJsonPrintPolicy<wchar_t>>::Create(&write); //Our Writer Factory
	FJsonSerializer::Serialize(InnerObj.ToSharedRef(), JsonWriter);
	return write;
}
