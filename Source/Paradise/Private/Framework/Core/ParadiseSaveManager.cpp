// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Core/ParadiseSaveManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/AES.h"
#include "Misc/SecureHash.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

// 실서비스에서는 이 키값을 다른 헤더에 숨기거나 서버에서 받아오는 방식을 사용 예정
const ANSICHAR* AES_SECRET_KEY = "ParadiseMySecretKey1234567890123";

bool UParadiseSaveManager::SaveGameEncrypted(USaveGame* SaveGameObject, const FString& SlotName)
{
	if (!SaveGameObject) return false;

	// 1. SaveGame 오브젝트를 바이트 배열(메모리)로 변환
	TArray<uint8> SaveData;
	if (!UGameplayStatics::SaveGameToMemory(SaveGameObject, SaveData)) return false;

	// 2. AES 블록 사이즈(16바이트)에 맞게 패딩(빈 공간) 추가
	int32 OriginalSize = SaveData.Num();
	int32 PaddedSize = Align(OriginalSize, FAES::AESBlockSize);

	// 쓰레기 값이 암호화되어 해시 위변조(Tamper) 에러가 터지는 것을 방지합니다.
	SaveData.SetNumZeroed(PaddedSize);

	// 3. AES 암호화 수행 (SaveData 배열 내부 데이터가 암호문으로 덮어씌워짐)
	FAES::EncryptData(SaveData.GetData(), PaddedSize, AES_SECRET_KEY);

	// 4. 위변조(Tamper) 방지용 해시 생성 (바이트 배열을 바로 MD5 Hex 문자열로 변환)
	FString HashString = FMD5::HashBytes(SaveData.GetData(), PaddedSize);

	// 5. 최종적으로 디스크에 쓸 파일 구조 생성 (원본 크기 + 해시 지문 + 암호화 데이터)
	FBufferArchive FinalArchive;
	FinalArchive << OriginalSize; // 복호화 후 패딩을 자르기 위해 저장
	FinalArchive << HashString;   // 위변조 검사용 해시
	FinalArchive.Serialize(SaveData.GetData(), PaddedSize);

	// 6. 실제 파일로 쓰기 (Saved/SaveGames/슬롯이름.sav)
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav"));
	return FFileHelper::SaveArrayToFile(FinalArchive, *SavePath);
}

USaveGame* UParadiseSaveManager::LoadGameEncrypted(const FString& SlotName)
{
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".sav"));

	// 1. 디스크에서 바이트 배열로 파일 읽어오기
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *SavePath)) return nullptr;

	FMemoryReader MemoryReader(FileData, true);

	int32 OriginalSize;
	FString SavedHash;

	// 2. 파일 맨 앞의 헤더 정보(원본 크기, 해시 지문) 빼오기
	MemoryReader << OriginalSize;
	MemoryReader << SavedHash;

	// 3. 나머지 실제 암호화된 본문 데이터 추출
	int32 EncryptedDataSize = FileData.Num() - MemoryReader.Tell();
	TArray<uint8> EncryptedData;
	EncryptedData.SetNumUninitialized(EncryptedDataSize);
	MemoryReader.Serialize(EncryptedData.GetData(), EncryptedDataSize);

	// 4. 저장된 데이터의 현재 해시를 구해서 원본 해시와 비교 (위변조 검사)
	FString CurrentHash = FMD5::HashBytes(EncryptedData.GetData(), EncryptedDataSize);
	if (CurrentHash != SavedHash)
	{
		//UE_LOG(LogTemp, Error, TEXT("🛑 [보안 경고] 세이브 파일이 위변조되었습니다!! 로드를 중단합니다."));
		return nullptr;
	}

	// 5. AES 복호화 수행
	FAES::DecryptData(EncryptedData.GetData(), EncryptedDataSize, AES_SECRET_KEY);

	// 6. 패딩(빈 공간) 잘라내고 원래 크기로 복원
	EncryptedData.SetNum(OriginalSize);

	// 7. 메모리에서 USaveGame 오브젝트로 변환하여 리턴
	return UGameplayStatics::LoadGameFromMemory(EncryptedData);
}
