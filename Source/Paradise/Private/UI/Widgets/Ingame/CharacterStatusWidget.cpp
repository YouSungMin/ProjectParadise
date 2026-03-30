// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/CharacterStatusWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "GAS/Attributes/BaseAttributeSet.h"

#pragma region 외부 인터페이스 구현
void UCharacterStatusWidget::SetCharacterPortrait(UTexture2D* NewPortrait)
{
	if (Image_Portrait && NewPortrait)
	{
		Image_Portrait->SetBrushFromTexture(NewPortrait);
	}
}

void UCharacterStatusWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[CharacterStatusWidget] 유효하지 않은 ASC가 전달되어 바인딩을 취소합니다."));
		return;
	}

	/** @section 1. 기존 델리게이트 안전 해제 (재바인딩 대비) */
	if (CachedASC.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).RemoveAll(this);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetManaAttribute()).RemoveAll(this);
	}

	CachedASC = InASC;

	/** @section 2. 어트리뷰트 변경 델리게이트 구독 */
	CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UCharacterStatusWidget::OnHealthChanged);

	CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetManaAttribute())
		.AddUObject(this, &UCharacterStatusWidget::OnManaChanged);

	/** @section 3. 바인딩 직후 현재 스탯으로 UI 즉시 동기화 */
	const float CurrentHP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	const float MaxHP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());


	if (PB_HealthBar && MaxHP > 0.f)
	{
		PB_HealthBar->SetPercent(CurrentHP / MaxHP);
	}
	if (Text_Health)
	{
		FString HPString = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(CurrentHP), FMath::RoundToInt(MaxHP));
		Text_Health->SetText(FText::FromString(HPString));
	}

	const float CurrentMP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetManaAttribute());
	const float MaxMP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetMaxManaAttribute());
	if (PB_ManaBar && MaxMP > 0.f)
	{
		PB_ManaBar->SetPercent(CurrentMP / MaxMP);
	}
	if (Text_Mana)
	{
		FString MPString = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(CurrentMP), FMath::RoundToInt(MaxMP));
		Text_Mana->SetText(FText::FromString(MPString));
	}

	//UE_LOG(LogTemp, Log, TEXT("[CharacterStatusWidget] ASC 바인딩 및 UI 동기화 완료"));
}
#pragma endregion 외부 인터페이스 구현

void UCharacterStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

}

void UCharacterStatusWidget::NativeDestruct()
{
#pragma region 메모리 및 델리게이트 정리
	// 메모리 누수 및 댕글링 포인터 방지를 위한 클린업
	if (CachedASC.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).RemoveAll(this);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetManaAttribute()).RemoveAll(this);
	}

	CachedASC = nullptr;
	Super::NativeDestruct();
#pragma endregion 메모리 및 델리게이트 정리
}

#pragma region 어트리뷰트 콜백
void UCharacterStatusWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		const float MaxHP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		if (MaxHP > 0.f)
		{
			// 프로그레스 바 갱신
			if (PB_HealthBar) PB_HealthBar->SetPercent(Data.NewValue / MaxHP);

			// ⭐ 체력 텍스트 갱신
			if (Text_Health)
			{
				FString HPString = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Data.NewValue), FMath::RoundToInt(MaxHP));
				Text_Health->SetText(FText::FromString(HPString));
			}
		}
	}
}

void UCharacterStatusWidget::OnManaChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		const float MaxMP = CachedASC->GetNumericAttribute(UBaseAttributeSet::GetMaxManaAttribute());
		if (MaxMP > 0.f)
		{
			// 프로그레스 바 갱신
			if (PB_ManaBar) PB_ManaBar->SetPercent(Data.NewValue / MaxMP);

			// ⭐ 마나 텍스트 갱신
			if (Text_Mana)
			{
				FString MPString = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Data.NewValue), FMath::RoundToInt(MaxMP));
				Text_Mana->SetText(FText::FromString(MPString));
			}
		}
	}
}
#pragma endregion 어트리뷰트 콜백