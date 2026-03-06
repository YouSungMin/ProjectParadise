// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Lobby/LobbySetupActor.h"

#include "Framework/System/InventorySystem.h" //0226 김성현 - 시스템 헤더들 치트함수 때문에 추가 이후 삭제예정
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/GrowthSubsystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/GachaSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"

#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "UI/HUD/Lobby/ParadiseLobbyHUDWidget.h"
#include "UI/Widgets/Gacha/ParadiseGachaResultWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 1. [최적화] 태그 검색 대신, 레벨 설정 액터 하나만 찾습니다.
	// (GetActorOfClass는 찾으면 즉시 리턴하므로 GetAllActors보다 훨씬 빠릅니다)
	ALobbySetupActor* LobbySetup = Cast<ALobbySetupActor>(UGameplayStatics::GetActorOfClass(this, ALobbySetupActor::StaticClass()));
	if (LobbySetup)
	{
		// 설정 액터에서 이미 할당된 카메라를 가져옵니다. (String 비교 없음, NULL 체크만 하면 됨)
		Camera_Main = LobbySetup->Camera_Main;
		Camera_Battle = LobbySetup->Camera_Battle;
		Camera_Summon = LobbySetup->Camera_Summon;
		Camera_GachaAction = LobbySetup->Camera_GachaAction;

		UE_LOG(LogTemp, Log, TEXT("[LobbyController] 카메라 설정 로드 완료 via SetupActor"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyController] 로비 셋업 액터를 찾을 수 없습니다! 태그 방식으로 폴백하거나 배치를 확인하세요."));
	}

	// 2. 초기 카메라 설정
	if (Camera_Main)
	{
		SetViewTarget(Camera_Main);
	}
	else
	{
		// 카메라를 못 찾았을 때의 방어 코드
		UE_LOG(LogTemp, Error, TEXT("❌ [LobbyController] Main Camera가 설정되지 않았습니다!"));
	}

	//1. 마우스 커서 보이게 설정
	bShowMouseCursor = true;

	//2. UI 전용 입력 모드 설정
	FInputModeUIOnly InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputModeData);

	UE_LOG(LogTemp, Log, TEXT("LobbyController: Mouse Cursor On"));

	// 2. HUD 위젯 생성 및 부착
	if (LobbyHUDClass)
	{
		// 위젯 생성
		UParadiseLobbyHUDWidget* LobbyHUD = CreateWidget<UParadiseLobbyHUDWidget>(this, LobbyHUDClass);

		if (LobbyHUD)
		{
			// 화면에 띄우기
			LobbyHUD->AddToViewport();

			// 캐싱 (이미 HUD NativeConstruct에서 SetLobbyHUD를 호출하지만, 확실하게 한번 더 해도 무방)
			SetLobbyHUD(LobbyHUD);

			UE_LOG(LogTemp, Log, TEXT("[LobbyController] WBP_LobbyHUD 생성 및 부착 성공!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyController] LobbyHUDClass가 설정되지 않았습니다! BP를 확인하세요."));
	}
}

void ALobbyPlayerController::CheatAddCharacter(FName CharacterID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			InvSys->AddCharacter(CharacterID);
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 획득: %s"), *CharacterID.ToString());
		}
	}
}

void ALobbyPlayerController::CheatAddFamiliar(FName FamiliarID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			InvSys->AddFamiliar(FamiliarID);
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 퍼밀리어 획득: %s"), *FamiliarID.ToString());
		}
	}
}

void ALobbyPlayerController::CheatAddItem(FName ItemID, int32 Count)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			InvSys->AddItem(ItemID, Count, 0); // 0강 상태로 지급
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 아이템 획득: %s (%d개)"), *ItemID.ToString(), Count);
		}
	}
}

void ALobbyPlayerController::CheatAddExp(FName CharacterID, int32 ExpAmount)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UGrowthSubsystem* GrowthSys = GI->GetSubsystem<UGrowthSubsystem>())
		{
			GrowthSys->AddCharacterExp(CharacterID, ExpAmount);
		}
	}
}

void ALobbyPlayerController::CheatSetPlayerSlot(int32 SlotIndex, FName CharacterID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>())
		{
			// UI 대신 직접 서브시스템에 데이터 꽂아넣기
			SquadSys->SetPlayerToSlot(SlotIndex, CharacterID);
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 편성 완료: 슬롯[%d] -> %s"), SlotIndex, *CharacterID.ToString());
		}
	}
}

void ALobbyPlayerController::CheatSetFamiliarSlot(int32 SlotIndex, FName FamiliarID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>())
		{
			SquadSys->SetFamiliarToSlot(SlotIndex, FamiliarID);
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 퍼밀리어 편성 완료: 슬롯[%d] -> %s"), SlotIndex, *FamiliarID.ToString());
		}
	}
}

