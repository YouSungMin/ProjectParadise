// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Data/Enums/GameEnums.h"
#include "MasterCueNotifyStatic.generated.h"

/**
 * @class UGC_UnitHit
 * @brief 모든 유닛(플레이어, 몬스터)의 피격 연출을 담당하는 단일 단발성(Static) 게임플레이 큐
 * @details ICombatInterface를 통해 타겟의 데이터를 읽어와 동적으로 이펙트를 재생합니다.
 */
UCLASS()
class PARADISE_API UMasterCueNotifyStatic : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UMasterCueNotifyStatic();

	/**
	 * @brief 큐가 실행될 때(즉, 데미지가 들어갈 때) 호출되는 핵심 함수
	 */
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
public:
	// 💡 언리얼 에디터 디테일 패널에서 이 큐가 어떤 이벤트를 요청할지 선택하게 합니다.
	UPROPERTY(EditDefaultsOnly, Category = "FX Setup")
	EFXEventType TargetEventType = EFXEventType::Hit;
};
