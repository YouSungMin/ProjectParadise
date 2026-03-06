// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/ActionControlPanel.h"
#include "UI/Widgets/Ingame/ParadiseCommonButton.h"
#include "UI/Widgets/InGame/SkillSlotWidget.h"

#include "Framework/InGame/InGameController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/SquadSubsystem.h"

#include "Components/SquadControlComponent.h"

#include "Characters/Base/PlayerBase.h"
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

	// 2. 기본 공격 버튼 바인딩
	if (AttackBtn)
	{
		// Common UI의 OnSelected 혹은 OnClicked를 사용합니다.
		AttackBtn->OnClicked().RemoveAll(this);
		AttackBtn->OnClicked().AddUObject(this, &UActionControlPanel::OnAttackButtonClicked);
		UE_LOG(LogTemp, Warning, TEXT(" 공격 키 바인드 됨 "));
	}

	// 3. 액티브 스킬 슬롯 바인딩
	if (SkillSlot_Active)
	{
		SkillSlot_Active->OnSkillActionRequested.RemoveDynamic(this, &UActionControlPanel::OnActiveSkillRequested);
		SkillSlot_Active->OnSkillActionRequested.AddDynamic(this, &UActionControlPanel::OnActiveSkillRequested);
	}

	//  4. 궁극기 스킬 슬롯 바인딩
	if (SkillSlot_Ultimate)
	{
		SkillSlot_Ultimate->OnSkillActionRequested.RemoveDynamic(this, &UActionControlPanel::OnUltimateSkillRequested);
		SkillSlot_Ultimate->OnSkillActionRequested.AddDynamic(this, &UActionControlPanel::OnUltimateSkillRequested);
	}

	// 5. 로비 데이터 연동 초기화
	//InitTagButtons();
}

void UActionControlPanel::NativeDestruct()
{
	// 위젯 파괴 시 남아있는 포인터들을 깔끔하게 정리 (메모리 릭 원천 차단)
	if (AttackBtn) AttackBtn->OnClicked().RemoveAll(this);
	if (SkillSlot_Active) SkillSlot_Active->OnSkillActionRequested.RemoveAll(this);
	if (SkillSlot_Ultimate) SkillSlot_Ultimate->OnSkillActionRequested.RemoveAll(this);

	for (TObjectPtr<UCommonButtonBase> Btn : TagButtons)
	{
		if (Btn) Btn->OnClicked().RemoveAll(this);
	}

	TagButtons.Empty();
	CachedPlayer = nullptr;

	Super::NativeDestruct();
}

#pragma region 외부 인터페이스 구현
void UActionControlPanel::InitActionPanel(FName WeaponActionID, FName UltimateActionID, UTexture2D* AttackIcon)
{
	// 게임 인스턴스의 공용 데이터 테이블 로직을 활용합니다.
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	/** @section 1. 무기 스킬 (일반 스킬) 데이터 연동 */
	if (AttackBtn && AttackIcon)
	{
		AttackBtn->SetButtonIcon(AttackIcon);
	}

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
void UActionControlPanel::OnAttackButtonClicked()
{
	ProcessAbilityInput(EInputID::Attack);
}

void UActionControlPanel::OnActiveSkillRequested()
{
	UE_LOG(LogTemp, Log, TEXT("스킬키 입력 들어옴"));
	ProcessAbilityInput(EInputID::Skill);
}

void UActionControlPanel::OnUltimateSkillRequested()
{
	UE_LOG(LogTemp, Log, TEXT("궁극키 입력 들어옴"));
	ProcessAbilityInput(EInputID::Ultimate);
}

void UActionControlPanel::ProcessAbilityInput(EInputID InputID)
{

	//0303 김성현 - Fix 스위치 한 영웅이 어빌리티를 발동하지 않는 문제 수정
	APlayerBase* CurrentActivePawn = Cast<APlayerBase>(GetOwningPlayerPawn());

	if (CurrentActivePawn)
	{
		CurrentActivePawn->SendAbilityInputToASC(InputID, true);
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