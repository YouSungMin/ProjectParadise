// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/ActionControlPanel.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "UI/Widgets/InGame/SkillSlotWidget.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/SquadSubsystem.h"

#include "Characters/Player/PlayerData.h"
#include "Characters/Base/PlayerBase.h"

#include "Components/SquadControlComponent.h"
#include "Components/SkillIndicatorComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Components/EquipmentComponent.h"
#include "Components/UltimateEffectComponent.h"

#include "Engine/Texture2D.h"
#include "Data/Structs/UnitStructs.h"

void UActionControlPanel::NativeConstruct()
{
	Super::NativeConstruct();

#pragma region 태그 버튼 배열화 및 캐싱

	// 최적화: 루프 처리를 위해 개별 바인딩된 버튼들을 배열에 담습니다.
	TagButtons.Empty();

	if (TagBtn_A) TagButtons.Add(TagBtn_A);
	if (TagBtn_B) TagButtons.Add(TagBtn_B);
	if (TagBtn_C) TagButtons.Add(TagBtn_C);

	for (int32 i = 0; i < TagButtons.Num(); ++i)
	{
		if (TagButtons[i])
		{
			TagButtons[i]->OnClicked().RemoveAll(this); // 안전장치 (중복 바인딩 방지)
			TagButtons[i]->OnClicked().AddUObject(this, &UActionControlPanel::OnTagButtonClicked, i);
		}
	}
#pragma endregion 태그 버튼 배열화 및 캐싱


	//0317 김성현 - 어빌리티 사거리 및 사용 취소등의 기능 구현을 위한 로직 변경
	// 2. 기본 공격 버튼 바인딩 (Common UI의 기본 OnPressed/OnReleased 사용)
	if (AttackBtn)
	{
		AttackBtn->OnPressed().RemoveAll(this);
		AttackBtn->OnPressed().AddUObject(this, &UActionControlPanel::OnAttackButtonPressed);

		AttackBtn->OnReleased().RemoveAll(this);
		AttackBtn->OnReleased().AddUObject(this, &UActionControlPanel::OnAttackButtonReleased);
	}

	// 3. 액티브 스킬 슬롯 바인딩 (이전에 추가한 OnSkillPressed/Released 델리게이트 사용)
	if (SkillSlot_Active)
	{
		SkillSlot_Active->OnSkillPressed.RemoveDynamic(this, &UActionControlPanel::OnActiveSkillPressed);
		SkillSlot_Active->OnSkillPressed.AddDynamic(this, &UActionControlPanel::OnActiveSkillPressed);

		SkillSlot_Active->OnSkillReleased.RemoveDynamic(this, &UActionControlPanel::OnActiveSkillReleased);
		SkillSlot_Active->OnSkillReleased.AddDynamic(this, &UActionControlPanel::OnActiveSkillReleased);
	}

	// 4. 궁극기 스킬 슬롯 바인딩
	if (SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->OnSkillPressed.RemoveDynamic(this, &UActionControlPanel::OnUltimateSkillPressed);
		SkillSlot_Ultimate->OnSkillPressed.AddDynamic(this, &UActionControlPanel::OnUltimateSkillPressed);

		SkillSlot_Ultimate->OnSkillReleased.RemoveDynamic(this, &UActionControlPanel::OnUltimateSkillReleased);
		SkillSlot_Ultimate->OnSkillReleased.AddDynamic(this, &UActionControlPanel::OnUltimateSkillReleased);
	}


	//// 2. 기본 공격 버튼 바인딩
	//if (AttackBtn)
	//{
	//	// Common UI의 OnSelected 혹은 OnClicked를 사용합니다.
	//	AttackBtn->OnClicked().RemoveAll(this);
	//	AttackBtn->OnClicked().AddUObject(this, &UActionControlPanel::OnAttackButtonClicked);
	//	UE_LOG(LogTemp, Warning, TEXT(" 공격 키 바인드 됨 "));
	//}

	//// 3. 액티브 스킬 슬롯 바인딩
	//if (SkillSlot_Active)
	//{
	//	SkillSlot_Active->OnSkillActionRequested.RemoveDynamic(this, &UActionControlPanel::OnActiveSkillRequested);
	//	SkillSlot_Active->OnSkillActionRequested.AddDynamic(this, &UActionControlPanel::OnActiveSkillRequested);
	//}

	////  4. 궁극기 스킬 슬롯 바인딩
	//if (SkillSlot_Ultimate)
	//{
	//	SkillSlot_Ultimate->OnSkillActionRequested.RemoveDynamic(this, &UActionControlPanel::OnUltimateSkillRequested);
	//	SkillSlot_Ultimate->OnSkillActionRequested.AddDynamic(this, &UActionControlPanel::OnUltimateSkillRequested);
	//}

	if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
	{
		if (UAutoCombatComponent* AutoComp = InGamePC->GetAutoCombatComponent())
		{
			AutoComp->OnAutoBattleStateChanged.RemoveDynamic(this, &UActionControlPanel::HandleAutoBattleStateChanged);
			AutoComp->OnAutoBattleStateChanged.AddDynamic(this, &UActionControlPanel::HandleAutoBattleStateChanged);

			// 현재 상태 바로 반영
			HandleAutoBattleStateChanged(AutoComp->IsAutoMode());
		}
	}

	// 5. 로비 데이터 연동 초기화
	//InitTagButtons();
}

