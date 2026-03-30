// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/GrowthSubsystem.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Paradise/Paradise.h"

void UGrowthSubsystem::AddCharacterExp(FName CharacterID, int32 ExpAmount)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
	if (!InvSys) return;

	// 인벤토리에서 현재 상태 받아옴
	const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(CharacterID);
	if (!CharData) return;

	int32 CurrentLevel = CharData->Level;
	int32 CurrentExp = CharData->CurrentExp + ExpAmount;

	//UE_LOG(LogParadiseGrowth, Log, TEXT("✨ [%s] 경험치 획득: +%d (총 누적: %d)"), *CharacterID.ToString(), ExpAmount, CurrentExp);


	// 🌟 현재 각성(Awakening) 단계에 따른 최대 레벨 상한(MaxLevelCap) 가져오기
	int32 CurrentMaxLevelCap = 99; // 기본 방어값
	FName AwakenRowName = FName(*FString::FromInt(CharData->AwakeningLevel));

	if (FCharacterAwakenData* AwakenInfo = GI->GetDataTableRow<FCharacterAwakenData>(GI->CharacterAwakenDataTable, AwakenRowName))
	{
		CurrentMaxLevelCap = AwakenInfo->MaxLevelCap;
	}
	/*else
	{
		UE_LOG(LogParadiseGrowth, Warning, TEXT("⚠️ [%s] 각성 단계(%d)에 해당하는 데이터가 없습니다! 상한 검사를 건너뜁니다."), *CharacterID.ToString(), CharData->AwakeningLevel);
	}*/


	// 🌟 2. 레벨업 계산 루프 (상한선 체크 추가)
	while (true)
	{
		// 🛑 [상한 검사] 현재 레벨이 이미 돌파 상한선에 도달했거나 넘어섰다면 중지!
		if (CurrentLevel >= CurrentMaxLevelCap)
		{
			//UE_LOG(LogParadiseGrowth, Warning, TEXT("👑 [%s] 현재 각성 단계의 최대 레벨(%d)에 도달했습니다! 추가 성장을 위해 돌파가 필요합니다."), *CharacterID.ToString(), CurrentMaxLevelCap);
			CurrentExp = 0; // 최대 상한 도달 시 초과 경험치 증발 (기획에 따라 보존하려면 이 줄을 삭제하세요)
			break;
		}

		int32 NextLevel = CurrentLevel + 1;
		FName RowName = FName(*FString::FromInt(NextLevel));

		FCharacterLevelUpData* LevelData = GI->GetDataTableRow<FCharacterLevelUpData>(GI->CharacterLevelUpDataTable, RowName);

		// 시스템상 만렙 도달 시
		if (!LevelData)
		{
			//UE_LOG(LogParadiseGrowth, Warning, TEXT("👑 [%s] 게임 시스템 최대 레벨 도달! (Lv.%d)"), *CharacterID.ToString(), CurrentLevel);
			CurrentExp = 0; // 초과 경험치 증발
			break;
		}

		if (CurrentExp >= LevelData->RequiredExp)
		{
			CurrentExp -= LevelData->RequiredExp;
			CurrentLevel++;
			//UE_LOG(LogParadiseGrowth, Warning, TEXT("🎉 [%s] 레벨 업! (Lv.%d -> %d)"), *CharacterID.ToString(), CurrentLevel - 1, CurrentLevel);
		}
		else
		{
			break;
		}
	}

	// 계산된 최종 수치를 인벤토리에 Set 요청 
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

	//현재 각성 레벨에서 최대 각성(Max Level)까지 가기 위해 '추가로 필요한 총 조각 수' 계산
	int32 TotalRequiredPieces = 0;
	int32 CheckLevel = CharData->AwakeningLevel + 1;

	while (true)
	{
		FName RowName = FName(*FString::FromInt(CheckLevel));
		FCharacterAwakenData* AwakenInfo = GI->GetDataTableRow<FCharacterAwakenData>(GI->CharacterAwakenDataTable, RowName);

		// 데이터 테이블에 다음 레벨이 없다면 최대 레벨(Max)이므로 루프 종료
		if (!AwakenInfo) break;

		TotalRequiredPieces += AwakenInfo->RequiredAwakeningPieces;
		CheckLevel++;
	}

	//현재 보유 중인 조각이 앞으로 필요한 조각의 총합보다 적을 때만 조각 지급
	if (CharData->AwakeningPieces < TotalRequiredPieces)
	{
		InvSys->AddAwakeningPiece(CharacterID, 1);

		// 갱신된 데이터를 다시 가져와서 로그에 출력
		const FOwnedCharacterData* UpdatedCharData = InvSys->GetCharacterDataByID(CharacterID);

		/*UE_LOG(LogParadiseGrowth, Warning, TEXT("✨ [%s] 중복 획득! 영웅의 돌파 조각을 얻었습니다. (현재: %d / 필요 총량: %d)"),
			*CharacterID.ToString(), UpdatedCharData->AwakeningPieces, TotalRequiredPieces);*/
	}
	else
	{
		//이미 최대 각성이거나, 최대 각성까지 가기 위한 조각을 미리 다 모아둔 경우
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("👑 [%s] 이미 최대 각성 상태이거나 필요한 조각을 모두 보유 중입니다! (마일리지 등 대체 재화 지급)"), *CharacterID.ToString());

		//최대각성이므로 범용재화 에테르 지급
		if (UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>())
		{
		    EconSys->AddCurrency(ECurrencyType::Aether, 50); // 잉여 조각 대신 에테르 지급 등
		}
	}
}

