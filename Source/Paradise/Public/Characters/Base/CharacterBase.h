// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/CombatInterface.h"
#include "GameplayTagContainer.h"
#include "CharacterBase.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class APlayerData;

UCLASS()
class PARADISE_API ACharacterBase : public ACharacter, public ICombatInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/*
	 * @brief GAS 필수 인터페이스
	 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	/*
	 * @brief AttributeSet Getter
	 */
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/**
	 * @brief 현재 장착된 무기를 기반으로 특정 행동(평타/스킬)에 필요한 전투 데이터를 반환합니다.
	 * @detail UnitBase와 PlayerBase는 이를 오버라이딩하여 
	 * 서로 다른 데이터 테이블을 참조하여 구조체를 리턴
	 * @return FCombatActionData 몽타주, 데미지 이펙트 클래스, 데미지 계수가 포함된 구조체.
	 */
	virtual FCombatActionData GetCombatActionData(ECombatActionType ActionType) const override { return FCombatActionData();}

	/** @brief 특정 상황(EventType)에 맞는 최종 연출 데이터(Payload)를 반환합니다. (기본값: nullptr) */
	virtual TArray<struct FFXPayload*> GetFXPayloads(EFXEventType EventType) const override { return {}; }

	/*
	 * @brief 죽은후 Destroy() 하는 함수
	 * @details 적은 일단 Destory() 변경예정 , 혹은 상위 클래스에서 오버라이드
	 */
	virtual void Die();

	/** @brief 현재 캐릭터가 이동 가능한 상태인지 (태그 기반) 검사합니다. */
	UFUNCTION(BlueprintPure, Category = "Status")
	virtual bool CanMove() const;

	/* @brief 현재 캐릭터의 사망여부 체크 함수 */
	bool IsDead() const { return bIsDead; } // Getter 추가

	/*
	 * @brief 외부에서 생성된 무기 액터를 이 캐릭터의 손(Socket)에 부착하는 함수
	 * @param NewWeapon 부착할 무기 액터
	 * @param SocketName 부착할 소켓 이름
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AttachWeapon(AActor* NewWeapon, FName SocketName);

	/** * @brief [디버그용] 강제로 Die() 함수를 호출하여 사망 처리 로직을 테스트합니다.
	 * @details 콘솔 명령어(~) 창에서 "TestKillSelf"를 입력하여 실행할 수 있습니다.
	 */
	UFUNCTION(BlueprintCallable, Exec, Category = "Debug")
	void TestKillSelf();

	/*
	 * @brief 공격 판정 수행 (공용)
	 * @param SocketName : 판정의 기준이 될 소켓 이름 (예: hand_r, Jaw, WeaponTip)
	 * @param AttackRadius : 공격 사거리 (범위)
	 * @param ForwardOffset : 타격 시작점을 앞으로 얼마나 밀어낼 것인가
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHit(FName SocketName,ESocketTargetType TargetType);

	/*
	 * @brief 새로운 공격이 시작될 때 타격 목록 초기화
	 * @details 몽타주 시작이나 AnimNotifyState_Trail의 Begin 등에서 호출
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetHitActors();

	/**
	 * @brief 상대방이 적대적인지(공격 대상인지) 확인합니다.
	 * @param Target 확인할 대상 캐릭터
	 * @return true면 적군(타격 가능), false면 아군(타격 불가)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsHostile(ACharacterBase* Target) const;

	/*
	 * @brief ExecCalc에서 호출되어 데미지 텍스트를 풀에서 꺼내 띄우는 함수
	 * @param DamageAmount 데미지수치, bIsCritical 크리티컬 여부
	 */
	UFUNCTION(BlueprintCallable)
	void SpawnDamagePopup(float DamageAmount, bool bIsCritical);

	/** @brief 현재 액션 데이터 세팅 (어빌리티에서 스킬 시작 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCurrentActionData(const FCombatActionData& InData) { CurrentActiveActionData = InData; }

	/** @brief 현재 액션 데이터 반환 (노티파이에서 타격 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatActionData GetCurrentActionData() const { return CurrentActiveActionData; }

	/** @brief 자식 클래스들이 무기 메쉬를 던져줄 수 있도록 가상 함수 선언 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual USceneComponent* GetWeaponMesh() const;

	/** @brief 사망 몽타주를 반환하는 가상 함수 (자식 클래스에서 오버라이드) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UAnimMontage* GetDeathMontage() const { return nullptr; }

	/** @brief 피격 몽타주를 반환하는 가상 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UAnimMontage* GetHitMontage() const { return nullptr; }

	/** @brief 데미지를 받았을 때 호출되는 피격 연출 통합 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void PlayHitReaction();

	/** @brief 애니메이션이 끝나거나 노티파이에서 호출될 가상 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnDeathAnimationFinished();

	/** @brief 노티파이에서 발사 소켓 정보(타겟+이름)를 캐릭터에 저장(캐싱)해주는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCurrentMuzzleSocketInfo(FName InSocketName, ESocketTargetType InSocketTarget);

	/** @brief 캐싱된 타겟(무기 or 몸체)을 기준으로 최종 발사 트랜스폼(위치+회전) 반환 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FTransform GetCurrentMuzzleTransform() const;
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/*
	 * @brief Hit 이펙트를 적용 시키는 함수 (마테리얼 변화)
	 */
	UFUNCTION(BlueprintCallable)
	void PlayHitFlash();

	/*
	 * @brief Hit 이펙트를 리셋 시키는 함수 (마테리얼 변화)
	*/
	UFUNCTION(BlueprintCallable)
	void ResetHitFlash();
