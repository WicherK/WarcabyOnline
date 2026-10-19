#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MenuGameMode.generated.h"

USTRUCT(BlueprintType)
struct FSettings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool isMuted;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float volume;
};

UCLASS()
class WARCABYONLINE_API AMenuGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AMenuGameMode();

	UFUNCTION(BlueprintCallable)
	void SaveSettings(FSettings settings);

	UFUNCTION(BlueprintCallable)
	FSettings GetSettings();

private:
	TArray<APlayerController*> players;

protected:
	void BeginPlay() override;
	void PostLogin(APlayerController* NewPlayer) override;
};
