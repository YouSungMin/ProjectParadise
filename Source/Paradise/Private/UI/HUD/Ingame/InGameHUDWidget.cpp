// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Ingame/InGameHUDWidget.h"

#include "UI/Panel/Ingame/ActionControlPanel.h"
#include "UI/Panel/Ingame/PartyStatusPanel.h"
#include "UI/Panel/Ingame/SummonControlPanel.h"

#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "UI/Widgets/Ingame/VirtualJoystickWidget.h"
#include "UI/Widgets/Ingame/CharacterStatusWidget.h"
#include "UI/Widgets/Ingame/GameTimerWidget.h"
#include "UI/Widgets/Ingame/Popup/VictoryPopupWidget.h"
#include "UI/Widgets/Ingame/Popup/DefeatPopupWidget.h"
#include "UI/Widgets/Setting/SettingsPopupWidget.h"

#include "Framework/InGame/InGameGameState.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/System/InventorySystem.h"
#include "Framework/Core/ParadiseGameInstance.h"

#include "Camera/CameraComponent.h"


#include "Data/Structs/UnitStructs.h"
#include "Data/Structs/GrowthStruct.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

#pragma region 초기화 (Initialization)
	// 1. 결과 팝업 초기화 (숨김 상태로 시작)
	if (Widget_VictoryPopup) Widget_VictoryPopup->SetVisibility(ESlateVisibility::Collapsed);
	if (Widget_DefeatPopup) Widget_DefeatPopup->SetVisibility(ESlateVisibility::Collapsed);

	// 2. 버튼 이벤트 바인딩
	if (Btn_Setting)
	{
		Btn_Setting->OnClicked().AddUObject(this, &UInGameHUDWidget::OnSettingButtonClicked);
	}

	if (Btn_AutoMode)
	{
		Btn_AutoMode->OnClicked().AddUObject(this, &UInGameHUDWidget::OnAutoModeButtonClicked);
		// 초기 텍스트 설정 (필요 시)
		// Btn_AutoMode->SetButtonText(FText::FromString(TEXT("MANUAL")));
	}

	// 3. 조이스틱 바인딩
	if (VirtualJoystick)
	{
		VirtualJoystick->OnJoystickInput.AddDynamic(this, &UInGameHUDWidget::OnJoystickInput);
	}

	// 4. 설정 팝업 객체 풀링(캐싱)
	if (SettingsPopupClass && !SettingsPopupInstance)
	{
		SettingsPopupInstance = CreateWidget<USettingsPopupWidget>(GetOwningPlayer(), SettingsPopupClass);
		if (SettingsPopupInstance)
		{
			SettingsPopupInstance->AddToViewport(100);
			SettingsPopupInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 5. 시스템 초기화 및 데이터 동기화
	InitializeHUD();
#pragma endregion 초기화

	// 6. UI 갱신 타이머 시작 (최적화를 위해 0.5초 간격 유지)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			HUDUpdateTimerHandle,
			this,
			&UInGameHUDWidget::OnUpdateHUD,
			0.5f,
			true
		);
	}
}

void UInGameHUDWidget::NativeDestruct()
{
	// 1. 타이머 정지
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HUDUpdateTimerHandle);
	}

	// 2. 델리게이트 안전 해제
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnGamePhaseChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

#pragma region 내부 로직 구현
void UInGameHUDWidget::InitializeHUD()
{
	// GameState를 가져와서 캐싱 (매번 Cast하지 않기 위함)
	AInGameGameState* GS = Cast<AInGameGameState>(UGameplayStatics::GetGameState(this));
	if (GS)
	{
		CachedGameState = GS;

		// 페이즈 변경 이벤트 바인딩 ("상태 바뀌면 연락해!")
		GS->OnGamePhaseChanged.AddUniqueDynamic(this, &UInGameHUDWidget::HandleGamePhaseChanged);

		// 현재 상태 즉시 반영 (이미 게임 진행 중일 경우 대비)
		HandleGamePhaseChanged(GS->CurrentPhase);
	}
}

