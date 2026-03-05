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

			// 기존 바인딩 제거 (안전장치)
			if (SummonSlots[i]->OnSlotClicked.IsAlreadyBound(this, &USummonControlPanel::HandleSlotClickRequest))
			{
				SummonSlots[i]->OnSlotClicked.RemoveDynamic(this, &USummonControlPanel::HandleSlotClickRequest);
			}
			// 클릭 이벤트 연결
			SummonSlots[i]->OnSlotClicked.AddDynamic(this, &USummonControlPanel::HandleSlotClickRequest);
		}
	}
	// 3. (추가) 시간표 배열 초기화 - 처음에는 딜레이 없이 즉시 다 보이도록 0.0f 할당
	SlotRevealTimes.Init(0.0f, SummonSlots.Num());
	NextAvailableRefillTime = 0.0f;

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

		//초기 슬롯 데이터 요청
		SummonComp->RefreshAllSlots();
		if (!SummonComp->OnSummonSlotsUpdated.IsAlreadyBound(this, &USummonControlPanel::HandleSummonSlotsUpdate))
		{
			SummonComp->OnSummonSlotsUpdated.AddDynamic(this, &USummonControlPanel::HandleSummonSlotsUpdate);
		}
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


void USummonControlPanel::HandleSummonSlotsUpdate(const TArray<FSummonSlotInfo>& Slots)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 1. 시간표 갱신 (Shift 애니메이션은 이미 재생되었고, 여기서는 새로 들어올 맨 끝 슬롯의 등장 시간만 조율합니다)
	if (SlotRevealTimes.Num() > 0)
	{
		// 맨 앞의 딜레이만 하나 빼주고 뒤로 밀어줍니다.
		SlotRevealTimes.RemoveAt(0);

		if (NextAvailableRefillTime < CurrentTime)
		{
			NextAvailableRefillTime = CurrentTime;
		}

		float ActualDelay = SlotRefillDelay;
		if (NextAvailableRefillTime > CurrentTime + 0.5f)
		{
			ActualDelay = 0.1f; // 큐 밀림 방지
		}

		NextAvailableRefillTime += ActualDelay;
		SlotRevealTimes.Add(NextAvailableRefillTime);

		int32 ExcessCount = SlotRevealTimes.Num() - Slots.Num();
		if (ExcessCount > 0) SlotRevealTimes.RemoveAt(0, ExcessCount);
	}

	// 2. UI 데이터 갱신 및 새 슬롯 ScheduleReveal
	for (int32 i = 0; i < SummonSlots.Num(); ++i)
	{
		if (!SummonSlots.IsValidIndex(i) || !SummonSlots[i] || !Slots.IsValidIndex(i)) continue;

		UTexture2D* LoadedIcon = nullptr;
		if (!Slots[i].FamiliarIcon.IsNull())
		{
			LoadedIcon = Slots[i].FamiliarIcon.LoadSynchronous();
		}
		int32 Cost = Slots[i].FamiliarCost;

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

		// 3. 애니메이션
		if (LastClickedSlotIndex >= 0)
		{
			for (int32 i = LastClickedSlotIndex; i < SummonSlots.Num() - 1; ++i)
			{
				if (SummonSlots.IsValidIndex(i) && SummonSlots[i])
				{
					SummonSlots[i]->PlayShiftAnimation();
				}
			}
			LastClickedSlotIndex = -1;
		}
}
#pragma endregion 시스템 초기화

#pragma region 입력 처리
void USummonControlPanel::HandleSlotClickRequest(int32 SlotIndex)
{
	if (CachedSummonComponent.IsValid())
	{
		LastClickedSlotIndex = SlotIndex;
		bool bSuccess = CachedSummonComponent->RequestPurchase(SlotIndex);

		if (!bSuccess)
		{
			LastClickedSlotIndex = -1;
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
