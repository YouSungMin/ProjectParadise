// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpec.h"
#include "BTTaskNode_BossGASAttack.generated.h"


struct FAbilityEndedData;
/**
 * 
 */
UCLASS()
class PARADISE_API UBTTaskNode_BossGASAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTaskNode_BossGASAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:

	/** 에디터에서 보스가 발동할 어빌리티 태그 (Ability.Type.BossPattern 서브태그)지정 */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag AbilityTagToActivate;

	/** 타겟을 바라보기 위해 블랙보드에서 타겟을 가져올 키 */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FBlackboardKeySelector TargetKey;

private:
	//스킬이 끝나면 ASC가 이 함수를 호출(이벤트)해 줍니다!
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	// 이벤트를 받았을 때 트리를 깨우기 위해 컴포넌트를 기억해 둡니다.
	UPROPERTY()
	UBehaviorTreeComponent* CachedOwnerComp;

	UPROPERTY()
	class UAbilitySystemComponent* CachedASC;

	// 이벤트를 구독(Bind)했다는 핸들
	FDelegateHandle AbilityEndedDelegateHandle;
};