void ALobbyPlayerController::CheatEquipItem(FName CharacterID, FName ItemID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			FGuid TargetCharUID;
			FGuid TargetItemUID;

			// 1. 내 인벤토리에서 해당 ID를 가진 캐릭터의 실제 GUID 찾기
			for (const auto& Char : InvSys->GetOwnedCharacters())
			{
				if (Char.CharacterID == CharacterID)
				{
					TargetCharUID = Char.CharacterUID;
					break;
				}
			}

			// 2. 내 인벤토리에서 해당 ID를 가진 아이템의 실제 GUID 찾기
			for (const auto& Item : InvSys->GetOwnedItems())
			{
				if (Item.ItemID == ItemID)
				{
					TargetItemUID = Item.ItemUID;
					break;
				}
			}

			// 3. 둘 다 찾았다면 장착 시스템 호출!
			if (TargetCharUID.IsValid() && TargetItemUID.IsValid())
			{
				InvSys->EquipItemToCharacter(TargetCharUID, TargetItemUID);
				UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 장비 장착 성공: [%s]가 [%s] 장착!"), *CharacterID.ToString(), *ItemID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 장착 실패: 인벤토리에서 %s 캐릭터나 %s 아이템을 찾지 못했습니다."), *CharacterID.ToString(), *ItemID.ToString());
			}
		}
	}
}

void ALobbyPlayerController::CheatAddGold(int32 Amount)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>())
		{
			EconSys->AddCurrency(ECurrencyType::Gold, Amount);
			UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 골드 %d 획득! (현재 총 골드: %d)"), Amount, EconSys->GetCurrency(ECurrencyType::Gold));
		}
	}
}

void ALobbyPlayerController::CheatAwakenCharacter(FName CharacterID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UGrowthSubsystem* GrowthSys = GI->GetSubsystem<UGrowthSubsystem>())
		{
			bool bSuccess = GrowthSys->AwakenCharacter(CharacterID);
			if (bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 각성 성공: %s"), *CharacterID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 캐릭터 각성 실패: %s (조각/골드 부족 또는 최대 레벨)"), *CharacterID.ToString());
			}
		}
	}
}

void ALobbyPlayerController::CheatEnhanceEquipment(FName ItemID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		UInventorySystem* InvSys = GI->GetMainInventory();
		UGrowthSubsystem* GrowthSys = GI->GetSubsystem<UGrowthSubsystem>();

		if (InvSys && GrowthSys)
		{
			FGuid TargetItemUID;

			// 내 인벤토리에서 해당 ID를 가진 아이템의 실제 GUID 찾기
			for (const auto& Item : InvSys->GetOwnedItems())
			{
				if (Item.ItemID == ItemID)
				{
					TargetItemUID = Item.ItemUID;
					break;
				}
			}

			if (TargetItemUID.IsValid())
			{
				bool bSuccess = GrowthSys->EnhanceEquipment(TargetItemUID);
				if (bSuccess)
				{
					UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 장비 강화 성공: %s"), *ItemID.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 장비 강화 실패: %s (골드 부족 또는 최대 레벨)"), *ItemID.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 인벤토리에서 %s 아이템을 찾지 못했습니다."), *ItemID.ToString());
			}
		}
	}
}

void ALobbyPlayerController::CheatAddAwakeningPiece(FName CharacterID, int32 Count)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			// 1. 캐릭터가 인벤토리에 없으면 먼저 지급 (조각만 받을 순 없으니)
			if (!InvSys->HasCharacter(CharacterID))
			{
				InvSys->AddCharacter(CharacterID);
			}

			// 2. 조각 개수 추가
			InvSys->AddAwakeningPiece(CharacterID, Count);
			UE_LOG(LogTemp, Warning, TEXT("🧩 [Cheat] %s 캐릭터의 각성 조각 %d개 획득!"), *CharacterID.ToString(), Count);
		}
	}
}

