// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Base/CharacterBase.h"
#include "Framework/System/ObjectPoolSubsystem.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Framework/InGame/Actors/DamageTextActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/ObjectPoolInterface.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	HealthWidget->SetupAttachment(RootComponent);

	HealthWidget->SetWidgetSpace(EWidgetSpace::Screen);
	//UI 만들어지면 사이즈 조정 예정
	HealthWidget->SetDrawSize(FVector2D(300.0f,50.0f));

}


void ACharacterBase::TestKillSelf()
{
	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [Debug] 이미 사망한 상태입니다."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("💀 [Debug] 강제 사망 명령 실행! (TestKillSelf)"));
	Die();
}

void ACharacterBase::CheckHit(FName SocketName, float AttackRange, float AttackRadius, float ForwardOffset)
{
	FVector TraceStart;

	// 소켓 위치 찾기 시도
	if (GetMesh()->DoesSocketExist(SocketName))
	{
		TraceStart = GetMesh()->GetSocketLocation(SocketName);
	}
	else
	{
		// ⚠️ 예외 처리: 소켓이 없거나 이름이 틀렸을 때
		// 캐릭터의 위치 + 전방 100cm 앞을 타격 지점으로 설정
		TraceStart = GetActorLocation() + (GetActorForwardVector() * 100.0f);

		// 디버그용 로그
		// UE_LOG(LogTemp, Warning, TEXT("[%s] 소켓(%s)을 찾을 수 없어 전방 위치를 사용합니다."), *GetName(), *SocketName.ToString());
	}

	// ForwardOffset 적용: 시작점을 캐릭터 전방으로 밀어줍니다.
	TraceStart += GetActorForwardVector() * ForwardOffset;

	// 사거리(AttackRange) 적용: 밀어낸 시작점으로부터 '사거리'만큼 뻗어나갑니다. 
	FVector TraceEnd = TraceStart + (GetActorForwardVector() * AttackRange);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this); // 나는 때리면 안 됨

	TArray<FHitResult> HitResults;
	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart,      // 시작점
		TraceEnd,        // 끝점 (앞으로 길게 뻗음)
		AttackRadius,    // 반경
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true
	);

	// 3. 결과 처리
	if (bHit)
	{
		for (const FHitResult& Result : HitResults)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor) continue;

			// ACharacterBase로 캐스팅 시도
			ACharacterBase* HitChar = Cast<ACharacterBase>(HitActor);

			// 캐스팅 실패 시 (바닥, 배경 프랍 등) 무시하고 다음 타겟으로 넘어감
			if (HitChar == nullptr)
			{
				// UE_LOG(LogTemp, Log, TEXT("Skipping non-character actor: %s"), *HitActor->GetName());
				continue;
			}

			// 중복 타격 방지 (캐릭터인 경우에만 리스트에 추가)
			if (HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			// 피아 식별 (이미 위에서 HitChar를 구했으므로 바로 사용)
			if (!IsHostile(HitChar))
			{
				continue;
			}

			// GAS 이벤트 전송
			FGameplayEventData Payload;
			Payload.Instigator = this;
			Payload.Target = HitActor;
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Result);

			// 태그를 고정하거나, 인자로 받을 수도 있음
			FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.Hit"));

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, HitTag, Payload);

			UE_LOG(LogTemp, Log, TEXT("⚔️ [%s] 타격 성공! 대상: %s (소켓: %s)"), *GetName(), *HitActor->GetName(), *SocketName.ToString());
		}
	}
}

void ACharacterBase::ResetHitActors()
{
	HitActors.Empty();
}

bool ACharacterBase::IsHostile(ACharacterBase* Target) const
{
	if (!Target) return false;

	// 태그가 완전히 똑같으면 무조건 아군
	if (this->FactionTag == Target->FactionTag) return false;

	// 상위 태그(부모 태그)를 기준으로 그룹 검사
	FGameplayTag FriendlyTag = FGameplayTag::RequestGameplayTag("Unit.Faction.Friendly");

	bool bAmIFriendly = this->FactionTag.MatchesTag(FriendlyTag);
	bool bIsTargetFriendly = Target->FactionTag.MatchesTag(FriendlyTag);

	// Friendly 그룹이고 상대도 Friendly 그룹이면 false (아군)
	// Friendly 그룹인데 상대가 아니면(Enemy면) true (적군)
	return bAmIFriendly != bIsTargetFriendly;
}

