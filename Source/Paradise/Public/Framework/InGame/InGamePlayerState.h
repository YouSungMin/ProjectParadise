// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "InGamePlayerState.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;
class APlayerData;
class UInventorySystem;
class UCostManageComponent;
class UFamiliarSummonComponent;

/**
 * @class AInGamePlayerState
 * @brief 게임 세션 동안의 지휘관(Player) 상태 및 스쿼드 데이터를 관리하는 클래스
 * @details
 * - **스쿼드 관리:** 전투에 참여하는 영웅(PlayerData)들의 생명주기를 관리합니다.
 * - **자원 관리:** 전역 자원(코스트, 골드)을 GAS(Gameplay Ability System)를 통해 처리합니다.
 * - **데이터 연동:** 전역 인벤토리 서브시스템과 통신하여 초기 스쿼드를 구성합니다.
 */
UCLASS()
class PARADISE_API AInGamePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AInGamePlayerState();

	virtual void BeginPlay() override;

	//  GAS 인터페이스 (지휘관 전용: 코스트/골드 관리)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
	UAttributeSet* GetAttributeSet() const { return CommanderAttributeSet; }

	//  스쿼드 관리 (Squad Management)
public:
	/*
	 * @brief 게임 시작 시 스쿼드 데이터를 생성하는 함수
	 * @param StartingHeroIDs : 로비나 저장 데이터에서 넘어온 영웅들의 ID 목록 (예: "Arthur", "Merlin")
	 */
	void InitSquad(const TArray<FName>& StartingHeroIDs);

	/**
	 * @brief 게임 인스턴스에 상주하는 전역 인벤토리 시스템을 반환합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventorySystem* GetInventorySystem() const;

	/*
	 * @brief 인덱스에 해당하는 영웅 데이터(영혼) 반환
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	APlayerData* GetSquadMemberData(int32 Index) const;

	/*
	 * @brief 현재 스쿼드 멤버 수 반환
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	int32 GetSquadSize() const { return SquadMembers.Num(); }


	/** @brief 코스트 관리 컴포넌트 접근자 (Getter) */ 
	UFUNCTION(BlueprintCallable, Category = "Economy")
	UCostManageComponent* GetCostManageComponent() const { return CostManageComponent; }

	/** @brief 퍼밀리어 소환 컴포넌트 접근자 (Getter)*/
	UFUNCTION(BlueprintCallable, Category = "Summon")
	UFamiliarSummonComponent* GetFamiliarSummonComponent() const { return FamiliarSummonComponent; }
protected:
	/*
	 * @brief 실제 스쿼드 멤버들 (영혼/데이터)
	 * @details 육체(Pawn)가 죽거나 교체되어도 이 데이터는 유지됩니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TArray<TObjectPtr<APlayerData>> SquadMembers;

	//  GAS 컴포넌트 (Commander Resources)
	/* 지휘관용 ASC (코스트/쿨타임/패시브 효과 관리용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	/** @brief 코스트 관리 컴포넌트 (UI용 Getter 제공) */ 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCostManageComponent> CostManageComponent = nullptr;

	/** @brief 퍼밀리어 소환 컴포넌트 (UI용 Getter 제공) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFamiliarSummonComponent> FamiliarSummonComponent;

	/* 지휘관용 어트리뷰트 (Cost, MaxCost, Gold, RegenRate 등) */
	UPROPERTY()
	TObjectPtr<UAttributeSet> CommanderAttributeSet = nullptr;

	/** * @brief 스폰할 영혼(PlayerData)의 클래스 (BP_PlayerData 할당용)
	 * @details 이 값이 비어있으면 기본 C++ 클래스로 스폰됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Config")
	TSubclassOf<APlayerData> PlayerDataClass = nullptr;

};