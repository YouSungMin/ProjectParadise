// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Cue/UnitHitCueNotify.h"
#include "Interfaces/CombatInterface.h" 
#include "Data/Assets/FXDataAsset.h"
#include "Data/Structs/FXStructs.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"

UUnitHitCueNotify::UUnitHitCueNotify()
{
	//GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Unit.Hit"));
}

bool UUnitHitCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogTemp, Warning, TEXT("🔥🔥🔥 큐 클래스 내부 OnExecute 진입 성공! 대상: %s"), MyTarget ? *MyTarget->GetName() : TEXT("None"));
	// 방어 코드
	if (!MyTarget) return false;

	UE_LOG(LogTemp, Log, TEXT("🎯 GameplayCue UnitHit 실행됨: 대상 %s"), *MyTarget->GetName());

	// 핵심 로직: 맞은 대상에게 ICombatInterface를 구현했는지 물어봄
	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(MyTarget))
	{
		// 인터페이스를 통해 데이터 에셋과 피격 태그를 받아오기
		UFXDataAsset* FXData = CombatInterface->GetUnitFXData();
		FGameplayTag HitTag = CombatInterface->GetHitReactionTag();

		if (!FXData)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ %s에게서 FXDataAsset을 찾을 수 없습니다!"), *MyTarget->GetName());
		}

		// 데이터 에셋이 정상이고, 피격 태그에 해당하는 Payload가 있는지 확인
		if (FXData && FXData->EffectMap.Contains(HitTag))
		{
			// 연출 보따리 꺼내기
			const FFXPayload& Payload = FXData->EffectMap[HitTag];
			UE_LOG(LogTemp, Log, TEXT("✅ 최종 연출 실행 준비 완료 (Sound: %s, Niagara: %s)"),
				!Payload.SoundEffect.IsNull() ? TEXT("Valid") : TEXT("Null"),
				!Payload.VisualEffect.IsNull() ? TEXT("Valid") : TEXT("Null"));

			FVector HitLocation;
			if (Parameters.Location.IsZero())
			{
				HitLocation = MyTarget->GetActorLocation();
			}
			else
			{
				HitLocation = FVector(Parameters.Location);
			}

			// 회전값도 안전하게 처리 (Normal 값이 비어있을 경우 타겟의 기본 회전값 사용)
			FRotator HitRotation;
			if (Parameters.Normal.IsZero())
			{
				HitRotation = MyTarget->GetActorRotation();
			}
			else
			{
				HitRotation = Parameters.Normal.Rotation();
			}

			// =========================================================
			//  연출 실행 파트 (Visual, Audio, Camera Shake)
			// =========================================================

			// 나이아가라 파티클 재생
			if (!Payload.VisualEffect.IsNull())
			{
				UNiagaraSystem* LoadedNiagara = Payload.VisualEffect.LoadSynchronous();
				if (LoadedNiagara)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						MyTarget,
						LoadedNiagara,
						HitLocation + Payload.LocationOffset, // 오프셋 적용
						HitRotation,
						Payload.Scale                         // 스케일 적용
					);
				}
			}

			// 사운드 재생
			if (!Payload.SoundEffect.IsNull())
			{
				USoundBase* LoadedSound = Payload.SoundEffect.LoadSynchronous();
				if (LoadedSound)
				{
					UGameplayStatics::PlaySoundAtLocation(MyTarget, LoadedSound, HitLocation);
				}
			}

			// C. 카메라 쉐이크 (주로 플레이어가 맞았을 때나, 아주 강한 공격이 들어갔을 때)
			//if (Payload.CameraShake)
			//{
			//	// 첫 번째 로컬 플레이어의 화면을 흔듭니다.
			//	if (APlayerController* PC = UGameplayStatics::GetPlayerController(MyTarget, 0))
			//	{
			//		PC->ClientStartCameraShake(Payload.CameraShake);
			//	}
			//}
		}
	}

	// 부모 클래스 로직 정상 실행
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
