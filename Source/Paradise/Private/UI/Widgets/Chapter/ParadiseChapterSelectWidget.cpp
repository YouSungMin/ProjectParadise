// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Chapter/ParadiseChapterSelectWidget.h"
#include "UI/Widgets/Chapter/ParadiseChapterSlotWidget.h"
#include "Components/ScrollBox.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/StageSubsystem.h"
#include "Data/Structs/StageStructs.h"

#pragma region 생명주기
void UParadiseChapterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 게임 인스턴스 캐싱 (최적화)
	CachedGI = Cast<UParadiseGameInstance>(GetGameInstance());

	// 2. 챕터 리스트 동적 생성
	BuildChapterList();
}

void UParadiseChapterSelectWidget::NativeDestruct()
{
	// 안전한 메모리 해제
	CachedGI = nullptr;
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 내부 로직 구현
void UParadiseChapterSelectWidget::BuildChapterList()
{
	// 필수 컴포넌트 및 에셋 널 체크
	if (!Scroll_ChapterList || !DT_ChapterData || !ChapterSlotClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ChapterSelect] 필수 데이터 테이블이나 슬롯 클래스가 누락되었습니다."));
		return;
	}

	Scroll_ChapterList->ClearChildren();

	// 1. 데이터 테이블에서 모든 챕터 정보 로드
	TArray<FChapterData*> AllChapters;
	DT_ChapterData->GetAllRows<FChapterData>(TEXT("ChapterList"), AllChapters);

	// 2. 챕터 번호순으로 정렬 (데이터 무결성 보장)
	AllChapters.Sort([](const FChapterData& A, const FChapterData& B) {
		return A.ChapterID < B.ChapterID;
		});

	// 3. 서브시스템 1회 로드 (최적화: 반복문 외부에서 호출)
	UStageSubsystem* StageSys = nullptr;
	if (CachedGI.IsValid())
	{
		StageSys = CachedGI->GetSubsystem<UStageSubsystem>();
	}

	// 4. 슬롯 동적 생성 및 데이터 주입
	for (const FChapterData* Chapter : AllChapters)
	{
		UParadiseChapterSlotWidget* NewSlot = CreateWidget<UParadiseChapterSlotWidget>(this, ChapterSlotClass);
		if (NewSlot)
		{
			bool bIsUnlocked = true;

			// 서브시스템을 통한 해금 검사 (단일 책임 원칙: UI는 묻기만 하고 판단은 Subsystem이 함)
			if (StageSys && !Chapter->FirstStageID.IsNone())
			{
				bIsUnlocked = StageSys->IsStageUnlocked(Chapter->FirstStageID);
			}

			// 엑셀(데이터 테이블)에서 텍스처를 메모리에 로드합니다.
			UTexture2D* LoadedTexture = nullptr;
			if (!Chapter->ChapterMapTexture.IsNull())
			{
				LoadedTexture = Chapter->ChapterMapTexture.LoadSynchronous();
			}

			// 파라미터 4개를 전부 다 넣어줍니다! (ID, 이름, 해금 여부, 텍스처)
			NewSlot->InitSlot(Chapter->ChapterID, Chapter->ChapterName, bIsUnlocked, LoadedTexture);

			Scroll_ChapterList->AddChild(NewSlot);
		}
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