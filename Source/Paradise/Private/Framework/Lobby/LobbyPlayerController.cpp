// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Lobby/LobbySetupActor.h"

#include "Framework/System/InventorySystem.h" //0226 김성현 - 시스템 헤더들 치트함수 때문에 추가 이후 삭제예정
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/GrowthSubsystem.h"
#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/GachaSubsystem.h"
#include "Framework/System/StageSubsystem.h"
#include "Framework/System/ParadiseCursorSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"

#include "Components/AudioComponent.h"
#include "Actors/Environment/ParadiseMapEnvironmentActor.h"
#include "Actors/Gacha/ParadiseGachaBoxActor.h"
#include "UI/Widgets/Squad/ParadiseSquadMainWidget.h"
#include "UI/HUD/Lobby/ParadiseLobbyHUDWidget.h"
#include "UI/Widgets/Lobby/Stage/ParadiseStageSelectWidget.h"
#include "UI/Widgets/Gacha/ParadiseGachaResultWidget.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"
#include "TimerManager.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 1. [최적화] 태그 검색 대신, 레벨 설정 액터 하나만 찾습니다.
	ALobbySetupActor* LobbySetup = Cast<ALobbySetupActor>(UGameplayStatics::GetActorOfClass(this, ALobbySetupActor::StaticClass()));
	if (LobbySetup)
	{
		// 설정 액터에서 이미 할당된 카메라를 가져옵니다. (String 비교 없음, NULL 체크만 하면 됨)
		Camera_Main = LobbySetup->Camera_Main;
		Camera_Battle = LobbySetup->Camera_Battle;
		Camera_Summon = LobbySetup->Camera_Summon;
		Camera_GachaAction = LobbySetup->Camera_GachaAction;

		//UE_LOG(LogTemp, Log, TEXT("[LobbyController] 카메라 설정 로드 완료 via SetupActor"));
	}
	AParadiseGachaBoxActor* FoundBox = Cast<AParadiseGachaBoxActor>(UGameplayStatics::GetActorOfClass(this, AParadiseGachaBoxActor::StaticClass()));
	if (FoundBox)
	{
		CachedGachaBox = FoundBox;

		// 게임 시작 시에는 가챠 상자가 안 보여야 하므로 일단 숨겨둡니다.
		FoundBox->SetActorHiddenInGame(true);

		//UE_LOG(LogTemp, Log, TEXT("✅ [LobbyController] 가챠 박스 캐싱 완료!"));
	}
	/*else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [LobbyController] 맵에 배치된 BP_GachaBoxActor가 없습니다! 맵에 끌어다 놓으세요."));
	}*/

	// 2. 초기 카메라 설정
	if (Camera_Main)
	{
		SetViewTarget(Camera_Main);
	}
	//else
	//{
	//	// 카메라를 못 찾았을 때의 방어 코드
	//	UE_LOG(LogTemp, Error, TEXT("❌ [LobbyController] Main Camera가 설정되지 않았습니다!"));
	//}

	//1. 기존 os 마우스 커서는 끄기
	bShowMouseCursor = false;
	// 커서 서브시스템으로 커서 초기화
	if (UParadiseCursorSubsystem* CursorSys = GetGameInstance()->GetSubsystem<UParadiseCursorSubsystem>())
	{
		CachedCursorSubsystem = CursorSys;
		CursorSys->InitializeCursor(CursorWidgetClass, Tex_CustomCursor, this);
		CursorSys->ShowCursor(true);
	}
	bEnableClickEvents = true; // 이거 없으면 3D 액터 클릭 안 됨!
	bEnableTouchEvents = true;

	//2. UI 전용 입력 모드 설정
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);

	//UE_LOG(LogTemp, Log, TEXT("LobbyController: Mouse Cursor On"));

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

			//UE_LOG(LogTemp, Log, TEXT("[LobbyController] WBP_LobbyHUD 생성 및 부착 성공!"));
		}
	}
	/*else
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyController] LobbyHUDClass가 설정되지 않았습니다! BP를 확인하세요."));
	}*/
}

void ALobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_OpenSettings)
		{
			EnhancedInputComponent->BindAction(IA_OpenSettings, ETriggerEvent::Started, this, &ALobbyPlayerController::OnInputOpenSettings);
		}
	}
}

void ALobbyPlayerController::OnInputOpenSettings(const FInputActionValue& Value)
{
	if (bIsTogglingSettings) return;
	bIsTogglingSettings = true;

	// 로비 HUD가 캐싱되어 있는지 확인 (없으면 에러)
	if (!CachedLobbyHUD)
	{
		bIsTogglingSettings = false;
		return;
	}

	// 컨트롤러는 그저 HUD에게 "팝업 토글해!" 라고 지시만 합니다. (캡슐화 달성)
	CachedLobbyHUD->ToggleSettingsPopup();

	bIsTogglingSettings = false;
}

