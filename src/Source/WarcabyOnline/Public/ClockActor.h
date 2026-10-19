// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckerGameState.h"
#include "ClockActor.generated.h"

UCLASS()
class WARCABYONLINE_API AClockActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AClockActor();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	USceneComponent* defaultSceneComponenet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* whiteSecondsHand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* whiteMinutesHand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* blackSecondsHand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* blackMinutesHand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* whiteTapper;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Clock")
	UStaticMeshComponent* blackTapper;

	UPROPERTY(EditDefaultsOnly, Category = "Clock")
	float tapperPressedOffset = -5.f; // how much it moves down

private:
	ACheckerGameState* gameState;

	FVector whiteTapperStart;
	FVector blackTapperStart;
	
	void InitializeClock(float timeInSeconds, UStaticMeshComponent* secondsHand, UStaticMeshComponent* minutesHand);

	void UpdateClock(float timeLeft, UStaticMeshComponent* secondsHand, UStaticMeshComponent* minutesHand);

	void HandleTurnChanged(ECheckerPawnType newTurn);

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
