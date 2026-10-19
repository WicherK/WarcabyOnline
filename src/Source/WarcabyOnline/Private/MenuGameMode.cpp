#include "MenuGameMode.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

AMenuGameMode::AMenuGameMode()
{
	//bUseSeamlessTravel = true;
}

void AMenuGameMode::SaveSettings(FSettings settings)
{
	// SYSTEM PLIKOW FILE MANAGER
	FString SaveDir = FPaths::ProjectUserDir();
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	FString SavePath = SaveDir / TEXT("settings.txt");

	FString Data = FString::Printf(TEXT("%d\n%f"),
		settings.isMuted ? 1 : 0,
		settings.volume);

	if (!FFileHelper::SaveStringToFile(Data, *SavePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save settings: %s"), *SavePath);
	}
}

FSettings AMenuGameMode::GetSettings()
{
	FSettings settings;
	settings.isMuted = false;
	settings.volume = 1.0f;

	FString SavePath = FPaths::ProjectUserDir() / TEXT("settings.txt");

	FString FileContent;

	if (FFileHelper::LoadFileToString(FileContent, *SavePath))
	{
		TArray<FString> Lines;
		FileContent.ParseIntoArrayLines(Lines);

		if (Lines.Num() >= 2)
		{
			settings.isMuted = FCString::Atoi(*Lines[0]) != 0;
			settings.volume = FCString::Atof(*Lines[1]);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Settings file not found: %s"), *SavePath);
	}

	return settings;
}

void AMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("New player joined."));
	
		APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>();

		FString ID = PS->GetUniqueId()->ToString();

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("Your id: ") + ID);
	}

	if (GameState && GameState->PlayerArray.Num() >= 2)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString("All 2 players joined, we travel to game map."));
	
		GetWorld()->ServerTravel("/Game/Levels/BoardMap?listen");
	}
}