public:
	/** * @brief 소속 진영 태그
		 * @details 데이터 테이블에서 로드되어 부여되며, 피아식별(IsHostile) 및 GAS 타겟팅에 사용됩니다.
		 * 예: Unit.Faction.Friendly.Player, Unit.Faction.Enemy
		 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	FGameplayTag FactionTag;

protected:
	/*
	 * @brief 사망 여부 체크 플래그
	*/
	bool bIsDead = false;

	/*
	 * @brief 머리위 hp위젯컴포넌트
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UWidgetComponent> HealthWidget = nullptr;

	/*
	 * @brief 데미지팝업위젯 액터 할당예정 (클래스타입도 해당 UI타입으로 변경예정)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamagePopupActor")
	TSubclassOf<class AActor> DamageTextActorClass = nullptr;

	/*
	 * @brief 실제 무기 액터 인스턴스
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> CurrentWeaponActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	/*
	 * @brief 현재 실행 중인 전투 액션의 임시 데이터 (택배 상자)
	 * @details GA가 실행될 때 세팅해주며, AnimNotify가 꺼내서 CheckHit에 사용합니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Data")
	FCombatActionData CurrentActiveActionData;

	/*
	 * @brief 이미 때린 적을 중복 타격하지 않게 저장하는 목록
	 * @details 공격 시작 시(ResetHitActors) 비워줘야 함
	 */
	UPROPERTY()
	TArray<AActor*> HitActors;

	// 에디터에서 할당할 데미지 텍스트 블루프린트 클래스 (BP_DamageTextActor)
	UPROPERTY(EditAnywhere, Category = "Combat|UI")
	TSubclassOf<class ADamageTextActor> DamageTextClass;

	/** @brief 투사체를 발사할 최신 소켓 이름 (노티파이에서 세팅됨) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	FName CurrentMuzzleSocketName = NAME_None;

	/** @brief 투사체 발사 소켓을 찾을 메쉬 대상 (몸체 vs 무기) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cached")
	ESocketTargetType CurrentMuzzleSocketTarget = ESocketTargetType::CharacterBody;

private:
		
	/*
	 * @brief 피격 이펙트 타이머 핸들
	*/
	FTimerHandle HitEffectTimerHandle;

	
	/*
	 * @brief 피격 이펙트 리셋 시간 ex) 3초후 리셋
	*/
	float HitResetTime = 0.3f;
};
