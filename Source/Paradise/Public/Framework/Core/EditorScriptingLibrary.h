#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EditorScriptingLibrary.generated.h"

UCLASS()
class PARADISE_API UEditorScriptingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:


	UFUNCTION(BlueprintCallable, Category = "PythonBypass")
	static void AddSocketToSkeleton(class USkeleton* TargetSkeleton, FName SocketName, FName BoneName, FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale);
};