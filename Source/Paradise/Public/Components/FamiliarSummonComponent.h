// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Structs/UnitStructs.h"
#include "FamiliarSummonComponent.generated.h"

class UCostManageComponent;
class UObjectPoolSubsystem;
class AFamiliarUnit;
class AFamiliarSpawner;
class USquadSubsystem;	// [추가] 02/25 담당자:최지원
class UTexture2D;

USTRUCT(BlueprintType)
struct FSummonSlotInfo
{
	GENERATED_BODY()

	/** @brief 데이터 테이블 행 이름 (유닛 ID) */
	UPROPERTY(BlueprintReadOnly, Category = "Summon")
	FName FamiliarID = NAME_None;

	/** @brief 소환에 책정된 가격 */
	UPROPERTY(BlueprintReadOnly, Category = "Summon")
	int32 FamiliarCost = 0;

	/** @brief UI 아이콘 (Assets 테이블에서 로드해서 UI에 전달) */
	UPROPERTY(BlueprintReadOnly, Category = "Summon")
	TSoftObjectPtr<UTexture2D> FamiliarIcon = nullptr; // FUnitBaseAssets에 Icon이 있다고 가정하거나 추가 필요

	/** @brief 빈 슬롯인지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Summon")
	bool bIsSoldOut = false;
};

/** @brief 슬롯 정보가 갱신되었을 때 UI에 알리는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSummonSlotsUpdated, const TArray<FSummonSlotInfo>&, Slots);

/**
 * @class UFamiliarSummonComponent
 * @brief 유닛 소환 시스템을 관리하는 컴포넌트
 * @details
 * - 5개의 랜덤 유닛 슬롯을 관리.
 * - CostManageComponent와 연동하여 유닛 소환 시 코스트 차감 및 잔액 확인.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARADISE_API UFamiliarSummonComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFamiliarSummonComponent();

protected:
	virtual void BeginPlay() override;

public:	
	/**
	* @brief 슬롯 데이터를 초기화하고 5개를 랜덤으로 채우는 함수(게임 시작 시)
	*/
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void RefreshAllSlots();

	/**
	* @brief 특정 슬롯 구매 요청 함수
	* @param SlotIndex 소환할 슬롯의 인덱스 (0~4)
	* @return 소환 성공 여부(돈 부족, 슬롯 비어있음 등 실패시 false)
	*/
	UFUNCTION(BlueprintCallable, Category = "Summon")
	bool RequestPurchase(int32 SlotIndex);

	/** @brief 현재 슬롯 갱신 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Summon")
	FOnSummonSlotsUpdated OnSummonSlotsUpdated;

public:
	//스포너 등록 함수
	void RegisterSpawner(AFamiliarSpawner* NewSpawner);

protected:
	/** * @brief 소환 성공 후 품절 처리하고 쿨타임 타이머 함수
	 * @param SlotIndex 비워야 할 슬롯 번호
	 */
	UFUNCTION()
	void ConsumeSpecificSlot(int32 SlotIndex);

	///** @brief 랜덤 유닛을 하나 생성하여 슬롯 정보를 반환함 
	//* @return 생성된 슬롯 정보 구조체 -> RefreshAllSlots함수에서 반복문으로 배열 생성
	//*/
	//FSummonSlotInfo GenerateRandomSlot(UDataTable* StatsTable, UDataTable* AssetsTable);

	/** * @brief SquadSubsystem에서 편성 정보를 가져와 데이터 테이블 조회를 선행하여 캐싱합니다.
	 * @details 인게임 프레임 드랍을 막기 위한 필수 최적화 과정입니다.
	 */
	void InitializeDeckPool();

	/** * @brief 메모리에 캐싱된 덱(Deck) 풀에서 무작위로 하나의 카드를 즉시 뽑아옵니다. (O(1))
	 * @return 완성된 슬롯 정보 구조체
	 */
	FSummonSlotInfo DrawRandomCardFromPool();

protected:
	/** @brief 현재 관리 중인 소환 슬롯들 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	TArray<FSummonSlotInfo> CurrentSlots;

	/** @brief 데이터 테이블 조회를 마치고 캐싱 완료된 5마리 유닛 덱(Deck) 풀 */
	UPROPERTY(VisibleAnywhere, Category = "Paradise|Deck")
	TArray<FSummonSlotInfo> CachedDeckPool;

	/** @brief 슬롯 자동 갱신 쿨타임 (초 단위) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float RefillCooldownTime = 3.0f;

	/** @brief 슬롯별 리필 타이머 핸들 관리(5개)*/
	FTimerHandle RefillTimers[5];

	/** @brief 슬롯 개수 (기본 5개) */
	const int32 MaxSlotCount = 5;

	//스포너를 저장할 포인터
	UPROPERTY()
	TObjectPtr<AFamiliarSpawner> LinkedSpawner = nullptr;
};