void ALobbyPlayerController::CheatGrantAll()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	UInventorySystem* InvSys = GI->GetMainInventory();
	UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>();

	//골드 9999999 지급
	if (EconSys)
	{
		EconSys->AddCurrency(ECurrencyType::Gold, 9999999);
		UE_LOG(LogTemp, Warning, TEXT("💰 [Cheat] 9,999,999 골드 지급 완료!"));
	}

	//모든 데이터 지급
	if (InvSys)
	{
		//모든 캐릭터 지급
		if (GI->CharacterStatsDataTable)
		{
			for (const FName& RowName : GI->CharacterStatsDataTable->GetRowNames())
			{
				InvSys->AddCharacter(RowName);
			}
			UE_LOG(LogTemp, Warning, TEXT("👥 [Cheat] 모든 캐릭터 지급 완료!"));
		}

		//모든 퍼밀리어 지급
		if (GI->FamiliarStatsDataTable)
		{
			for (const FName& RowName : GI->FamiliarStatsDataTable->GetRowNames())
			{
				InvSys->AddFamiliar(RowName);
			}
			UE_LOG(LogTemp, Warning, TEXT("🐾 [Cheat] 모든 퍼밀리어 지급 완료!"));
		}

		//모든 무기 지급
		if (GI->WeaponStatsDataTable)
		{
			for (const FName& RowName : GI->WeaponStatsDataTable->GetRowNames())
			{
				InvSys->AddItem(RowName, 1, 0); // 1개, 0강
			}
			UE_LOG(LogTemp, Warning, TEXT("⚔️ [Cheat] 모든 무기 지급 완료!"));
		}

		//모든 방어구/장신구 지급
		if (GI->ArmorStatsDataTable)
		{
			for (const FName& RowName : GI->ArmorStatsDataTable->GetRowNames())
			{
				InvSys->AddItem(RowName, 1, 0); // 1개, 0강
			}
			UE_LOG(LogTemp, Warning, TEXT("🛡️ [Cheat] 모든 방어구 및 장신구 지급 완료!"));
		}

		//지급이 끝난 후 UI 갱신
		InvSys->OnInventoryUpdated.Broadcast();
	}
}



void ALobbyPlayerController::MoveCameraToMenu(EParadiseLobbyMenu TargetMenu)
{
	ACameraActor* TargetCamera = nullptr;

	// 목표 메뉴에 따른 카메라 선정
	switch (TargetMenu)
	{
	case EParadiseLobbyMenu::None:   TargetCamera = Camera_Main; break;
	case EParadiseLobbyMenu::Battle: TargetCamera = Camera_Battle; break;
	case EParadiseLobbyMenu::Summon: TargetCamera = Camera_Summon; break;
		// 필요하면Enhance 등도 다른 카메라로 확장 가능
	default:                         TargetCamera = Camera_Main; break;
	}

	if (!TargetCamera) return;

	// 1. UI 먼저 숨기기 (연출을 위해)
	if (CachedLobbyHUD)
	{
		// HUD 함수: 이동 중엔 싹 다 숨겨라! (아래에서 구현 예정)
		CachedLobbyHUD->OnStartCameraMove();
	}

	// 2. 카메라 블렌드 (부드러운 이동)
	SetViewTargetWithBlend(TargetCamera, CameraBlendTime, CameraBlendFunc, 0.0f, true);

	// 3. 이동 끝나는 시간에 맞춰 타이머 설정
	GetWorldTimerManager().SetTimer(
		TimerHandle_CameraBlend,
		[this, TargetMenu]() { OnCameraMoveFinished(TargetMenu); },
		CameraBlendTime,
		false
	);
}

void ALobbyPlayerController::SetLobbyHUD(UParadiseLobbyHUDWidget* InHUD)
{
    CachedLobbyHUD = InHUD;
}

void ALobbyPlayerController::OnCameraMoveFinished(EParadiseLobbyMenu TargetMenu)
{
	// 4. 이동 완료! 이제 해당 메뉴 UI 띄우기
	SetLobbyMenu(TargetMenu);
}

void ALobbyPlayerController::SetLobbyMenu(EParadiseLobbyMenu InNewMenu)
{
    if (CurrentMenu == InNewMenu) return;

	// 메뉴를 변경하기 직전에 현재 메뉴를 '이전 메뉴'로 저장합니다.
	PreviousMenu = CurrentMenu;

    CurrentMenu = InNewMenu;
    UE_LOG(LogTemp, Log, TEXT("[Controller] 메뉴 변경: %d"), (int32)CurrentMenu);

    // HUD에게 UI 변경 지시
    if (CachedLobbyHUD)
    {
        CachedLobbyHUD->UpdateMenuStats(CurrentMenu);
    }
}

void ALobbyPlayerController::RequestBackToPreviousMenu()
{
	// 저장해둔 직전 메뉴로 다시 돌아갑니다.
	// 만약 Battle에서 왔다면 다시 Battle로, 메인에서 왔다면 메인(None)으로 갑니다.
	SetLobbyMenu(PreviousMenu);
}