bool UGrowthSubsystem::AwakenCharacter(FName CharacterID)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return false;

	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
	UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>();
	if (!InvSys || !EconSys) return false;

	//캐릭터 데이터 가져오기
	FOwnedCharacterData* CharData = InvSys->GetMutableCharacterDataByID(CharacterID);
	if (!CharData) return false;

	//다음 각성 단계의 데이터 테이블 행(Row) 찾기
	int32 NextAwakenLevel = CharData->AwakeningLevel + 1;
	FName RowName = FName(*FString::FromInt(NextAwakenLevel));

	//각성 정보 데이터 테이블 조회
	FCharacterAwakenData* AwakenInfo = GI->GetDataTableRow<FCharacterAwakenData>(GI->CharacterAwakenDataTable, RowName);

	if (!AwakenInfo)
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("👑 [%s] 최대 각성 레벨에 도달했습니다!"), *CharacterID.ToString());
		return false;
	}

	//재화 조건 검증 
	if (CharData->AwakeningPieces < AwakenInfo->RequiredAwakeningPieces)
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("❌ [%s] 각성 조각이 부족합니다. (필요: %d, 현재: %d)"), *CharacterID.ToString(), AwakenInfo->RequiredAwakeningPieces, CharData->AwakeningPieces);
		return false;
	}

	//각성에 골드는 필요하지 않으나 추후 추가시 적용위함
	if (!EconSys->HasEnoughCurrency(ECurrencyType::Gold, AwakenInfo->RequiredGold))
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("❌ 골드가 부족합니다!"));
		return false;
	}

	//재화 소모
	CharData->AwakeningPieces -= AwakenInfo->RequiredAwakeningPieces;
	if (AwakenInfo->RequiredGold > 0)
	{
		EconSys->ConsumeCurrency(ECurrencyType::Gold, AwakenInfo->RequiredGold);
	}

	//성장 적용
	CharData->AwakeningLevel = NextAwakenLevel;

	//UI 델리게이트 발송
	InvSys->OnInventoryUpdated.Broadcast();

	//UE_LOG(LogParadiseGrowth, Log, TEXT("🌟 [%s] 캐릭터가 %d단계로 각성했습니다!"), *CharacterID.ToString(), NextAwakenLevel);
	return true;
}

bool UGrowthSubsystem::EnhanceEquipment(FGuid ItemUID)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return false;

	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();
	UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>();
	if (!InvSys || !EconSys) return false;

	//장비 데이터 가져오기
	FOwnedItemData* ItemData = InvSys->GetItemByGUID(ItemUID);
	if (!ItemData) return false;

	//무기/방어구 스탯에서 LevelUpCostId 추출하기
	FName TargetCostID = NAME_None;

	if (FWeaponStats* WpnStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, ItemData->ItemID))
	{
		TargetCostID = WpnStats->LevelUpCostId;
	}
	else if (FArmorStats* ArmorStats = GI->GetDataTableRow<FArmorStats>(GI->ArmorStatsDataTable, ItemData->ItemID))
	{
		TargetCostID = ArmorStats->LevelUpCostId;
	}

	//강화 불가 아이템 체크 (None일 경우)
	if (TargetCostID.IsNone())
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("❌ 해당 아이템은 강화할 수 없는 아이템입니다."));
		return false;
	}

	//강화 정보 데이터 테이블 조회 (LevelUpCostId를 RowName으로 사용)
	FEquipmentEnhanceData* EnhanceInfo = GI->GetDataTableRow<FEquipmentEnhanceData>(GI->EquipmentEnhanceDataTable, TargetCostID);

	if (!EnhanceInfo)
	{
		//UE_LOG(LogParadiseGrowth, Error, TEXT("❌ 강화 데이터 테이블에서 %s 를 찾을 수 없습니다."), *TargetCostID.ToString());
		return false;
	}

	//최대 레벨 도달 여부 확인
	if (ItemData->EnhancementLevel >= EnhanceInfo->MaxEnhanceLevel)
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("👑 아이템이 최대 강화 레벨(+%d)에 도달했습니다!"), EnhanceInfo->MaxEnhanceLevel);
		return false;
	}

	//요구 재화(골드) 동적 계산
	int32 RequiredGold = EnhanceInfo->BaseGoldCost + (EnhanceInfo->GoldCostPerLevel * ItemData->EnhancementLevel);

	//재화 검증 및 소모
	if (!EconSys->HasEnoughCurrency(ECurrencyType::Gold, RequiredGold))
	{
		//UE_LOG(LogParadiseGrowth, Warning, TEXT("❌ 장비 강화 실패: 골드가 부족합니다! (필요: %d G)"), RequiredGold);
		return false;
	}

	if (RequiredGold > 0)
	{
		EconSys->ConsumeCurrency(ECurrencyType::Gold, RequiredGold);
	}

	//성장 적용
	ItemData->EnhancementLevel++;

	//UI 갱신 이벤트 호출 
	InvSys->OnInventoryUpdated.Broadcast();
	InvSys->OnEquipmentUpdated.Broadcast();

	//UE_LOG(LogParadiseGrowth, Log, TEXT("🔨 [%s] 장비가 +%d(으)로 강화되었습니다! (소모 골드: %d)"), *ItemData->ItemID.ToString(), ItemData->EnhancementLevel, RequiredGold);
	return true;
}