void UActionControlPanel::NativeDestruct()
{
	//0317 김성현 - 어빌리티 사거리 및 사용 취소등의 기능 구현을 위한 로직 변경
	if (AttackBtn)
	{
		AttackBtn->OnPressed().RemoveAll(this);
		AttackBtn->OnReleased().RemoveAll(this);
	}
	if (SkillSlot_Active)
	{
		SkillSlot_Active->OnSkillPressed.RemoveAll(this);
		SkillSlot_Active->OnSkillReleased.RemoveAll(this);
	}
	if (SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->OnSkillPressed.RemoveAll(this);
		SkillSlot_Ultimate->OnSkillReleased.RemoveAll(this);
	}

	// 위젯 파괴 시 남아있는 포인터들을 깔끔하게 정리 (메모리 릭 원천 차단)
	/*if (AttackBtn) AttackBtn->OnClicked().RemoveAll(this);
	if (SkillSlot_Active) SkillSlot_Active->OnSkillActionRequested.RemoveAll(this);
	if (SkillSlot_Ultimate) SkillSlot_Ultimate->OnSkillActionRequested.RemoveAll(this);*/

	for (TObjectPtr<UCommonButtonBase> Btn : TagButtons)
	{
		if (Btn) Btn->OnClicked().RemoveAll(this);
	}

	TagButtons.Empty();
	CachedPlayer = nullptr;

	Super::NativeDestruct();
}

void UActionControlPanel::HandleAutoBattleStateChanged(bool bIsAuto)
{
	// 오토가 켜지면(true) 상호작용은 꺼져야(false) 합니다.
	const bool bEnableCombatUI = !bIsAuto;

	if (AttackBtn) AttackBtn->SetIsEnabled(bEnableCombatUI);
	if (SkillSlot_Active) SkillSlot_Active->SetIsEnabled(bEnableCombatUI);
	if (SkillSlot_Ultimate) SkillSlot_Ultimate->SetIsEnabled(bEnableCombatUI);

	for (TObjectPtr<UParadiseCommonButton> TagBtn : TagButtons)
	{
		if (TagBtn) TagBtn->SetIsEnabled(bEnableCombatUI);
	}

	UE_LOG(LogTemp, Log, TEXT("[ActionPanel] 오토 모드 상태 변경 수신! 액션 UI %s"), bEnableCombatUI ? TEXT("잠금 해제") : TEXT("잠금 처리"));
}

