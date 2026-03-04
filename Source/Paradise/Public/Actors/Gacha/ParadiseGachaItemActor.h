// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Structs/GachaTypes.h"
#include "Data/Enums/GameEnums.h"
#include "ParadiseGachaItemActor.generated.h"

#pragma region 전방 선언
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInstance;
class USoundBase;
#pragma endregion 전방 선언

/** @brief 아이템이 터치되어 실루엣이 해제되었을 때 상자나 UI에 알리는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGachaItemRevealed, const FGachaResult&, ItemResult);

/**
 * @class AParadiseGachaItemActor
 * @brief 오버워치 스타일 가챠 연출에서 튀어나오는 개별 아이템 액터
 * @details 생성 시점에는 실루엣 머티리얼로 가려져 있으며, 터치 시 원본으로 변경됩니다. (SRP 적용)
 */
UCLASS()
class PARADISE_API AParadiseGachaItemActor : public AActor
{
	GENERATED_BODY()

public:
	AParadiseGachaItemActor();

#pragma region 생명주기
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
#pragma endregion 생명주기

#pragma region 외부 인터페이스 (주입식 설계)
public:
	/**
	 * @brief 아이템의 데이터와 시각적 에셋을 외부에서 주입하여 초기화합니다.
	 * @param InResult 이 아이템이 담고 있는 가챠 결과 데이터
	 * @param InSilhouetteMat 덮어씌울 실루엣(검은색) 머티리얼 인스턴스
	 * @param InRealMat 터치 시 보여줄 진짜 캐릭터/아이템 머티리얼 인스턴스
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha|Item")
	void InitializeItemData(const FGachaResult& InResult, UMaterialInstance* InSilhouetteMat, UMaterialInstance* InRealMat);

	/**
	 * @brief 지정된 목표 위치로 포물선을 그리며 날아갑니다.
	 * @param TargetLocation 바닥에 안착할 최종 목표 좌표
	 * @param FlightDuration 날아가는 데 걸리는 시간 (초)
	 * @param ArcHeight 포물선의 최고점 높이 (언리얼 유닛)
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha|Item")
	void LaunchToTarget(FVector TargetLocation, float FlightDuration = 1.0f, float ArcHeight = 300.0f);

	/**
	 * @brief 유저가 터치했을 때 호출하여 실루엣을 벗기고 진짜 모습을 보여줍니다.
	 * @details 멀티 터치 환경이나 스킵 상황에서 호출될 수 있습니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Paradise|Gacha|Item")
	void RevealItem();
#pragma endregion 외부 인터페이스 (주입식 설계)

#pragma region 내부 이벤트 핸들러 (Delegate Wrapper)
private:
	// 언리얼 엔진의 OnClicked 델리게이트 시그니처에 맞춘 래퍼 함수
	UFUNCTION()
	void HandleItemClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

	// 언리얼 엔진의 OnInputTouchEnd 델리게이트 시그니처에 맞춘 래퍼 함수
	UFUNCTION()
	void HandleItemTouchEnd(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);
#pragma endregion 내부 이벤트 핸들러 (Delegate Wrapper)

#pragma region 컴포넌트
protected:
	/** @brief 아이템 외형 (충돌 및 터치 감지용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<UStaticMeshComponent> ItemMesh = nullptr;

	/** @brief 등급별 아우라/빛기둥 이펙트를 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Paradise|Components")
	TObjectPtr<UNiagaraComponent> RarityAuraEffect = nullptr;
#pragma endregion 컴포넌트

#pragma region 시각 효과 데이터 
protected:
	// 단일 이펙트가 아니라, 등급(Rarity)별로 다른 파티클을 맵핑합니다!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Visuals")
	TMap<EItemRarity, TObjectPtr<UNiagaraSystem>> RevealVFXByRarity;

	/** @brief 터치 시 재생될 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|Visuals")
	TObjectPtr<USoundBase> RevealSound = nullptr;
#pragma endregion 시각 효과 데이터 

#pragma region 내부 상태 및 캐싱
private:
	/** @brief 현재 아이템에 할당된 가챠 결과 데이터 */
	UPROPERTY(VisibleAnywhere, Category = "Paradise|State")
	FGachaResult CachedItemData;

	/** @brief 리빌 시 교체할 원본 머티리얼 캐싱 */
	UPROPERTY()
	TObjectPtr<UMaterialInstance> CachedRealMaterial = nullptr;

	/** @brief 현재 액터의 상태기계 */
	EGachaItemState CurrentState = EGachaItemState::Flying;

	// --- 비행 수학 계산용 변수 (Tick 최적화) ---
	FVector StartLoc = FVector::ZeroVector;
	FVector EndLoc = FVector::ZeroVector;
	float TotalFlightTime = 1.0f;
	float CurrentFlightTime = 0.0f;
	float MaxArcHeight = 300.0f;
#pragma endregion 내부 상태 및 캐싱

#pragma region 델리게이트
public:
	/** @brief 리빌(연출)이 완료되었음을 알리는 이벤트 방송 */
	UPROPERTY(BlueprintAssignable, Category = "Paradise|Events")
	FOnGachaItemRevealed OnItemRevealed;
#pragma endregion 델리게이트
};
