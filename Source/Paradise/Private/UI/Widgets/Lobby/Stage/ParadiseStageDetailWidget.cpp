// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Lobby/Stage/ParadiseStageDetailWidget.h"
#include "UI/Widgets/Squad/ParadiseSquadFormationWidget.h" 
#include "UI/Widgets/Lobby/Stage/ParadiseEnemyIconWidget.h"
#include "UI/Widgets/Common/ParadiseResourceWarningWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/System/StageSubsystem.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/AudioManagementSubsystem.h"

#include "Data/Assets/ParadiseFXAudioData.h"
#include "Data/Structs/InventoryStruct.h"
#include "Data/Structs/StageStructs.h" 
#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/ItemStructs.h"

#include "Actors/Squad/ParadiseSquadSceneManager.h" 
#include "Kismet/GameplayStatics.h"

void UParadiseStageDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	CachedLobbyPC = Cast<ALobbyPlayerController>(GetOwningPlayer());

	if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickClose);
	if (Btn_Formation) Btn_Formation->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickFormation);
	if (Btn_EnterBattle) Btn_EnterBattle->OnClicked.AddDynamic(this, &UParadiseStageDetailWidget::OnClickEnterBattle);

	// 서브시스템의 변화를 감시하기 시작합니다!
	if (auto* SquadSys = GetGameInstance()->GetSubsystem<USquadSubsystem>())
	{
		SquadSys->OnPlayerSlotChanged.AddDynamic(this, &UParadiseStageDetailWidget::OnPlayerSlotUpdated);
		SquadSys->OnFamiliarSlotChanged.AddDynamic(this, &UParadiseStageDetailWidget::OnFamiliarSlotUpdated);
	}
}

void UParadiseStageDetailWidget::NativeDestruct()
{
	// 타이머 해제
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BattleTransition);
	}

	// 메모리 누수 방지를 위해 해제는 필수!
	if (auto* SquadSys = GetGameInstance()->GetSubsystem<USquadSubsystem>())
	{
		SquadSys->OnPlayerSlotChanged.RemoveAll(this);
		SquadSys->OnFamiliarSlotChanged.RemoveAll(this);
	}

	if (Btn_Close) Btn_Close->OnClicked.RemoveAll(this);
	if (Btn_Formation) Btn_Formation->OnClicked.RemoveAll(this);
	if (Btn_EnterBattle) Btn_EnterBattle->OnClicked.RemoveAll(this);

	CachedGI = nullptr;
	CachedLobbyPC = nullptr;

	Super::NativeDestruct();
}

void UParadiseStageDetailWidget::InitDetailPopup(FName InStageID)
{
	CachedStageID = InStageID;

	// 1. 타이틀 설정
	if (Text_StageTitle)
	{
		// 기본값은 ID로 두되, 테이블에서 찾으면 기획자가 적어둔 이름으로 덮어씌웁니다.
		FText FinalStageName = FText::FromName(CachedStageID);

		if (CachedGI.IsValid() && CachedGI->StatgeStatsDataTable)
		{
			if (FStageStats* StageStats = CachedGI->GetDataTableRow<FStageStats>(CachedGI->StatgeStatsDataTable, CachedStageID))
			{
				// 테이블에 StageName이 비어있지 않다면 적용!
				if (!StageStats->StageName.IsEmpty())
				{
					FinalStageName = StageStats->StageName;
				}
			}
		}

		Text_StageTitle->SetText(FinalStageName);
	}

	// 2. 내 스쿼드 UI 초기화 (기존 위젯 재활용)
	if (UI_SquadPreview)
	{
		UI_SquadPreview->SetPreviewMode(true);

		UI_SquadPreview->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetupSquadPreview();
	}

	// 3. 적 리스트 표기 (데이터 테이블 연동)
	SetupEnemyList(CachedStageID);

	SetVisibility(ESlateVisibility::Visible);
}