#pragma region 외부 인터페이스 구현
void UActionControlPanel::RefreshActionPanel(int32 PlayerIndex)
{
	// 1. 시스템 캐싱 (위젯 내부에서 스스로 가져옵니다)
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	AInGamePlayerState* PS = Cast<AInGamePlayerState>(GetOwningPlayerState());

	if (!GI || !PS) return;

	FDataTableRowHandle CurrentWeaponSkillHandle;
	FDataTableRowHandle CurrentUltimateHandle;
	UTexture2D* TargetAttackIcon = Tex_DefaultAttackIcon.Get(); // 기본 아이콘으로 초기화

	// 2. [데이터 드리븐] 영혼 데이터(Soul)로부터 장착 및 스탯 정보 추출
	if (APlayerData* Soul = PS->GetSquadMemberData(PlayerIndex))
	{
		// A. 캐릭터 스탯 테이블로부터 궁극기(Ultimate) ID 획득
		if (const FCharacterStats* CharStats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, Soul->CharacterID))
		{
			CurrentUltimateHandle = CharStats->UltimateActionHandle;
		}

		// B. 장비 컴포넌트로부터 무기 스킬(Weapon Skill) ID 획득
		if (UEquipmentComponent* EquipComp = Soul->GetEquipmentComponent())
		{
			const FName EquippedWeaponID = EquipComp->GetEquippedItemID(EEquipmentSlot::Weapon);

			if (EquippedWeaponID != NAME_None)
			{
				if (const FWeaponStats* WeaponStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, EquippedWeaponID))
				{
					CurrentWeaponSkillHandle = WeaponStats->SkillActionHandle;
				}
			}
		}
	}

	// 3. 추출된 데이터를 자신의 패널에 주입
	InitActionPanel(CurrentWeaponSkillHandle, CurrentUltimateHandle, TargetAttackIcon);
	UpdateTagButtons(PlayerIndex);

	UE_LOG(LogTemp, Log, TEXT("[ActionPanel] UI 자율 갱신 성공 (Index: %d)"), PlayerIndex);
}

