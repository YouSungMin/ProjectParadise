// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StageSubsystem.generated.h"

/**
 * @class UStageSubsystem
 * @brief 스테이지 진행도(해금, 별점) 및 레벨 이동 간의 데이터 전달을 중앙에서 관리하는 게임 인스턴스 서브시스템입니다.
 * * @details
 * GameInstance의 수명주기를 따르므로 맵(레벨)이 전환되어도 데이터가 소멸하지 않고 안전하게 유지됩니다.
 * 다음과 같은 3가지 핵심 역할을 수행합니다:
 * * 1. [진행도 관리] : 유저가 입장할 수 있는 스테이지 목록과 각 스테이지의 최고 클리어 랭크(별점)를 관리합니다.
 * 2. [데이터 브릿지] : 로비(UI)에서 유저가 선택한 타겟 스테이지 ID를 임시로 기억하고, 인게임 로딩 완료 후 GameMode에게 전달합니다.
 * 3. [세이브/로드] : 메모리에 유지되던 진행도 데이터를 `UParadiseSaveGame` 객체와 연동하여 영구적으로 보존 및 복구합니다.
 */
UCLASS()
class PARADISE_API UStageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** @brief 특정 스테이지가 해금(입장 가능) 상태인지 확인합니다. */
	UFUNCTION(BlueprintPure, Category = "Stage|Progress")
	bool IsStageUnlocked(FName StageID) const;

	/** @brief 새로운 스테이지를 해금 목록에 추가합니다. */
	UFUNCTION(BlueprintCallable, Category = "Stage|Progress")
	void UnlockStage(FName StageID);

	/** @brief 특정 스테이지의 클리어 별(랭크)을 가져옵니다. (클리어 안했으면 0 반환) */
	UFUNCTION(BlueprintPure, Category = "Stage|Progress")
	int32 GetStageClearStar(FName StageID) const;

	/** @brief 스테이지 클리어 별을 기록합니다. (기존 기록보다 높을 때만 갱신됨) */
	UFUNCTION(BlueprintCallable, Category = "Stage|Progress")
	void RecordStageClearStar(FName StageID, int32 StarCount);

	/** @brief 로비에서 선택한 스테이지 ID를 저장합니다. */
	UFUNCTION(BlueprintCallable, Category = "Stage|Flow")
	void SetSelectedStageID(FName InStageID) { SelectedStageID = InStageID; }

	/** @brief 인게임 진입 시 현재 플레이할 스테이지 ID를 꺼내옵니다. */
	UFUNCTION(BlueprintPure, Category = "Stage|Flow")
	FName GetSelectedStageID() const { return SelectedStageID; }

	// 세이브 / 로드 연동
	UFUNCTION(BlueprintCallable, Category = "Stage|Save")
	void LoadFromSaveGame(class UParadiseSaveGame* SaveGameObj);

	UFUNCTION(BlueprintCallable, Category = "Stage|Save")
	void SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const;

private:

	/** @brief 인게임으로 넘겨줄 타겟 스테이지 ID */
	UPROPERTY()
	FName SelectedStageID = NAME_None;

	/** @brief 해금된 스테이지 ID 목록 */
	UPROPERTY()
	TArray<FName> UnlockedStages;

	/** @brief 스테이지별 최고 클리어 별 갯수 (Key: 스테이지ID, Value: 별 갯수) */
	UPROPERTY()
	TMap<FName, int32> StageClearStars;

};
