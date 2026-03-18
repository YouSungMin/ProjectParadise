// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SkillIndicatorComponent.h"
#include "Components/DecalComponent.h"
#include "Characters/Base/CharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Paradise/Paradise.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
USkillIndicatorComponent::USkillIndicatorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USkillIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		// 1. 컴포넌트 내부에서 데칼 컴포넌트를 동적으로 생성합니다.
		RangeDecal = NewObject<UDecalComponent>(Owner, TEXT("SkillRangeDecal"));
		if (RangeDecal)
		{
			// 2. 캐릭터의 Root에 부착합니다.
			RangeDecal->SetupAttachment(Owner->GetRootComponent());
			RangeDecal->RegisterComponent();

			// 3. 데칼이 바닥을 향하도록 회전시킵니다. (Pitch -90도)
			RangeDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

			// 4. 발밑으로 살짝 내려줍니다. (캐릭터의 캡슐 절반 높이 정도 내리면 좋습니다)
			RangeDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));

			// 5. 마테리얼 적용 및 초기 숨김 처리
			if (IndicatorMaterial)
			{
				RangeDecal->SetDecalMaterial(IndicatorMaterial);
			}
			RangeDecal->SetVisibility(false);
		}
	}

}

void USkillIndicatorComponent::ShowIndicator(float AttackRange, float AttackRadius, float ForwardOffset)
{
	if (RangeDecal)
	{
		// CheckHit의 기본 오프셋 보정
		float BaseOffset = 100.0f;

		// 실제 스피어 트레이스가 시작되는 중심점 위치
		float ActualTraceStart = BaseOffset + ForwardOffset;

		// 실제 물리 판정의 데칼 절반 길이
		float ActualHalfLength = (AttackRange * 0.5f) + AttackRadius;

		// 🌟 판정 여유(Leniency)를 주기 위한 시각적 축소 비율 설정
		// 1.0f = 100% (실제 판정과 동일)
		// 0.9f = 90% 크기로 표시
		float VisualScale = 0.9f;

		float VisualRadius = AttackRadius * VisualScale;
		float VisualHalfLength = ActualHalfLength * VisualScale;


		// 데칼 크기 적용 (축소된 수치 사용)
		// X: 투사 깊이, Y: 좌우 두께(VisualRadius), Z: 캡슐의 절반 길이(VisualHalfLength)
		RangeDecal->DecalSize = FVector(DecalDepth, VisualRadius, VisualHalfLength);

		// 데칼 위치 적용 (캡슐의 정중앙 유지)
		float CenterX = ActualTraceStart + (AttackRange * 0.5f);

		RangeDecal->SetRelativeLocation(FVector(CenterX, 0.0f, -80.0f));
		RangeDecal->SetVisibility(true);

		// 스캔에 필요한 수치를 저장
		CachedRange = AttackRange;
		CachedRadius = AttackRadius;
		CachedOffset = ForwardOffset;

		// 0.1초마다 ScanTargets 함수를 반복 실행
		GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &USkillIndicatorComponent::ScanTargets, 0.1f, true);

		// [로그 추가] 수치 확인용
		UE_LOG(LogTemp, Warning, TEXT("🔵 [SkillIndicator] 사거리 데이터 (공격 범위: %.1f, 공격 반경: %.1f, 전방 오프셋: %.1f) | 시각적 스케일: %.2f"), AttackRange, AttackRadius, ForwardOffset, VisualScale);
	}
}

void USkillIndicatorComponent::HideIndicator()
{
	if (RangeDecal)
	{
		RangeDecal->SetVisibility(false);
	}

	GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	ClearTargetOutlines();
}

void USkillIndicatorComponent::ScanTargets()
{
	ClearTargetOutlines(); // 이전 프레임의 외곽선 지우기

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	float BaseOffset = 100.0f;

	// CheckHit과 완벽히 동일한 물리적 트레이스 좌표 계산
	FVector TraceStart = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * (BaseOffset + CachedOffset));
	FVector TraceEnd = TraceStart + (OwnerActor->GetActorForwardVector() * CachedRange);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerActor); // 자신은 스캔 제외

	TArray<FHitResult> HitResults;



	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart, TraceEnd, CachedRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn), // 캐릭터만 스캔
		false, ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults, true , FLinearColor::Black,FLinearColor::White
	);

	if (bHit)
	{
		// 1. 트레이스가 무언가를 맞췄을 때
		UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("=== 🔵 [스캔 시작] %d 개의 물체 감지 ==="), HitResults.Num());

		for (const FHitResult& Result : HitResults)
		{
			AActor* HitActor = Result.GetActor();
			FString ActorName = HitActor ? HitActor->GetName() : TEXT("Null");

			UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("  ▶ 감지된 액터: [%s]"), *ActorName);

			ACharacterBase* HitEnemy = Cast<ACharacterBase>(HitActor);
			ACharacterBase* OwnerChar = Cast<ACharacterBase>(OwnerActor);

			// 2. 캐릭터 캐스팅 실패 (벽이나 바닥, 또는 다른 클래스를 맞춤)
			if (!HitEnemy)
			{
				UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("    -> ❌ 실패: ACharacterBase가 아닙니다. (무시)"));
				continue;
			}

			if (!OwnerChar)
			{
				UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("    -> ❌ 실패: OwnerChar(스킬 시전자)가 유효하지 않습니다!"));
				continue;
			}

			// 3. 가장 중요한 적군 판정 (IsHostile) 결과 확인
			bool bIsHostile = OwnerChar->IsHostile(HitEnemy);
			UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("    -> 🔍 IsHostile(적군 판별) 결과: %s"), bIsHostile ? TEXT("True (적군임!)") : TEXT("False (아군이거나 판정 불가)"));

			// 4. 조건을 모두 통과한 경우
			if (bIsHostile)
			{
				if (USkeletalMeshComponent* EnemyMesh = HitEnemy->GetMesh())
				{
					// 외곽선 켜기!
					EnemyMesh->SetRenderCustomDepth(true);
					HighlightedEnemies.Add(HitEnemy);

					UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("    -> 🟢 성공: [%s]에 외곽선(CustomDepth) 적용 완료!"), *ActorName);
				}
				else
				{
					UE_LOG(LogParadiseSkillIndicator, Warning, TEXT("    -> ❌ 실패: 적군이지만 스켈레탈 메쉬(Mesh)를 찾을 수 없습니다."));
				}
			}
		}
	}
	else
	{
		// 트레이스에 아무것도 안 맞았을 때 (너무 많이 출력될 수 있으니 원치 않으시면 주석 처리하세요)
		// UE_LOG(LogTemp, Warning, TEXT("🔴 [스캔] 사거리 내에 아무
	}
}

void USkillIndicatorComponent::ClearTargetOutlines()
{
	for (ACharacterBase* Enemy : HighlightedEnemies)
	{
		// 적이 중간에 죽어서 삭제되었을 수도 있으므로 IsValid 체크를 꼼꼼히 합니다.
		if (IsValid(Enemy) && Enemy->GetMesh())
		{
			// 외곽선 끄기!
			Enemy->GetMesh()->SetRenderCustomDepth(false);
		}
	}
	HighlightedEnemies.Empty(); // 리스트 비우기
}