void UActionControlPanel::InitActionPanel(FDataTableRowHandle WeaponActionHandle, FDataTableRowHandle UltimateActionHandle, UTexture2D* AttackIcon)
{
	// 게임 인스턴스의 공용 데이터 테이블 로직을 활용합니다.
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	CachedWeaponActionHandle = WeaponActionHandle;
	CachedUltimateActionHandle = UltimateActionHandle;

	/** @section 1. 무기 스킬 (일반 스킬) 데이터 연동 */
	if (AttackBtn && AttackIcon)
	{
		AttackBtn->SetButtonIcon(AttackIcon);
	}

	if (!WeaponActionHandle.IsNull())
	{
		 if (const FActionStats* WeaponActionData = WeaponActionHandle.GetRow<FActionStats>(TEXT("WeaponActionLookup")))
		 {
		 	if (SkillSlot_Active)
		 	{
				//SkillSlot_Active->SetSkillIcon(WeaponActionData->SkillIcon);
		 	}
		 }
		 UE_LOG(LogTemp, Log, TEXT("✅ [UI] 액티브 스킬 연결 완료: %s"), *WeaponActionHandle.RowName.ToString());
	}

	/** @section 2. 캐릭터 스킬 (궁극기) 데이터 연동 */
	if (!UltimateActionHandle.IsNull())
	{
		if (const FActionStats* UltimateActionData = UltimateActionHandle.GetRow<FActionStats>(TEXT("AttackIndicatorLookup")))
		{
			if (SkillSlot_Ultimate)
			{
				//SkillSlot_Ultimate->SetSkillIcon(UltimateActionData->SkillIcon);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("✅ [UI] 궁극기 연결 완료: %s"), *UltimateActionHandle.RowName.ToString());
	}
}

void UActionControlPanel::InitTagButtons()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	USquadSubsystem* SquadSys = GI ? GI->GetSubsystem<USquadSubsystem>() : nullptr;
	if (!SquadSys || !GI) return;

	// 서브시스템에서 편성된 캐릭터 ID 목록 획득
	const TArray<FName>& PlayerSquad = SquadSys->GetPlayerSquad();

	for (int32 i = 0; i < TagButtons.Num(); ++i)
	{
		if (!TagButtons.IsValidIndex(i) || !TagButtons[i]) continue;

		// 편성이 존재하는 슬롯인 경우
		if (PlayerSquad.IsValidIndex(i) && !PlayerSquad[i].IsNone())
		{
			FName CharID = PlayerSquad[i];
			if (FCharacterAssets* Asset = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharID))
			{
				// UParadiseCommonButton의 캡슐화된 인터페이스를 통해 아이콘 세팅
				UTexture2D* FaceIcon = Asset->FaceIcon.LoadSynchronous();

				TagButtons[i]->SetButtonIcon(FaceIcon);
				TagButtons[i]->SetVisibility(ESlateVisibility::Visible);

				if (i == 0 && AttackBtn)
				{
					// TODO: FCharacterAssets에 WeaponIcon 필드 추가 후 Asset->WeaponIcon.LoadSynchronous()로 교체
					UTexture2D* AttackIcon = Tex_DefaultAttackIcon.Get();
					AttackBtn->SetButtonIcon(AttackIcon);
				}
			}
		}
		else
		{
			// 편성되지 않은 빈 슬롯은 UI에서 숨김 처리
			TagButtons[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 게임 시작 시 무조건 0번(메인) 캐릭터가 조작 중이므로, A버튼을 비활성화시킴
	UpdateTagButtons(0);
}

void UActionControlPanel::UpdateSkillCooldown(int32 SkillIndex, float CurrentTime, float MaxTime)
{
	// 기본 스킬 쿨타임 정보 호출
	if (SkillIndex == 0 && SkillSlot_Active)
	{
		SkillSlot_Active->RefreshCooldown(CurrentTime, MaxTime);
	}
	// 궁극기 쿨타임 정보 호출
	else if (SkillIndex == 1 && SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->RefreshCooldown(CurrentTime, MaxTime);
	}
}

void UActionControlPanel::UpdateTagButtons(int32 ActiveCharIndex)
{
	CurrentActiveTagIndex = ActiveCharIndex;

	/**
	 * @brief 현재 조작 캐릭터에 따른 태그 버튼 활성화/비활성화 제어
	 * @details 배열 루프를 사용하여 하드코딩 없이 상태를 일괄 업데이트합니다.
	 */
	for (int32 i = 0; i < TagButtons.Num(); ++i)
	{
		if (TagButtons[i])
		{
			// 현재 조작 중인 캐릭터(Active)는 뚜렷하게(true), 나머지는 연하게(false)
			const bool bIsCurrentlyActive = (i == ActiveCharIndex);
			TagButtons[i]->SetTagActiveState(bIsCurrentlyActive);
		}
	}
}
void UActionControlPanel::SetOwningPlayerBase(APlayerBase* InPlayer)
{
	CachedPlayer = InPlayer;
	UE_LOG(LogTemp, Log, TEXT("✅ [ActionPanel] 플레이어 폰 주입 완료!"));
}

void UActionControlPanel::OnAttackButtonPressed()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		float AttackRange = 100.0f;
		float AttackRadius = 100.0f;
		float ForwardOffset = 100.0f;

		if (!CachedWeaponActionHandle.IsNull())
		{
			if (const FActionStats* ActionData = CachedWeaponActionHandle.GetRow<FActionStats>(TEXT("SkillIndicatorLookup")))
			{
				AttackRange = (ActionData->AttackRange > 0.0f) ? ActionData->AttackRange : 0.0f;
				AttackRadius = (ActionData->AttackRadius > 0.0f) ? ActionData->AttackRadius : 0.0f;
				ForwardOffset = ActionData->ForwardOffset;
			}
		}

		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			AttackRange, AttackRadius, ForwardOffset);
	}
}

void UActionControlPanel::OnActiveSkillPressed()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		float SkillAttackRange = 100.0f;
		float SkillAttackRadius = 100.0f;
		float SkillForwardOffset = 100.0f;

		if (!CachedWeaponActionHandle.IsNull())
		{
			if (const FActionStats* ActionData = CachedWeaponActionHandle.GetRow<FActionStats>(TEXT("SkillIndicatorLookup")))
			{
				SkillAttackRange = (ActionData->AttackRange > 0.0f) ? ActionData->AttackRange : 0.0f;
				SkillAttackRadius = (ActionData->AttackRadius > 0.0f) ? ActionData->AttackRadius : 0.0f;
				SkillForwardOffset = ActionData->ForwardOffset;
			}
		}

		// 반경(Radius)과 앞으로 쏠린 거리(Offset)를 모두 전달!
		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			SkillAttackRange, SkillAttackRadius, SkillForwardOffset);
	}
}

