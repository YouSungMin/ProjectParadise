// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/StageSubsystem.h"
#include "Framework/System/ParadiseSaveGame.h"
#include "Paradise/Paradise.h"

void UStageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UnlockStage(FName("Stage1_1"));

	UE_LOG(LogStage, Log, TEXT("🗺️ [StageSubsystem] 초기화 완료 및 기본 스테이지(1-1) 해금 적용"));
}

bool UStageSubsystem::IsStageUnlocked(FName StageID) const
{
	return UnlockedStages.Contains(StageID);
}

void UStageSubsystem::UnlockStage(FName StageID)
{
	// 빈칸이면 무시
	if (StageID.IsNone()) return;

	// 배열에 없을 때만 추가 (중복 추가 방지)
	if (!UnlockedStages.Contains(StageID))
	{
		UnlockedStages.Add(StageID);
		UE_LOG(LogStage, Warning, TEXT("🗺️ [Stage] 새로운 스테이지 해금: %s"), *StageID.ToString());
	}
}

int32 UStageSubsystem::GetStageClearStar(FName StageID) const
{
	// TMap에서 찾아서 있으면 별 개수 반환, 아예 안 깬 스테이지면 0 반환
	if (const int32* FoundStar = StageClearStars.Find(StageID))
	{
		return *FoundStar;
	}
	return 0;
}

void UStageSubsystem::RecordStageClearStar(FName StageID, int32 StarCount)
{
	if (StageID.IsNone() || StarCount <= 0) return;

	int32 CurrentStar = GetStageClearStar(StageID);

	// 기존 기록보다 더 높은 기록을 땄을 때만 갱신합니다.
	if (StarCount > CurrentStar)
	{
		StageClearStars.Add(StageID, StarCount); 
		UE_LOG(LogStage, Log, TEXT("⭐ [Stage] %s 스테이지 %d별 기록 달성!"), *StageID.ToString(), StarCount);
	}
}

void UStageSubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
	if (!SaveGameObj) return;

	// 세이브 파일에서 데이터를 가져와서 메모리에 덮어씌웁니다.
	UnlockedStages = SaveGameObj->SavedUnlockedStages;
	StageClearStars = SaveGameObj->SavedStageClearStars;
	MaxClearedStageIndex = SaveGameObj->SavedMaxClearedStageIndex;

	
	if (!UnlockedStages.Contains(FName("Stage1_1")))
	{
		UnlockStage(FName("Stage1_1"));
		UE_LOG(LogStage, Warning, TEXT("🛠️ [Stage] 세이브 파일에 기본 스테이지가 없어 강제로 복구(해금)합니다."));
	}
}

void UStageSubsystem::SaveToSaveGame(UParadiseSaveGame* SaveGameObj)const
{
	if (!SaveGameObj) return;

	// 현재 메모리에 있는 진행도를 세이브 파일 객체에 담습니다.
	SaveGameObj->SavedUnlockedStages = UnlockedStages;
	SaveGameObj->SavedStageClearStars = StageClearStars;
	SaveGameObj->SavedMaxClearedStageIndex = MaxClearedStageIndex;
}
