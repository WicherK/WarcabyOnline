#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckerPawn.generated.h"

UENUM(BlueprintType)
enum class ECheckerPawnType : uint8
{
	None,
	White,
	Black
};

USTRUCT(BlueprintType)
struct FCheckerPawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint tilePosition = FIntPoint::ZeroValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool isKing = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECheckerPawnType pawnType;
};

class UBoxComponent;

UCLASS()
class WARCABYONLINE_API ACheckerPawn : public AActor
{
	GENERATED_BODY()

private:
	UMaterialInstanceDynamic* dynamicMaterial;

	UPROPERTY()
	FLinearColor rememberedLastSlotColor;

	bool playCrownAnimation = false;

public:	
	ACheckerPawn();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checker Pawn")
	UStaticMeshComponent* checkerPawnMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checker Pawn")
	UStaticMeshComponent* crown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checker Pawn")
	UBoxComponent* collider;

	UPROPERTY(ReplicatedUsing = OnRep_PawnType)
	ECheckerPawnType pawnType;

	UPROPERTY(Replicated)
	FIntPoint tilePosition;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_IsKing)
	bool isKing = false;

	FIntPoint destinationTilePosition;

	float originalZ;

	void SetColor(FLinearColor color);

	UFUNCTION()
	void OnRep_PawnType();

	UFUNCTION()
	void OnRep_IsKing();

	void Promote();

protected:
	virtual void BeginPlay() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
