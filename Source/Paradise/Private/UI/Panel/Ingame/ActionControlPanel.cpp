// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/ActionControlPanel.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "UI/Widgets/InGame/SkillSlotWidget.h"

#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/System/SquadSubsystem.h"
#include "Framework/Core/ParadiseCameraManager.h"

#include "Characters/Player/PlayerData.h"
#include "Characters/Base/PlayerBase.h"

#include "Components/SquadControlComponent.h"
#include "Components/SkillIndicatorComponent.h"
#include "Components/AutoCombatComponent.h"
#include "Components/EquipmentComponent.h"
#include "Components/UltimateEffectComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include "GAS/Attributes/BaseAttributeSet.h"
#include "Engine/Texture2D.h"
#include "Data/Structs/UnitStructs.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Paradise/Paradise.h"

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

	// 태그 쿨타임 바 배열화
	TagCooldownBars.Empty();
	if (PB_TagCooldown_A) TagCooldownBars.Add(PB_TagCooldown_A);
	if (PB_TagCooldown_B) TagCooldownBars.Add(PB_TagCooldown_B);
	if (PB_TagCooldown_C) TagCooldownBars.Add(PB_TagCooldown_C);
	// 태그 쿨타임 텍스트
	TagCooldownTexts.Empty();
	if (Text_TagCooldown_A) TagCooldownTexts.Add(Text_TagCooldown_A);
	if (Text_TagCooldown_B) TagCooldownTexts.Add(Text_TagCooldown_B);
	if (Text_TagCooldown_C) TagCooldownTexts.Add(Text_TagCooldown_C);

	ClearTagCooldownVisual();

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

	for (TObjectPtr<UCommonButtonBase> Btn : TagButtons)
	{
		if (Btn) Btn->OnClicked().RemoveAll(this);
	}

	TagButtons.Empty();
	CachedPlayer = nullptr;
	TagCooldownBars.Empty();
	TagCooldownTexts.Empty();
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TagVisual);
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

	//UE_LOG(LogTemp, Log, TEXT("[ActionPanel] 오토 모드 상태 변경 수신! 액션 UI %s"), bEnableCombatUI ? TEXT("잠금 해제") : TEXT("잠금 처리"));
}

void UActionControlPanel::UpdateTagCooldownVisual()
{
	// 0.05초 간격으로 깎음
	CurrentTagCooldown -= 0.05f;

	if (CurrentTagCooldown <= 0.0f)
	{
		ClearTagCooldownVisual();
		return;
	}

	// 퍼센트 계산
	const float Percent = FMath::Clamp(CurrentTagCooldown / MaxTagCooldown, 0.0f, 1.0f);
	const int32 RemainSeconds = FMath::CeilToInt(CurrentTagCooldown);

	// 3개의 프로그레스 바를 동시에 깎아줍니다.
	for (UProgressBar* PB : TagCooldownBars)
	{
		if (PB) PB->SetPercent(Percent);
	}
	// 3개의 쿨타임 텍스트 숫자 갱신
	for (UTextBlock* Text : TagCooldownTexts)
	{
		if (Text) Text->SetText(FText::AsNumber(RemainSeconds));
	}
}

