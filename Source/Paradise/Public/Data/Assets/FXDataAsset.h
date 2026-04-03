// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/Structs/FXStructs.h"
#include "FXDataAsset.generated.h"

/**
 * @class UParadiseFXDataAsset
 * @brief GameplayTag를 키(Key)로 사용하여 이펙트 세트를 검색하는 저장소
 */
UCLASS()
class PARADISE_API UFXDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    // 태그 : 이펙트 매핑
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data", meta = (Categories = "FX"))
    TMap<FGameplayTag, FFXPayload> EffectMap;

    // 검색 함수
    FFXPayload* FindEffect(const FGameplayTag& Tag)
    {
        return EffectMap.Find(Tag);
    }

    UFUNCTION(BlueprintCallable, Category = "Editor Scripting")
    void ClearEffectMap()
    {
        EffectMap.Empty();
    }

    UFUNCTION(BlueprintCallable, Category = "Editor Scripting")
    void AddTagToEffectMap(FString TagString)
    {
        //문자열을 GameplayTag로 변환
        FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);

        //태그가 유효하다면 Map에 빈 구조체와 함께 추가
        if (NewTag.IsValid())
        {
            EffectMap.Add(NewTag, FFXPayload());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ 등록되지 않은 태그입니다: %s"), *TagString);
        }
    }

};

