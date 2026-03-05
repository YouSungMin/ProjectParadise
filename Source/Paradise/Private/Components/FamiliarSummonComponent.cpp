// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FamiliarSummonComponent.h"
#include "Components/CostManageComponent.h"
#include "Framework/InGame/InGamePlayerState.h"
#include "Framework/InGame/InGameController.h"
#include "Characters/AIUnit/UnitBase.h"
#include "Framework/Core/ParadiseGameInstance.h"
#include "Framework/System/SquadSubsystem.h"
#include "Objects/FamiliarSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "Data/Structs/UnitStructs.h"
#include "TimerManager.h"

UFamiliarSummonComponent::UFamiliarSummonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFamiliarSummonComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 전에 편성한 유닛들의 데이터를 메모리에 올려놓기
	InitializeDeckPool();

	// 게임 시작 시 슬롯 채우기
	RefreshAllSlots();

	// 게임 시작 시 월드에 있는 스포너 찾아서 저장함
	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AFamiliarSpawner::StaticClass());
	if (FoundActor)
	{
		LinkedSpawner = Cast<AFamiliarSpawner>(FoundActor);
		UE_LOG(LogTemp, Log, TEXT("✅ 스포너를 찾아서 연결했습니다."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ 월드에 배치된 AFamiliarSpawner가 없습니다!"));
	}
}

void UFamiliarSummonComponent::InitializeDeckPool()
{
	UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	USquadSubsystem* SquadSys = GI->GetSubsystem<USquadSubsystem>();
	if (!SquadSys) return;

	CachedDeckPool.Empty();
	const TArray<FName>& SelectedIDs = SquadSys->GetFamiliarSquad();

	UE_LOG(LogTemp, Warning, TEXT("========== 🎴 [내 덱 캐싱 시작] 🎴 =========="));

	for (const FName& UnitID : SelectedIDs)
	{
		if (UnitID.IsNone()) continue;

		FSummonSlotInfo CachedSlot;
		CachedSlot.FamiliarID = UnitID;

		//0305 김성현 - 데이터 테이블 조회 방식 템플릿 함수 사용으로 변경
		// 데이터 테이블을 여기서 단 한 번만 조회합니다.
		if (FFamiliarStats* Stats = GI->GetDataTableRow<FFamiliarStats>(GI->FamiliarStatsDataTable,UnitID))
		{
			CachedSlot.CardCost = Stats->SummonCost;
		}
		if (FFamiliarAssets* Assets = GI->GetDataTableRow<FFamiliarAssets>(GI->FamiliarAssetsDataTable, UnitID))
		{
			CachedSlot.CardIcon = Assets->FaceIcon;
		}

		CachedDeckPool.Add(CachedSlot);
		UE_LOG(LogTemp, Log, TEXT("  [덱에 추가됨] 유닛: %s | 가격: %d"), *UnitID.ToString(), CachedSlot.CardCost);
	}

	if (CachedDeckPool.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [FamiliarSummon] 편성된 유닛이 하나도 없습니다! 편성창을 확인하세요."));
	}

	UE_LOG(LogTemp, Warning, TEXT("============================================="));
}

void UFamiliarSummonComponent::InjectCharacterRespawnCard(int32 TargetCharacterIndex, int32 RespawnCost, TSoftObjectPtr<UTexture2D> InCardIcon)
{
	FSummonSlotInfo RespawnCard;
	RespawnCard.CardType = ESummonCardType::CharacterRespawn;
	RespawnCard.CharacterIndex = TargetCharacterIndex;
	RespawnCard.CardCost = RespawnCost;
	RespawnCard.CardIcon = InCardIcon;
	RespawnCard.FamiliarID = FName(*FString::Printf(TEXT("Character_%d"), TargetCharacterIndex));

	//리스폰 가능해진 캐릭터를 스폰 가능 덱 풀에 추가
	CachedDeckPool.Add(RespawnCard);

	UE_LOG(LogTemp, Warning, TEXT("🔀 [SummonDeck] %d번 캐릭터 부활 카드가 덱에 섞였습니다! (현재 덱 크기: %d)"), TargetCharacterIndex, CachedDeckPool.Num());
}

void UFamiliarSummonComponent::RefreshAllSlots()
{
	// 캐싱된 덱이 없으면 진행 불가
	if (CachedDeckPool.IsEmpty()) return;

	UE_LOG(LogTemp, Warning, TEXT("========== 🎰 [게임 시작: 5장 드로우] 🎰 =========="));
	CurrentSlots.Empty();

	// 메모리 풀(CachedDeckPool)에서 무작위로 5장을 뽑습니다.
	for (int32 i = 0; i < MaxSlotCount; i++)
	{
		FSummonSlotInfo NewCard = DrawRandomCardFromPool();
		CurrentSlots.Add(NewCard);

		UE_LOG(LogTemp, Log, TEXT("[%d번 슬롯] 유닛: %s | 가격: %d"), i + 1, *NewCard.FamiliarID.ToString(), NewCard.CardCost);
	}

	UE_LOG(LogTemp, Warning, TEXT("==================================================="));

	// UI 갱신 방송
	if (OnSummonSlotsUpdated.IsBound())
	{
		OnSummonSlotsUpdated.Broadcast(CurrentSlots);
	}

	//// 1. GameInstance 가져오기
	//UParadiseGameInstance* GI = Cast<UParadiseGameInstance>(GetWorld()->GetGameInstance());
	//if (!GI) return;

	//// 2. 인스턴스에 선언된 변수명 그대로 사용
	//UDataTable* StatsTable = GI->FamiliarStatsDataTable;
	//UDataTable* AssetsTable = GI->FamiliarAssetsDataTable;

	//if (!StatsTable || !AssetsTable) return;
	//UE_LOG(LogTemp, Warning, TEXT("========== 🎰 [상점 리스트 갱신] 🎰 =========="));
	//
	//CurrentSlots.Empty();

	////5개 슬롯을 랜덤 유닛으로 채움
	//for (int32 i = 0; i < MaxSlotCount; i++)
	//{
	//	//슬롯하나 생성 
	//	FSummonSlotInfo NewSlot = GenerateRandomSlot(StatsTable, AssetsTable);
	//	//배열에 추가
	//	CurrentSlots.Add(NewSlot);

	//	// [디버깅용] 가격 슬롯 1-5까지 출력
	//	UE_LOG(LogTemp, Log, TEXT("[%d번 키] 유닛: %s | 가격: %d"),
	//		i + 1, *NewSlot.FamiliarID.ToString(), NewSlot.FamiliarCost);
	//}
	//
	//UE_LOG(LogTemp, Warning, TEXT("=============================================="));

	//// UI에 슬롯 갱신 알림(델리게이트 호출)
	//if (OnSummonSlotsUpdated.IsBound())	OnSummonSlotsUpdated.Broadcast(CurrentSlots);
}

bool UFamiliarSummonComponent::RequestPurchase(int32 SlotIndex)
{
	//슬롯 유효성 검사
	if (!CurrentSlots.IsValidIndex(SlotIndex)) return false;
	//if (CurrentSlots[SlotIndex].bIsSoldOut)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("빈 슬롯입니다. 쿨타임 대기 중.."))
	//		return false;
	//}

	//PlayerState 및 CostManger 가져오기
	AInGamePlayerState* PS = GetOwner<AInGamePlayerState>();
	if (!PS) return false;

	UCostManageComponent* CostManager = PS->GetCostManageComponent();
	//슬롯 정보 가져오기
	FSummonSlotInfo& SlotInfo = CurrentSlots[SlotIndex];

	//비용 비교 및 결제(돈이 부족하면 false처리)
	float PriceToPay = (float)SlotInfo.CardCost; // 혹은 FinalCost
	if (!CostManager || !CostManager->TrySpendCost(PriceToPay))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ 잔액 부족! (필요: %.0f)"), PriceToPay);
		return false;
	}

	//0305 김성현 - 캐릭터 부활 로직 추가
	switch (SlotInfo.CardType)
	{
	case ESummonCardType::CharacterRespawn:
	{
		// 캐릭터 부활 카드일 경우 컨트롤러를 찾아 부활 함수 호출
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AInGameController* InGamePC = Cast<AInGameController>(PC))
			{
				InGamePC->RespawnSquadPlayer(SlotInfo.CharacterIndex);
			}
		}
		break;
	}
	case ESummonCardType::Familiar:
	{
		// 일반 퍼밀리어 소환 로직
		if (LinkedSpawner)
		{
			LinkedSpawner->SpawnFamiliarByID(SlotInfo.FamiliarID);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ 월드에서 AFamiliarSpawner를 찾을 수 없습니다!"));
		}
		break;
	}
	}

	// 소환/부활 성공 후 슬롯을 당기는 공통 로직
	ConsumeSpecificSlot(SlotIndex);
	return true;
}


