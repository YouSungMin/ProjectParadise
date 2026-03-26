// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ParadiseAudioConfigData.generated.h"

#pragma region 전방 선언
class USoundMix;
class USoundClass;
#pragma endregion 전방 선언

/**
 * @class UParadiseAudioConfigData
 * @brief 볼륨 제어에 필요한 사운드 믹스 및 사운드 클래스 레퍼런스를 중앙 관리하는 데이터 에셋입니다.
 * @details 기획자가 에디터에서 이 에셋 하나만 수정하여 볼륨 제어 구조를 변경할 수 있습니다.
 */
UCLASS()
class PARADISE_API UParadiseAudioConfigData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    /**
     * @brief 볼륨 제어의 기준이 되는 마스터 사운드 믹스.
     * @details SM_Master 할당 필수.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Config")
    TObjectPtr<USoundMix> MasterSoundMix = nullptr;

    /**
     * @brief BGM 볼륨을 일괄 제어할 사운드 클래스.
     * @details SC_BGM 할당 필수.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Config")
    TObjectPtr<USoundClass> BGMSoundClass = nullptr;

    /**
     * @brief SFX 볼륨을 일괄 제어할 사운드 클래스.
     * @details SC_SFX 할당 필수.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Audio|Config")
    TObjectPtr<USoundClass> SFXSoundClass = nullptr;
};
