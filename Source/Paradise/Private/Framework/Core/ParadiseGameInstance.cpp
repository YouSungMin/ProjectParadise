// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/Core/ParadiseSaveManager.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/ParadiseSaveGame.h"
#include "Framework/System/GachaSubsystem.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/StageSubsystem.h"
#include "Components/EquipmentComponent.h"
#include "Characters/Player/PlayerData.h"
#include "Data/Structs/ItemStructs.h"
#include "Kismet/GameplayStatics.h"
#include "Paradise/Paradise.h"
//0317 김성현 - CheckMeshScale 디버그 함수때문에 추가 추후 삭제
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"


UParadiseGameInstance::UParadiseGameInstance()
{
}

void UParadiseGameInstance::Init()
{
	Super::Init();

	SaveGameSlotName = DefaultSaveSlot;


	//게임 세이브 데이터 로드 함수 호출
	LoadGameData();

	// [핵심] 서브시스템에 로딩 위젯 클래스 전달
	if (ULevelLoadingSubsystem* LoadingSystem = GetSubsystem<ULevelLoadingSubsystem>())
	{
		LoadingSystem->SetLoadingWidgetClass(LoadingWidgetClass);
	}

	UE_LOG(LogTemp, Log, TEXT("[ParadiseGameInstance] 초기화 및 로딩 서브시스템 설정 완료."));
}

void UParadiseGameInstance::Shutdown()
{
	SaveGameData();
	UE_LOG(LogTemp, Warning, TEXT("🛑 [GameInstance] 게임 종료 전 자동 저장을 완료했습니다."));


	Super::Shutdown();
}

