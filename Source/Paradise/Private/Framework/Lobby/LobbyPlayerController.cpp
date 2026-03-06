// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Lobby/LobbySetupActor.h"

#include "Framework/System/InventorySystem.h" //0226 김성현 - 시스템 헤더들 치트함수 때문에 추가 이후 삭제예정
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/GrowthSubsystem.h"
#include "Framework/System/EconomySubsystem.h"

#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "UI/HUD/Lobby/ParadiseLobbyHUDWidget.h"
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
	// 1. 떠 있던 가챠 팝업 UI 숨기기 (연출을 봐야 하니까!)
	if (CachedLobbyHUD)
	{
		CachedLobbyHUD->OnStartCameraMove(); // 만들어두신 숨김 함수 재활용
	}

	// 2. 카메라를 상자가 있는 시네마틱 카메라로 즉시(0초) 컷 전환!
	// (블렌드를 줘도 되지만, 오버워치나 원신은 보통 여기서 컷 씬으로 확 넘어갑니다)
	if (Camera_GachaAction)
	{
		SetViewTargetWithBlend(Camera_GachaAction, 0.0f);
	}

	// 3. 서버/서브시스템에서 실제 뽑기 결과(데이터)를 가져옵니다.
	// (이 부분은 유저님의 GachaSubsystem 연동 방식에 맞춰 작성하시면 됩니다)
	TArray<FGachaResult> PulledResults;
	// 예시: PulledResults = GI->GetSubsystem<UGachaSubsystem>()->PullGacha(DrawCount);

	// 4. 레벨에 배치된 상자 액터를 찾아서 시퀀스 재생 명령을 내립니다!
	AParadiseGachaBoxActor* GachaBox = Cast<AParadiseGachaBoxActor>(UGameplayStatics::GetActorOfClass(this, AParadiseGachaBoxActor::StaticClass()));

	if (GachaBox)
	{
		GachaBox->PlayGachaSequence(PulledResults);
	}
}
