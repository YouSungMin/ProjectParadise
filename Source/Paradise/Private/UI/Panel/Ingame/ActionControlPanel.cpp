// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/ActionControlPanel.h"

#include "Characters/Base/PlayerBase.h"
#include "CommonButtonBase.h"
#include "UI/Widgets/InGame/SkillSlotWidget.h"
#include "Framework/InGame/InGameController.h"
#include "Framework/Core/ParadiseGameInstance.h"

void UActionControlPanel::NativeConstruct()
{
	Super::NativeConstruct();

	// 생성 시점에서 플레이어 폰을 캐싱한다.
	CachedPlayer = Cast<APlayerBase>(GetOwningPlayerPawn());
	if (!CachedPlayer.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionControlPanel] 플레이어 폰을 찾을 수 없어 캐싱에 실패했습니다."));
	}

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

	// 2. 기본 공격 버튼 바인딩
	if (AttackBtn)
	{
		// Common UI의 OnSelected 혹은 OnClicked를 사용합니다.
		AttackBtn->OnClicked().RemoveAll(this);
		AttackBtn->OnClicked().AddUObject(this, &UActionControlPanel::OnAttackButtonClicked);
		UE_LOG(LogTemp, Warning, TEXT(" 공격 키 바인드 됨 "));
	}

	// 3. 액티브 스킬 슬롯 내 버튼 바인딩
	if (SkillSlot_Active && SkillSlot_Active->GetSlotButton())
	{
		SkillSlot_Active->GetSlotButton()->OnClicked().RemoveAll(this);
		SkillSlot_Active->GetSlotButton()->OnClicked().AddUObject(this, &UActionControlPanel::ProcessAbilityInput, EInputID::Skill);
	}

	// 4. 궁극기 스킬 슬롯 내 버튼 바인딩
	if (SkillSlot_Ultimate && SkillSlot_Ultimate->GetSlotButton())
	{
		SkillSlot_Ultimate->GetSlotButton()->OnClicked().RemoveAll(this);
		SkillSlot_Ultimate->GetSlotButton()->OnClicked().AddUObject(this, &UActionControlPanel::ProcessAbilityInput, EInputID::Ultimate);
	}
}

void UActionControlPanel::NativeDestruct()
{
	// 위젯 파괴 시 남아있는 포인터들을 깔끔하게 정리 (메모리 릭 원천 차단)
	if (AttackBtn) AttackBtn->OnClicked().RemoveAll(this);
	if (SkillSlot_Active && SkillSlot_Active->GetSlotButton()) SkillSlot_Active->GetSlotButton()->OnClicked().RemoveAll(this);
	if (SkillSlot_Ultimate && SkillSlot_Ultimate->GetSlotButton()) SkillSlot_Ultimate->GetSlotButton()->OnClicked().RemoveAll(this);

	for (TObjectPtr<UCommonButtonBase> Btn : TagButtons)
	{
		if (Btn) Btn->OnClicked().RemoveAll(this);
	}

	TagButtons.Empty();
	CachedPlayer = nullptr;

	Super::NativeDestruct();
}

#pragma region 외부 인터페이스 구현
void UActionControlPanel::InitActionPanel(FName WeaponActionID, FName UltimateActionID)
{
	// 게임 인스턴스의 공용 데이터 테이블 로직을 활용합니다.
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	/** @section 1. 무기 스킬 (일반 스킬) 데이터 연동 */
	if (WeaponActionID != NAME_None)
	{
		 if (const FActionStats* WeaponActionData = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, WeaponActionID))
		 {
		 	if (SkillSlot_Active)
		 	{
				//SkillSlot_Active->SetSkillIcon(WeaponActionData->SkillIcon);
		 	}
		 }
		UE_LOG(LogTemp, Log, TEXT("✅ [UI] 액티브 스킬 ID 연결 완료: %s"), *WeaponActionID.ToString());
	}

	/** @section 2. 캐릭터 스킬 (궁극기) 데이터 연동 */
	if (UltimateActionID != NAME_None)
	{
		 if (const FActionStats* UltimateActionData = GI->GetDataTableRow<FActionStats>(GI->ActionStatsDataTable, UltimateActionID))
		 {
		 	if (SkillSlot_Ultimate)
		 	{
				//SkillSlot_Ultimate->SetSkillIcon(UltimateActionData->SkillIcon);
		 	}
		 }
		UE_LOG(LogTemp, Log, TEXT("✅ [UI] 궁극기 ID 연결 완료: %s"), *UltimateActionID.ToString());
	}
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
	/**
	 * @brief 현재 조작 캐릭터에 따른 태그 버튼 활성화/비활성화 제어
	 * @details 배열 루프를 사용하여 하드코딩 없이 상태를 일괄 업데이트합니다.
	 */
	for (int32 i = 0; i < TagButtons.Num(); ++i)
	{
		if (TagButtons[i])
		{
			// 현재 조작 중인 캐릭터의 버튼은 비활성화(교체 대상에서 제외)
			const bool bIsActiveTag = (i != ActiveCharIndex);
			TagButtons[i]->SetIsEnabled(bIsActiveTag);
		}
	}
}
void UActionControlPanel::OnAttackButtonClicked()
{
	ProcessAbilityInput(EInputID::Attack);
}

void UActionControlPanel::ProcessAbilityInput(EInputID InputID)
{
	if (CachedPlayer.IsValid())
	{
		CachedPlayer->SendAbilityInputToASC(InputID, true);
	}
}

void UActionControlPanel::OnTagButtonClicked(int32 CharacterIndex)
{
	if (AInGameController* InGamePC = Cast<AInGameController>(GetOwningPlayer()))
	{
		// 2. 컨트롤러에 구현해두신 안전한 캐릭터 교체 및 AI 배정 로직 실행
		InGamePC->RequestSwitchPlayer(CharacterIndex);

		// 3. 교체가 완료되었으므로 버튼 상태 즉시 갱신 (자신은 비활성화, 나머진 활성화)
		UpdateTagButtons(CharacterIndex);
	}
}
#pragma endregion 외부 인터페이스 구현