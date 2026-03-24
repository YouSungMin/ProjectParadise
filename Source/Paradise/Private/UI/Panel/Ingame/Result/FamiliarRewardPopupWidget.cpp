// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/Result/FamiliarRewardPopupWidget.h"
#include "Components/Image.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/UnitStructs.h"

#pragma region 생명주기
void UFamiliarRewardPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 초기 숨김 상태
    SetVisibility(ESlateVisibility::Collapsed);
}
#pragma endregion 생명주기

#pragma region 외부 인터페이스
void UFamiliarRewardPopupWidget::ShowFamiliarReward(FName InFamiliarID)
{
    if (InFamiliarID.IsNone()) return;

    UE_LOG(LogTemp, Warning, TEXT("[FamiliarReward] ShowFamiliarReward 호출 - ID: %s"), *InFamiliarID.ToString());

    UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[FamiliarReward] GI가 null"));
        return;
    }

    if (FFamiliarAssets* Asset = GI->GetDataTableRow<FFamiliarAssets>(
        GI->FamiliarAssetsDataTable, InFamiliarID))
    {
        UTexture2D* FamiliarIcon = Asset->FaceIcon.LoadSynchronous();
        if (FamiliarIcon && Img_FamiliarIcon)
        {
            Img_FamiliarIcon->SetBrushFromTexture(FamiliarIcon);
            UE_LOG(LogTemp, Warning, TEXT("[FamiliarReward] 아이콘 세팅 완료"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[FamiliarReward] 아이콘 로드 실패 또는 Img_FamiliarIcon null"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[FamiliarReward] FamiliarAssetsDataTable에서 %s를 찾을 수 없음"), *InFamiliarID.ToString());
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UE_LOG(LogTemp, Warning, TEXT("[FamiliarReward] Visibility 설정 완료"));

    if (Anim_Show)
    {
        PlayAnimation(Anim_Show);
        UE_LOG(LogTemp, Warning, TEXT("[FamiliarReward] 애니메이션 재생"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[FamiliarReward] Anim_Show가 null — 블루프린트에서 애니메이션 바인딩 확인 필요"));
    }
}

void UFamiliarRewardPopupWidget::HideReward()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
#pragma endregion 외부 인터페이스
