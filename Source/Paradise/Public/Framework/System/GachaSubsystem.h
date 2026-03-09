// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/ParadiseSaveInterface.h"
#include "Data/Structs/GachaTypes.h"
#include "GachaSubsystem.generated.h"

#pragma region 전방 선언
class UDataTable;
#pragma endregion 전방 선언

/**
 * @class UGachaSubsystem
 * @brief 가챠 확률 계산 및 천장(Pity), 중복 조각 변환 로직을 전담하는 백엔드 코어
 * @details 캐싱을 통한 최적화로 100연차를 돌려도 프레임 드랍이 발생하지 않습니다.
 */
UCLASS()
class PARADISE_API UGachaSubsystem : public UGameInstanceSubsystem ,public IParadiseSaveInterface
{
	GENERATED_BODY()

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 새로운 배너의 마스터 데이터를 등록하고 확률 풀을 캐싱합니다.
	 * @param InBannerData 기획자가 세팅한 배너 데이터 구조체 (타입, 확률, 풀 테이블 포함)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	void InitializeBanner(const FGachaBannerData& InBannerData);

	/**
	 * @brief 지정된 횟수만큼 가챠를 돌리고 상세 결과를 반환합니다.
	 * @param PullCount 뽑기 횟수 (1연차, 10연차 등)
	 * @param OwnedItems 유저가 현재 보유 중인 캐릭터 ID 목록 (장비 배너일 경우 빈 배열 전달 무방)
	 * @return TArray<FGachaResult> UI 연출 및 인벤토리 처리에 필요한 결과 리스트
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	TArray<FGachaResult> PerformGacha(int32 PullCount, const TArray<FName>& OwnedItems);

	/** @brief 현재 활성화된 배너의 1회 뽑기 에테르 요구량을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Paradise|Gacha")
	int32 GetCurrentAetherRequirement() const { return CachedRequiredAether; }

	/** @brief 현재 쌓인 천장 스택을 확인합니다. (UI 표기용) */
	UFUNCTION(BlueprintPure, Category = "Paradise|Gacha")
	int32 GetCurrentPityStack() const { return CurrentPityStack; }
#pragma endregion 외부 인터페이스

#pragma region 내부 로직
private:
	/** @brief 확률에 따라 등급을 1차 추첨합니다. */
	EItemRarity RollRarity(const TMap<EItemRarity, float>& Rates) const;

	/** @brief 캐싱된 풀에서 가중치를 기반으로 아이템을 최종 추첨합니다. */
	FGachaResult PickItemFromRarity(EItemRarity TargetRarity) const;
#pragma endregion 내부 로직


//0309 김성현 - 가챠 스택 저장 
#pragma region 게임 데이터 저장 및 로드

public:
	UFUNCTION(BlueprintCallable, Category = "Gacha|Save")
	virtual void SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const override;

	UFUNCTION(BlueprintCallable, Category = "Gacha|Save")
	virtual void LoadFromSaveGame(class UParadiseSaveGame* SaveGameObj)override;

#pragma endregion 게임 데이터 저장 및 로드

#pragma region 내부 데이터
private:
	/** @brief 현재 활성화된 배너의 종류 (캐릭터 vs 장비 분기용) */
	EGachaBannerType CurrentBannerType = EGachaBannerType::Character;

	/** @brief 현재 활성화된 배너의 등급별 확률표 */
	TMap<EItemRarity, float> CurrentRarityRates;

	/** @brief [최적화] 등급별로 O(1) 접근이 가능하도록 쪼개어 캐싱된 데이터 풀 */
	TMap<EItemRarity, TArray<FGachaPoolRow>> CachedGachaPool;

	/** @brief 누적된 천장(Pity) 스택 (전설 획득 시 0으로 리셋) */
	UPROPERTY()
	int32 CurrentPityStack = 0;

	/** @brief 현재 활성화된 배너의 1회 뽑기 에테르 비용 (결제용) */
	int32 CachedRequiredAether = 0;

	UPROPERTY(EditAnywhere, Category = "Gacha|Config", meta = (ClampMin = "1"))
	int32 DefaultPityThreshold = 50;
#pragma endregion 내부 데이터
};