void UFamiliarSummonComponent::RegisterSpawner(AFamiliarSpawner* NewSpanwer)
{
	LinkedSpawner = NewSpanwer;
	UE_LOG(LogTemp, Warning, TEXT("✅ Familiar Spawner Linked 성공"));
}

//구매한 슬롯 제거하고 맨 뒤에 채우는 함수(Queue 방식)
void UFamiliarSummonComponent::ConsumeSpecificSlot(int32 SlotIndex)
{
	if (!CurrentSlots.IsValidIndex(SlotIndex)) return;

	//선택한 슬롯을 배열에서 제거
	//TArray의 RemoveAt 쓰면 요소들이 자동으로 앞으로 한칸씩 당겨짐
	CurrentSlots.RemoveAt(SlotIndex);

	// 덱에서 즉시(O(1)) 새로운 카드를 한 장 뽑아서 맨 뒤에 추가
	FSummonSlotInfo NewCard = DrawRandomCardFromPool();
	CurrentSlots.Add(NewCard);

	UE_LOG(LogTemp, Log, TEXT("🔥 슬롯[%d] 사용 완료 -> 맨 뒤에 [%s] 추가됨"), SlotIndex, *NewCard.FamiliarID.ToString());

	// 3. UI 갱신 방송
	if (OnSummonSlotsUpdated.IsBound())
	{
		OnSummonSlotsUpdated.Broadcast(CurrentSlots);
	}
}

FSummonSlotInfo UFamiliarSummonComponent::DrawRandomCardFromPool()
{
	// 캐싱된 덱 풀에서 무작위 인덱스를 뽑아 즉시 리턴 (데이터 테이블 검색 0회)
	int32 RandomIndex = FMath::RandRange(0, CachedDeckPool.Num() - 1);
	FSummonSlotInfo DrawnCard = CachedDeckPool[RandomIndex];

	//0305 김성현 - 캐릭터 리스폰 로직 추가
	//캐릭터 부활 카드가 나왔다면 덱 풀에서 제거
	if (DrawnCard.CardType == ESummonCardType::CharacterRespawn)
	{
		CachedDeckPool.RemoveAt(RandomIndex);
		UE_LOG(LogTemp, Warning, TEXT("🎟️ [SummonDeck] 부활 카드가 뽑혔습니다! (덱에서 제거됨)"));
	}

	return DrawnCard;
}
