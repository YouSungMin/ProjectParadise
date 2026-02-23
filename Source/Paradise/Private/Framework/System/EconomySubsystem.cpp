// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/System/EconomySubsystem.h"
#include "Framework/System/ParadiseSaveGame.h"

void UEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
}

int32 UEconomySubsystem::GetCurrency(ECurrencyType Type) const
{
	return int32();
}

void UEconomySubsystem::AddCurrency(ECurrencyType Type, int32 Amount)
{
}

bool UEconomySubsystem::ConsumeCurrency(ECurrencyType Type, int32 Amount)
{
	return false;
}

bool UEconomySubsystem::HasEnoughCurrency(ECurrencyType Type, int32 Amount) const
{
	return false;
}

void UEconomySubsystem::LoadFromSaveGame(UParadiseSaveGame* SaveGameObj)
{
}

void UEconomySubsystem::SaveToSaveGame(class UParadiseSaveGame* SaveGameObj) const
{
}