// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ParadiseSaveInterface.generated.h"

class UParadiseSaveGame;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UParadiseSaveInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PARADISE_API IParadiseSaveInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// 공통으로 사용할 세이브/로드 함수 선언
	virtual void SaveToSaveGame(UParadiseSaveGame* SaveObject) const = 0;
	virtual void LoadFromSaveGame(UParadiseSaveGame* SaveObject) = 0;
};