void ALobbyPlayerController::CheatAddCharacter(FName CharacterID)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvSys = GI->GetMainInventory())
		{
			InvSys->AddCharacter(CharacterID);
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 획득: %s"), *CharacterID.ToString());
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
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 퍼밀리어 획득: %s"), *FamiliarID.ToString());
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
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 아이템 획득: %s (%d개)"), *ItemID.ToString(), Count);
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
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 편성 완료: 슬롯[%d] -> %s"), SlotIndex, *CharacterID.ToString());
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
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 퍼밀리어 편성 완료: 슬롯[%d] -> %s"), SlotIndex, *FamiliarID.ToString());
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
				//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 장비 장착 성공: [%s]가 [%s] 장착!"), *CharacterID.ToString(), *ItemID.ToString());
			}
			/*else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 장착 실패: 인벤토리에서 %s 캐릭터나 %s 아이템을 찾지 못했습니다."), *CharacterID.ToString(), *ItemID.ToString());
			}*/
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
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 골드 %d 획득! (현재 총 골드: %d)"), Amount, EconSys->GetCurrency(ECurrencyType::Gold));
		}
	}
}

void ALobbyPlayerController::CheatAddAether(int32 Amount)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UEconomySubsystem* EconSys = GI->GetSubsystem<UEconomySubsystem>())
		{
			EconSys->AddCurrency(ECurrencyType::Aether, Amount);
			//UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 에테르 %d 획득! (현재 총 에테르: %d)"), Amount, EconSys->GetCurrency(ECurrencyType::Gold));
		}
	}
}

