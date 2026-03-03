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
		if (!SummonComp->OnSummonSlotsUpdated.IsAlreadyBound(this, &USummonControlPanel::HandleSummonSlotsUpdate))
		{
			SummonComp->OnSummonSlotsUpdated.AddDynamic(this, &USummonControlPanel::HandleSummonSlotsUpdate);
		}

		// 초기 슬롯 데이터 요청 (필요 시)
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
void USummonControlPanel::HandleSummonSlotsUpdate(const TArray<FSummonSlotInfo>& Slots)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 1. 시간표 갱신
	if (LastClickedSlotIndex >= 0 && SlotRevealTimes.IsValidIndex(LastClickedSlotIndex))
	{
		SlotRevealTimes.RemoveAt(LastClickedSlotIndex);

		if (NextAvailableRefillTime < CurrentTime)
		{
			NextAvailableRefillTime = CurrentTime;
		}
		NextAvailableRefillTime += SlotRefillDelay;
		SlotRevealTimes.Add(NextAvailableRefillTime);

		// ★ 최적화: O(N²) → O(N) 개선
		int32 ExcessCount = SlotRevealTimes.Num() - Slots.Num();
		if (ExcessCount > 0)
		{
			SlotRevealTimes.RemoveAt(0, ExcessCount);  // 한 번에 여러 개 제거
		}
	}

	// 2. UI 갱신 (중복 제거)
	for (int32 i = 0; i < SummonSlots.Num(); ++i)
	{
		if (!SummonSlots.IsValidIndex(i) || !SummonSlots[i] || !Slots.IsValidIndex(i))
			continue;

		// ★ 중복 로딩 제거
		UTexture2D* LoadedIcon = nullptr;
		if (!Slots[i].FamiliarIcon.IsNull())
		{
			LoadedIcon = Slots[i].FamiliarIcon.LoadSynchronous();
		}
		int32 Cost = Slots[i].FamiliarCost;

		// 시간표 체크 후 UI 업데이트
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
		// ★ SlotRevealTimes 조작 코드 전부 삭제 (146-163줄 제거)
	}

	//if (CachedSummonComponent.IsValid())
	//{
	//	LastClickedSlotIndex = SlotIndex;
	//	bool bSuccess = CachedSummonComponent->RequestPurchase(SlotIndex);

	//	if (bSuccess)
	//	{
	//		// (추가) 구매 성공 시 시간표 배열도 왼쪽으로 당기고, 새 슬롯의 등장 시간을 추가
	//		if (SlotRevealTimes.IsValidIndex(SlotIndex))
	//		{
	//			SlotRevealTimes.RemoveAt(SlotIndex);

	//			float CurrentTime = GetWorld()->GetTimeSeconds();

	//			// 1. 만약 대기열이 비어있다면(앞에 기다리는 놈이 없다면), 기준 시간은 '지금'
	//			// 2. 만약 앞에 기다리는 놈이 있다면, 기준 시간은 '앞사람이 끝나는 시간'
	//			if (NextAvailableRefillTime < CurrentTime)
	//			{
	//				NextAvailableRefillTime = CurrentTime;
	//			}

	//			// 기준 시간에 1초(SlotRefillDelay)를 더해서 내 번호표로 만듦
	//			NextAvailableRefillTime += SlotRefillDelay;

	//			// 배열 맨 끝(빈자리)에 내 번호표를 등록
	//			SlotRevealTimes.Add(NextAvailableRefillTime);
	//		}
	//	}
	//	else
	//	{
	//		LastClickedSlotIndex = -1;
	//		UE_LOG(LogTemp, Warning, TEXT("[SummonPanel] 구매 실패: %d"), SlotIndex);
	//	}
	//}
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

//void USummonControlPanel::UpdateSummonCooldown(int32 SlotIndex, float CurrentTime, float MaxTime)
//{
//	//if (SummonSlots.IsValidIndex(SlotIndex) && SummonSlots[SlotIndex])
//	//{
//	//	//SummonSlots[SlotIndex]->RefreshCooldown(CurrentTime, MaxTime);
//	//}
//}
void USummonControlPanel::UpdateCostDisplay(float CurrentCost, float MaxCost)
{
	if (CostWidget)
	{
		CostWidget->UpdateCost(CurrentCost, MaxCost);
	}
}
#pragma endregion 외부 인터페이스 구현
