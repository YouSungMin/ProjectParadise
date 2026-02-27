// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/CharacterBase.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/CombatInterface.h"
#include "Data/Enums/GameEnums.h"
#include "PlayerBase.generated.h"


class UInputAction;
struct FInputActionValue;
/**
 * 
 */
UCLASS()
class PARADISE_API APlayerBase : public ACharacterBase
{
	GENERATED_BODY()
	

public:
	APlayerBase();

	virtual void PossessedBy(AController* NewController) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

	/**
	 * @brief 생성자에서 컴포넌트를 생성합니다.
	 */
	void InitializeComponents();

	/**
	 * @brief 슬롯에 해당하는 스켈레탈 메쉬 컴포넌트를 반환합니다.
	 * @details EquipmentComponent에서 외형 변경 시 호출합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "Visual")
	USkeletalMeshComponent* GetArmorComponent(EEquipmentSlot Slot) const;

	/**
	 * @brief 현재 장착된 무기를 기반으로 특정 행동(평타/스킬)에 필요한 전투 데이터를 반환합니다.
	 * * @details ICombatInterface의 구현부입니다.
	 * 1. PlayerData와 EquipmentComponent를 통해 현재 무기 ID를 조회합니다.
	 * 2. GameInstance의 데이터 테이블에서 해당 무기의 에셋(몽타주)과 스탯(계수)을 검색합니다.
	 * 3. 요청된 ActionType(평타 vs 스킬)에 맞춰 데이터를 패키징하여 반환합니다.
	 * * @param ActionType 수행하려는 행동 타입 (예: BasicAttack, WeaponSkill 등).
	 * @return FCombatActionData 몽타주, 데미지 이펙트 클래스, 데미지 계수가 포함된 구조체. (무기가 없으면 빈 구조체 반환)
	 */
	virtual FCombatActionData GetCombatActionData(ECombatActionType ActionType) const override;

	/** @brief 특정 상황(EventType)에 맞는 최종 연출 데이터(Payload)를 반환합니다. (기본값: nullptr) */
	virtual struct FFXPayload* GetFXPayload(EFXEventType EventType) const override;

	UFUNCTION(BlueprintCallable)
	class APlayerData* GetPlayerData() const { return LinkedPlayerData.Get(); }

	/** @brief 캐릭터 사망 여부 반환 (컨트롤러 확인용) */
	UFUNCTION(BlueprintPure, Category = "Status")
	bool IsDead() const { return bIsDead; }

	/** * @brief [옵션 1] 2.5D Default
	 * @details 멀리서 망원 렌즈로 당겨 찍는 느낌. 원근 왜곡이 적어 거리 재기가 편함.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Preset")
	void SetCamera_Default();

	/** * @brief [옵션 2] 클래식
	 * @details 적당한 거리감과 하이 앵글. 깊이감이 잘 느껴지는 스탠다드 횡스크롤.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Preset")
	void SetCamera_Classic();

	/** * @brief [옵션 3] 다이나믹
	 * @details 가까운 거리, 광각 렌즈. 캐릭터 동작이 크고 시원하게 보임.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Preset")
	void SetCamera_Dynamic();

	/** * @brief 카메라 모드를 순차적으로 변경합니다. (0 -> 1 -> 2 -> 0 loop)
	 * @details Default -> Classic -> Dynamic 순서로 변경됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Control")
	void SwitchCameraMode();

	/*
	* @brief 플레이어가 죽은후 플레이어 데이터 액터에게 알림
	*/
	virtual void Die() override;

	/**
	 * @brief 캐릭터 스폰 직후, 영혼(PlayerData)과 육체를 연결하고 외형을 초기화합니다.
	 * @details
	 * - GAS의 오너(Owner)와 아바타(Avatar) 관계를 설정합니다.
	 * - 연결된 영혼이 보유한 장비 데이터를 기반으로 즉시 비주얼을 렌더링합니다.
	 * @param InPlayerData 이 육체를 제어할 데이터 주체(영혼) 액터
	 */
	void InitializePlayer(APlayerData* InPlayerData);

	/**
	 * @brief 입력 액션이 들어오면 ASC로 신호를 보내는 배달부 함수
	 * @param InputId : 어떤 키인가? (Enum)
	 * @param bIsPressed : 눌렀는가(true), 뗐는가(false)
	 */
	void SendAbilityInputToASC(EInputID InputId, bool bIsPressed);

	/** @brief 소켓 이름을 주면 위치(좌표)를 반환하는 부모의 가상 함수 오버라이드*/
	virtual USceneComponent* GetWeaponMesh() const override;
protected:

	/*
	* @brief IA_Move 입력액션에 바인딩할 함수
	 * @param InValue 입력 액션의 입력값
	 */
	UFUNCTION()
	void OnMoveInput(const FInputActionValue& InValue);


protected:

	/** 현재 카메라 모드 인덱스 (0: Default, 1: Classic, 2: Dynamic) */
	int32 CurrentCameraIndex = 0;

	/**  모듈형 캐릭터 파츠(Body는 ACharacter의 Mesh 사용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMeshComponent> HatMesh = nullptr;

	/** 
	  * @brief 무기 전용 메쉬 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh = nullptr;

	/*
	 * @brief 입력 액션 Move
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;


	/*
	 * @brief 전투 스킬(GAS)용 입력 설정 데이터 에셋
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UParadiseInputConfig> InputConfig = nullptr;

	/*
	 * @brief 스프링암 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom = nullptr;


	/*
	 * @brief 카메라 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<class UCameraComponent> FollowCamera = nullptr;

	
	/*
	 * @brief 클래스 타입 변경예정 ,실제 데이터를 가진 액터
	 */
	UPROPERTY()
	TWeakObjectPtr<class APlayerData> LinkedPlayerData = nullptr;

	/*
	 * @brief 무기 붙일 소켓 이름
	 * @details 부착될 소켓이름도 데이터 에셋에 있을예정
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FName WeaponSocketName;
};
