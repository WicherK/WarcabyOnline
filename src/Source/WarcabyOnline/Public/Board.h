#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckerPawn.h"
#include "BoardSlot.h"
#include "Board.generated.h"

class ABoardSlot;
class ABoardPawn;

UCLASS()
class WARCABYONLINE_API ABoard : public AActor
{
	GENERATED_BODY()
	
private:
	void SpawnSlots();
	void SpawnPawns();

public:	
	ABoard();

	UPROPERTY(ReplicatedUsing = OnRep_BoardState)
	TArray<FBoardSlotData> boardState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Board")
	TSubclassOf<ABoardSlot> BP_BoardSlot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Board")
	TSubclassOf<ACheckerPawn> BP_CheckerPawn;

	bool TryMove(FIntPoint from, FIntPoint to);

	void ShowAvailableMoves(FIntPoint from);

	void HideAvailableMoves();

	void TryPromotePawn(FBoardSlotData* slot, ACheckerPawn* pawn);

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* moveSound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* tapperSound;

	UFUNCTION()
	void OnRep_BoardState();

	FORCEINLINE bool IsValidPos(FIntPoint pos) const
	{
		return pos.X >= 0 && pos.X < 8 && pos.Y >= 0 && pos.Y < 8;
	}

	FBoardSlotData* GetSlot(FIntPoint pos);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Board")
	UStaticMeshComponent* staticMesh;

	UPROPERTY(VisibleAnywhere, Category = "Board")
	USceneComponent* refPoint;

	UPROPERTY(EditAnywhere, Category = "Board")
	float spaceOffset = 22.9f;

	FORCEINLINE int32 ToIndex(FIntPoint pos) const
	{
		return pos.Y * 8 + pos.X;
	}

	ACheckerPawn* GetPawnActorAt(FIntPoint pos);

	ABoardSlot* GetBoardSlotActorAt(FIntPoint pos);

	TArray<FIntPoint> GetNormalMoves(FIntPoint from);
	TArray<FIntPoint> GetCaptureMoves(FIntPoint from);
	bool CanCapture(ECheckerPawnType playerSide);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMoveSound(FVector location);

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