void ALobbyPlayerController::CheatSellItem(FName ItemID, int32 QuantityToSell)
{
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (UInventorySystem* InvenSys = GI->GetSubsystem<UInventorySystem>())
		{
			FGuid TargetUID;
			bool bFound = false;

			// 1. 인벤토리를 뒤져서 입력한 ItemID와 일치하는 아이템의 고유 UID를 찾습니다.
			// (주의: 인벤토리 데이터 구조체 접근 방식이 GetOwnedItems()가 아니라면 본인의 프로젝트에 맞게 배열 이름을 수정해주세요)
			for (const FOwnedItemData& ItemData : InvenSys->GetOwnedItems())
			{
				if (ItemData.ItemID == ItemID)
				{
					TargetUID = ItemData.ItemUID;
					bFound = true;
					break; // 가장 먼저 발견된 아이템을 판매 타겟으로 지정
				}
			}

			// 2. 보유하지 않은 아이템일 경우 거부
			if (!bFound)
			{
				FString Msg = FString::Printf(TEXT("[Cheat] 실패: 인벤토리에 [%s] 아이템이 없습니다!"), *ItemID.ToString());
				//UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Msg);
				return;
			}

			// 3. 실제 판매 로직 호출 및 결과 출력
			FString ErrorMsg;
			if (InvenSys->SellItem(TargetUID, QuantityToSell, ErrorMsg))
			{
				FString Msg = FString::Printf(TEXT("[Cheat] 성공: [%s] 아이템을 %d개 판매했습니다!"), *ItemID.ToString(), QuantityToSell);
				//UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Msg);
			}
			else
			{
				// 실패 시 (누군가 장착 중이거나 갯수 부족 등) ErrorMsg를 화면에 띄워줍니다.
				FString Msg = FString::Printf(TEXT("[Cheat] 판매 거부: %s"), *ErrorMsg);
				//UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Msg);
			}
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
			/*if (bSuccess)
			{
				UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 캐릭터 각성 성공: %s"), *CharacterID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 캐릭터 각성 실패: %s (조각/골드 부족 또는 최대 레벨)"), *CharacterID.ToString());
			}*/
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
				/*if (bSuccess)
				{
					UE_LOG(LogTemp, Warning, TEXT("🕹️ [Cheat] 장비 강화 성공: %s"), *ItemID.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 장비 강화 실패: %s (골드 부족 또는 최대 레벨)"), *ItemID.ToString());
				}*/
			}
			/*else
			{
				UE_LOG(LogTemp, Error, TEXT("❌ [Cheat] 인벤토리에서 %s 아이템을 찾지 못했습니다."), *ItemID.ToString());
			}*/
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
			//UE_LOG(LogTemp, Warning, TEXT("🧩 [Cheat] %s 캐릭터의 각성 조각 %d개 획득!"), *CharacterID.ToString(), Count);
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
		//UE_LOG(LogTemp, Warning, TEXT("💰 [Cheat] 9,999,999 골드 지급 완료!"));
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
			//UE_LOG(LogTemp, Warning, TEXT("👥 [Cheat] 모든 캐릭터 지급 완료!"));
		}

		//모든 퍼밀리어 지급
		if (GI->FamiliarStatsDataTable)
		{
			for (const FName& RowName : GI->FamiliarStatsDataTable->GetRowNames())
			{
				InvSys->AddFamiliar(RowName);
			}
			//UE_LOG(LogTemp, Warning, TEXT("🐾 [Cheat] 모든 퍼밀리어 지급 완료!"));
		}

		//모든 무기 지급
		if (GI->WeaponStatsDataTable)
		{
			for (const FName& RowName : GI->WeaponStatsDataTable->GetRowNames())
			{
				InvSys->AddItem(RowName, 1, 0); // 1개, 0강
			}
			//UE_LOG(LogTemp, Warning, TEXT("⚔️ [Cheat] 모든 무기 지급 완료!"));
		}

		//모든 방어구/장신구 지급
		if (GI->ArmorStatsDataTable)
		{
			for (const FName& RowName : GI->ArmorStatsDataTable->GetRowNames())
			{
				InvSys->AddItem(RowName, 1, 0); // 1개, 0강
			}
			//UE_LOG(LogTemp, Warning, TEXT("🛡️ [Cheat] 모든 방어구 및 장신구 지급 완료!"));
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
	case EParadiseLobbyMenu::Battle: TargetCamera = Camera_Main; break;
	case EParadiseLobbyMenu::StageMap: TargetCamera = Camera_Battle; break;
	case EParadiseLobbyMenu::Summon: TargetCamera = Camera_Summon; break;
		// 필요하면Enhance 등도 다른 카메라로 확장 가능
	default:                         TargetCamera = Camera_Main; break;
	}

	if (!TargetCamera) return;

	// 카메라 효과음
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_CameraMoveSwoosh)
		{
			// 기존에 재생 중인 카메라 이동 소리가 있다면 즉시 정지 (오버랩 방지)
			if (CameraSwooshAudioComp && CameraSwooshAudioComp->IsPlaying())
			{
				CameraSwooshAudioComp->Stop();
			}

			// 새롭게 소리를 틀고 제어권 확보
			CameraSwooshAudioComp = UGameplayStatics::SpawnSound2D(this, GI->GlobalAudioData->SFX_CameraMoveSwoosh);
		}
	}

	// 1. UI 먼저 숨기기 (연출을 위해)
	if (CachedLobbyHUD)
	{
		// HUD 함수: 이동 중엔 싹 다 숨겨라! (아래에서 구현 예정)
		CachedLobbyHUD->OnStartCameraMove();
	}

	if (CachedStageSelectWidget && TargetMenu != EParadiseLobbyMenu::StageMap)
	{
		CachedStageSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
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

void ALobbyPlayerController::StopCameraSwoosh()
{
	if (CameraSwooshAudioComp && CameraSwooshAudioComp->IsPlaying())
	{
		CameraSwooshAudioComp->Stop();
	}
}

void ALobbyPlayerController::SetLobbyHUD(UParadiseLobbyHUDWidget* InHUD)
{
    CachedLobbyHUD = InHUD;
}

void ALobbyPlayerController::OnCameraMoveFinished(EParadiseLobbyMenu TargetMenu)
{
	if (CameraSwooshAudioComp && CameraSwooshAudioComp->IsPlaying())
	{
		CameraSwooshAudioComp->Stop();
	}
	// 이동 완료! 이제 해당 메뉴 UI 띄우기
	SetLobbyMenu(TargetMenu);
}

void ALobbyPlayerController::SetLobbyMenu(EParadiseLobbyMenu InNewMenu)
{
    if (CurrentMenu == InNewMenu) return;

	// 메뉴를 변경하기 직전에 현재 메뉴를 '이전 메뉴'로 저장합니다.
	PreviousMenu = CurrentMenu;
    CurrentMenu = InNewMenu;

    // HUD에게 UI 변경 지시
    if (CachedLobbyHUD)
    {
		CachedLobbyHUD->SetVisibility(ESlateVisibility::Visible);
		CachedLobbyHUD->UpdateMenuStats(CurrentMenu);
    }

	// 2. 카메라가 지도로 가서 StageMap 상태가 되었을 때만 노드 뷰 띄우기
	if (CurrentMenu == EParadiseLobbyMenu::StageMap)
	{
		if (!CachedStageSelectWidget && StageSelectWidgetClass)
		{
			CachedStageSelectWidget = CreateWidget<UParadiseStageSelectWidget>(this, StageSelectWidgetClass);
			if (CachedStageSelectWidget)
			{
				CachedStageSelectWidget->AddToViewport(); // 스테이지 뷰는 전체화면
			}
		}

		if (CachedStageSelectWidget)
		{
			CachedStageSelectWidget->InitStageMap(CurrentSelectedChapter);
			CachedStageSelectWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	// 3. StageMap 상태가 아닐 때는 무조건 숨기기 (로비 복귀 시 꼬임 방지)
	else
	{
		if (CachedStageSelectWidget)
		{
			CachedStageSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
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
		//UE_LOG(LogTemp, Warning, TEXT("❌ [Gacha] 에테르가 부족합니다! (필요: %d)"), TotalAetherNeeded);
		return;
	}

	// 2. UI 숨기고 가챠 전용 카메라로 즉시 전환
	if (CachedLobbyHUD) CachedLobbyHUD->OnStartCameraMove();
	if (Camera_GachaAction) SetViewTargetWithBlend(Camera_GachaAction, 0.5f);

	// 3. 중복 검사용 캐릭터 ID 배열
	TArray<FName> OwnedCharacterIDs;
	for (const FOwnedCharacterData& CharData : InvSys->GetOwnedCharacters())
	{
		OwnedCharacterIDs.Add(CharData.CharacterID);
	}

	// 4. 가챠 확률 계산 실행
	TArray<FGachaResult> PulledResults = GachaSys->PerformGacha(DrawCount, OwnedCharacterIDs);

	// 5. 레벨에 캐싱된 박스 유효성 확인
	if (!CachedGachaBox.IsValid())
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [Gacha] 레벨에 BP_GachaBoxActor 가 없습니다! 레벨에 배치했는지 확인하세요."));
		return;
	}

	// 6. 델리게이트 중복 바인딩 방지 후 연출 시작
	CachedGachaBox->OnGachaResultScreenRequested.RemoveDynamic(
		this, &ALobbyPlayerController::OnShowGachaResultScreen);
	CachedGachaBox->OnGachaResultScreenRequested.AddDynamic(
		this, &ALobbyPlayerController::OnShowGachaResultScreen);

	CachedGachaBox->PlayGachaSequence(PulledResults);
}

void ALobbyPlayerController::ReturnFromGachaToSummon()
{
	// 1. 구슬 정리 + 박스 내부 상태 리셋 (박스 자신은 레벨에 유지 — 시퀀스 바인딩 보존)
	if (CachedGachaBox.IsValid())
	{
		CachedGachaBox->ResetState();
	}

	// 2. 결과창 숨기기
	if (CachedResultWidget)
	{
		CachedResultWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 3. ★ CurrentMenu 강제 초기화
	//    가챠 연출 중 CurrentMenu 는 Summon 인 채로 남아있으므로
	//    SetLobbyMenu 의 동일 메뉴 가드에 걸리지 않도록 None 으로 리셋합니다.
	CurrentMenu = EParadiseLobbyMenu::None;

	// 4. 카메라 블렌드 → SummonPopup 복귀
	MoveCameraToMenu(EParadiseLobbyMenu::Summon);
}

void ALobbyPlayerController::OnShowGachaResultScreen(const TArray<FGachaResult>& FinalResults)
{
	//UE_LOG(LogTemp, Log, TEXT("결과창을 띄우고 보상을 지급합니다."));

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

	// 3. 실제 보상 인벤토리에 확정 지급 로직
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (GI)
	{
		UInventorySystem* InvSys = GI->GetMainInventory();
		if (InvSys)
		{
			for (const FGachaResult& Result : FinalResults)
			{
				if (GI->IsValidItemID(Result.PulledItemID))
				{
					InvSys->AddItem(Result.PulledItemID, 1, 0);
				}
				else if (GI->IsValidPlayerID(Result.PulledItemID))
				{
					InvSys->AddCharacter(Result.PulledItemID);
				}
			}
			// 4. 모든 지급이 끝난 후 게임 자동 세이브!
			GI->SaveGameData();
			//UE_LOG(LogTemp, Log, TEXT("가챠 보상 지급 및 게임 저장 완료!"));
		}
	}
}

#pragma region 챕터 및 스테이지 제어 구현
void ALobbyPlayerController::EnterChapterMap(int32 ChapterID, UTexture2D* MapTexture)
{
	CurrentSelectedChapter = ChapterID;

	// 환경 액터 찾기 및 텍스처 교체
	if (!CachedMapEnvActor)
	{
		CachedMapEnvActor = Cast<AParadiseMapEnvironmentActor>(
			UGameplayStatics::GetActorOfClass(this, AParadiseMapEnvironmentActor::StaticClass())
		);
	}

	if (CachedMapEnvActor)
	{
		CachedMapEnvActor->ChangeMapBackground(MapTexture);
	}

	//  카메라 이동 명령 (완료 시 OnCameraMoveFinished가 자동으로 호출됨)
	MoveCameraToMenu(EParadiseLobbyMenu::StageMap);
}
#pragma endregion 챕터 및 스테이지 제어 구현