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

void ACharacterBase::CheckHit(FName SocketName,ESocketTargetType TargetType)
{
	FVector TraceStart = FVector::ZeroVector;
	FVector TraceDirection = FVector::ZeroVector;

	USceneComponent* TargetMesh = GetMesh();

	// 노티파이에서 '무기' 타겟이라고 넘겨줬다면 메쉬 교체
	if (TargetType == ESocketTargetType::EquippedWeapon)
	{
		if (USceneComponent* WpnMesh = GetWeaponMesh())
		{
			TargetMesh = WpnMesh; // 타겟 성공적 교체
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("⚠️ [%s] 무기 소켓을 찾으려 했으나 무기 메쉬가 없습니다! 몸통을 대신 사용합니다."), *GetName());
		}
	}

	// 결정된 TargetMesh에서 소켓 위치 찾기
	if (TargetMesh && TargetMesh->DoesSocketExist(SocketName))
	{
		TraceStart = TargetMesh->GetSocketLocation(SocketName);

		TraceDirection = TargetMesh->GetSocketRotation(SocketName).Vector();
	}
	else
	{
		// 예외 처리: 소켓이 없거나 이름이 틀렸을 때
		TraceStart = GetActorLocation() + (GetActorForwardVector() * 100.0f);
		TraceDirection = GetActorForwardVector();
	}

	// ForwardOffset 적용: 시작점을 캐릭터 전방으로 밀어줍니다.
	TraceStart += GetActorForwardVector() * CurrentActiveActionData.Stats.ForwardOffset;
	//TraceDirection

	// 사거리(AttackRange) 적용: 밀어낸 시작점으로부터 '사거리'만큼 뻗어나갑니다. 
	FVector TraceEnd = TraceStart + (TraceDirection * CurrentActiveActionData.Stats.AttackRange);

	TArray<AActor*> ActorsToIgnore;
	if (CurrentActiveActionData.Stats.TargetFilter == ETargetFilter::Enemy)
	{
		ActorsToIgnore.Add(this);
	}

	TArray<FHitResult> HitResults;
	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		TraceStart,      // 시작점
		TraceEnd,        // 끝점 (앞으로 길게 뻗음)
		CurrentActiveActionData.Stats.AttackRadius,    // 반경
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
		//UE_LOG(LogTemp, Warning, TEXT("[트레이스 적중!] 스피어에 뭔가 닿았습니다. 총 %d개"), HitResults.Num());

		for (const FHitResult& Result : HitResults)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor) continue;

			// ACharacterBase로 캐스팅 시도
			ACharacterBase* HitChar = Cast<ACharacterBase>(HitActor);

			// 캐스팅 실패 시 (바닥, 배경 프랍 등) 무시하고 다음 타겟으로 넘어감
			if (HitChar == nullptr)
			{
				//UE_LOG(LogTemp, Warning, TEXT("⚠️ [CheckHit] 캐릭터가 아닌 대상(%s)을 쳤습니다. 무시합니다."), *HitActor->GetName());
				continue;
			}

			// 중복 타격 방지 (캐릭터인 경우에만 리스트에 추가)
			if (HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			bool bIsHostile = IsHostile(HitChar);
			ETargetFilter Filter = CurrentActiveActionData.Stats.TargetFilter;

			if (Filter == ETargetFilter::Enemy && !bIsHostile)
			{
				// 적 공격용 스킬인데 아군이면 무시
				continue;
			}
			else if (Filter == ETargetFilter::Friendly && bIsHostile)
			{
				// 아군 버프용 스킬인데 적군이면 무시
				continue;
			}

			// GAS 이벤트 전송
			FGameplayEventData Payload;
			Payload.Instigator = this;
			Payload.Target = HitActor;
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Result);

			// 태그를 고정하거나, 인자로 받을 수도 있음
			FGameplayTag EventTag;
			EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.ApplyEffect"));

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Payload);

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

USceneComponent* ACharacterBase::GetWeaponMesh() const
{
	if (CurrentWeaponActor)
	{
		return CurrentWeaponActor->GetComponentByClass<UMeshComponent>();
	}
	return nullptr;
}

void ACharacterBase::PlayHitReaction()
{
	PlayHitFlash(); // 매쉬 빨개지기 (이건 정상 작동하는지 눈으로 확인)

	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [%s] AnimInstance를 찾을 수 없습니다!"), *GetName());
		return;
	}

	// 1. 슈퍼아머(다른 몽타주 재생 중) 체크
	if (AnimInst->IsAnyMontagePlaying())
	{
		UE_LOG(LogTemp, Warning, TEXT("🛡️ [%s] 다른 몽타주 재생 중이라 피격 모션 생략! (슈퍼아머 작동)"), *GetName());
		return;
	}

	// 2. 피격 몽타주 데이터 확인
	UAnimMontage* HitMontage = GetHitMontage();
	if (!HitMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [%s] HitMontage가 Null입니다! (데이터 테이블이나 캐싱 코드 확인 필요)"), *GetName());
		return;
	}

	// 3. 정상 재생 명령
	UE_LOG(LogTemp, Log, TEXT("✅ [%s] 피격 몽타주 재생 성공: %s"), *GetName(), *HitMontage->GetName());
	AnimInst->Montage_Play(HitMontage);
}

void ACharacterBase::OnDeathAnimationFinished()
{
	if (GetMesh())
	{
		GetMesh()->bPauseAnims = true;
	}
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

	// HitResetTime 후 이펙트 리셋 함수호출
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

void ACharacterBase::SetCurrentMuzzleSocketInfo(FName InSocketName, ESocketTargetType InSocketTarget)
{
	CurrentMuzzleSocketName = InSocketName;
	CurrentMuzzleSocketTarget = InSocketTarget;
}

FTransform ACharacterBase::GetCurrentMuzzleTransform() const
{
	// 무기(Weapon)에서 소켓을 찾는 경우
	if (CurrentMuzzleSocketTarget == ESocketTargetType::EquippedWeapon && CurrentWeaponActor)
	{
		// 무기 액터가 가진 메쉬 컴포넌트를 탐색 (Static/Skeletal 모두 대응)
		if (UMeshComponent* WeaponMesh = CurrentWeaponActor->FindComponentByClass<UMeshComponent>())
		{
			// 해당 무기 메쉬에서 지정된 소켓의 월드 트랜스폼 추출
			return WeaponMesh->GetSocketTransform(CurrentMuzzleSocketName);
		}
	}

	// 캐릭터 몸체(CharacterBody)에서 찾거나, 무기를 장착하지 않은 상태일 경우
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		return BodyMesh->GetSocketTransform(CurrentMuzzleSocketName);
	}

	// 만약 위에서 아무것도 찾지 못했다면 안전망으로 캐릭터 본체의 트랜스폼 반환
	return GetActorTransform();
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

	// 충돌 해제 및 조작 불가 처리
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (Controller)
	{
		Controller->UnPossess();
	}

	// 몽타주 재생
	UAnimMontage* DeathMontage = GetDeathMontage();
	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
	}
	else
	{
		// 몽타주가 없다면 예외 처리로 즉시 종료 함수 호출
		OnDeathAnimationFinished();
	}
}
