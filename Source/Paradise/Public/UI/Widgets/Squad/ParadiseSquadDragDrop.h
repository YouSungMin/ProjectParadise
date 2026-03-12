// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/Data/SquadUITypes.h"
#include "ParadiseSquadDragDrop.generated.h"

/**
 * @class UParadiseSquadDragDrop
 * @brief 드래그 앤 드롭 이동 시 데이터를 운반하는 페이로드 (Payload) 객체
 */
UCLASS()
class PARADISE_API UParadiseSquadDragDrop : public UDragDropOperation
{
	GENERATED_BODY()
	
#pragma region 드래그 데이터
public:
	/**
	 * @brief 드래그 중인 아이템의 통합 UI 데이터 구조체
	 * @details 아이콘, ID, 등급 등 렌더링에 필요한 모든 정보를 담고 있습니다.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Squad|DragDrop", meta = (ExposeOnSpawn = "true"))
	FSquadItemUIData DraggedData;

	/**
	 * @brief 드래그가 시작된 슬롯의 인덱스
	 * @details -1: 인벤토리에서 출발 / 0 이상: 특정 편성 슬롯에서 출발 (자리 스왑/해제용)
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Squad|DragDrop", meta = (ExposeOnSpawn = "true"))
	int32 SourceSlotIndex = -1;

	/**
	 * @brief 드래그 출발지의 카테고리 (Tab Index)
	 * @details SquadTabs 네임스페이스 기준: Character(0), Weapon(1), Armor(2), Unit(3)
	 * 이 값을 통해 Target 슬롯의 하이라이트/회색처리 범위를 결정합니다.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Squad|DragDrop", meta = (ExposeOnSpawn = "true"))
	int32 DragSourceTabIndex = -1;

	/**
	 * @brief 드래그 중인 장비의 세부 부위 정보
	 * @details 캐릭터/유닛 드래그 시에는 None이며, 장비 드래그 시 Detail창의 특정 슬롯 하이라이트에 사용됩니다.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Squad|DragDrop", meta = (ExposeOnSpawn = "true"))
	EEquipmentSlot EquipPart = EEquipmentSlot::None;

	/**
	 * @brief 드롭 성공 여부 플래그
	 * @details NativeOnDrop이 성공적으로 수행되면 true로 설정됩니다.
	 * false인 상태로 드래그가 종료되면 '취소' 혹은 '허공 드롭'으로 간주하여 복귀 로직을 수행합니다.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Squad|DragDrop")
	bool bWasDropped = false;
#pragma endregion 드래그 데이터
};
