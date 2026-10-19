#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString playerID;
	
	UPROPERTY(BlueprintReadWrite)
	FString nickname;

	UPROPERTY(BlueprintReadWrite)
	int32 wins;

	UPROPERTY(BlueprintReadWrite)
	int32 loses;

	UPROPERTY(BlueprintReadWrite)
	int32 rankpoints;

	UPROPERTY(BlueprintReadWrite)
	int32 level;

	UPROPERTY(BlueprintReadWrite)
	int32 levelpoints;

	UPROPERTY(BlueprintReadWrite)
	int32 nextLevelPoints;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerInfoDetails, FPlayerInfo, PlayerInfo);

UCLASS()
class WARCABYONLINE_API UMultiplayerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
private:
	FName mySessionName;

	IOnlineSubsystem* subsystem;

	IOnlineSessionPtr sessionInterface;

	TSharedPtr<FOnlineSessionSearch> sessionSearch;

	bool createGameAfterDestroy = false;
	bool findGameAfterDestroy = false;
	bool goToMenuAfterDestroy = false;

	bool cancelRequested = false;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerInfoDetails onPlayerInfoDetails;

	void GoToMenu();

protected:
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnDestroySessionComplete(FName sessionName, bool bWasSuccessful);
	void OnCreateSessionComplete(FName sessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);

public:
	UPROPERTY(BlueprintReadOnly)
	FPlayerInfo playerInfoStruct;

	UPROPERTY(BlueprintReadOnly)
	bool isInfoSet = false;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly)
	bool isSearching = false;

	UFUNCTION(BlueprintCallable)
	void SetPlayerInfo();

	UFUNCTION(BlueprintCallable)
	void FindGame();

	UFUNCTION()
	void CreateGame();

	UFUNCTION(BlueprintCallable)
	void Cancel();

	UFUNCTION(BlueprintCallable)
	void Quit();
};