void UInGameHUDWidget::HandleGamePhaseChanged(EGamePhase NewPhase)
{
	if (!CachedGameState.IsValid()) return;

	UE_LOG(LogTemp, Log, TEXT("[InGameHUD] 페이즈 변경 감지: %d"), (int32)NewPhase);

	// 1. 결과 관련 상태 판별 (정산 혹은 종료 단계인지 확인)
	const bool bIsFinishing = (NewPhase == EGamePhase::Victory || NewPhase == EGamePhase::Defeat || NewPhase == EGamePhase::Result);

	// 2. 인게임 조작 인터페이스 일괄 가시성 제어 (최적화)
	ESlateVisibility ControlUIVis = bIsFinishing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;

	if (VirtualJoystick)   VirtualJoystick->SetVisibility(ControlUIVis);
	if (GameTimerWidget)   GameTimerWidget->SetVisibility(ControlUIVis);
	if (ActionControlPanel) ActionControlPanel->SetVisibility(ControlUIVis);
	if (SummonControlPanel) SummonControlPanel->SetVisibility(ControlUIVis);
	if (PartyStatusPanel)  PartyStatusPanel->SetVisibility(ControlUIVis);
	if (Btn_AutoMode)      Btn_AutoMode->SetVisibility(ControlUIVis);
	if (Btn_Setting)       Btn_Setting->SetVisibility(ControlUIVis);

	// 3. 단계별 팝업 노출 처리 (상태 머신)
	switch (NewPhase)
	{
	case EGamePhase::Victory:
	case EGamePhase::Defeat:
		// 연출 대기 시간(3초) 동안에는 팝업을 띄우지 않고 확실히 숨겨둡니다.
		if (Widget_VictoryPopup) Widget_VictoryPopup->SetVisibility(ESlateVisibility::Collapsed);
		if (Widget_DefeatPopup)  Widget_DefeatPopup->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case EGamePhase::Result:
		// 모든 정산이 끝난 '진짜' 결과 시점
		if (CachedGameState.IsValid())
		{
			// 골드 보상이 0보다 크면 승리로 간주 (또는 별점 로직 연동)
			bool bIsActuallyVictory = (CachedGameState->AcquiredGold > 0);

			if (bIsActuallyVictory)
			{
				if (Widget_VictoryPopup)
				{
					// 유지된 정산 함수 호출!
					UpdateVictoryPopupData();
					Widget_VictoryPopup->SetVisibility(ESlateVisibility::Visible);
				}
				if (Widget_DefeatPopup) Widget_DefeatPopup->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				// 패배 시 정산 없이 패배창만 띄움
				if (Widget_DefeatPopup)  Widget_DefeatPopup->SetVisibility(ESlateVisibility::Visible);
				if (Widget_VictoryPopup) Widget_VictoryPopup->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		break;

	default:
		// 전투 중 혹은 대기 중에는 모든 결과창 강제 숨김
		if (Widget_VictoryPopup) Widget_VictoryPopup->SetVisibility(ESlateVisibility::Collapsed);
		if (Widget_DefeatPopup)  Widget_DefeatPopup->SetVisibility(ESlateVisibility::Collapsed);
		break;
	}
}

void UInGameHUDWidget::UpdateVictoryPopupData()
{
	if (!Widget_VictoryPopup || !CachedGameState.IsValid()) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. 게임 스테이트에서 최종 확정된 보상값 추출
	FText StageName = FText::FromString(CachedGameState->DisplayStageName);
	int32 Gold = CachedGameState->AcquiredGold;
	int32 Aether = CachedGameState->AcquiredAether;
	int32 Exp = CachedGameState->AcquiredExp;
	FName NextStage = CachedGameState->NextStageID;
	int32 StarCount = 3; // TODO: GameState의 별점 로직 연동 예정

	// 2. 참여한 캐릭터 데이터 수집 (최신 인벤토리 동기화)
	TArray<FResultCharacterData> CharResults;
	USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>();
	UInventorySystem* InvSys = GI->GetSubsystem<UInventorySystem>();

	if (SquadSys && InvSys)
	{
		for (const FName& HeroID : SquadSys->GetPlayerSquad())
		{
			if (HeroID.IsNone()) continue;

			FResultCharacterData Result;
			Result.CharacterName = FText::FromName(HeroID);
			Result.GainedExp = Exp;

			// 초상화 데이터 로드
			if (auto* AssetData = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, HeroID))
			{
				Result.PortraitImage = AssetData->FaceIcon.LoadSynchronous();
			}

			// 최신 경험치 반영율(%) 계산 (Data-Driven)
			if (const FOwnedCharacterData* OwnedData = InvSys->GetCharacterDataByID(HeroID))
			{
				FName NextRow = FName(*FString::FromInt(OwnedData->Level + 1));
				if (auto* LvData = GI->GetDataTableRow<FCharacterLevelUpData>(GI->CharacterLevelUpDataTable, NextRow))
				{
					float SafeRequired = FMath::Max(1.0f, (float)LvData->RequiredExp);
					Result.ExpPercent = FMath::Clamp((float)OwnedData->CurrentExp / SafeRequired, 0.0f, 1.0f);
				}
				else
				{
					Result.ExpPercent = 1.0f; // 만렙 도달 시
				}
			}
			CharResults.Add(Result);
		}
	}

	// 3. 조립된 데이터를 뷰(Popup)에 전달
	Widget_VictoryPopup->SetVictoryData(StageName, StarCount, Gold, Aether, CharResults, NextStage);
}

void UInGameHUDWidget::OnSettingButtonClicked()
{
	/** @section 팝업 열기 (인게임은 열릴 때 시간이 멈춥니다!) */
	if (SettingsPopupInstance)
	{
		SettingsPopupInstance->OpenSettings();
	}
}

void UInGameHUDWidget::OnAutoModeButtonClicked()
{
	// 1. 상태 토글
	bIsAutoMode = !bIsAutoMode;

	// 2. UI 텍스트/아이콘 변경 (피드백)
	if (Btn_AutoMode)
	{
		// 텍스트 변경 예시 (아이콘 교체로 해도 됨)
		FText StatusText = bIsAutoMode ? FText::FromString(TEXT("AUTO")) : FText::FromString(TEXT("MANUAL"));
		Btn_AutoMode->SetButtonText(StatusText);
	}

	// 3. 실제 게임플레이 로직에 알림 (Controller or PlayerState)
	// GetOwningPlayer()->GetComponent<UAutoCombatComponent>()->SetAutoMode(bIsAutoMode);
	UE_LOG(LogTemp, Log, TEXT("Auto Mode Toggled: %s"), bIsAutoMode ? TEXT("ON") : TEXT("OFF"));
}

void UInGameHUDWidget::OnJoystickInput(FVector2D InputVector)
{
	// 현재 조종 중인 캐릭터를 가져옵니다.
	if (APawn* OwnedPawn = GetOwningPlayerPawn())
	{
		// 캐릭터에 붙어있는 카메라 컴포넌트를 직접 찾습니다. (유저님 방식 적용! 가장 안전함)
		if (UCameraComponent* CameraComp = OwnedPawn->FindComponentByClass<UCameraComponent>())
		{
			// 카메라 매니저의 '블렌딩되는 회전'이 아닌, 카메라 컴포넌트의 '고정된 절대 회전값' 사용
			const FRotator CameraRot = CameraComp->GetComponentRotation();
			const FRotator YawRot(0, CameraRot.Yaw, 0);

			const FVector CameraForward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			const FVector CameraRight = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

			//UMG 조이스틱 보정 (위로 올릴 때 Y가 음수이므로 -1 곱하기)
			OwnedPawn->AddMovementInput(CameraForward, InputVector.Y * -1.0f);
			OwnedPawn->AddMovementInput(CameraRight, InputVector.X);
		}
	}

	// 조이스틱 입력이 오면 폰(캐릭터)에게 이동 명령 전달
	//if (APawn* OwnedPawn = GetOwningPlayerPawn())
	//{
	//	// 조이스틱에서 넘어온 순수 입력값을 90도 회전하여 보정
	//	FVector2D TransformedInput;
	//	TransformedInput.X = InputVector.Y;
	//	TransformedInput.Y = -InputVector.X;

	//	const FRotator ControlRot = GetOwningPlayer()->GetControlRotation();
	//	const FRotator YawRot(0, ControlRot.Yaw, 0);

	//	const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	//	const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	//	// 보정된 벡터(TransformedInput)를 기준으로 캐릭터 이동 적용
	//	OwnedPawn->AddMovementInput(ForwardDir, TransformedInput.Y * -1.0f);
	//	OwnedPawn->AddMovementInput(RightDir, TransformedInput.X);
	//}
}

void UInGameHUDWidget::OnUpdateHUD()
{
	// [최적화] 타이머에 의해 0.5초마다 호출됨
	if (CachedGameState.IsValid())
	{
		// 타이머 위젯이 있고, 게임 상태가 타이머가 돌아가는 상태일 때만 갱신
		if (GameTimerWidget)
		{
			// GameState에 남은 시간 변수가 public이라고 가정
			GameTimerWidget->UpdateTime(CachedGameState->RemainingTime);
		}
	}
}
#pragma endregion 내부 로직 구현