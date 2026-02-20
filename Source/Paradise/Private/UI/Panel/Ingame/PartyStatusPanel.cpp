// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/PartyStatusPanel.h"
#include "UI/Widgets/InGame/CharacterStatusWidget.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/ItemStructs.h"
#include "Engine/Texture2D.h"

void UPartyStatusPanel::NativeConstruct()
{
	Super::NativeConstruct();

#pragma region 위젯 캐싱 최적화
	// 배열을 미리 비우고 BindWidget된 위젯들을 캐싱하여 루프 처리가 가능하게 함
	MemberWidgets.Empty();

	if (WBP_Member_0) MemberWidgets.Add(WBP_Member_0);
	if (WBP_Member_1) MemberWidgets.Add(WBP_Member_1);
	if (WBP_Member_2) MemberWidgets.Add(WBP_Member_2);
#pragma endregion 위젯 캐싱 최적화
}

#pragma region 데이터 업데이트 로직
void UPartyStatusPanel::InitializeMember(int32 Index, FName CharacterID)
{
	/**
	 * @brief 데이터 드라이븐 방식 적용
	 * @details 실제 프로젝트에서는 DT_CharacterAssets 등에서 CharacterID를 통해
	 * 초상화 텍스처를 가져와 자식 위젯의 SetCharacterPortrait을 호출합니다.
	 */
	if (MemberWidgets.IsValidIndex(Index) && MemberWidgets[Index])
	{
		// 1. GameInstance 가져오기
		if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
		{
			// 2. 데이터 테이블에서 CharacterID로 에셋 정보(FCharacterAssets) 안전하게 조회
			if (FCharacterAssets* AssetData = GI->GetDataTableRow<FCharacterAssets>(GI->CharacterAssetsDataTable, CharacterID))
			{
				// 3. Soft 포인터에서 텍스처 로드 (FaceIcon 사용)
				if (UTexture2D* LoadedPortrait = AssetData->FaceIcon.LoadSynchronous())
				{
					// 4. 자식 위젯에 전달 (단일 책임 원칙: 그리는 건 자식이 알아서 함)
					MemberWidgets[Index]->SetCharacterPortrait(LoadedPortrait);

					UE_LOG(LogTemp, Log, TEXT("[PartyStatusPanel] %d번 슬롯 초상화 업데이트 완료: %s"), Index, *CharacterID.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[PartyStatusPanel] %s 의 FaceIcon을 로드할 수 없습니다. 에셋 경로를 확인하세요."), *CharacterID.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[PartyStatusPanel] %s 에 해당하는 CharacterAssets 데이터를 찾을 수 없습니다."), *CharacterID.ToString());
			}
		}
	}
}

void UPartyStatusPanel::BindMemberASC(int32 Index, UAbilitySystemComponent* InASC)
{
	if (MemberWidgets.IsValidIndex(Index) && MemberWidgets[Index])
	{
		// 실제 체력/마나바가 움직이도록 중계합니다.
		MemberWidgets[Index]->BindToASC(InASC);
	}
}
#pragma endregion 데이터 업데이트 로직