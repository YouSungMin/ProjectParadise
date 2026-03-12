// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Combat/DamageTextWidget.h"
#include "Components/TextBlock.h"
#include "Components/RetainerBox.h"
#include "Animation/WidgetAnimation.h"

#pragma region 외부 인터페이스
void UDamageTextWidget::SetDamageText(float DamageAmount, bool bIsCritical)
{
	if (!Text_Damage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DamageTextWidget] Text_Damage가 바인딩되지 않았습니다."));
		return;
	}

	/** @section 1. 데미지 수치 텍스트 설정 (소수점 절사) */
	const int32 RoundedDamage = FMath::RoundToInt(DamageAmount);
	Text_Damage->SetText(FText::AsNumber(RoundedDamage));

	/** @section 2. 크리티컬 여부에 따른 폰트 머티리얼 스킨 적용 */
	if (bIsCritical)
	{
		if (CriticalDamageMaterial && RetainerBox_Damage)
		{
			RetainerBox_Damage->SetEffectMaterial(CriticalDamageMaterial);
		}
		SetRenderScale(FVector2D(CriticalScale, CriticalScale));
		// 크리티컬 팝업 애니메이션 등 추가 처리가 있다면 이곳에.
	}
	else
	{
		if (NormalDamageMaterial && RetainerBox_Damage)
		{
			RetainerBox_Damage->SetEffectMaterial(NormalDamageMaterial);
		}
		SetRenderScale(FVector2D(1.0f, 1.0f));
	}

	/** @section 3. 블루프린트 애니메이션 실행 */
	PlayPopupAnimation();
}

void UDamageTextWidget::PlayPopupAnimation()
{
	if (Anim_FlyUp)
	{
		PlayAnimation(Anim_FlyUp, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void UDamageTextWidget::ResetWidget()
{
	if (Text_Damage)
	{
		Text_Damage->SetText(FText::GetEmpty());

		// 풀 반납 시 일반 머티리얼로 초기화
		if (NormalDamageMaterial && RetainerBox_Damage)
		{
			RetainerBox_Damage->SetEffectMaterial(NormalDamageMaterial);
		}
	}
	/** @section 렌더 스케일 및 투명도 초기화 */
	SetRenderScale(FVector2D(1.0f, 1.0f));
	SetRenderOpacity(1.0f);
}
#pragma endregion 외부 인터페이스