// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Panel/Ingame/SummonControlPanel.h"
#include "UI/Widgets/Ingame/SummonSlotWidget.h"
#include "UI/Widgets/Ingame/SummonCostWidget.h"
#include "GameFramework/PlayerController.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Components/CostManageComponent.h"
#include "Components/FamiliarSummonComponent.h"

#pragma region 생명주기
void USummonControlPanel::NativeConstruct()
{
	Super::NativeConstruct();

	// 배열에 캐싱하여 인덱스로 접근 가능하게 최적화
	SummonSlots.Empty();
	if (SummonSlot_0) SummonSlots.Add(SummonSlot_0);
	if (SummonSlot_1) SummonSlots.Add(SummonSlot_1);
	if (SummonSlot_2) SummonSlots.Add(SummonSlot_2);
	if (SummonSlot_3) SummonSlots.Add(SummonSlot_3);
	if (SummonSlot_4) SummonSlots.Add(SummonSlot_4);

	// 2. 슬롯 초기화 및 클릭 이벤트 바인딩 (핵심)
	for (int32 i = 0; i < SummonSlots.Num(); ++i)
	{
		if (SummonSlots[i])
		{
			// 인덱스 부여
			SummonSlots[i]->InitSlot(i);
			SummonSlots[i]->OnSlotClicked.RemoveDynamic(this, &USummonControlPanel::HandleSlotClickRequest);
			SummonSlots[i]->OnSlotClicked.AddDynamic(this, &USummonControlPanel::HandleSlotClickRequest);
		}
	}
	// 3. 상태 변수 초기화
	bIsFirstLoad = true;
	NextAvailableRefillTime = 0.0f;
	SlotRevealTimes.Init(0.0f, SummonSlots.Num());

	InitComponents();
}
void USummonControlPanel::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_InitCost);
	}
	if (CachedCostComponent.IsValid())
	{
		CachedCostComponent->OnCostChanged.RemoveAll(this);
	}
	if (CachedSummonComponent.IsValid())
	{
		CachedSummonComponent->OnSummonSlotsUpdated.RemoveAll(this);
		CachedSummonComponent->OnSummonSlotConsumed.RemoveAll(this);
	}
	Super::NativeDestruct();
}
#pragma endregion 생명주기

#pragma region 시스템 초기화
void USummonControlPanel::InitComponents()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_InitCost, this, &USummonControlPanel::InitComponents, 0.1f, false);
		return;
	}

	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
	if (!PS)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_InitCost, this, &USummonControlPanel::InitComponents, 0.1f, false);
		return;
	}

	bool bAllComponentsReady = true;

	// 1. CostManageComponent 연결
	UCostManageComponent* CostComp = PS->GetCostManageComponent();
	if (CostComp)
	{
		CachedCostComponent = CostComp;
		if (!CostComp->OnCostChanged.IsAlreadyBound(this, &USummonControlPanel::HandleCostUpdate))
		{
			CostComp->OnCostChanged.AddDynamic(this, &USummonControlPanel::HandleCostUpdate);
		}
	}
	else
	{
		bAllComponentsReady = false;
	}

	// 2. FamiliarSummonComponent 연결 (소환 로직)
	UFamiliarSummonComponent* SummonComp = PS->GetFamiliarSummonComponent();
	if (SummonComp)
	{
		CachedSummonComponent = SummonComp;

		//  데이터 갱신 수신기 바인딩
		if (!SummonComp->OnSummonSlotsUpdated.IsAlreadyBound(this, &USummonControlPanel::HandleSummonSlotsUpdate))
			SummonComp->OnSummonSlotsUpdated.AddDynamic(this, &USummonControlPanel::HandleSummonSlotsUpdate);

		//  애니메이션 갱신 수신기 바인딩
		if (!SummonComp->OnSummonSlotConsumed.IsAlreadyBound(this, &USummonControlPanel::HandleSummonSlotConsumed))
			SummonComp->OnSummonSlotConsumed.AddDynamic(this, &USummonControlPanel::HandleSummonSlotConsumed);

		// 바인딩이 끝난 후 초기 데이터 요청!
		SummonComp->RefreshAllSlots();
	}
	else
	{
		bAllComponentsReady = false;
	}

	// 하나라도 없으면 재시도
	if (bAllComponentsReady)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_InitCost);
		UE_LOG(LogTemp, Log, TEXT("[SummonPanel] 모든 컴포넌트 연결 완료"));
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_InitCost, this, &USummonControlPanel::InitComponents, 0.1f, false);
	}
}
void USummonControlPanel::HandleCostUpdate(float CurrentCost, float MaxCost)
{
	if (CostWidget)
	{
		CostWidget->UpdateCost(CurrentCost, MaxCost);
	}
}

