// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Lobby/LobbySetupActor.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/System/SquadSubsystem.h"
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
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			InvSys->AddCharacterExp(CharacterID, ExpAmount);
			// 로그는 시스템 내부에서 출력되므로 생략
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