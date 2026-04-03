// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/Popup/VictoryPopupWidget.h"
#include "UI/Panel/Ingame/Result/ResultCharacterPanelWidget.h"
#include "UI/Panel/Ingame/Result/FamiliarRewardPopupWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/StageSubsystem.h"
#include "Data/Assets/ParadiseFXAudioData.h"
#include "Data/Structs/StageStructs.h"
#include "Data/Structs/UnitStructs.h"
#include "Kismet/GameplayStatics.h"

#pragma region 생명주기
void UVictoryPopupWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모(UGameResultWidgetBase)의 버튼(Home, Retry) 바인딩 실행

	if (Btn_NextStage)
	{
		Btn_NextStage->OnClicked.AddUniqueDynamic(this, &UVictoryPopupWidget::OnNextStageClicked);
	}
}

void UVictoryPopupWidget::NativeDestruct()
{
	if (Btn_NextStage)
	{
		Btn_NextStage->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 데이터 설정 로직 (View Rendering)
void UVictoryPopupWidget::SetVictoryData(
	FText InStageName,
	int32 InStarCount,
	int32 InEarnedGold,
	int32 InEarnedAether,
	const TArray<FResultCharacterData>& InCharacterResults,
	FName InNextStageID,
	FName InAcquiredFamiliar)
{
	//UE_LOG(LogTemp, Log, TEXT("[VictoryPopup] 데이터 갱신 - 별:%d, 골드:%d, 에테르:%d, 다음 스테이지:%s"),
		//InStarCount, InEarnedGold, InEarnedAether, *InNextStageID.ToString());

	// 1. 내부 상태 캐싱
	CachedNextStageID = InNextStageID;

	// 2. 텍스트 위젯 갱신 (SRP: 숫자는 콤마 처리 등 서식을 적용)
	if (Text_Stage)      Text_Stage->SetText(InStageName);
	if (Text_GoldValue)  Text_GoldValue->SetText(FText::AsNumber(InEarnedGold));
	if (Text_AetherValue)Text_AetherValue->SetText(FText::AsNumber(InEarnedAether));

	// 3. 별 이미지 갱신
	SetStarImage(Img_Star1, 1, InStarCount);
	SetStarImage(Img_Star2, 2, InStarCount);
	SetStarImage(Img_Star3, 3, InStarCount);

	// 4. 캐릭터 슬롯 렌더링을 자식 패널에게 위임 (SRP 준수)
	if (WBP_CharacterResultPanel)
	{
		WBP_CharacterResultPanel->UpdateCharacterSlots(InCharacterResults);
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("[VictoryPopup] WBP_CharacterResultPanel 바인딩 누락."));
	}

	// 5. 다음 스테이지가 없다면(마지막 스테이지) 다음 버튼을 숨기거나 비활성화 처리
	if (Btn_NextStage)
	{
		Btn_NextStage->SetVisibility(CachedNextStageID.IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// 퍼밀리어 보상 이미지 처리 (초회 3별 클리어 시에만 표시)
	if (WBP_FamiliarRewardPopup)
	{
		if (InAcquiredFamiliar.IsNone())
		{
			//UE_LOG(LogTemp, Warning, TEXT("[VictoryPopup] AcquiredFamiliar가 None — 퍼밀리어 위젯 숨김"));
			WBP_FamiliarRewardPopup->HideReward();
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[VictoryPopup] AcquiredFamiliar: %s — ShowFamiliarReward 호출"), *InAcquiredFamiliar.ToString());
			WBP_FamiliarRewardPopup->ShowFamiliarReward(InAcquiredFamiliar);
		}
	}
	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_ResultVictory)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_ResultVictory);
		}
	}
	if (Anim_PopupAppear)
	{
		PlayAnimation(Anim_PopupAppear);
	}
}
#pragma endregion 데이터 설정 로직 (View Rendering)

#pragma region 내부 로직
void UVictoryPopupWidget::SetStarImage(UImage* StarImage, int32 StarIndex, int32 InStarCount)
{
	if (!StarImage) return;

	// StarIndex 가 InStarCount 이하면 금색(On), 초과면 흑색(Off)
	const bool bIsOn = (StarIndex <= InStarCount);
	UTexture2D* TargetTexture = bIsOn ? StarOnTexture.Get() : StarOffTexture.Get();

	if (TargetTexture)
	{
		StarImage->SetBrushFromTexture(TargetTexture);
	}
	/*else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("⚠️ [VictoryPopup] Star%d 텍스처가 비어있습니다!"
				" WBP_VictoryPopup 디테일 패널에서 StarOnTexture / StarOffTexture 를 할당하세요."),
			StarIndex);
	}*/
}

void UVictoryPopupWidget::OnNextStageClicked()
{
	// 방어 코드: 다음 스테이지가 비어있으면 무시 (버튼이 숨겨지겠지만 혹시 모를 클릭 방지)
	if (CachedNextStageID.IsNone()) return;

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}

	if (UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance()))
	{
		if (GI->GlobalAudioData && GI->GlobalAudioData->SFX_IngameNext)
		{
			UGameplayStatics::PlaySound2D(this, GI->GlobalAudioData->SFX_IngameNext);
		}
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_NextStage,
			this,
			&UVictoryPopupWidget::ExecuteNextStage,
			0.3f,
			false
		);
	}
}

void UVictoryPopupWidget::ExecuteNextStage()
{

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. 데이터 테이블에서 다음 스테이지의 에셋(Assets) 정보 조회
	// [A] 리소스 정보 (맵 경로, 로딩 이미지 등)
	FStageAssets* NextAssets = GI->GetDataTableRow<FStageAssets>(GI->StageAssetsDataTable, CachedNextStageID);
	// [B] 규칙 및 텍스트 정보 (스테이지 이름, 설명 등)
	FStageStats* NextStats = GI->GetDataTableRow<FStageStats>(GI->StatgeStatsDataTable, CachedNextStageID);

	// 유효성 검사 (맵 에셋이 반드시 있어야 함)
	if (!NextAssets || NextAssets->MapAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[VictoryPopup] 다음 스테이지(%s)의 MapAsset 정보를 찾을 수 없습니다!"), *CachedNextStageID.ToString());
		return;
	}

	// 3. 목적지 레벨 이름 추출
	FName TargetLevelName = FName(*NextAssets->MapAsset.GetAssetName());

	if (UStageSubsystem* StageSubsystem = GI->GetSubsystem<UStageSubsystem>())
	{
		StageSubsystem->SetSelectedStageID(CachedNextStageID);
		UE_LOG(LogTemp, Log, TEXT("✅ [VictoryPopup] StageSubsystem ID 업데이트 완료: %s"), *CachedNextStageID.ToString());
	}

	// 4. 로딩 서브시스템 호출
	if (ULevelLoadingSubsystem* LoadingSystem = GI->GetSubsystem<ULevelLoadingSubsystem>())
	{
		// 프리로드할 에셋 배열 (필요 시 추가 가능)
		TArray<TSoftObjectPtr<UObject>> AssetsToPreload;

		// 순서: 타겟맵, 로딩맵, 프리로드배열, 로딩이미지, 스테이지이름, 스테이지설명
		LoadingSystem->StartLevelTransition(
			TargetLevelName,
			FName("L_Loading"),
			AssetsToPreload,
			NextAssets->LoadingImage,
			(NextStats ? NextStats->StageName : FText::GetEmpty()),
			(NextStats ? NextStats->Description : FText::GetEmpty()),
			true
		);
	}
}
#pragma endregion 내부 로직
