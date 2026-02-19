// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/InGame/Actors/DamageTextActor.h"
#include "Components/WidgetComponent.h"
#include "UI/Widgets/Combat/DamageTextWidget.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "TimerManager.h"

#pragma region 생명주기
ADamageTextActor::ADamageTextActor()
{
	PrimaryActorTick.bCanEverTick = false;

	/** @section 컴포넌트 생성 및 설정 */
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidgetComponent"));
	DamageWidgetComponent->SetupAttachment(RootComp);
	DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 항상 카메라를 바라보는 스크린 모드
	DamageWidgetComponent->SetDrawSize(FVector2D(250.0f, 100.0f));
	DamageWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
#pragma endregion 생명주기

#pragma region 오브젝트 풀 인터페이스
void ADamageTextActor::OnPoolActivate_Implementation()
{
	SetActorHiddenInGame(false);
	SetActorTickEnabled(false);

	if (DamageWidgetComponent)
	{
		DamageWidgetComponent->SetVisibility(true);
	}
}

void ADamageTextActor::OnPoolDeactivate_Implementation()
{
	/** @section 1. 타이머 정리 */
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PoolReturnTimerHandle);
	}

	/** @section 2. 가시성 숨김 */
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	if (DamageWidgetComponent)
	{
		DamageWidgetComponent->SetVisibility(false);

		/** @section 3. 위젯 초기 상태로 복원 */
		if (UDamageTextWidget* DamageWidget = Cast<UDamageTextWidget>(DamageWidgetComponent->GetUserWidgetObject()))
		{
			DamageWidget->ResetWidget();
		}
	}
}
#pragma endregion 오브젝트 풀 인터페이스

#pragma region 외부 인터페이스
void ADamageTextActor::InitializeDamageText(float DamageAmount, bool bIsCritical, const FVector& WorldLocation)
{
	/** @section 1. 위치 설정 */
	SetActorLocation(WorldLocation);
	StartLocation = WorldLocation;
	TargetLocation = StartLocation + FVector(0.0f, 0.0f, FlyUpDistance);

	/** @section 2. 위젯 컴포넌트에서 내부 UDamageTextWidget을 가져와 수치를 전달합니다. */
	if (DamageWidgetComponent)
	{
		UDamageTextWidget* DamageWidget = Cast<UDamageTextWidget>(DamageWidgetComponent->GetUserWidgetObject());
		if (DamageWidget)
		{
			DamageWidget->SetDamageText(DamageAmount, bIsCritical);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[DamageTextActor] DamageTextWidget을 찾을 수 없습니다."));
		}
	}

	/** @section 3. 상승 애니메이션 (간단한 보간) */
	// 주의: 실제 프로젝트에서는 Timeline을 사용하는 것이 더 정밀합니다.
	// 현재는 DisplayTime 동안 서서히 이동합니다.
	if (FlyUpDistance > 0.0f)
	{
		// 매 프레임이 아닌 타이머로 간단히 처리 (최적화)
		// 실제로는 Timeline Component를 추천
	}

	/** @section 4. DisplayTime 이후에 스스로 풀로 돌아가도록 타이머 설정 */
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PoolReturnTimerHandle,
			this,
			&ADamageTextActor::ReturnSelfToPool,
			DisplayTime,
			false
		);
	}
}

void ADamageTextActor::ReturnSelfToPool()
{
	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->ReturnToPool(this);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[DamageTextActor] ObjectPoolSubsystem을 찾을 수 없어 직접 파괴합니다."));
			Destroy();
		}
	}
}
#pragma endregion 외부 인터페이스


