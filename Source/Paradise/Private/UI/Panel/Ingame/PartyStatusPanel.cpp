// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/PartyStatusPanel.h"
#include "UI/Widgets/InGame/CharacterStatusWidget.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/ItemStructs.h"
#include "Engine/Texture2D.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UPartyStatusPanel::NativeConstruct()
{
	Super::NativeConstruct();

#pragma region 위젯 캐싱 최적화
	// 배열을 미리 비우고 BindWidget된 위젯들을 캐싱하여 루프 처리가 가능하게 함
	MemberWidgets.Empty();
#pragma endregion 위젯 캐싱 최적화
}

#pragma region 데이터 업데이트 로직
void UPartyStatusPanel::AddPartyMemberUI(FName CharacterID, UAbilitySystemComponent* InASC)
{
	if (!StatusWidgetClass || !HB_MemberContainer || CharacterID.IsNone()) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	/** @section 1. 위젯 동적 할당 및 컨테이너 부착 */
	UCharacterStatusWidget* NewMemberWidget = CreateWidget<UCharacterStatusWidget>(GetOwningPlayer(), StatusWidgetClass);
	if (NewMemberWidget)
	{
		UHorizontalBoxSlot* NewSlot = HB_MemberContainer->AddChildToHorizontalBox(NewMemberWidget);
		if (NewSlot)
		{
			NewSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		MemberWidgets.Add(NewMemberWidget);

		/** @section 2. 초상화 데이터 주입 (Data-Driven) */
		if (FCharacterAssets* AssetData = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID))
		{
			if (UTexture2D* LoadedPortrait = AssetData->FaceIcon.LoadSynchronous())
			{
				NewMemberWidget->SetCharacterPortrait(LoadedPortrait);
			}
		}

		/** @section 3. GAS(ASC) 연동 */
		if (InASC)
		{
			NewMemberWidget->BindToASC(InASC);
		}

		//UE_LOG(LogTemp, Log, TEXT("[PartyStatusPanel] %s 캐릭터 UI 동적 생성 및 바인딩 완료!"), *CharacterID.ToString());
	}
}

void UPartyStatusPanel::ClearPartyUI()
{
	if (HB_MemberContainer)
	{
		HB_MemberContainer->ClearChildren();
	}
	MemberWidgets.Empty();
}
#pragma endregion 데이터 업데이트 로직