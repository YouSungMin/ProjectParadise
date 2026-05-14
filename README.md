# Project Paradise (스쿼드 타워 디펜스 RPG)

<kbd>
  <img height="400" alt="화면 캡처 2026-05-06 141820" src="https://github.com/user-attachments/assets/40d71852-4fba-4088-8748-0ac3ecaad9e6" />
</kbd>

<kbd>
  <img height="400" alt="화면 캡처 2026-05-06 141503" src="https://github.com/user-attachments/assets/decc9b3c-7b84-4157-b71a-872142cab942" />
</kbd>

## 📑 목차
1. [프로젝트 개요](#프로젝트-개요)
2. [기술적 하이라이트](#기술적-하이라이트)
3. [세부 구현 내용](#세부-구현-내용)
	* 3.1. [데이터 테이블 구조체](#1-데이터-테이블-구조체)
    * 3.2. [공통 어트리뷰트 셋](#2-공통-어트리뷰트-셋)
    * 3.3. [커스텀 연산 클래스](#3-커스텀-연산-클래스)
    * 3.4. [전투 어빌리티](#4-전투-어빌리티)
    * 3.5. [전투 데이터 패키징](#5-전투-데이터-패키징)
    * 3.6. [정밀 타격/발사 판정](#6-정밀-타격발사-판정)
    * 3.7. [데이터 주도형 연출 시스템](#7-데이터-주도형-연출-시스템)

## 프로젝트 개요
* **개발 기간** : 2026.02.03 - 2026.04.03 
* **개발 인원** : 5명 ( 클라이언트 4명, 기획 1명 ) 
* **내 역할** : 클라이언트 프로그래머 ( 전투 시스템 및 데이터 아키텍처 설계)
* **개발 환경** : Unreal Engine 5.5.4, C++, GitHub Desktop, Notion

## 기술적 하이라이트
기획자와의 협업 효율을 높이는 데이터 주도 설계와 GAS를 활용한 확장성 높은 전투 프레임워크 구축에 집중했습니다.

### 데이터 주도 설계 파이프 라인 구축
* **데이터 주도 설계:** 수식용 엑셀과 엔진용 CSV를 분리하여 데이터 오염을 방지하고, 코드 수정 없는 즉각적인 밸런싱 환경을 구축했습니다.
* **종속성 분리:** 기존 FName 기반 하드코딩의 문제를 인지하고, 핸들(Handle) 방식을 도입해 데이터 참조 구조를 객체 지향적으로 리팩토링했습니다.
* **메모리 지연 로딩:** 스탯과 에셋 구조체를 분리하고, 무거운 리소스는 비동기 로드(Async Load)하여 초기 로딩 속도와 메모리를 최적화했습니다.

### GAS 기반 전투 아키텍처
* **전투 연산 커스텀 (Execution & MMC):** 단순 타격을 넘어 방어력 경감이 적용되는 데미지 연산과, 시전자 스탯에 비례해 동적으로 커지는 버프 시스템을 구현했습니다.
* **데이터 주도형 투사체 패턴:** 단일 투사체 클래스(`ProjectileBase`)가 CSV 수치 조작만으로 연사, 샷건, 폭발 등 다양한 패턴으로 동작하도록 설계했습니다.
* **통합 연출 제어 (Gameplay Cue):** 수많은 타격/피격 FX 요청을 단일 마스터 큐(`UMasterCueNotifyStatic`)로 통제하여 클래스 폭발을 방지하고 결합도를 낮췄습니다.

## 세부 구현 내용

### 1. 데이터 테이블 구조체

**관련 파일** :
* [데이터 테이블 구조체](./Source/Paradise/Public/Data/Structs)
  
**설계 방향**
공통 속성은 부모 구조체에 정의하고 고유 속성은 상속을 통해 확장하여 여러 구조체에 적용하였고, 에셋과 수치 데이터를 분리하여 코드 수정없이 CSV를 통해 밸런싱 할 수 있는 데이터 주도 환경을 설계했습니다.

용량이 큰 리소스는 TSoftObjectPtr를 통해 비동기 로드하도록 구성하여 로딩 속도와 메모리 최적화를 고려했습니다.

엑셀의 수식 기능을 활용하는 Master 파일과 엔진이 읽는 CSV 폴더를 분리하여, 협업 시 데이터 오염 및 버전 관리 충돌을 방지했습니다.

**유닛 데이터 테이블 관계도**

<img width="940" height="344" alt="image" src="https://github.com/user-attachments/assets/76c86d92-cb11-41b2-8d0b-d831879a024e" />


### 2. 공통 어트리뷰트 셋

**관련 파일** :
* [BaseAttributeSet.cpp](./Source/Paradise/Private/GAS/Attributes/BaseAttributeSet.cpp) 

**설계 방향**

코드의 중복을 줄이고 유지보수 효율을 높이기 위해 플레이어, 적(Enemy), 소환수(Familiar)가 하나의 어트리뷰트 셋 클래스를 사용하도록 설계했습니다.

<img width="794" height="283" alt="화면 캡처 2026-05-12 165302" src="https://github.com/user-attachments/assets/8cbc28f4-317c-4e6e-a59a-ed74d172ef1b" />

**방어 로직**

```cpp
// 체력이나 마나 등 0 이하로 떨어지거나 최대치를 초과하지 않도록 방어하는 로직
void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// 체력 (Health)
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
  // ... (이하 생략)
}
```
PreAttributeChange를 오버라이드 하여 스탯이 설계된 최소/최대 범위를 벗어나지 않도록 실시간 클램핑(Clamping) 처리를 적용했습니다.

### 3. 커스텀 연산 클래스

**관련 파일** :
* [ExecCalcCombat.cpp](./Source/Paradise/Private/GAS/Calculations/ExecCalcCombat.cpp)
* [ExecCalcHeal.cpp](./Source/Paradise/Private/GAS/Calculations/ExecCalcHeal.cpp)
* [ModCalcAttackPower.cpp](./Source/Paradise/Private/GAS/Calculations/ModCalcAttackPower.cpp)

**설계 방향**

시전자와 대상의 어트리뷰트를 동적으로 캡처하여 전투 공식을 산출하는 로직으로 설계했으며, 방어력 경감 및 크리티컬 연산을 반영했습니다.

고정 수치 버프가 아닌 버프 시전자의 능력치에 비례하여 효과의 위력이 결정되는 시스템을 구축하고자 MMC(ModMagnitudeCalculation)를 활용했습니다.

<img width="445" height="333" alt="화면 캡처 2026-05-13 150517" src="https://github.com/user-attachments/assets/3cf98cce-2e7d-46bb-9370-ee907d7e8e8f" />

### 4. 전투 어빌리티

**관련 파일** :
* [BaseGameplayAbility.cpp](./Source/Paradise/Private/GAS/Abilities/BaseGameplayAbility.cpp)
* [AreaActionBase.cpp](./Source/Paradise/Private/GAS/Abilities/AreaActionBase.cpp)
* [ProjectileAttackBase.cpp](./Source/Paradise/Private/GAS/Abilities/ProjectileAttackBase.cpp)

**설계 방향**

전투 데이터 캐싱, 코스트 및 쿨타임 적용, 몽타주 재생 등 반복되는 공통 로직을 Base 클래스에 통합하여 코드 중복을 최소화하고 이를 상속받는 자식 클래스들은 근접/광역 타격이나 투사체 발사 등 각 어빌리티 고유의 핵심 전투 판정 로직에만 집중할 수 있는 구조로 설계하였습니다.


**클래스 계층 구조**

<img width="552" height="355" alt="image" src="https://github.com/user-attachments/assets/a7802e07-cafa-4516-a006-8fe10cc6a4dd" />

**어빌리티 발동 흐름도**

<img width="1064" height="279" alt="image" src="https://github.com/user-attachments/assets/334753cc-9c65-4f71-98ac-cf5dc3bff2d8" />

WaitGameplayEvent를 통해 애니메이션의 정확한 타격 시점을 동기화하고 자식 클래스에 따라 즉시 타격 과 투사체 스폰 로직으로 분기되도록 설계하였습니다

**AreaActionBase(근접/광역타격)**

WaitGameplayEvent를 통해 애니메이션의 실제 타격 프레임(Hit 태그)을 수신하고, 데이터 테이블에 정의된 범위 내의 적들을 탐색하여 즉각적인 데미지를 적용합니다.

**ProjectileAttackBase(투사체 발사)**

발사 프레임(Fire 태그) 수신 시, 오브젝트 풀 서브시스템과 연동하여 투사체를 스폰하며, 단일 클래스만으로 CSV 데이터에 따라 연사, 샷건, 관통 등 다양한 패턴이 동적으로 작동하도록 설계했습니다.


### 5. 전투 데이터 패키징 

**관련 파일** :
* [CombatTypes.h](./Source/Paradise/Public/Data/Structs/CombatTypes.h)
* [BaseGameplayAbility.cpp](./Source/Paradise/Private/GAS/Abilities/BaseGameplayAbility.cpp)

**설계 방향**

전투 어빌리티에 필요한 데이터를 FCombatActionData 구조체로 패키징하여 ICombatInterface를 통해 유닛과 어빌리티 간의 직접적인 참조를 끊고, 전투에 필요한 데이터만 주고 받도록 설계했습니다.

스킬 시전 시 필요한 데이터 구조체를 인터페이스로 가져오고 최초 1회 캐싱하여 불필요한 데이터 검색 부하를 줄였습니다.

**전투 패키징 과정**

<img width="764" height="294" alt="화면 캡처 2026-05-13 161052" src="https://github.com/user-attachments/assets/f1bdc044-e3e4-4dcf-ae30-39d934901637" />

### 6. 정밀 타격/발사 판정

**관련 파일** :
* [근접 타격 AnimNotifyState 클래스](./Source/Paradise/Private/Characters/Player/TestNotifyState.cpp)
* [투사체 발사 AnimNotify 클래스](./Source/Paradise/Private/Characters/Player/SendGameplayEventNotify.cpp)

**설계 방향**
공격 애니메이션의 실제 타격/발사 프레임에 맞춰 AnimNotifyStat/AnimNotify를 배치하여 해당 프레임에 타격/발사 판정을 실행되도록 설계했습니다.

타격 범위나 타격 대상의 경우 데이터 테이블의 속성 값을 통해 설정되도록 구현했습니다.

**애니메이션 타이밍 동기화**

<kbd>
  <img width="400" height="300" alt="image" src="https://github.com/user-attachments/assets/31cdcec2-2ba8-4326-bab1-8ae96fa28eba" />
</kbd>
<kbd>
  <img width="400" height="300" alt="image" src="https://github.com/user-attachments/assets/526e3473-7bfa-43e9-9f76-9376941caa34" />
</kbd>


### 7. 데이터 주도형 연출 시스템

**관련 파일** :
* [MasterCueNotifyStatic.cpp](./Source/Paradise/Private/GAS/Cue/MasterCueNotifyStatic.cpp)

**설계 방향**
타격, 무기 스윙 등 여러 연출 요청을 UMasterCueNotifyStatic 단일 클래스로 통제하여 클래스 폭발을 방지했습니다.
ICombatInterface를 통해 마스터 큐와 타격/피격자 간의 결합도를 낮추고 상황에 맞는 이펙트를 동적으로 로드하도록 설계했습니다.

**연출 시스템 매핑**

<img width="557" height="306" alt="image" src="https://github.com/user-attachments/assets/3c8608c2-6f54-4806-9925-1d072a504d1f" />

파티클, 사운드, 카메라 쉐이크 등의 연출 데이터를 FFXPayLoad 구조체로 생성 후, UFXDataAssets을 통해 코드 개입 없이 에디터에서 직접 세팅 할 수 있는 환경 구축했습니다.

**마스터 큐 실행 흐름도**

<img width="364" height="517" alt="image" src="https://github.com/user-attachments/assets/555db5d6-3773-4817-b9e9-b905cd0e8105" />

[API 문서 웹사이트 (Doxygen + GitHub Pages) 보러가기] https://paradiseproject.github.io/ParadiseProject_Docs/

## 🎮 조작 방법 (Controls)

본 프로젝트는 Windows(PC) 및 Android(Mobile) 크로스 플랫폼 조작을 지원합니다.

| 기능 | Windows (Key) | Android (UI) | 동작 정의 |
| :--- | :--- | :--- | :--- |
| **캐릭터 이동** | <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> | 가상 조이스틱 (좌측 하단) | 8방향 자유 이동 및 이동 속도 비례 애니메이션 |
| **캐릭터 태그** | <kbd>U</kbd> <kbd>I</kbd> <kbd>O</kbd> | 캐릭터 아이콘 (우측 상단) | 해당 번호 캐릭터로 즉시 교체 |
| **기본 공격** | <kbd>J</kbd> | 공격 버튼 (우측 하단) | 장착된 무기로 고유 공격 |
| **무기 스킬** | <kbd>K</kbd> | 스킬 아이콘 (공격 버튼 좌측) | 장착된 무기로 콤보 공격 |
| **궁극기 (필살기)** | <kbd>L</kbd> | 궁극기 아이콘 (공격 버튼 상단) | 캐릭터의 고유한 궁극기 사용 |
| **퍼밀리어 소환** | <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd> <kbd>5</kbd> | 퍼밀리어 슬롯 5 (중앙 하단) | 슬롯에 등록된 유닛 소환 |