FVector ACharacterBase::GetMuzzleLocation(FName SocketName) const
{
	// 장착한 무기가 있다면 무기에서 먼저 찾기
	if (CurrentWeaponActor)
	{
		// 무기 액터의 첫 번째 메쉬 컴포넌트를 가져옴
		if (UMeshComponent* WeaponMesh = CurrentWeaponActor->GetComponentByClass<UMeshComponent>())
		{
			if (WeaponMesh->DoesSocketExist(SocketName))
			{
				return WeaponMesh->GetSocketLocation(SocketName);
			}
		}
	}

	// 무기가 없거나 무기에 소켓이 없다면 캐릭터 몸통에서 찾기
	if (GetMesh() && GetMesh()->DoesSocketExist(SocketName))
	{
		return GetMesh()->GetSocketLocation(SocketName);
	}

	// 둘 다 없으면 기본 높이(가슴/배꼽) 반환
	return GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}


void ACharacterBase::AttachWeapon(AActor* NewWeapon, FName SocketName)
{
	//기존 무기 정리
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
	}

	if (!NewWeapon || !GetMesh()) return;

	//새 무기 등록
	CurrentWeaponActor = NewWeapon;

	//소켓에 부착 (SnapToTarget: 위치/회전/크기 모두 소켓 기준)
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	CurrentWeaponActor->AttachToComponent(GetMesh(), AttachRules, SocketName);

	//소유자 설정 (GAS 데미지 계산 시 Instigator로 활용됨)
	CurrentWeaponActor->SetOwner(this);

	UE_LOG(LogTemp, Warning, TEXT("⚔️ [CharacterBase] 무기 장착 완료: %s -> 소켓: %s"),
		*NewWeapon->GetName(), *SocketName.ToString());
}

void ACharacterBase::PlayHitFlash()
{
	if (USkeletalMeshComponent* MyMesh = GetMesh())
	{
		//0번인덱스의 커스텀 프리미티브 데이터 1.0f 로 변경 //intensity
		MyMesh->SetCustomPrimitiveDataFloat(0, 1.0f);
		//1~3번인덱스의 벡터값 변경 //Red로 변하게
		MyMesh->SetCustomPrimitiveDataVector3(1, FVector(1.0f, 0.0f, 0.0f));

		//4번인덱스의 float값 변경 //투명도
		MyMesh->SetCustomPrimitiveDataFloat(4, 100.0f);
	}

	//3초후 이펙트 리셋 함수호출
	GetWorldTimerManager().SetTimer(
		HitEffectTimerHandle,
		this,
		&ACharacterBase::ResetHitFlash,
		HitResetTime,
		false
	);
}

void ACharacterBase::ResetHitFlash()
{
	if (USkeletalMeshComponent* MyMesh = GetMesh())
	{
		//0번인덱스의 커스텀 프리미티브 데이터 0.0f 로 리셋
		MyMesh->SetCustomPrimitiveDataFloat(0, 0.0f);

		//1~3번인덱스의 벡터값 변경 //Red로 변하게
		MyMesh->SetCustomPrimitiveDataVector3(1, FVector(0.0f, 0.0f, 0.0f));

		//4번인덱스의 float값 변경 //투명도
		MyMesh->SetCustomPrimitiveDataFloat(4, 0.0f);
	}
}

void ACharacterBase::SpawnDamagePopup(float DamageAmount, bool bIsCritical)
{
	// 클래스가 비어있으면 안전하게 리턴
	if (!DamageTextClass) return;

	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			// 타겟 머리 위 위치 계산 (캐릭터 Z축 위로 80cm 정도 띄움)
			UE_LOG(LogTemp,Log,TEXT("SpawnDamagePopup"));
			FVector SpawnLoc = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);

			ADamageTextActor* DmgText = PoolSubsystem->SpawnPoolActor<ADamageTextActor>(
				DamageTextClass,        // 스폰할 클래스
				SpawnLoc,               // 위치
				FRotator::ZeroRotator,  // 회전
				this,                   // Owner
				this                    // Instigator
			);

			// 데미지 수치 초기화
			if (DmgText)
			{
				DmgText->InitializeDamageText(DamageAmount, bIsCritical, SpawnLoc);
			}
		}
	}
}

void ACharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ACharacterBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	UE_LOG(LogTemp, Error, TEXT("☠️ [CharacterBase] Die() 로직 시작 - 래그돌 전환"));

	//물리적 처리 (서 있는 캡슐은 끄고, 메쉬는 흐물거리는 래그돌로)
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetMesh())
	{
		// 래그돌 프리셋 적용 (PhysicsAsset이 설정되어 있어야 함)
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
	}

	//조작 차단
	if (Controller)
	{
		Controller->UnPossess(); // 영혼 이탈
	}

	//시체 청소 (5초 뒤에 액터 삭제)
	SetLifeSpan(5.0f);
}
