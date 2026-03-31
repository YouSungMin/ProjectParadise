// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/Ingame/HomeBaseHPWidget.h"
#include "Objects/HomeBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/Attributes/BaseAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UHomeBaseHPWidget::NativeDestruct()
{
    // 델리게이트 안전 해제
    if (CachedASC.IsValid())
    {
        CachedASC->GetGameplayAttributeValueChangeDelegate(
            UBaseAttributeSet::GetHealthAttribute()).RemoveAll(this);
        CachedASC->GetGameplayAttributeValueChangeDelegate(
            UBaseAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UHomeBaseHPWidget::InitWithHomeBase(AHomeBase* InHomeBase)
{
    if (!InHomeBase) return;

    UAbilitySystemComponent* ASC = InHomeBase->GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Error, TEXT("[HomeBaseHP] ASC가 없음!"));
        return;
    }

    CachedASC = ASC;

    // 현재 체력 즉시 동기화
    CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
    MaxHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());

    UE_LOG(LogTemp, Warning, TEXT("[HomeBaseHP] 초기화 완료 - HP: %.0f / %.0f"), CurrentHP, MaxHP);

    UpdateHPBar();

    // 실시간 감지 바인딩
    ASC->GetGameplayAttributeValueChangeDelegate(
        UBaseAttributeSet::GetHealthAttribute())
        .AddUObject(this, &UHomeBaseHPWidget::OnHealthChanged);

    ASC->GetGameplayAttributeValueChangeDelegate(
        UBaseAttributeSet::GetMaxHealthAttribute())
        .AddUObject(this, &UHomeBaseHPWidget::OnMaxHealthChanged);
}

void UHomeBaseHPWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    UE_LOG(LogTemp, Warning, TEXT("[HomeBaseHP] 체력 변경 감지 - %.0f → %.0f"), Data.OldValue, Data.NewValue);
    CurrentHP = Data.NewValue;
    UpdateHPBar();
}

void UHomeBaseHPWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
    MaxHP = Data.NewValue;
    UpdateHPBar();
}

void UHomeBaseHPWidget::UpdateHPBar()
{
    const float SafeMax = FMath::Max(1.0f, MaxHP);
    const float Percent = FMath::Clamp(CurrentHP / SafeMax, 0.0f, 1.0f);

    if (PB_HP) PB_HP->SetPercent(Percent);

    if (Text_HP)
    {
        Text_HP->SetText(FText::FromString(
            FString::Printf(TEXT("%.0f / %.0f"), CurrentHP, MaxHP)));
    }
}