void UActionControlPanel::ClearTagCooldownVisual()
{
	CurrentTagCooldown = 0.0f;

	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TagVisual);

	// 3개의 프로그레스 바 초기화 및 숨김
	for (UProgressBar* PB : TagCooldownBars)
	{
		if (PB)
		{
			PB->SetPercent(0.0f);
			PB->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// 3개의 텍스트 초기화 및 숨김
	for (UTextBlock* Text : TagCooldownTexts)
	{
		if (Text)
		{
			Text->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#pragma region 외부 인터페이스 구현
void UActionControlPanel::RefreshActionPanel(int32 PlayerIndex)
{
	LockOtherActionButtons(false, ECombatActionType::BasicAttack);

	// 1. 시스템 캐싱 (위젯 내부에서 스스로 가져옵니다)
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	AInGamePlayerState* PS = Cast<AInGamePlayerState>(GetOwningPlayerState());

	if (!GI || !PS) return;

	FDataTableRowHandle CurrentWeaponAttackHandle;
	FDataTableRowHandle CurrentWeaponSkillHandle;
	FDataTableRowHandle CurrentUltimateHandle;

	UTexture2D* TargetAttackIcon = Tex_DefaultAttackIcon.Get(); // 기본 아이콘으로 초기화
	UTexture2D* TargetSkillIcon = nullptr;
	UTexture2D* TargetUltimateIcon = nullptr;

	// 2. [데이터 드리븐] 영혼 데이터(Soul)로부터 장착 및 스탯 정보 추출

	APlayerData* Soul = PS->GetSquadMemberData(PlayerIndex);

	// 마나 요구량 캐싱 초기화
	CachedActiveManaCost = 0.0f;
	CachedUltimateManaCost = 0.0f;

	if (Soul)
	{
		// A. 캐릭터 스탯 테이블로부터 궁극기(Ultimate) ID 획득
		if (const FCharacterStats* CharStats = GI->GetDataTableRow<FCharacterStats>(GI->CharacterStatsDataTable, Soul->CharacterID))
		{
			CurrentUltimateHandle = CharStats->UltimateActionHandle;
		}

		if (const FCharacterAssets* CharAssets = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, Soul->CharacterID))
		{
			if (!CharAssets->UltimateIcon.IsNull())
			{
				TargetUltimateIcon = CharAssets->UltimateIcon.LoadSynchronous();
			}
		}

		// B. 장비 컴포넌트로부터 무기 스킬(Weapon Skill) ID 획득
		if (UEquipmentComponent* EquipComp = Soul->GetEquipmentComponent())
		{
			const FName EquippedWeaponID = EquipComp->GetEquippedItemID(EEquipmentSlot::Weapon);

			if (EquippedWeaponID != NAME_None)
			{
				if (const FWeaponStats* WeaponStats = GI->GetDataTableRow<FWeaponStats>(GI->WeaponStatsDataTable, EquippedWeaponID))
				{
					CurrentWeaponAttackHandle = WeaponStats->BasicAttackActionHandle;
					CurrentWeaponSkillHandle = WeaponStats->SkillActionHandle;
				}

				if (const FWeaponAssets* WeaponAssets = GI->GetDataTableRow<FWeaponAssets>(GI->WeaponAssetsDataTable, EquippedWeaponID))
				{
					if (!WeaponAssets->WeaponBasicAttackIcon.IsNull())
					{
						TargetAttackIcon = WeaponAssets->WeaponBasicAttackIcon.LoadSynchronous();
					}
					if (!WeaponAssets->WeaponSkillIcon.IsNull())
					{
						TargetSkillIcon = WeaponAssets->WeaponSkillIcon.LoadSynchronous();
					}
				}
			}
		}
		// C. 엑셀(데이터 테이블)에서 마나 코스트 추출
		if (const FActionStats* ActionRow = CurrentWeaponSkillHandle.GetRow<FActionStats>(TEXT("SkillCost")))
		{
			CachedActiveManaCost = ActionRow->ManaCost;
		}

		if (const FActionStats* ActionRow = CurrentUltimateHandle.GetRow<FActionStats>(TEXT("UltCost")))
		{
			CachedUltimateManaCost = ActionRow->ManaCost;
		}

		// D. 마나 실시간 감시 바인딩 (이전 델리게이트 안전 해제)
		if (CachedASC.IsValid())
		{
			CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetManaAttribute()).RemoveAll(this);
		}

		CachedASC = Soul->GetAbilitySystemComponent();
		if (CachedASC.IsValid())
		{
			CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetManaAttribute()).AddUObject(this, &UActionControlPanel::OnManaChanged);

			// 패널이 켜지는 즉시 현재 마나량으로 스킬 사용 가능 여부 1회 갱신
			float CurrentMana = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetManaAttribute());
			UpdateSkillUsabilityByMana(CurrentMana);
		}
	}

	// 3. 추출된 데이터를 자신의 패널에 주입
	InitActionPanel(CurrentWeaponAttackHandle,CurrentWeaponSkillHandle, CurrentUltimateHandle, TargetAttackIcon);

	if (AttackBtn)
	{
		AttackBtn->SetButtonIcon(TargetAttackIcon);
	}

	// 동적으로 로드된 스킬 및 궁극기 아이콘을 슬롯에 적용 (쿨타임은 Handle에서 추출)
	if (SkillSlot_Active)
	{
		float MaxCD = 0.0f;
		if (FActionStats* ActionRow = CurrentWeaponSkillHandle.GetRow<FActionStats>(TEXT("UI_SkillCooldown_Lookup")))
		{
			MaxCD = ActionRow->Cooldown; // 데이터 테이블의 Cooldown 값!
		}

		// 먼저 기본 정보 갱신 (초기화)
		SkillSlot_Active->UpdateSlotInfo(TargetSkillIcon, MaxCD);

		// 태그-인 했을 때, 아까 기록해둔 쿨타임 종료 시간이 남아있다면?
		if (Soul && ActiveSkillEndTimes.Contains(Soul->CharacterID))
		{
			float Remaining = ActiveSkillEndTimes[Soul->CharacterID] - GetWorld()->GetTimeSeconds();
			if (Remaining > 0.0f)
			{
				SkillSlot_Active->RefreshCooldown(Remaining, MaxCD);
			}
		}
		/*float SkillCooldown = 0.0f;
		if (FActionStats* ActionRow = CurrentWeaponSkillHandle.GetRow<FActionStats>(TEXT("UI_SkillCooldown_Lookup")))
		{
			SkillCooldown = ActionRow->Cooldown;
		}
		SkillSlot_Active->UpdateSlotInfo(TargetSkillIcon, SkillCooldown);*/
	}

	if (SkillSlot_Ultimate)
	{
		float MaxCD = 0.0f;
		if (FActionStats* ActionRow = CurrentUltimateHandle.GetRow<FActionStats>(TEXT("UI_UltCooldown_Lookup")))
		{
			MaxCD = ActionRow->Cooldown; // 데이터 테이블의 Cooldown 값!
		}

		SkillSlot_Ultimate->UpdateSlotInfo(TargetUltimateIcon, MaxCD);

		if (Soul && UltimateEndTimes.Contains(Soul->CharacterID))
		{
			float Remaining = UltimateEndTimes[Soul->CharacterID] - GetWorld()->GetTimeSeconds();
			if (Remaining > 0.0f)
			{
				SkillSlot_Ultimate->RefreshCooldown(Remaining, MaxCD);
			}
		}
		/*float UltCooldown = 0.0f;
		if (FActionStats* ActionRow = CurrentUltimateHandle.GetRow<FActionStats>(TEXT("UI_UltCooldown_Lookup")))
		{
			UltCooldown = ActionRow->Cooldown;
		}
		SkillSlot_Ultimate->UpdateSlotInfo(TargetUltimateIcon, UltCooldown);*/
	}

	UpdateTagButtons(PlayerIndex);

	//UE_LOG(LogTemp, Log, TEXT("[ActionPanel] UI 자율 갱신 성공 (Index: %d)"), PlayerIndex);
}

