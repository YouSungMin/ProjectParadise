// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Enums/GameEnums.h"
#include "EconomySubsystem.generated.h"


/**
 * @delegate FOnCurrencyChangedSignature
 * @brief 재화 보유량이 변경되었을 때 호출되는 델리게이트입니다.
 * @param CurrencyType 변경된 재화의 종류 (Gold, Gem, SummonTicket 등)
 * @param OldAmount 변경되기 전의 기존 재화량
 * @param NewAmount 변경된 후의 최종 재화량
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCurrencyChangedSignature, ECurrencyType, CurrencyType, int32, OldAmount, int32, NewAmount);
/**
 * @class UEconomySubsystem
 * @brief 게임 내 모든 재화(화폐)의 획득, 소모, 보유량 조회를 전담하는 경제 매니저 클래스입니다.
 * @details GameInstance 수명주기와 함께하며, 맵 전환 시에도 유저의 재화 데이터가 안전하게 유지됩니다.
 * 상점, 가챠, 스테이지 보상 등 재화가 오가는 모든 시스템의 허브 역할을 수행합니다.
 */
UCLASS()
class PARADISE_API UEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

	/**
	 * @brief 서브시스템이 생성될 때 최초 1회 호출되는 초기화 함수입니다.
	 * @details 내부적으로 Wallet(지갑) 맵을 순회하며 초기값을 0으로 세팅합니다.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** * @brief 특정 재화의 현재 보유량을 반환합니다.
	 * @param Type 조회할 재화의 종류
	 * @return 현재 보유 중인 해당 재화의 수량 (발견하지 못할 경우 0)
	 */
	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetCurrency(ECurrencyType Type) const;

	/** * @brief 특정 재화를 지갑에 추가(획득)합니다.
	 * @param Type 획득할 재화의 종류
	 * @param Amount 획득할 수량 (0 이하는 무시됨)
	 * @details 호출 완료 시 OnCurrencyChanged 델리게이트가 자동으로 방송(Broadcast)됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddCurrency(ECurrencyType Type, int32 Amount);

	/** * @brief 특정 재화를 지갑에서 차감(소모)합니다.
	 * @param Type 소모할 재화의 종류
	 * @param Amount 소모할 수량
	 * @return 소모에 성공하면 true, 잔액이 부족하여 실패하면 false를 반환합니다.
	 * @details 잔액 검사를 먼저 수행하며, 소모에 성공했을 때만 재화가 차감되고 OnCurrencyChanged가 방송됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool ConsumeCurrency(ECurrencyType Type, int32 Amount);

	/** * @brief 특정 재화를 요구량만큼 충분히 가지고 있는지 검사합니다.
	 * @param Type 검사할 재화의 종류
	 * @param Amount 필요한 요구 수량
	 * @return 보유량이 요구량 이상이면 true, 부족하면 false 반환
	 */
	UFUNCTION(BlueprintPure, Category = "Economy")
	bool HasEnoughCurrency(ECurrencyType Type, int32 Amount) const;

	// 세이브 / 로드 연동
	/**
	 * @brief 세이브 파일 객체에서 저장된 지갑 데이터를 읽어와 메모리에 복구합니다.
	 * @param SaveGameObj 로드된 세이브 게임 객체 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy|Save")
	void LoadFromSaveGame(class UParadiseSaveGame* SaveGameObj);

	/**
	 * @brief 현재 서브시스템이 가진 지갑 데이터를 세이브 파일 객체에 기록합니다.
	 * @param SaveGameObj 기록할 세이브 게임 객체 포인터
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy|Save")
	void SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const;


public:

	/** * @brief 재화 수량이 바뀔 때마다 UI(상단 바, 로비 등)에 갱신 신호를 보내는 이벤트 디스패처입니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Economy|Event")
	FOnCurrencyChangedSignature OnCurrencyChanged;

private:
	/** * @brief 핵심 데이터: 플레이어의 재화 지갑 (TMap)
	* @details 재화 종류(Key)와 보유량(Value)을 매핑하여 저장합니다.
	*/
	UPROPERTY()
	TMap<ECurrencyType, int32> Wallet;
};
