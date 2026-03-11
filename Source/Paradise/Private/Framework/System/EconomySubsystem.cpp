// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/ParadiseSaveGame.h"
#include "Paradise/Paradise.h"

void UEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Wallet.Add(ECurrencyType::Gold, 0);
	Wallet.Add(ECurrencyType::Aether, 0);
	Wallet.Add(ECurrencyType::SummonTicket, 0);

	UE_LOG(LogParadiseEconomy, Log, TEXT("💰 [EconomySubsystem] 경제 서브시스템 초기화 완료."));
}

int32 UEconomySubsystem::GetCurrency(ECurrencyType Type) const
{
	// TMap의 Find는 키가 존재하면 값의 포인터를, 없으면 nullptr을 반환합니다.
	if (const int32* FoundAmount = Wallet.Find(Type))
	{
		return *FoundAmount;
	}

	// 맵에 해당 재화가 없다면 기본값 0 반환
	return 0;
}

void UEconomySubsystem::AddCurrency(ECurrencyType Type, int32 Amount)
{
	//0 이하 획득 시도 무시
	if (Amount <= 0) return;

	int32 OldAmount = GetCurrency(Type);
	int32 NewAmount = OldAmount + Amount;

	//새로운 값으로 재화 설정
	Wallet.Add(Type, NewAmount);

	UE_LOG(LogParadiseEconomy, Log, TEXT("💎 [Economy] %d 획득. (현재 %d)"), Amount, NewAmount);

	//UI 갱신용 델리게이트 발송
	OnCurrencyChanged.Broadcast(Type, OldAmount, NewAmount);
}

bool UEconomySubsystem::ConsumeCurrency(ECurrencyType Type, int32 Amount)
{
	// 0원 이하 소모는 항상 성공
	if (Amount <= 0) return true;

	//재화가 충분한지 검사
	if (!HasEnoughCurrency(Type, Amount))
	{
		UE_LOG(LogParadiseEconomy, Warning, TEXT("💸 [Economy] 소모 실패: 잔액 부족! (필요: %d / 보유: %d)"), Amount, GetCurrency(Type));
		return false; 
	}

	int32 OldAmount = GetCurrency(Type);
	int32 NewAmount = OldAmount - Amount;

	//지갑에서 재화 차감
	Wallet.Add(Type, NewAmount);

	UE_LOG(LogParadiseEconomy, Log, TEXT("💸 [Economy] %d 소모 완료. (남은 잔액: %d)"), Amount, NewAmount);

	//UI 갱신용 델리게이트 발송
	OnCurrencyChanged.Broadcast(Type, OldAmount, NewAmount);

	return true;
}

bool UEconomySubsystem::HasEnoughCurrency(ECurrencyType Type, int32 Amount) const
{
	return GetCurrency(Type) >= Amount;
}

void UEconomySubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
	if (!SaveGameObj) return;

	// 세이브된 지갑 데이터들을 순회하면서 현재 지갑(Wallet)에 덮어씌우기
	for (const auto& Pair : SaveGameObj->SavedWallet)
	{
		Wallet.Add(Pair.Key, Pair.Value);
	}
	UE_LOG(LogParadiseEconomy, Log, TEXT("💰 [Economy] 세이브 파일에서 재화 정보를 성공적으로 불러왔습니다."));
}

void UEconomySubsystem::SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const
{
	if (!SaveGameObj) return;

	// 현재 서브시스템이 가진 지갑 데이터를 세이브 객체로 그대로 복사
	SaveGameObj->SavedWallet = Wallet;

	UE_LOG(LogParadiseEconomy, Log, TEXT("💾 [Economy] 세이브 객체에 재화 정보를 기록했습니다."));
}