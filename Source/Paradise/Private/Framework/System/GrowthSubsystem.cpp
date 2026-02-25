// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GrowthSubsystem.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"

void UGrowthSubsystem::AddCharacterExp(FName CharacterID, int32 ExpAmount)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
	if (!InvSys) return;

	//인벤토리에서 현재 상태 받아옴
	const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(CharacterID);
	if (!CharData) return;

	int32 CurrentLevel = CharData->Level;
	int32 CurrentExp = CharData->CurrentExp + ExpAmount;

	UE_LOG(LogTemp, Log, TEXT("✨ [%s] 경험치 획득: +%d (총 누적: %d)"), *CharacterID.ToString(), ExpAmount, CurrentExp);

	//레벨업 계산 루프 (데이터 테이블 기반)
	while (true)
	{
		int32 NextLevel = CurrentLevel + 1;
		FName RowName = FName(*FString::FromInt(NextLevel));

		FCharacterLevelUpData* LevelData = GI->GetDataTableRow<FCharacterLevelUpData>(GI->CharacterLevelUpDataTable, RowName);

		// 만렙 도달 시
		if (!LevelData)
		{
			UE_LOG(LogTemp, Warning, TEXT("👑 [%s] 최대 레벨 도달! (Lv.%d)"), *CharacterID.ToString(), CurrentLevel);
			CurrentExp = 0; // 초과 경험치 증발
			break;
		}

		if (CurrentExp >= LevelData->RequiredExp)
		{
			CurrentExp -= LevelData->RequiredExp;
			CurrentLevel++;
			UE_LOG(LogTemp, Warning, TEXT("🎉 [%s] 레벨 업! (Lv.%d -> %d)"), *CharacterID.ToString(), CurrentLevel - 1, CurrentLevel);
		}
		else
		{
			break;
		}
	}

	//계산된 최종 수치를 인벤토리에 Set 요청 
	InvSys->SetCharacterLevelAndExp(CharacterID, CurrentLevel, CurrentExp);
}

void UGrowthSubsystem::HandleDuplicateCharacter(FName CharacterID)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
	if (!InvSys) return;

	const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(CharacterID);
	if (!CharData) return;

	//최대 각성(6돌) 미만이면 조각 추가
	if (CharData->AwakeningPieces < 6)
	{
		InvSys->AddAwakeningPiece(CharacterID, 1);
		UE_LOG(LogTemp, Warning, TEXT("✨ [%s] 중복 획득! 영웅의 돌파 조각을 얻었습니다. (현재: %d / 6)"), *CharacterID.ToString(), CharData->AwakeningPieces + 1);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("👑 [%s] 이미 최대 돌파 조각을 보유 중입니다!"), *CharacterID.ToString());
		//대체 재화 지급? 일단 로그만 찍음
	}
}

bool UGrowthSubsystem::AwakenCharacter(FName CharacterID)
{
	return false;
}

bool UGrowthSubsystem::EnhanceEquipment(FGuid ItemUID)
{
	return false;
}