void UParadiseGameInstance::CheckMeshScale(FString FolderPath)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================================="));
	UE_LOG(LogTemp, Warning, TEXT("🔍 [지뢰 탐지기 작동] 스켈레탈 메쉬 스케일 검사를 시작합니다."));
	UE_LOG(LogTemp, Warning, TEXT("📂 검사 경로: %s"), *FolderPath);
	UE_LOG(LogTemp, Warning, TEXT("========================================================="));

	// 에셋 레지스트리를 통해 폴더 내의 모든 에셋 정보를 가져옵니다.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	// 폴더 내 에셋 검색 (true = 하위 폴더까지 전부 검색)
	AssetRegistryModule.Get().GetAssetsByPath(FName(*FolderPath), AssetDataList, true);

	int32 TotalChecked = 0;
	int32 TotalBadMeshes = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		// 이 에셋이 스켈레탈 메쉬인지 확인
		if (AssetData.GetClass() == USkeletalMesh::StaticClass())
		{
			// 검사를 위해 에셋 메모리에 로드
			USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
			if (SkelMesh)
			{
				TotalChecked++;
				bool bIsSafe = true;
				FString BadBoneNames = TEXT("");

				// 뼈대 정보 가져오기
				const FReferenceSkeleton& RefSkel = SkelMesh->GetRefSkeleton();
				const TArray<FTransform>& RefBonePose = RefSkel.GetRefBonePose();

				// 모든 뼈 검사
				for (int32 i = 0; i < RefBonePose.Num(); ++i)
				{
					FVector BoneScale = RefBonePose[i].GetScale3D();

					// 스케일이 0이거나 NaN이면 지뢰로 판정
					if (BoneScale.ContainsNaN() || BoneScale.IsNearlyZero())
					{
						bIsSafe = false;
						FName BoneName = RefSkel.GetBoneName(i);
						BadBoneNames += FString::Printf(TEXT("\n    👉 뼈: [%s], 스케일: %s"), *BoneName.ToString(), *BoneScale.ToString());
					}
				}

				// 지뢰가 발견되었다면 빨간색 에러 로그로 출력!
				if (!bIsSafe)
				{
					TotalBadMeshes++;
					UE_LOG(LogTemp, Error, TEXT("💣 [지뢰 발견] 에셋: %s %s"), *SkelMesh->GetName(), *BadBoneNames);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================================="));
	UE_LOG(LogTemp, Warning, TEXT("🏁 [검사 완료] 총 %d개의 메쉬 검사됨 / 💣 지뢰(불량 메쉬) %d개 발견됨!"), TotalChecked, TotalBadMeshes);
	UE_LOG(LogTemp, Warning, TEXT("========================================================="));

}

void UParadiseGameInstance::SaveGameData()
{
	//이미 세이브 파일이 존재하는데 로드에 실패했던 상태라면, 빈 데이터 저장을 차단합니다.
	if (!bIsGameDataLoaded && UGameplayStatics::DoesSaveGameExist(SaveGameSlotName, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("🛑 [SaveSystem] 비정상적인 로드 상태이므로 기존 데이터를 보호하기 위해 덮어쓰기를 취소합니다!"));
		return;
	}

	//저장할 SaveGame 객체 생성
	UParadiseSaveGame* SaveObj = Cast<UParadiseSaveGame>(UGameplayStatics::CreateSaveGameObject(UParadiseSaveGame::StaticClass()));
	if (!SaveObj) return;

	const TArray<UGameInstanceSubsystem*>& Subsystems = GetSubsystemArrayCopy<UGameInstanceSubsystem>();

	for (UGameInstanceSubsystem* Subsystem : Subsystems)
	{
		//서브시스템이 인터페이스 구현이 되어있는지 확인
		if (IParadiseSaveInterface* SaveableSubsystem = Cast<IParadiseSaveInterface>(Subsystem))
		{
			SaveableSubsystem->SaveToSaveGame(SaveObj);
		}
	}

	//저장 데이터 위변조 방지 암호화 적용
	if (UParadiseSaveManager::SaveGameEncrypted(SaveObj, SaveGameSlotName))
	{
		UE_LOG(LogParadiseSaveGame, Log, TEXT("💾 [SaveSystem] 게임 데이터 [보안 암호화] 영구 저장 완료! (슬롯: %s)"), *SaveGameSlotName);
	}
	else
	{
		UE_LOG(LogParadiseSaveGame, Error, TEXT("❌ [SaveSystem] 게임 데이터 보안 저장에 실패했습니다."));
	}
}

void UParadiseGameInstance::LoadGameData()
{
	UInventorySystem* MainInventory = GetMainInventory();
	if (!MainInventory) return;

	UParadiseSaveGame* LoadObj = Cast<UParadiseSaveGame>(UParadiseSaveManager::LoadGameEncrypted(SaveGameSlotName));

	if (LoadObj)
	{

		bIsGameDataLoaded = true;
		//세이브 객체에 들어있는 배열들을 인벤토리의 InitInventory 함수에 호출
		//InitInventory 함수 내부에서 자동으로 유효성 검사 후 인벤토리 초기화
		MainInventory->InitInventory(
			LoadObj->SavedOwnedCharacters,
			LoadObj->SavedOwnedFamiliars,
			LoadObj->SavedOwnedInventoryItems
		);

		//스쿼드 편성 정보 로드
		if (USquadSubsystem* SquadSys = GetSubsystem<USquadSubsystem>())
		{
			SquadSys->LoadFromSaveGame(LoadObj);
		}

		//플레이어 보유 재화 정보 로드
		if (UEconomySubsystem* EconomySys = GetSubsystem<UEconomySubsystem>())
		{
			EconomySys->LoadFromSaveGame(LoadObj);
		}

		//스테이지 정보 로드
		if (UStageSubsystem* StageSys = GetSubsystem<UStageSubsystem>()) {
			StageSys->LoadFromSaveGame(LoadObj);
		}

		// 가챠 천장 정보 로드
		if (UGachaSubsystem* GachaSys = GetSubsystem<UGachaSubsystem>())
		{
			GachaSys->LoadFromSaveGame(LoadObj);
		}

		UE_LOG(LogParadiseSaveGame, Log, TEXT("📂 [SaveSystem] 저장된 게임 불러오기 성공!"));
	}
	else
	{
		//세이브 파일이 없다면 (처음 게임을 켰거나 데이터가 날아간 경우)
		UE_LOG(LogParadiseSaveGame, Warning, TEXT("📂 [SaveSystem] 세이브 파일이 없습니다. 빈 인벤토리로 시작합니다."));

		if (!UGameplayStatics::DoesSaveGameExist(SaveGameSlotName, 0))
		{
			bIsGameDataLoaded = true;
			//만약 튜토리얼 기본 지급 영웅/무기가 필요하다면 여기서 AddCharacter() 등을 호출하시면 됩니다.

		}
	}
}

bool UParadiseGameInstance::IsValidPlayerID(FName PlayerID) const
{
	//None값이 들어오면 무조건 false (빈 데이터니까)
	if (PlayerID.IsNone()) return false;

	//캐릭터 데이터 테이블이 유효한지 확인 
	if (CharacterStatsDataTable && CharacterAssetsDataTable)
	{
		
		FCharacterAssets* AssetData = GetDataTableRow<FCharacterAssets>(CharacterAssetsDataTable, PlayerID);
		FCharacterStats* StatData = GetDataTableRow<FCharacterStats>(CharacterStatsDataTable, PlayerID);

		if (AssetData != nullptr && StatData != nullptr)
		{
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [GameInstance] 유효성 검증 실패: '%s' (에셋 또는 스탯 데이터가 누락되었습니다)"), *PlayerID.ToString());
			return false;
		}

	}
	UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] CharacterStatsDataTable 또는 CharacterAssetsDataTable이 할당되지 않았습니다!"));
	return false;
}

bool UParadiseGameInstance::IsValidFamiliarID(FName FamiliarID) const
{
	//None값이 들어오면 무조건 false (빈 데이터)
	if (FamiliarID.IsNone()) return false;

	//퍼밀리어 스탯 & 에셋 데이터 테이블이 유효한지 확인
	if (FamiliarStatsDataTable && FamiliarAssetsDataTable)
	{
		FFamiliarAssets* AssetData = GetDataTableRow<FFamiliarAssets>(FamiliarAssetsDataTable, FamiliarID);
		FFamiliarStats* StatData = GetDataTableRow<FFamiliarStats>(FamiliarStatsDataTable, FamiliarID);

		if (AssetData != nullptr && StatData != nullptr)
		{
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [GameInstance] 유효성 검증 실패: '%s' (퍼밀리어 에셋 또는 스탯 데이터가 누락되었습니다)"), *FamiliarID.ToString());
			return false;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] FamiliarStatsDataTable 또는 FamiliarAssetsDataTable이 할당되지 않았습니다!"));
	return false;
}

bool UParadiseGameInstance::IsValidItemID(FName ItemID) const
{
	//None값이 들어오면 무조건 false (빈 데이터)
	if (ItemID.IsNone()) return false;

	bool bIsValidWeapon = false;
	bool bIsValidArmor = false;

	//무기(Weapon) 스탯 & 에셋 테이블에서 검사
	if (WeaponStatsDataTable && WeaponAssetsDataTable)
	{
		FWeaponAssets* WeaponAsset = GetDataTableRow<FWeaponAssets>(WeaponAssetsDataTable, ItemID);
		FWeaponStats* WeaponStat = GetDataTableRow<FWeaponStats>(WeaponStatsDataTable, ItemID);

		if (WeaponAsset != nullptr && WeaponStat != nullptr)
		{
			bIsValidWeapon = true; // 무기로서 유효함!
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] WeaponStatsDataTable 또는 WeaponAssetsDataTable이 할당되지 않았습니다!"));
	}

	//방어구(Armor) 스탯 & 에셋 테이블에서 검사
	if (ArmorStatsDataTable && ArmorAssetsDataTable)
	{
		FArmorAssets* ArmorAsset = GetDataTableRow<FArmorAssets>(ArmorAssetsDataTable, ItemID);
		FArmorStats* ArmorStat = GetDataTableRow<FArmorStats>(ArmorStatsDataTable, ItemID);

		if (ArmorAsset != nullptr && ArmorStat != nullptr)
		{
			bIsValidArmor = true; // 방어구로서 유효함!
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] ArmorStatsDataTable 또는 ArmorAssetsDataTable이 할당되지 않았습니다!"));
	}

	//무기이거나 방어구 중 하나라도 쌍이 맞게 존재한다면 유효한 아이템
	if (bIsValidWeapon || bIsValidArmor)
	{
		return true;
	}
	else
	{
		// 양쪽 다 없거나 데이터가 누락(반쪽짜리)된 경우
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [GameInstance] 유효성 검증 실패: '%s' (무기 또는 방어구 테이블 쌍에 완벽하게 존재하지 않습니다)"), *ItemID.ToString());
		return false;
	}
}

bool UParadiseGameInstance::IsValidUnitID(FName UnitID) const
{
	//None값이 들어오면 무조건 false (빈 데이터)
	if (UnitID.IsNone()) return false;

	//적 스탯 & 에셋 데이터 테이블이 유효한지 확인
	if (EnemyStatsDataTable && EnemyAssetsDataTable)
	{
		FEnemyAssets* AssetData = GetDataTableRow<FEnemyAssets>(EnemyAssetsDataTable, UnitID);
		FEnemyStats* StatData = GetDataTableRow<FEnemyStats>(EnemyStatsDataTable, UnitID);

		if (AssetData != nullptr && StatData != nullptr)
		{
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [GameInstance] 유효성 검증 실패: '%s' (적/유닛 에셋 또는 스탯 데이터가 누락되었습니다)"), *UnitID.ToString());
			return false;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("❌ [GameInstance] EnemyStatsDataTable 또는 EnemyAssetsDataTable이 할당되지 않았습니다!"));
	return false;
}

UInventorySystem* UParadiseGameInstance::GetMainInventory() const
{
	return GetSubsystem<UInventorySystem>();
}
