// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/Popup/VictoryPopupWidget.h"
#include "UI/Panel/Ingame/Result/ResultCharacterPanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Framework/System/LevelLoadingSubsystem.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Data/Structs/StageStructs.h"

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
	FName InNextStageID)
{
	UE_LOG(LogTemp, Log, TEXT("[VictoryPopup] 데이터 갱신 - 별:%d, 골드:%d, 에테르:%d, 다음 스테이지:%s"),
		InStarCount, InEarnedGold, InEarnedAether, *InNextStageID.ToString());

	// 1. 내부 상태 캐싱
	CachedNextStageID = InNextStageID;

	// 2. 텍스트 위젯 갱신 (SRP: 숫자는 콤마 처리 등 서식을 적용)
	if (Text_Stage)      Text_Stage->SetText(InStageName);
	if (Text_GoldValue)  Text_GoldValue->SetText(FText::AsNumber(InEarnedGold));
	if (Text_AetherValue)Text_AetherValue->SetText(FText::AsNumber(InEarnedAether));

	// 3. 별 이미지 갱신 (최적화: 분기문을 삼항 연산자로 깔끔하게 처리)
	if (StarOnTexture && StarOffTexture)
	{
		if (Img_Star1) Img_Star1->SetBrushFromTexture(InStarCount >= 1 ? StarOnTexture : StarOffTexture);
		if (Img_Star2) Img_Star2->SetBrushFromTexture(InStarCount >= 2 ? StarOnTexture : StarOffTexture);
		if (Img_Star3) Img_Star3->SetBrushFromTexture(InStarCount >= 3 ? StarOnTexture : StarOffTexture);
	}

	// 4. 캐릭터 슬롯 렌더링을 자식 패널에게 위임 (SRP 준수)
	if (WBP_CharacterResultPanel)
	{
		WBP_CharacterResultPanel->UpdateCharacterSlots(InCharacterResults);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VictoryPopup] WBP_CharacterResultPanel 바인딩 누락."));
	}

	// 5. 다음 스테이지가 없다면(마지막 스테이지) 다음 버튼을 숨기거나 비활성화 처리
	if (Btn_NextStage)
	{
		Btn_NextStage->SetVisibility(CachedNextStageID.IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
#pragma endregion 데이터 설정 로직 (View Rendering)

#pragma region 내부 로직
void UVictoryPopupWidget::OnNextStageClicked()
{
	// 방어 코드: 다음 스테이지가 비어있으면 무시 (버튼이 숨겨지겠지만 혹시 모를 클릭 방지)
	if (CachedNextStageID.IsNone()) return;

	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. 데이터 테이블에서 다음 스테이지의 에셋(Assets) 정보 조회
	FStageAssets* NextStageAssets = GI->GetDataTableRow<FStageAssets>(GI->StageAssetsDataTable, CachedNextStageID);
	if (!NextStageAssets || NextStageAssets->MapAsset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[VictoryPopup] 다음 스테이지(%s)의 에셋(MapAsset) 정보를 찾을 수 없습니다!"), *CachedNextStageID.ToString());
		return;
	}

	// 2. TSoftObjectPtr에서 실제 맵(Level) 이름 추출
	FName TargetLevelName = FName(*NextStageAssets->MapAsset.GetAssetName());

	UE_LOG(LogTemp, Log, TEXT("[VictoryPopup] 다음 스테이지 맵으로 이동 시작: %s"), *TargetLevelName.ToString());

	// 3. 로딩 서브시스템을 통한 레벨 이동 처리
	if (ULevelLoadingSubsystem* LoadingSystem = GI->GetSubsystem<ULevelLoadingSubsystem>())
	{
		TArray<TSoftObjectPtr<UObject>> EmptyAssets;
		// 시스템에 로딩 스크린과 함께 타겟 맵 이름 전달
		LoadingSystem->StartLevelTransition(TargetLevelName, FName("L_Loading"), EmptyAssets);
	}
}
#pragma endregion 내부 로직