void UActionControlPanel::InitActionPanel(FDataTableRowHandle WeaponAttackHandle, FDataTableRowHandle WeaponSkillHandle, FDataTableRowHandle UltimateActionHandle, UTexture2D* AttackIcon)
{
	// 게임 인스턴스의 공용 데이터 테이블 로직을 활용합니다.
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	CachedWeaponAttackActionHandle = WeaponAttackHandle;
	CachedWeaponSkillActionHandle = WeaponSkillHandle;
	CachedUltimateActionHandle = UltimateActionHandle;

	/** @section 1. 무기 스킬 (일반 스킬) 데이터 연동 */
	if (AttackBtn && AttackIcon)
	{
		AttackBtn->SetButtonIcon(AttackIcon);
	}

	if (!WeaponAttackHandle.IsNull())
	{
		if (const FActionStats* WeaponAttackActionData = WeaponAttackHandle.GetRow<FActionStats>(TEXT("WeaponAttackActionLookup")))
		{
			if (SkillSlot_Active)
			{
				//SkillSlot_Active->SetSkillIcon(WeaponActionData->SkillIcon);
			}
		}
		//UE_LOG(LogTemp, Log, TEXT("✅ [UI] 액티브 스킬 연결 완료: %s"), *WeaponSkillHandle.RowName.ToString());
	}

	if (!WeaponSkillHandle.IsNull())
	{
		 if (const FActionStats* WeaponSkillActionData = WeaponSkillHandle.GetRow<FActionStats>(TEXT("WeaponSkillActionLookup")))
		 {
		 	if (SkillSlot_Active)
		 	{
				//SkillSlot_Active->SetSkillIcon(WeaponActionData->SkillIcon);
		 	}
		 }
		// UE_LOG(LogTemp, Log, TEXT("✅ [UI] 액티브 스킬 연결 완료: %s"), *WeaponSkillHandle.RowName.ToString());
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
		//UE_LOG(LogTemp, Log, TEXT("✅ [UI] 궁극기 연결 완료: %s"), *UltimateActionHandle.RowName.ToString());
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
	//UE_LOG(LogTemp, Log, TEXT("✅ [ActionPanel] 플레이어 폰 주입 완료!"));
}

void UActionControlPanel::LockOtherActionButtons(bool bLocked, ECombatActionType ExecutingActionType)
{
	// 1. 잠금 해제(false)일 때: 내 타입이 뭐든 상관없이 그냥 모든 버튼을 다시 터치 가능하게 살립니다!
	if (!bLocked)
	{
		if (AttackBtn) AttackBtn->SetVisibility(ESlateVisibility::Visible);
		if (SkillSlot_Active) SkillSlot_Active->SetVisibility(ESlateVisibility::Visible);
		if (SkillSlot_Ultimate) SkillSlot_Ultimate->SetVisibility(ESlateVisibility::Visible);
		return; // 다 풀었으니 함수 즉시 종료 (최적화)
	}

	// 2. 잠금(true)일 때: 내가 누른 스킬(ExecutingActionType)이 아닌 녀석들만 투명하게(HitTestInvisible) 만듭니다.
	if (ExecutingActionType != ECombatActionType::BasicAttack && AttackBtn)
	{
		AttackBtn->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ExecutingActionType != ECombatActionType::WeaponSkill && SkillSlot_Active)
	{
		SkillSlot_Active->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ExecutingActionType != ECombatActionType::UltimateSkill && SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UActionControlPanel::SetTagButtonsEnabled(bool bShouldEnable)
{
	// 배열에 캐싱해둔 태그 버튼(A, B, C)을 돌면서 한 번에 잠금/해제
	for (UParadiseCommonButton* TagBtn : TagButtons)
	{
		if (TagBtn)
		{
			TagBtn->SetIsEnabled(bShouldEnable);
		}
	}
}

void UActionControlPanel::OnAttackButtonPressed()
{
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		float AttackRange = 100.0f;
		float AttackRadius = 100.0f;
		float ForwardOffset = 100.0f;
		ETargetFilter AttackTargetFilter = ETargetFilter::Enemy;

		FString DataRowName = TEXT("None(Default)"); // 기본값

		if (!CachedWeaponAttackActionHandle.IsNull())
		{
			DataRowName = CachedWeaponAttackActionHandle.RowName.ToString(); // 행 이름 저장

			if (const FActionStats* AttackActionData = CachedWeaponAttackActionHandle.GetRow<FActionStats>(TEXT("SkillIndicatorLookup")))
			{
				AttackRange = (AttackActionData->AttackRange > 0.0f) ? AttackActionData->AttackRange : 0.0f;
				AttackRadius = (AttackActionData->AttackRadius > 0.0f) ? AttackActionData->AttackRadius : 0.0f;
				ForwardOffset = AttackActionData->ForwardOffset;
				AttackTargetFilter = AttackActionData->TargetFilter;
			}
		}
		//UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("🟢 [Normal Attack | Row: %s] 최종 적용 수치 - Range: %f / Radius: %f / Offset: %f"), *DataRowName, AttackRange, AttackRadius, ForwardOffset);

		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			AttackRange, AttackRadius, ForwardOffset, AttackTargetFilter);
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
		ETargetFilter SkillTargetFilter = ETargetFilter::Enemy;
		FString DataRowName = TEXT("None(Default)");

		if (!CachedWeaponSkillActionHandle.IsNull())
		{
			DataRowName = CachedWeaponSkillActionHandle.RowName.ToString(); // 행 이름 저장

			if (const FActionStats* SkillActionData = CachedWeaponSkillActionHandle.GetRow<FActionStats>(TEXT("SkillIndicatorLookup")))
			{
				SkillAttackRange = (SkillActionData->AttackRange > 0.0f) ? SkillActionData->AttackRange : 0.0f;
				SkillAttackRadius = (SkillActionData->AttackRadius > 0.0f) ? SkillActionData->AttackRadius : 0.0f;
				SkillForwardOffset = SkillActionData->ForwardOffset;
				SkillTargetFilter = SkillActionData->TargetFilter;
			}
		}
		//UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("🔵 [Active Skill | Row: %s] 최종 적용 수치 - Range: %f / Radius: %f / Offset: %f"), *DataRowName, SkillAttackRange, SkillAttackRadius, SkillForwardOffset);

		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			SkillAttackRange, SkillAttackRadius, SkillForwardOffset, SkillTargetFilter);
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
		ETargetFilter UltimateTargetFilter = ETargetFilter::Enemy;
		FString DataRowName = TEXT("None(Default)");

		if (!CachedUltimateActionHandle.IsNull())
		{
			DataRowName = CachedUltimateActionHandle.RowName.ToString(); // 행 이름 저장

			if (const FActionStats* UltimateActionData = CachedUltimateActionHandle.GetRow<FActionStats>(TEXT("UltIndicatorLookup")))
			{
				UltimateAttackRange = (UltimateActionData->AttackRange > 0.0f) ? UltimateActionData->AttackRange : 0.0f;
				UltimateAttackRadius = (UltimateActionData->AttackRadius > 0.0f) ? UltimateActionData->AttackRadius : 0.0f;
				UltimateForwardOffset = UltimateActionData->ForwardOffset;
				UltimateTargetFilter = UltimateActionData->TargetFilter;
			}
		}

		//UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("🟣 [Ultimate Skill | Row: %s] 최종 적용 수치 - Range: %f / Radius: %f / Offset: %f"), *DataRowName, UltimateAttackRange, UltimateAttackRadius, UltimateForwardOffset);

		CurrentActivePawn->GetSkillIndicatorComponent()->ShowIndicator(
			UltimateAttackRange, UltimateAttackRadius, UltimateForwardOffset, UltimateTargetFilter);
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
	if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
	{
		if (AParadiseCameraManager* CamMgr = Cast<AParadiseCameraManager>(InGamePC->PlayerCameraManager))
		{
			if (CamMgr->bIsUltimatePlaying) return; // 입력을 취소하고 즉시 함수 종료!
		}
	}

	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (CurrentActivePawn && CurrentActivePawn->GetSkillIndicatorComponent())
	{
		CurrentActivePawn->GetSkillIndicatorComponent()->HideIndicator();
	}
	ProcessAbilityInput(EInputID::Ultimate); // 진짜 발동
}

void UActionControlPanel::ProcessAbilityInput(EInputID InputID)
{
	//0303 김성현 - Fix 스위치 한 영웅이 어빌리티를 발동하지 않는 문제 수정
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());

	if (CurrentActivePawn)
	{
		float CurrentMana = 0.0f;
		if (CachedASC.IsValid())
		{
			CurrentMana = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetManaAttribute());
		}

		if (InputID == EInputID::Skill && CurrentMana < CachedActiveManaCost) return;
		if (InputID == EInputID::Ultimate && CurrentMana < CachedUltimateManaCost) return;

		CurrentActivePawn->SendAbilityInputToASC(InputID, true);

		AInGamePlayerState* PS = Cast<AInGamePlayerState>(GetOwningPlayerState());
		APlayerData* Soul = PS ? PS->GetSquadMemberData(CurrentActiveTagIndex) : nullptr;
		FName CharID = Soul ? Soul->CharacterID : NAME_None;
		float CurrentTime = GetWorld()->GetTimeSeconds();


		// 1. 액티브 스킬 처리
		if (InputID == EInputID::Skill && !CachedWeaponSkillActionHandle.IsNull())
		{
			if (FActionStats* ActionRow = CachedWeaponSkillActionHandle.GetRow<FActionStats>(TEXT("Skill")))
			{
				float CD = ActionRow->Cooldown;
				if (CD > 0.0f)
				{
					UpdateSkillCooldown(0, CD, CD); // 개발자님이 만드신 함수로 UI 즉시 가동!
					ActiveSkillEndTimes.Add(CharID, CurrentTime + CD); // 쿨타임 끝나는 시각 저장
				}
			}
		}
		// 2. 궁극기 처리
		else if (InputID == EInputID::Ultimate)
		{
			if (!CachedUltimateActionHandle.IsNull())
			{
				if (FActionStats* ActionRow = CachedUltimateActionHandle.GetRow<FActionStats>(TEXT("Ult")))
				{
					float CD = ActionRow->Cooldown;
					if (CD > 0.0f)
					{
						UpdateSkillCooldown(1, CD, CD); // UI 즉시 가동!
						UltimateEndTimes.Add(CharID, CurrentTime + CD); // 쿨타임 끝나는 시각 저장
					}
				}
			}

			// 기존 궁극기 연출 시간 캐싱 (2.5초)
			float UltimateDuration = 2.5f;

			// (기존 연출 코드 유지)
			if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
			{
				//0327 김성현 - 자동모드시 궁극기 관련 연출 실행 X
				bool bIsAutoBattle = false;
				if (UAutoCombatComponent* AutoComp = InGamePC->GetAutoCombatComponent())
				{
					bIsAutoBattle = AutoComp->IsAutoMode();
				}

				if (bIsAutoBattle) return;
				
				if (UUltimateEffectComponent* UltEffectComp = InGamePC->GetUltimateEffectComponent())
				{
					UltEffectComp->PlayUltimateEffect(UltimateDuration);
				}
				

				
			}
			// 1. 궁극기 발동 즉시 태그 버튼 클릭 차단
			SetTagButtonsEnabled(false);
			//UE_LOG(LogTemp, Log, TEXT("🔒 [ActionPanel] 궁극기 발동! 교체 버튼을 %.1f초간 잠급니다."), UltimateDuration);

			// 태그 쿨타임 세팅
			MaxTagCooldown = UltimateDuration;
			CurrentTagCooldown = UltimateDuration;

			// 3개의 쿨타임 바를 화면에 띄움
			for (UProgressBar* PB : TagCooldownBars)
			{
				if (PB) PB->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			// 3개의 텍스트도 화면에 띄우고 초기 숫자 셋팅
			for (UTextBlock* Text : TagCooldownTexts)
			{
				if (Text)
				{
					Text->SetVisibility(ESlateVisibility::HitTestInvisible);
					Text->SetText(FText::AsNumber(FMath::CeilToInt(UltimateDuration)));
				}
			}

			// 2. 궁극기 연출 시간에 맞춰 UI 스스로 자물쇠 해제!
			GetWorld()->GetTimerManager().SetTimer(
				TimerHandle_TagLock,
				FTimerDelegate::CreateUObject(this, &UActionControlPanel::SetTagButtonsEnabled, true),
				UltimateDuration,
				false
			);

			// 3. 프로그레스 바 깎는 타이머 가동
			GetWorld()->GetTimerManager().SetTimer(
				TimerHandle_TagVisual,
				this,
				&UActionControlPanel::UpdateTagCooldownVisual,
				0.05f,
				true
			);
		}

		//UE_LOG(LogTemp, Warning, TEXT("[ActionPanel] 현재 빙의된 %s가 어빌리티를 발동합니다."), *CurrentActivePawn->GetName());
	}

	//	// 2. 궁극기일때 컨트롤러에 화면연출을 틀어라 라고 호출
	//	if (InputID == EInputID::Ultimate)
	//	{
	//		if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
	//		{
	//			if (UUltimateEffectComponent* UltEffectComp = InGamePC->GetUltimateEffectComponent())
	//			{
	//				// 연출 시간은 Data-Driven하게 스탯 테이블에서 가져올 수도 있지만, 일단 2.5초로 세팅
	//				UltEffectComp->PlayUltimateEffect(2.5f);
	//			}
	//		}
	//	}
	//	UE_LOG(LogTemp, Warning, TEXT("[ActionPanel] 현재 빙의된 %s가 어빌리티를 발동합니다."), *CurrentActivePawn->GetName());
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("❌ [ActionPanel] 현재 플레이어 폰을 찾을 수 없습니다. (스폰/빙의 문제)"));
	//}
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

#pragma region 코스트 연동 로직 구현
void UActionControlPanel::OnManaChanged(const FOnAttributeChangeData& Data)
{
	UpdateSkillUsabilityByMana(Data.NewValue);
}

void UActionControlPanel::UpdateSkillUsabilityByMana(float CurrentMana)
{
	// 마나 상태에 따른 UI 제어는 SkillSlot 위젯 스스로 처리하도록 위임합니다.
	if (SkillSlot_Active)
	{
		SkillSlot_Active->SetManaAffordable(CurrentMana >= CachedActiveManaCost);
	}

	if (SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->SetManaAffordable(CurrentMana >= CachedUltimateManaCost);
	}
}
#pragma endregion 코스트 연동 로직 구현
