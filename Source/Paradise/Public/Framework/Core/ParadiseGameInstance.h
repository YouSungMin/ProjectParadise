// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"
#include "Data/Structs/StageStructs.h"
#include "ParadiseGameInstance.generated.h"

#pragma region 전방 선언
class ULoadingWidget; 
class UInventorySystem;
#pragma endregion 전방 선언

/**
 * @class UParadiseGameInstance
 * @brief 게임의 수명 주기 동안 유지되는 전역 데이터 및 시스템 관리 클래스.
 * @details 이제 로딩 시스템은 LevelLoadingSubsystem이 담당 (추후 세이브 관리는 그때 담당자가 추가할 것)
 */
UCLASS()
class PARADISE_API UParadiseGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UParadiseGameInstance();
	virtual void Init() override;

	virtual void Shutdown() override;

#pragma region 게임 데이터 저장 및 로드

public:

	//세이브 슬롯 이름 (기본: "SaveSlot_01")
	UPROPERTY(VisibleAnywhere, Category = "Basic")
	FString SaveGameSlotName;

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void SaveGameData();

	UFUNCTION(BlueprintCallable, Category = "SaveSystem")
	void LoadGameData();

	// 기본 슬롯 이름
	const FString DefaultSaveSlot = TEXT("SaveSlot_01");

#pragma endregion 게임 데이터 저장 및 로드

#pragma region 설정
protected:
	/**
	 * @brief 로딩 화면 위젯 클래스 (BP_LoadingWidget 할당).
	 * @details Init()에서 LevelLoadingSubsystem에 전달합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Paradise|UI")
	TSubclassOf<UUserWidget> LoadingWidgetClass;
#pragma endregion 설정


#pragma region 데이터 테이블

public:

	/**
	 * @brief 데이터 테이블을 조회하여 해당 플레이어 ID가 존재하는지 확인하는 헬퍼 함수
	 * @param PlayerID 검사할 플레이어의 데이터 테이블 Row Name
	 * @return 데이터가 존재하면 true, 없으면 false
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|DataValidation")
	bool IsValidPlayerID(FName PlayerID) const;

	/**
	 * @brief 데이터 테이블을 조회하여 해당 퍼밀리어 ID가 존재하는지 확인하는 함수
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|DataValidation")
	bool IsValidFamiliarID(FName FamiliarID) const;

	/**
	 * @brief 데이터 테이블을 조회하여 해당 아이템 ID가 존재하는지 확인하는 함수
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|DataValidation")
	bool IsValidItemID(FName ItemID) const;

	/**
	 * @brief 데이터 테이블을 조회하여 해당 적/유닛 ID가 존재하는지 확인하는 함수
	 */
	UFUNCTION(BlueprintPure, Category = "Paradise|DataValidation")
	bool IsValidUnitID(FName UnitID) const;

	/**
	 * @brief [템플릿] 특정 테이블에서 ID로 데이터를 찾아 해당 구조체로 반환하는 함수
	 * @tparam T : 찾고자 하는 구조체 타입 (예: FCharacterStats)
	 * @param Table : 검색할 데이터 테이블 포인터
	 * @param RowName : 찾을 ID
	 * @return 찾은 데이터 포인터 (없으면 nullptr)
	 */
	template <typename T>
	T* GetDataTableRow(UDataTable* Table, FName RowName) const
	{
		if (!Table)
		{
			UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] 테이블이 연결되지 않았습니다!"));
			return nullptr;
		}

		// 언리얼 내부 FindRow 사용 (ContextString은 에러 로그용)
		static const FString ContextString(TEXT("GameInstance_GetData"));
		return Table->FindRow<T>(RowName, ContextString);
	}

public:

	// RequiredAssetDataTags 으로 다른 타입 데이터 테이블 설정하는 위험 X
	//유닛 데이터테이블
	/*영웅(플레이어) 생성에 사용할 에셋 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Character", meta = (RowType = "CharacterAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.CharacterAssets"))
	TObjectPtr<class UDataTable> CharacterAssetsDataTable = nullptr;

	/*영웅(플레이어) 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Character", meta = (RowType = "CharacterStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.CharacterStats"))
	TObjectPtr<class UDataTable> CharacterStatsDataTable = nullptr;

	/*적 유닛 생성에 사용할 에셋 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Player", meta = (RowType = "EnemyAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.EnemyAssets"))
	TObjectPtr<class UDataTable> EnemyAssetsDataTable = nullptr;

	/*적 유닛 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Player", meta = (RowType = "EnemyStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.EnemyStats"))
	TObjectPtr<class UDataTable> EnemyStatsDataTable = nullptr;

	/*퍼밀리어 생성에 사용할 에셋 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Player", meta = (RowType = "FamiliarAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.FamiliarAssets"))
	TObjectPtr<class UDataTable> FamiliarAssetsDataTable = nullptr;

	/*퍼밀리어 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Player", meta = (RowType = "FamiliarStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.FamiliarStats"))
	TObjectPtr<class UDataTable> FamiliarStatsDataTable = nullptr;

	//아이템 , 장비 데이터테이블

	/*방어구 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "ArmorAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.ArmorAssets"))
	TObjectPtr<class UDataTable> ArmorAssetsDataTable = nullptr;

	/*방어구 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "ArmorStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.ArmorStats"))
	TObjectPtr<class UDataTable> ArmorStatsDataTable = nullptr;

	/*세트보너스에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "SetBonusAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.SetBonusAssets"))
	TObjectPtr<class UDataTable> SetBonusAssetsDataTable = nullptr;

	/*세트보너스에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "SetBonusStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.SetBonusStats"))
	TObjectPtr<class UDataTable>SetBonusStatsDataTable = nullptr;

	/*무기 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "WeaponAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.WeaponAssets"))
	TObjectPtr<class UDataTable> WeaponAssetsDataTable = nullptr;

	/*무기 생성에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "WeaponStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.WeaponStats"))
	TObjectPtr<class UDataTable> WeaponStatsDataTable = nullptr;

	/*스킬 데미지 적용에 사용할 스탯 데이터 테이블 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Equipment", meta = (RowType = "WeaponStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.ActionStats"))
	TObjectPtr<class UDataTable> ActionStatsDataTable = nullptr;

	//스테이지 데이터 테이블
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Stage", meta = (RowType = "StatgeStats", RequiredAssetDataTags = "RowStructure=/Script/Paradise.StatgeStats"))
	TObjectPtr<class UDataTable> StatgeStatsDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Stage", meta = (RowType = "StageAssets", RequiredAssetDataTags = "RowStructure=/Script/Paradise.StageAssets"))
	TObjectPtr<class UDataTable> StageAssetsDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Stage", meta = (RowType = "StageWaveDetail", RequiredAssetDataTags = "RowStructure=/Script/Paradise.StageWaveDetail"))
	TObjectPtr<class UDataTable> StageWaveDetailDataTable = nullptr;

public:

#pragma region 인벤토리 

	UFUNCTION(BlueprintCallable, Category = "System")
	UInventorySystem* GetMainInventory() const;

#pragma endregion 인벤토리

private:

#pragma endregion 데이터 테이블



};
