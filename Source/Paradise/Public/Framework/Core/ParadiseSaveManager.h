// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ParadiseSaveManager.generated.h"

/**
 * 
 */
UCLASS()
class PARADISE_API UParadiseSaveManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	/** * @brief USaveGame 객체를 메모리로 변환한 뒤 AES 암호화 + 해시를 적용하여 디스크에 저장합니다.
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Save")
	static bool SaveGameEncrypted(USaveGame* SaveGameObject, const FString& SlotName);

	/** * @brief 디스크에서 파일을 읽어 위변조 검사 및 복호화를 거친 후 USaveGame 객체로 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Save")
	static class USaveGame* LoadGameEncrypted(const FString& SlotName);
};
