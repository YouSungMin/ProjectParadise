// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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
class PARADISE_API UGachaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region 외부 인터페이스
public:
	/**
	 * @brief 새로운 배너(데이터 테이블)를 등록하고 등급별로 캐싱합니다.
	 * @param BannerTable 기획자가 세팅한 가챠 풀 테이블
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	void InitializeBanner(UDataTable* BannerTable);

	/**
	 * @brief 지정된 횟수만큼 가챠를 돌리고 상세 결과를 반환합니다.
	 * @param PullCount 뽑기 횟수 (1연차, 10연차 등)
	 * @param RarityRates 등급별 기본 확률표
	 * @param OwnedItems 유저가 현재 보유 중인 캐릭터 ID 목록 (중복 검사용)
	 * @return TArray<FGachaResult> UI 연출 및 인벤토리 처리에 필요한 완벽한 결과 리스트
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha")
	TArray<FGachaResult> PerformGacha(
		int32 PullCount, 
		const TMap<EItemRarity, float>& RarityRates, 
		const TArray<FName>& OwnedItems);

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

#pragma region 내부 데이터
private:
	/** @brief [최적화] 등급별로 O(1) 접근이 가능하도록 쪼개어 캐싱된 데이터 풀 */
	TMap<EItemRarity, TArray<FGachaPoolRow>> CachedGachaPool;

	/** @brief 현재 적용된 배너 테이블 (스마트 포인터 안전 초기화) */
	UPROPERTY()
	TObjectPtr<UDataTable> CurrentBannerTable = nullptr;

	/** @brief 누적된 천장(Pity) 스택 (전설 획득 시 0으로 리셋) */
	UPROPERTY()
	int32 CurrentPityStack = 0;

	UPROPERTY(EditAnywhere, Category = "Gacha|Config", meta = (ClampMin = "1"))
	int32 DefaultPityThreshold = 50;
#pragma endregion 내부 데이터
};