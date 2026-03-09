// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Chapter/ParadiseChapterSelectWidget.h"
#include "UI/Widgets/Chapter/ParadiseChapterSlotWidget.h"
#include "Framework/Lobby/LobbyPlayerController.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/StageSubsystem.h"
#include "Data/Structs/StageStructs.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"

#pragma region 생명주기
void UParadiseChapterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 게임 인스턴스 캐싱 (최적화)
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());
	CachedController = GetOwningPlayer<ALobbyPlayerController>();
	if (Btn_Back)
	{
		Btn_Back->OnClicked.AddDynamic(this, &UParadiseChapterSelectWidget::OnBackClicked);
	}

	// 2. 챕터 리스트 동적 생성
	BuildChapterList();
}

void UParadiseChapterSelectWidget::NativeDestruct()
{
	if (Btn_Back)
	{
		Btn_Back->OnClicked.RemoveAll(this);
	}

	// 안전한 메모리 해제
	CachedController = nullptr;
	CachedGI = nullptr;
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 내부 로직 구현
void UParadiseChapterSelectWidget::OnBackClicked()
{
	// RemoveFromParent()를 쓰지 않고, 컨트롤러에게 상태 변경을 요청합니다! (MVC 패턴 완벽 준수)
	if (CachedController.IsValid())
	{
		// 컨트롤러의 상태를 None(메인 화면)으로 변경!
		// (이 함수가 호출되면 WBP_LobbyHUD가 알아서 이 창을 숨기고 원래 메뉴를 켭니다)
		CachedController->SetLobbyMenu(EParadiseLobbyMenu::None);
	}
}

void UParadiseChapterSelectWidget::OnChapterSlotClicked(int32 ChapterID, UTexture2D* MapTexture)
{
	if (!CachedController.IsValid()) return;

	UE_LOG(LogTemp, Log,
		TEXT("[ChapterSelect] 챕터 %d 선택 → 컨트롤러에 입장 요청."), ChapterID);

	// ★ StageSelectWidgetClass 를 함께 전달합니다.
	//   컨트롤러가 카메라 이동 완료 후 이 클래스로 StageSelect 위젯을 엽니다.
	CachedController->EnterChapterMap(ChapterID, MapTexture);
}

void UParadiseChapterSelectWidget::BuildChapterList()
{
	if (!Scroll_ChapterList || !DT_ChapterData || !ChapterSlotClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("❌ [ChapterSelect] DT_ChapterData 또는 ChapterSlotClass 가 세팅되지 않았습니다."));
		return;
	}

	Scroll_ChapterList->ClearChildren();

	// 1. DataTable 전체 로드
	TArray<FChapterData*> AllChapters;
	DT_ChapterData->GetAllRows<FChapterData>(TEXT("ChapterList"), AllChapters);

	// 2. 챕터 번호 오름차순 정렬 (데이터 무결성 보장)
	AllChapters.Sort([](const FChapterData& A, const FChapterData& B)
		{
			return A.ChapterID < B.ChapterID;
		});

	// 3. StageSubsystem 루프 외부에서 1회 캐싱 (매 반복 GetSubsystem 호출 방지)
	UStageSubsystem* StageSys = nullptr;
	if (CachedGI.IsValid())
	{
		StageSys = CachedGI->GetSubsystem<UStageSubsystem>();
	}

	// 4. 해금된 챕터만 슬롯 동적 생성
	for (const FChapterData* Chapter : AllChapters)
	{
		if (!Chapter) continue;

		// FirstStageID 가 NAME_None 이면 기본 해금(1챕터), 있으면 서브시스템에 질의
		const bool bIsUnlocked = (StageSys && !Chapter->FirstStageID.IsNone())
			? StageSys->IsStageUnlocked(Chapter->FirstStageID)
			: true;

		// 텍스처 동기 로드 (슬롯에 전달 + 슬롯이 나중에 컨트롤러에 재전달)
		UTexture2D* LoadedTexture = nullptr;
		if (!Chapter->ChapterMapTexture.IsNull())
		{
			LoadedTexture = Chapter->ChapterMapTexture.LoadSynchronous();
		}

		UParadiseChapterSlotWidget* NewSlot =
			CreateWidget<UParadiseChapterSlotWidget>(this, ChapterSlotClass);
		if (!NewSlot) continue;

		// 슬롯에 데이터 주입 (ID, 이름, 해금 여부, 텍스처)
		NewSlot->InitSlot(Chapter->ChapterID, Chapter->ChapterName, bIsUnlocked, LoadedTexture);

		// ★ 슬롯 클릭 델리게이트 바인딩
		//   슬롯이 클릭되면 OnChapterSlotClicked(ChapterID, MapTexture) 호출됩니다.
		NewSlot->OnChapterSelected.AddDynamic(
			this, &UParadiseChapterSelectWidget::OnChapterSlotClicked);

		Scroll_ChapterList->AddChild(NewSlot);
	}
}

bool UParadiseChapterSelectWidget::IsChapterUnlocked(FName RequiredStageID) const
{
	// 첫 스테이지 ID가 비어있다면(None) 기본으로 열려있는 1챕터로 간주합니다.
	if (RequiredStageID.IsNone()) return true;

	if (CachedGI.IsValid())
	{
		if (UStageSubsystem* StageSys = CachedGI->GetSubsystem<UStageSubsystem>())
		{
			// 서브시스템에 해당 스테이지가 해금 상태인지 묻습니다.
			return StageSys->IsStageUnlocked(RequiredStageID);
		}
	}
	return false;
}
#pragma endregion 내부 로직 구현