void ALobbyPlayerController::StartGachaActionSequence(int32 DrawCount)
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	UGachaSubsystem* GachaSys = GI->GetSubsystem<UGachaSubsystem>();
	UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>();
	UInventorySystem* InvSys = GI->GetMainInventory();

	if (!GachaSys || !EconSys || !InvSys) return;

	// 1. [트랜잭션] 총 에테르 비용 계산 및 소모 (Aether 통일)
	const int32 TotalAetherNeeded = GachaSys->GetCurrentAetherRequirement() * DrawCount;

	if (!EconSys->ConsumeCurrency(ECurrencyType::Aether, TotalAetherNeeded))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [Gacha] 에테르가 부족합니다! (필요: %d)"), TotalAetherNeeded);
		return;
	}

	// 2. [UI 및 카메라 전환] 
	if (CachedLobbyHUD) CachedLobbyHUD->OnStartCameraMove();
	if (Camera_GachaAction) SetViewTargetWithBlend(Camera_GachaAction, 0.0f);

	// 3. [데이터 준비] 중복 검사용 배열
	TArray<FName> OwnedCharacterIDs;
	for (const FOwnedCharacterData& CharData : InvSys->GetOwnedCharacters())
	{
		OwnedCharacterIDs.Add(CharData.CharacterID);
	}

	// 4. [가챠 실행!] 
	TArray<FGachaResult> PulledResults = GachaSys->PerformGacha(DrawCount, OwnedCharacterIDs);

	// 5. [연출 큐 사인] 상자에게 결과 전달 및 연출 시작
	AParadiseGachaBoxActor* GachaBox = Cast<AParadiseGachaBoxActor>(UGameplayStatics::GetActorOfClass(this, AParadiseGachaBoxActor::StaticClass()));
	if (GachaBox)
	{
		GachaBox->OnGachaResultScreenRequested.RemoveDynamic(this, &ALobbyPlayerController::OnShowGachaResultScreen);
		GachaBox->OnGachaResultScreenRequested.AddDynamic(this, &ALobbyPlayerController::OnShowGachaResultScreen);

		GachaBox->PlayGachaSequence(PulledResults);
	}
}

void ALobbyPlayerController::OnShowGachaResultScreen(const TArray<FGachaResult>& FinalResults)
{
	UE_LOG(LogTemp, Log, TEXT("🎉 [Gacha] 연출 종료! 결과창을 띄우고 보상을 지급합니다."));

	// 1. 위젯이 없으면 최초 1회만 생성 (Lazy Initialization)
	if (!CachedResultWidget && GachaResultWidgetClass)
	{
		CachedResultWidget = CreateWidget<UParadiseGachaResultWidget>(this, GachaResultWidgetClass);
		if (CachedResultWidget)
		{
			CachedResultWidget->AddToViewport(100);
		}
	}

	// 2. 캐싱된 위젯 재사용 (Object Pooling)
	if (CachedResultWidget)
	{
		CachedResultWidget->SetVisibility(ESlateVisibility::Visible);
		CachedResultWidget->ShowResults(FinalResults);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ GachaResultWidgetClass가 세팅되지 않았습니다!"));
	}

	// 3. 실제 보상 인벤토리에 확정 지급 로직
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (GI)
	{
		UInventorySystem* InvSys = GI->GetMainInventory();
		if (InvSys)
		{
			for (const FGachaResult& Result : FinalResults)
			{
				// GameInstance의 유효성 검사 함수를 통해 '장비'인지 '캐릭터'인지 똑똑하게 구분합니다.
				if (GI->IsValidItemID(Result.PulledItemID))
				{
					// [장비] 중복 상관없이 1개씩 온전하게 지급 (0강 상태)
					InvSys->AddItem(Result.PulledItemID, 1, 0);
				}
				else if (GI->IsValidPlayerID(Result.PulledItemID))
				{
					// [캐릭터] 중복 여부에 따라 분기
					if (Result.bIsDuplicate)
					{
						// 가챠 풀 엑셀에 적혀있던 그 조각 개수만큼! 정확하게 지급
						InvSys->AddAwakeningPiece(Result.PulledItemID, Result.ConvertedFragments);
						UE_LOG(LogTemp, Log, TEXT("🧩 [Gacha] 캐릭터 중복 획득! %s 조각 %d개 지급 완료."), *Result.PulledItemID.ToString(), Result.ConvertedFragments);
					}
					else
					{
						// 최초 획득 시 온전한 캐릭터 1개 추가
						InvSys->AddCharacter(Result.PulledItemID);
					}
				}
			}
			// 4. 모든 지급이 끝난 후 게임 자동 세이브!
			GI->SaveGameData();
			UE_LOG(LogTemp, Log, TEXT("💾 [Gacha] 가챠 보상 지급 및 게임 저장 완료!"));
		}
	}
}