void USummonControlPanel::HandleSummonSlotConsumed(int32 ConsumedIndex)
{
	// 모델이 "나 이 번호 썼어!" 라고 방송하면, 여기서부터 당기기 애니메이션 쫙 재생
	for (int32 i = ConsumedIndex; i < SummonSlots.Num() - 1; ++i)
	{
		if (SummonSlots.IsValidIndex(i) && SummonSlots[i])
		{
			SummonSlots[i]->PlayShiftAnimation();
		}
	}
}

void USummonControlPanel::HandleSummonSlotsUpdate(const TArray<FSummonSlotInfo>& Slots)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	//  bIsFirstLoad 플래그를 이용해 최초와 이후를 완벽히 분리
	if (!bIsFirstLoad)
	{
		if (SlotRevealTimes.Num() > 0)
		{
			SlotRevealTimes.RemoveAt(0);
			NextAvailableRefillTime = FMath::Max(NextAvailableRefillTime, CurrentTime);

			const float ActualDelay = (NextAvailableRefillTime > CurrentTime + 0.5f) ? 0.1f : SlotRefillDelay;
			NextAvailableRefillTime += ActualDelay;
			SlotRevealTimes.Add(NextAvailableRefillTime);

			// 배열 크기 초과 방지
			if (SlotRevealTimes.Num() > Slots.Num())
			{
				SlotRevealTimes.RemoveAt(0, SlotRevealTimes.Num() - Slots.Num());
			}
		}
	}
	else
	{
		// [초기 로드] 시간표 0.0s 리셋 (4+1 방지)
		SlotRevealTimes.Init(CurrentTime, SummonSlots.Num());
		NextAvailableRefillTime = CurrentTime;
		bIsFirstLoad = false;
	}

	// 2. UI 데이터 갱신
	for (int32 i = 0; i < SummonSlots.Num(); ++i)
	{
		if (!SummonSlots.IsValidIndex(i) || !SummonSlots[i] || !Slots.IsValidIndex(i)) continue;

		UTexture2D* LoadedIcon = Slots[i].CardIcon.IsNull() ? nullptr : Slots[i].CardIcon.LoadSynchronous();
		int32 Cost = Slots[i].CardCost;

		if (SlotRevealTimes.IsValidIndex(i) && CurrentTime < SlotRevealTimes[i])
		{
			float TimeLeft = SlotRevealTimes[i] - CurrentTime;
			SummonSlots[i]->ScheduleReveal(LoadedIcon, Cost, TimeLeft);
		}
		else
		{
			SummonSlots[i]->UpdateSlotInfo(LoadedIcon, Cost);
		}
	}
}
#pragma endregion 시스템 초기화

#pragma region 입력 처리
void USummonControlPanel::HandleSlotClickRequest(int32 SlotIndex)
{
	if (CachedSummonComponent.IsValid())
	{
		bool bSuccess = CachedSummonComponent->RequestPurchase(SlotIndex);

		if (!bSuccess)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SummonPanel] 구매 실패: %d"), SlotIndex);
		}
	}

}
#pragma endregion 입력 처리

#pragma region 외부 인터페이스 구현
void USummonControlPanel::SetSummonSlotData(int32 SlotIndex, UTexture2D* Icon, int32 InCost)
{
	if (SummonSlots.IsValidIndex(SlotIndex) && SummonSlots[SlotIndex])
	{
		SummonSlots[SlotIndex]->UpdateSlotInfo(Icon, InCost);
	}
}
void USummonControlPanel::UpdateCostDisplay(float CurrentCost, float MaxCost)
{
	if (CostWidget)
	{
		CostWidget->UpdateCost(CurrentCost, MaxCost);
	}
}
#pragma endregion 외부 인터페이스 구현
