// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CustomGameInstance.generated.h"

UENUM(BlueprintType)
enum class EImageFormatType : uint8
{
	/** Portable Network Graphics. */
	PNG,

	/** Joint Photographic Experts Group. */
	JPEG,

	/** OpenEXR (HDR) image file format. */
	EXR,
};

/**
 * 
 */
UCLASS()
class UNKNOWNSUBMERGENCE_API UCustomGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:

	static void OnScreenshotCapturedInternal(int32 Width, int32 Height, const TArray<FColor>& Bitmap);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "1"), Category = "TalkCameraDebug")
		FString TakeScreenShot(const FString fineName = FString(TEXT("ScreenShot")), EImageFormatType _format = EImageFormatType::PNG, int32 _quality = 100, bool bInsertTimeStampToFileName = false, bool bInShowUI = false, bool bAddFilenameSuffix = false);

	UFUNCTION(BlueprintCallable, Category = "OpenFlie")
		TArray<FString> ReturnWOpenFlies();

	static  UTexture2D* GetTexture2DFromDiskFile(const FString& FilePath);
	//‘ÿ»ÎÕºœÒ
	UFUNCTION(Category = "ExtendedContent|File", BlueprintCallable)
		UTexture2D* LoadTexture2D(const FString& ImagePath, bool& IsValid, int32& OutWidth, int32& OutHeight);
};