void UActionControlPanel::OnUltimateSkillPressed()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		float UltimateAttackRange = 100.0f;
		float UltimateAttackRadius = 100.0f;
		float UltimateForwardOffset = 100.0f;

		if (!CachedUltimateActionHandle.IsNull())
		{
			if (const FActionStats* UltimateActionData = CachedUltimateActionHandle.GetRow<FActionStats>(TEXT("UltIndicatorLookup")))
			{
				UltimateAttackRange = (UltimateActionData->AttackRange > 0.0f) ? UltimateActionData->AttackRange : 0.0f;
				UltimateAttackRadius = (UltimateActionData->AttackRadius > 0.0f) ? UltimateActionData->AttackRadius : 0.0f;
				UltimateForwardOffset = UltimateActionData->ForwardOffset;
			}
		}

		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			UltimateAttackRange, UltimateAttackRadius, UltimateForwardOffset );
	}
}

void UActionControlPanel::OnAttackButtonReleased()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		CurrentActivePawn->GetSkillIndicatorComponent()->HideIndicator();
	}
	ProcessAbilityInput(EInputID::Attack);
}

void UActionControlPanel::OnActiveSkillReleased()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		CurrentActivePawn->GetSkillIndicatorComponent()->HideIndicator();
	}
	ProcessAbilityInput(EInputID::Skill);
}

void UActionControlPanel::OnUltimateSkillReleased()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		CurrentActivePawn->GetSkillIndicatorComponent()->HideIndicator();
	}
	ProcessAbilityInput(EInputID::Ultimate); // 진짜 발동
}

//void UActionControlPanel::OnAttackButtonClicked()
//{
//	ProcessAbilityInput(EInputID::Attack);
//}
//
//void UActionControlPanel::OnActiveSkillRequested()
//{
//	UE_LOG(LogTemp, Log, TEXT("스킬키 입력 들어옴"));
//	ProcessAbilityInput(EInputID::Skill);
//}
//
//void UActionControlPanel::OnUltimateSkillRequested()
//{
//	UE_LOG(LogTemp, Log, TEXT("궁극키 입력 들어옴"));
//	ProcessAbilityInput(EInputID::Ultimate);
//}

void UActionControlPanel::ProcessAbilityInput(EInputID InputID)
{
	//0303 김성현 - Fix 스위치 한 영웅이 어빌리티를 발동하지 않는 문제 수정
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());

	if (CurrentActivePawn)
	{
		CurrentActivePawn->SendAbilityInputToASC(InputID, true);
		// 2. 궁극기일때 컨트롤러에 화면연출을 틀어라 라고 호출
		if (InputID == EInputID::Ultimate)
		{
			if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
			{
				if (UUltimateEffectComponent* UltEffectComp = InGamePC->GetUltimateEffectComponent())
				{
					// 연출 시간은 Data-Driven하게 스탯 테이블에서 가져올 수도 있지만, 일단 2.5초로 세팅
					UltEffectComp->PlayUltimateEffect(2.5f);
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[ActionPanel] 현재 빙의된 %s가 어빌리티를 발동합니다."), *CurrentActivePawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ActionPanel] 현재 플레이어 폰을 찾을 수 없습니다. (스폰/빙의 문제)"));
	}
}

void UActionControlPanel::OnTagButtonClicked(int32 CharacterIndex)
{
	if (CharacterIndex == CurrentActiveTagIndex) return;

	if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
	{
		// 2. 컨트롤러에 구현해두신 안전한 캐릭터 교체 및 AI 배정 로직 실행
		//0306 김성현 - 컨트롤러 로직 분리 작업
		InGamePC->GetSquadControlComponent()->RequestSwitchPlayer(CharacterIndex);

		// 3. 교체가 완료되었으므로 버튼 상태 즉시 갱신 (자신은 비활성화, 나머진 활성화)
		UpdateTagButtons(CharacterIndex);
	}
}
#pragma endregion 외부 인터페이스 구현