void UParadiseStageDetailWidget::SetupEnemyList(FName InStageID)
{
	if (!WrapBox_EnemyList) return;
	WrapBox_EnemyList->ClearChildren();

	if (!EnemyIconClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [StageDetail] EnemyIconClass가 누락되었습니다. BP 설정을 확인하세요."));
		return;
	}

	if (CachedGI.IsValid() && CachedGI->StageWaveDetailDataTable && CachedGI->EnemyAssetsDataTable)
	{
		// 1. 해당 스테이지에 등장하는 모든 몬스터 ID를 수집할 Set (중복 제거용)
		TSet<FName> UniqueEnemyIDs;

		// 2. StageWaveDetail 테이블의 모든 행을 순회하며, TargetStageID가 현재 스테이지와 일치하는 것만 뽑아냅니다.
		TArray<FStageWaveDetail*> AllWaves;
		CachedGI->StageWaveDetailDataTable->GetAllRows<FStageWaveDetail>(TEXT("StageDetail"), AllWaves);

		for (FStageWaveDetail* WaveData : AllWaves)
		{
			// 타겟 스테이지가 현재 스테이지와 일치하고, 몬스터 ID가 존재할 경우에만 추가!
			if (WaveData && WaveData->TargetStageID == InStageID && !WaveData->MonsterID.IsNone())
			{
				UniqueEnemyIDs.Add(WaveData->MonsterID);
			}
		}

		// 3. 수집된 고유 몬스터 ID들을 기반으로 아이콘 위젯 생성 및 주입
		for (const FName& EnemyID : UniqueEnemyIDs)
		{
			if (FEnemyAssets* EnemyAsset = CachedGI->GetDataTableRow<FEnemyAssets>(CachedGI->EnemyAssetsDataTable, EnemyID))
			{
				if (UParadiseEnemyIconWidget* IconWidget = CreateWidget<UParadiseEnemyIconWidget>(this, EnemyIconClass))
				{
					// [주의] UnitStructs.h의 FEnemyAssets 구조체 안에 TSoftObjectPtr<UTexture2D> FaceIcon; 이 존재해야 합니다!
					IconWidget->SetupIcon(EnemyAsset->FaceIcon);
					WrapBox_EnemyList->AddChildToWrapBox(IconWidget);
				}
			}
		}
	}
}

void UParadiseStageDetailWidget::SetupSquadPreview()
{
	if (!UI_SquadPreview) return;

	USquadSubsystem* SquadSys = GetGameInstance()->GetSubsystem<USquadSubsystem>();
	if (!SquadSys || !CachedGI.IsValid()) return;

	// 1. 플레이어(캐릭터) 스쿼드 세팅 (슬롯 0~2)
	TArray<FName> PlayerIDs = SquadSys->GetPlayerSquad();
	for (int32 i = 0; i < PlayerIDs.Num(); ++i)
	{
		FSquadItemUIData UIData;
		FName CharID = PlayerIDs[i];

		if (!CharID.IsNone())
		{
			UIData.ID = CharID;

			// 🚨 [핵심 버그 수정] 인벤토리에서 실제 레벨을 조회하여 대입합니다.
			UIData.Level = 1; // 기본값
			if (UInventorySystem* InvSys = GetGameInstance()->GetSubsystem<UInventorySystem>())
			{
				if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(CharID))
				{
					UIData.Level = CharData->Level;
				}
			}

			// 캐릭터는 스쿼드 프리뷰이므로 FaceIcon을 사용!
			if (FCharacterAssets* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, CharID))
			{
				UIData.Icon = Asset->FaceIcon.LoadSynchronous();
			}
		}
		// 뷰(FormationWidget) 업데이트
		UI_SquadPreview->UpdateSlot(i, UIData);
	}

	// 2. 퍼밀리어(유닛) 스쿼드 세팅 (슬롯 3~7)
	TArray<FName> FamiliarIDs = SquadSys->GetFamiliarSquad();
	for (int32 i = 0; i < FamiliarIDs.Num(); ++i)
	{
		FSquadItemUIData UIData;
		FName FamiliarID = FamiliarIDs[i];

		if (!FamiliarID.IsNone())
		{
			UIData.ID = FamiliarID;
			// 유닛도 FaceIcon 사용!
			if (FFamiliarAssets* Asset = CachedGI->GetDataTableRow<FFamiliarAssets>(CachedGI->FamiliarAssetsDataTable, FamiliarID))
			{
				UIData.Icon = Asset->FaceIcon.LoadSynchronous();
			}
		}
		// 유닛은 UI 인덱스가 3부터 시작하므로 i + 3
		UI_SquadPreview->UpdateSlot(i + 3, UIData);
	}
}

void UParadiseStageDetailWidget::OnClickClose()
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_CommonBack)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_CommonBack);
		}
	}

	SetVisibility(ESlateVisibility::Collapsed);

	if (OnDetailClosed.IsBound()) OnDetailClosed.Broadcast();
}

void UParadiseStageDetailWidget::OnClickFormation()
{
	// 1. UI 상태 전환만 요청함
	if (CachedLobbyPC.IsValid())
	{
		CachedLobbyPC->SetLobbyMenu(EParadiseLobbyMenu::Squad);
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		// 월드에 배치된 스튜디오 매니저를 찾아옵니다.
		AActor* SceneManager = UGameplayStatics::GetActorOfClass(GetWorld(), AParadiseSquadSceneManager::StaticClass());

		if (SceneManager)
		{
			// 블렌딩 없이 즉시 카메라 시점을 스튜디오로 이동!
			PC->SetViewTarget(SceneManager);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [StageDetail] 맵에 배치된 AParadiseSquadSceneManager를 찾을 수 없어 카메라 전환을 스킵합니다."));
		}
	}
}

