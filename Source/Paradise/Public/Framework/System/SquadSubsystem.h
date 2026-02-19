#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SquadSubsystem.generated.h"

// 3개의 슬롯 중 특정 슬롯의 영웅이 변경되었을 때 UI에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroSlotChangedSignature, int32, SlotIndex, FName, NewHeroID);

/**
 * @class USquadSubsystem
 * @brief 로비 편성 UI와 인게임(3인 영웅 스폰 및 스위칭) 사이의 데이터를 관리하는 서브시스템
 */
UCLASS()
class PARADISE_API USquadSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//UI 갱신용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Squad|Event")
	FOnHeroSlotChangedSignature OnHeroSlotChanged;

	/**
	 * @brief 로비 편성 UI에서 특정 슬롯(0, 1, 2)에 영웅을 배치할 때 호출합니다.
	 * @param SlotIndex 배치할 슬롯 번호 (0 ~ 2)
	 * @param NewHeroID 새로 배치할 영웅 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void SetHeroToSlot(int32 SlotIndex, FName NewHeroID);

	/**
	 * @brief 특정 슬롯에 어떤 영웅이 있는지 확인 (UI 표기용)
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	FName GetHeroAtSlot(int32 SlotIndex) const;

	/**
	 * @brief 인게임 진입 시 GameMode에서 3명의 영웅을 한 번에 스폰하기 위해 배열 전체를 가져옵니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	const TArray<FName>& GetHeroSquad() const;

	/**
	 * @brief 중복 편성 방지 (이미 스쿼드에 있는 영웅인지 체크)
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsHeroAlreadyAssigned(FName HeroID) const;


	// [Save / Load] 세이브 연동
	UFUNCTION(BlueprintCallable, Category = "Squad|Save")
	void SaveSquadData();

private:
	// 3명의 영웅 ID를 담을 스쿼드 배열 (크기 3으로 고정 관리)
	UPROPERTY()
	TArray<FName> SelectedHeroSquadIDs;
};