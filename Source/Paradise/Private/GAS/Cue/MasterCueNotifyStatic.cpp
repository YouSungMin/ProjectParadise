// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Cue/MasterCueNotifyStatic.h"
#include "Interfaces/CombatInterface.h" 
#include "Data/Assets/FXDataAsset.h"
#include "Data/Structs/FXStructs.h"
#include "Data/Enums/GameEnums.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"

UMasterCueNotifyStatic::UMasterCueNotifyStatic()
{
	//GameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Unit.Hit"));
}

bool UMasterCueNotifyStatic::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogTemp, Warning, TEXT("🔥🔥🔥 큐 클래스 내부 OnExecute 진입 성공! 대상: %s"), MyTarget ? *MyTarget->GetName() : TEXT("None"));
	// 방어 코드
	if (!MyTarget) return false;

	UE_LOG(LogTemp, Log, TEXT("🎯 GameplayCue UnitHit 실행됨: 대상 %s"), *MyTarget->GetName());

	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(MyTarget))
	{
		// 💡 단일 데이터가 아닌 배열(TArray)을 받아옵니다!
		TArray<FFXPayload*> Payloads = CombatInterface->GetFXPayloads(TargetEventType);

		if (Payloads.Num() > 0)
		{
			// 위치값 계산 (파라미터가 비어있으면 타겟의 현재 위치 사용)
			FVector HitLocation = Parameters.Location.IsZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);

			// 회전값 계산 (Normal 값이 비어있을 경우 타겟의 기본 회전값 사용)
			FRotator HitRotation = Parameters.Normal.IsZero() ? MyTarget->GetActorRotation() : Parameters.Normal.Rotation();

			for (FFXPayload* Payload : Payloads)
			{
				if (!Payload) continue; // 안전 장치

				// 1. 나이아가라 파티클 재생
				if (!Payload->VisualEffect.IsNull())
				{
					if (UNiagaraSystem* LoadedNiagara = Payload->VisualEffect.LoadSynchronous())
					{
						FVector RotatedOffset = HitRotation.RotateVector(Payload->LocationOffset);
						FVector FinalLocation = HitLocation + RotatedOffset;

						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							MyTarget,
							LoadedNiagara,
							FinalLocation,	// 오프셋 적용
							HitRotation,
							Payload->Scale	// 스케일 적용
						);
					}
				}

				// 2. 사운드 재생
				if (!Payload->SoundEffect.IsNull())
				{
					if (USoundBase* LoadedSound = Payload->SoundEffect.LoadSynchronous())
					{
						UGameplayStatics::PlaySoundAtLocation(MyTarget, LoadedSound, HitLocation);
					}
				}

				// 3. 카메라 쉐이크 (주석 해제 시 사용 가능)
				//if (Payload->CameraShake)
				//{
				//	if (APlayerController* PC = UGameplayStatics::GetPlayerController(MyTarget, 0))
				//	{
				//		PC->ClientStartCameraShake(Payload->CameraShake);
				//	}
				//}
			}
		}
		else
		{
			// 경고 메시지도 배열에 맞게 살짝 수정
			UE_LOG(LogTemp, Warning, TEXT("⚠️ [%s] GetFXPayloads가 빈 배열을 반환했습니다! 태그나 에셋을 확인하세요."), *MyTarget->GetName());
		}
	}

	// 부모 클래스 로직 정상 실행
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
