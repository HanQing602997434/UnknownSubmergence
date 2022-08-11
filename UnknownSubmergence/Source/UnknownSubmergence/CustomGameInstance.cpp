// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomGameInstance.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Runtime/Engine/Classes/Engine/GameViewportClient.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h" 


EImageFormatType format;
int32 quality;
FString fileName;
FDelegateHandle delegateHandle;



void UCustomGameInstance::OnScreenshotCapturedInternal(int32 Width, int32 Height, const TArray<FColor>& Bitmap)
{
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper;
	if (format == EImageFormatType::PNG)
	{
		TArray<uint8> PNGData;
		TArray<FColor> BitmapCopy(Bitmap);
		FImageUtils::CompressImageArray(Width, Height, BitmapCopy, PNGData);

		fileName += ".png";
		FFileHelper::SaveArrayToFile(PNGData, *fileName);
	}
	else if (format == EImageFormatType::JPEG)
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
		ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
		const TArray64<uint8>& JPEGData = ImageWrapper->GetCompressed(quality);
		fileName += ".jpg";
		FFileHelper::SaveArrayToFile(JPEGData, *fileName);
	}
	else if (format == EImageFormatType::EXR)
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);
		ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
		const TArray64<uint8>& Data = ImageWrapper->GetCompressed(quality);
		fileName += ".exr";
		FFileHelper::SaveArrayToFile(Data, *fileName);
	}
	else
	{
		//默认使用Jpg格式
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
		ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
		const TArray64<uint8>& JPEGData = ImageWrapper->GetCompressed(quality);
		fileName += ".jpg";
		FFileHelper::SaveArrayToFile(JPEGData, *fileName);
	}
	UGameViewportClient::OnScreenshotCaptured().Remove(delegateHandle);
	ImageWrapper.Reset();

}

void UCustomGameInstance::TakeScreenShot(const FString fineName /*= FString(TEXT("ScreenShot"))*/, EImageFormatType _format /*= EImageFormatType::PNG*/,
	int32 _quality /*= 100*/, bool bInsertTimeStampToFileName /*= true*/, bool bInShowUI /*= true*/, bool bAddFilenameSuffix /*= true*/)
{
	FString filename = fineName;

	format = _format;
	quality = _quality;

	//名字后面加上现在的时间
	if (bInsertTimeStampToFileName)
	{
		filename += "_" + FDateTime::Now().ToString();
	}
	//名字后面加上下划线
	if (bAddFilenameSuffix)
	{
		filename += "_";
	}

	fileName = FPaths::ProjectSavedDir() + "ScreenShots/WindowsNoEditor/" + filename;

	if (!UGameViewportClient::OnScreenshotCaptured().IsBound())
	{
		delegateHandle = UGameViewportClient::OnScreenshotCaptured().AddStatic(&OnScreenshotCapturedInternal);

		FScreenshotRequest::RequestScreenshot(filename, bInShowUI, bAddFilenameSuffix);
		//加这个是高分辨率截图但是会有锯齿
		//GEngine->GameViewport->Exec(nullptr, TEXT("HighResShot 2560x1280"), *GLog);
	}


}

TArray<FString> UCustomGameInstance::ReturnWOpenFlies()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	TArray<FString> OpenFilenames;

	FString ExtensionStr;


	/*if (EUploadMode::UploadFastMode == CurrrentUploadMode)

	{

		ExtensionStr = TEXT("FBX,JPG,BMP,PNG files|*.fbx;*.jpg;*.png;*.bmp");

	}

	else

	{

		if (EImportType::ImportMeshType == Type)

		{

			ExtensionStr = TEXT("FBX Files|*.fbx");

		}

		else

		{

			ExtensionStr = TEXT("JPG,BMP,PNG files|*.jpg;*.png;*.bmp");

		}

	}*/
	ExtensionStr = TEXT("JPG,BMP,PNG files|*.jpg;*.png;*.bmp");
	DesktopPlatform->OpenFileDialog(nullptr, TEXT("Open Dir"), FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir()), TEXT(""), *ExtensionStr, EFileDialogFlags::None, OpenFilenames);
	return OpenFilenames;
}



UTexture2D * UCustomGameInstance::GetTexture2DFromDiskFile(const FString & FilePath)
{
	TArray<uint8> RawFileData;
	UTexture2D* MyTexture = NULL;
	if (FFileHelper::LoadFileToArray(RawFileData, *FilePath /*"<path to file>"*/))
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		// Note: PNG format.  Other formats are supported
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			TArray<uint8>* UncompressedBGRA = NULL;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, *UncompressedBGRA))
			{
				// Create the UTexture for rendering
				MyTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

				// Fill in the source data from the file
				void* TextureData = MyTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
				MyTexture->PlatformData->Mips[0].BulkData.Unlock();

				// Update the rendering resource from data.
				MyTexture->UpdateResource();
			}
		}
	}
	return MyTexture;
}

static TSharedPtr<IImageWrapper> GetImageWrapperByExtention(const FString InImagePath)
{
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	if (InImagePath.EndsWith(".png"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	}
	else if (InImagePath.EndsWith(".jpg") || InImagePath.EndsWith(".jpeg"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	}
	else if (InImagePath.EndsWith(".bmp"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
	}
	else if (InImagePath.EndsWith(".ico"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::ICO);
	}
	else if (InImagePath.EndsWith(".exr"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);
	}
	else if (InImagePath.EndsWith(".icns"))
	{
		return ImageWrapperModule.CreateImageWrapper(EImageFormat::ICNS);
	}
	return nullptr;
}

UTexture2D * UCustomGameInstance::LoadTexture2D(const FString & ImagePath, bool & IsValid, int32 & OutWidth, int32 & OutHeight)
{
	UTexture2D* Texture = nullptr;
	IsValid = false;
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ImagePath))
	{
		return nullptr;
	}
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *ImagePath))
	{
		return nullptr;
	}
	TSharedPtr<IImageWrapper> ImageWrapper = GetImageWrapperByExtention(ImagePath);
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
	{
		TArray<uint8> UncompressedRGBA;
		if (ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, UncompressedRGBA))
		{
			Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_R8G8B8A8);
			if (Texture != nullptr)
			{
				IsValid = true;
				OutWidth = ImageWrapper->GetWidth();
				OutHeight = ImageWrapper->GetHeight();
				void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedRGBA.GetData(), UncompressedRGBA.Num());
				Texture->PlatformData->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	return Texture;

}