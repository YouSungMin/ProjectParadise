// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Cue/MasterCueNotifyStatic.h"
#include "Abilities/GameplayAbility.h" 
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Interfaces/CombatInterface.h" 
#include "Data/Assets/FXDataAsset.h"
#include "Data/Structs/FXStructs.h"
#include "Data/Enums/GameEnums.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/AIUnit/SkillCasterUnit.h"

UMasterCueNotifyStatic::UMasterCueNotifyStatic()
{
	//GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Unit.Hit"));
}

bool UMasterCueNotifyStatic::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		//UE_LOG(LogTemp, Error, TEXT("❌ [MasterCue] MyTarget이 Null이라 실행 취소됨."));
		return false;
	}

	//UE_LOG(LogTemp, Warning, TEXT("=================================================="));

	FVector HitLocation = Parameters.Location.IsZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
	FRotator HitRotation = Parameters.Normal.IsZero() ? MyTarget->GetActorRotation() : Parameters.Normal.Rotation();

	// 이펙트 재생 공용 로직 (람다)
	auto PlayFXForActor = [&](AActor* SourceActor, EFXEventType EventType, const FString& ActorRole)
		{
			if (!SourceActor) return;
			//UE_LOG(LogTemp, Warning, TEXT("🎯 [마스터 큐] 요청받은 EventType: %d / 현재 캐스팅 인덱스: %d"), (int32)EventType, Cast<ASkillCasterUnit>(SourceActor)->GetCurrentCastingSkillIndex());

			//UE_LOG(LogTemp, Log, TEXT("  -> [%s] %s에게서 데이터 조회 시도 중..."), *ActorRole, *SourceActor->GetName());

			if (SourceActor->Implements<UCombatInterface>())
			{
				ICombatInterface* CombatInterface = Cast<ICombatInterface>(SourceActor);
				TArray<FFXPayload*> Payloads = CombatInterface->GetFXPayloads(EventType);

				//UE_LOG(LogTemp, Log, TEXT("    -> 인터페이스 확인됨. 가져온 Payload 개수: %d"), Payloads.Num());

				for (FFXPayload* Payload : Payloads)
				{
					if (!Payload) continue;

					if (!Payload->VisualEffect.IsNull())
					{
						if (UNiagaraSystem* LoadedNiagara = Payload->VisualEffect.LoadSynchronous())
						{
							FVector RotatedOffset = HitRotation.RotateVector(Payload->LocationOffset);
							FRotator FinalRotation = HitRotation + Payload->RotationOffset;

							UNiagaraFunctionLibrary::SpawnSystemAtLocation(
								MyTarget, LoadedNiagara, HitLocation + RotatedOffset, FinalRotation, Payload->Scale
							);
							//UE_LOG(LogTemp, Log, TEXT("      🎇 나이아가라 스폰 완료: %s"), *LoadedNiagara->GetName());
						}
					}

					if (!Payload->SoundEffect.IsNull())
					{
						if (USoundBase* LoadedSound = Payload->SoundEffect.LoadSynchronous())
						{
							//UE_LOG(LogTemp, Warning, TEXT("🔊 [%s] %s가 피격음 재생! 소리 이름: %s"),
							//	*ActorRole,
							//	*SourceActor->GetName(),
							//	*LoadedSound->GetName());
							UGameplayStatics::PlaySoundAtLocation(MyTarget, LoadedSound, HitLocation);
							//UE_LOG(LogTemp, Log, TEXT("      🔊 사운드 재생 완료: %s"), *LoadedSound->GetName());
						}
					}
				}
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("    ⚠️ [%s] %s는 CombatInterface를 상속받지 않았습니다."), *ActorRole, *SourceActor->GetName());
			}
		};

	//UE_LOG(LogTemp, Warning, TEXT("🗡️ [MasterCue] 타격자(AttackerActor) 탐색 시작..."));
	AActor* AttackerActor = Parameters.EffectCauser.Get();

	if (!AttackerActor) AttackerActor = Parameters.Instigator.Get();

	// 파라미터 껍데기에 없으면, 핵심 원본인 EffectContext 내부를 탐색
	if (!AttackerActor)
	{
		AttackerActor = Parameters.EffectContext.GetEffectCauser();
		if (!AttackerActor) AttackerActor = Parameters.EffectContext.GetInstigator();
	}

	// SourceObject 탐색
	if (!AttackerActor)
	{
		if (UObject* SourceObj = const_cast<UObject*>(Parameters.SourceObject.Get()))
		{
			if (UGameplayAbility* SourceAbility = Cast<UGameplayAbility>(SourceObj))
			{
				AttackerActor = SourceAbility->GetAvatarActorFromActorInfo();
				//UE_LOG(LogTemp, Log, TEXT("  -> SourceObject(Ability)에서 타격자 추출 성공!"));
			}
			else if (AActor* SourceActorObj = Cast<AActor>(SourceObj))
			{
				AttackerActor = SourceActorObj;
				//UE_LOG(LogTemp, Log, TEXT("  -> SourceObject(Actor)에서 타격자 추출 성공!"));
			}
		}
	}

	// 최종 인터페이스 검증 및 Instigator 폴백
	if (AttackerActor && !AttackerActor->Implements<UCombatInterface>())
	{
		AActor* FallbackActor = Parameters.EffectContext.GetInstigator();
		if (FallbackActor && FallbackActor->Implements<UCombatInterface>())
		{
			//UE_LOG(LogTemp, Log, TEXT("  -> EffectCauser(%s)가 인터페이스가 없어 Instigator(%s)로 교체합니다."), *AttackerActor->GetName(), *FallbackActor->GetName());
			AttackerActor = FallbackActor;
		}
	}

	if (AttackerActor)
	{
		//UE_LOG(LogTemp, Log, TEXT("  -> 1차 타격자 탐색 결과: %s"), *AttackerActor->GetName());
	}

	AActor* TrueAttacker = AttackerActor;

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(AttackerActor))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			if (AActor* Avatar = ASC->GetAvatarActor())
			{
				TrueAttacker = Avatar;
			}
		}
	}

	if (TrueAttacker && TrueAttacker != MyTarget)
	{
		// 1. 피격자(맞은 적)는 무조건 'Hit(피격)' 데이터를 꺼내서 비명을 지릅니다.
		PlayFXForActor(MyTarget, EFXEventType::Hit, TEXT("피격자"));

		// 2. 타격자(플레이어)는 TargetEventType이 'Hit'가 아닐 때만 이펙트/사운드를 재생합니다.
		// (현재 타격 성공음은 무기가 알아서 처리하고 있으므로 패스!)
		if (TargetEventType != EFXEventType::Hit)
		{
			PlayFXForActor(TrueAttacker, TargetEventType, TEXT("타격자"));
		}
	}
	else
	{
		PlayFXForActor(MyTarget, TargetEventType, TEXT("시전자"));
	}

	//UE_LOG(LogTemp, Warning, TEXT("=================================================="));

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
