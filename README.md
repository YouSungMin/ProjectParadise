# Project Paradise (스쿼드 타워 디펜스 RPG)

<kbd>
  <img height="400" alt="화면 캡처 2026-05-06 141820" src="https://github.com/user-attachments/assets/40d71852-4fba-4088-8748-0ac3ecaad9e6" />
</kbd>

<kbd>
  <img height="400" alt="화면 캡처 2026-05-06 141503" src="https://github.com/user-attachments/assets/decc9b3c-7b84-4157-b71a-872142cab942" />
</kbd>

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine_5-000000?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

[API 문서 웹사이트 (Doxygen + GitHub Pages) 보러가기] https://paradiseproject.github.io/ParadiseProject_Docs/


## 프로젝트 개요
* **개발 기간** : 2026.02.03 - 2026.04.03 
* **개발 인원** : 5명 ( 클라이언트 4명, 기획 1명 ) 
* **내 역할** : 클라이언트 프로그래머 ( 핵심 전투 시스템 및 데이터 아키텍처 설계)
* **개발 환경** : Unreal Engine 5.5.4, C++, GitHub Desktop, Notion

## 기술적 하이라이트
기획자와의 협업 효율을 높이는 데이터 주도 설계와 GAS를 활용한 확장성 높은 전투 프레임워크 구축에 집중했습니다.

**데이터 주도 설계 파이프 라인 구축**
* **데이터 주도 설계:** 수식용 엑셀과 엔진용 CSV를 분리하여 데이터 오염을 방지하고, 코드 수정 없는 즉각적인 밸런싱 환경을 구축했습니다.
* **종속성 분리:** 기존 FName 기반 하드코딩의 문제를 인지하고, 핸들(Handle) 방식을 도입해 데이터 참조 구조를 객체 지향적으로 리팩토링했습니다.
* **메모리 지연 로딩:** 스탯과 에셋 구조체를 분리하고, 무거운 리소스는 비동기 로드(Async Load)하여 초기 로딩 속도와 메모리를 최적화했습니다.

**GAS 기반 전투 아키텍처**
* **전투 연산 커스텀 (Execution & MMC):** 단순 타격을 넘어 방어력 경감이 적용되는 데미지 연산과, 시전자 스탯에 비례해 동적으로 커지는 버프 시스템을 구현했습니다.
* **데이터 주도형 투사체 패턴:** 단일 투사체 클래스(`ProjectileBase`)가 CSV 수치 조작만으로 연사, 샷건, 폭발 등 다양한 패턴으로 동작하도록 설계했습니다.
* **통합 연출 제어 (Gameplay Cue):** 수많은 타격/피격 FX 요청을 단일 마스터 큐(`UMasterCueNotifyStatic`)로 통제하여 클래스 폭발을 방지하고 결합도를 낮췄습니다.

## 핵심 구현 시스템
### GAS 기반 전투 프레임 워크

**공통 어트리뷰트 셋**

**관련 클래스** :
* [BaseAttributeSet.cpp](./Source/Paradise/Private/GAS/Attributes/BaseAttributeSet.cpp) 

**설계 방향**

코드의 중복을 줄이고 유지보수 효율을 높이기 위해 플레이어, 적(Enemy), 소환수(Familiar)가 하나의 어트리뷰트 셋 클래스를 사용하도록 설계했습니다.

PreAttributeChange를 오버라이드 하여 스탯이 설계된 최소/최대 범위를 벗어나지 않도록 실시간 클램핑(Clamping) 처리를 적용했습니다.
<img width="794" height="283" alt="화면 캡처 2026-05-12 165302" src="https://github.com/user-attachments/assets/8cbc28f4-317c-4e6e-a59a-ed74d172ef1b" />




**커스텀 연산 클래스**

**어빌리티 계층 구조**


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
