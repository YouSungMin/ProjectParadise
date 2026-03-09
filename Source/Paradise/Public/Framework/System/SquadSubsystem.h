#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/ParadiseSaveInterface.h"
#include "SquadSubsystem.generated.h"

// 3개의 슬롯 중 특정 슬롯의 플레이어가 변경되었을 때 UI에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerSlotChangedSignature, int32, SlotIndex, FName, NewPlayerID);
// 5개의 슬롯 중 특정 슬롯의 퍼밀리어가 변경되었을 때 UI에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFamiliarSlotChangedSignature, int32, SlotIndex, FName, NewFamiliarID);
/**
 * @class USquadSubsystem
 * @brief 로비 편성 UI와 인게임(3인 플레이어 스폰 및 스위칭) 사이의 데이터를 관리하는 서브시스템
 */
UCLASS()
class PARADISE_API USquadSubsystem : public UGameInstanceSubsystem , public IParadiseSaveInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


#pragma region 퍼밀리어 편성

	// 퍼밀리어 UI 갱신용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Squad|Event")
	FOnFamiliarSlotChangedSignature OnFamiliarSlotChanged;

	/**
	 * @brief 로비 편성 UI에서 특정 슬롯(0 ~ 4)에 퍼밀리어를 배치할 때 호출합니다.
	 * @param SlotIndex 배치할 슬롯 번호 (0 ~ 4)
	 * @param NewFamiliarID 새로 배치할 퍼밀리어 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Familiar")
	void SetFamiliarToSlot(int32 SlotIndex, FName NewFamiliarID);

	/**
	 * @brief 특정 슬롯에 어떤 퍼밀리어가 있는지 확인 (UI 표기용)
	 */
	UFUNCTION(BlueprintPure, Category = "Squad|Familiar")
	FName GetFamiliarAtSlot(int32 SlotIndex) const;

	/**
	 * @brief 인게임 진입 시 사용될 5마리의 퍼밀리어 ID 배열 반환
	 */
	UFUNCTION(BlueprintPure, Category = "Squad|Familiar")
	const TArray<FName>& GetFamiliarSquad() const;

	/**
	 * @brief 퍼밀리어 중복 편성 방지 체크
	 */
	UFUNCTION(BlueprintPure, Category = "Squad|Familiar")
	bool IsFamiliarAlreadyAssigned(FName FamiliarID) const;

private:
	// 5마리의 퍼밀리어 ID를 담을 스쿼드 배열 (크기 5 고정)
	UPROPERTY()
	TArray<FName> SelectedFamiliarSquadIDs;


#pragma endregion 퍼밀리어 편성


#pragma region 플레이어 캐릭터 편성
public:
	//플레이어 UI 갱신용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Squad|Event")
	FOnPlayerSlotChangedSignature OnPlayerSlotChanged;

	/**
	 * @brief 로비 편성 UI에서 특정 슬롯(0, 1, 2)에 플레이어를 배치할 때 호출합니다.
	 * @param SlotIndex 배치할 슬롯 번호 (0 ~ 2)
	 * @param NewPlayerID 새로 배치할 플레이어 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void SetPlayerToSlot(int32 SlotIndex, FName NewPlayerID);

	/**
	 * @brief 특정 슬롯에 어떤 플레이어가 있는지 확인 (UI 표기용)
	 * @param SlotIndex 확인할 슬롯 번호 (0 ~ 2)
	 * @return 해당 슬롯에 배치된 플레이어 ID
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	FName GetPlayerAtSlot(int32 SlotIndex) const;

	/**
	 * @brief 인게임 진입 시 GameMode에서 3명의 플레이어를 한 번에 스폰하기 위해 배열 전체를 가져옵니다.
	 * @return 편성된 3인의 플레이어 ID 배열
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	const TArray<FName>& GetPlayerSquad() const;

	/**
	 * @brief 중복 편성 방지 (이미 스쿼드에 있는 플레이어인지 체크)
	 * @param PlayerID 중복 여부를 확인할 플레이어 ID
	 * @return 이미 다른 슬롯에 편성되어 있다면 true 반환
	 */
	UFUNCTION(BlueprintPure, Category = "Squad")
	bool IsPlayerAlreadyAssigned(FName PlayerID) const;

private:
	// 3명의 플레이어 ID를 담을 스쿼드 배열
	UPROPERTY()
	TArray<FName> SelectedPlayerSquadIDs;

#pragma endregion 플레이어 캐릭터 편성

public:


#pragma region 세이브 데이터 

public:
	/**
	 * @brief 세이브 게임 객체에서 편성 데이터를 읽어와 복구합니다. (게임 실행 시 1회 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Save")
	virtual void LoadFromSaveGame(class UParadiseSaveGame* SaveGameObj) override;


	/**
	 * @brief 현재 서브시스템의 편성 상태를 세이브 게임 객체에 기록합니다. (게임 저장 시 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad|Save")
	virtual void SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const override;

#pragma endregion 세이브 데이터 
};