void UParadiseStageDetailWidget::OnClickEnterBattle()
{
	//방어코드
	if (CachedStageID.IsNone() || !CachedGI.IsValid()) return;

	//0324 김성현 - 스쿼드 편성 상태 검증 
	if (USquadSubsystem* SquadSys = CachedGI->GetSubsystem<USquadSubsystem>())
	{
		FString ErrorMsg;
		if (!SquadSys->IsSquadValidForBattle(ErrorMsg))
		{
			//팝업 위젯이 있다면 경고 메시지를 띄워줍니다.
			if (Widget_ResourceWarning)
			{
				Widget_ResourceWarning->ShowWarning(FText::FromString(ErrorMsg), nullptr, true);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [StageDetail] 경고 위젯이 없어 화면에 띄울 수 없음! 사유: %s"), *ErrorMsg);
			}
			return;
		}
	}

	if (CachedGI->GlobalAudioData && CachedGI->GlobalAudioData->SFX_StageActionClick)
	{
		UGameplayStatics::PlaySound2D(this, CachedGI->GlobalAudioData->SFX_StageActionClick);
	}

	// 효과음이 살짝 들린 후 레벨 전환 (0.3초 딜레이)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_BattleTransition,
			this,
			&UParadiseStageDetailWidget::ExecuteBattleTransition,
			0.3f,
			false
		);
	}
}

void UParadiseStageDetailWidget::ExecuteBattleTransition()
{
	if (CachedStageID.IsNone() || !CachedGI.IsValid()) return;

	// 1. 스테이지 데이터 조회
	FStageStats* Stats = CachedGI->GetDataTableRow<FStageStats>(CachedGI->StatgeStatsDataTable, CachedStageID);
	FStageAssets* Assets = CachedGI->GetDataTableRow<FStageAssets>(CachedGI->StageAssetsDataTable, CachedStageID);

	if (!Stats || !Assets || Assets->MapAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [StageDetail] 스테이지 데이터 누락: %s"), *CachedStageID.ToString());
		return;
	}

	// 2. 서브시스템 상태 업데이트
	if (UStageSubsystem* StageSys = CachedGI->GetSubsystem<UStageSubsystem>())
	{
		StageSys->SetSelectedStageID(CachedStageID);
	}

	if (UAudioManagementSubsystem* AudioMag = CachedGI->GetSubsystem<UAudioManagementSubsystem>())
	{
		AudioMag->StopBGM(1.0f);
	}

	// 3. 레벨 전환
	if (ULevelLoadingSubsystem* LoadingSys = CachedGI->GetSubsystem<ULevelLoadingSubsystem>())
	{
		FName LevelToOpen = FName(*Assets->MapAsset.GetAssetName());
		TArray<TSoftObjectPtr<UObject>> PreloadAssets = Assets->ExtraPreloadAssets;

		LoadingSys->StartLevelTransition(
			LevelToOpen,
			FName("L_Loading"),
			PreloadAssets,
			Assets->LoadingImage,
			Stats->StageName,
			Stats->Description
		);
	}
}

void UParadiseStageDetailWidget::OnPlayerSlotUpdated(int32 SlotIndex, FName NewPlayerID)
{
	if (!UI_SquadPreview || !CachedGI.IsValid()) return;

	FSquadItemUIData UIData;
	if (!NewPlayerID.IsNone())
	{
		UIData.ID = NewPlayerID;

		// 슬롯이 갱신될 때도 실제 레벨을 조회하여 대입합니다.
		UIData.Level = 1;
		if (UInventorySystem* InvSys = GetGameInstance()->GetSubsystem<UInventorySystem>())
		{
			if (const FOwnedCharacterData* CharData = InvSys->GetCharacterDataByID(NewPlayerID))
			{
				UIData.Level = CharData->Level;
			}
		}

		if (FCharacterAssets* Asset = CachedGI->GetDataTableRow<FCharacterAssets>(CachedGI->CharacterAssetsDataTable, NewPlayerID))
		{
			UIData.Icon = Asset->FaceIcon.LoadSynchronous();
		}
	}
	UI_SquadPreview->UpdateSlot(SlotIndex, UIData);
}

void UParadiseStageDetailWidget::OnFamiliarSlotUpdated(int32 SlotIndex, FName NewFamiliarID)
{
	if (!UI_SquadPreview || !CachedGI.IsValid()) return;

	const int32 FormationIndex = SlotIndex + 3; // 유닛은 인덱스 3번부터
	FSquadItemUIData UIData;
	if (!NewFamiliarID.IsNone())
	{
		UIData.ID = NewFamiliarID;
		// 유닛은 레벨 표기가 없으므로 연산 생략 (최적화)

		if (FFamiliarAssets* Asset = CachedGI->GetDataTableRow<FFamiliarAssets>(CachedGI->FamiliarAssetsDataTable, NewFamiliarID))
		{
			UIData.Icon = Asset->FaceIcon.LoadSynchronous();
		}
	}
	UI_SquadPreview->UpdateSlot(FormationIndex, UIData);
}