#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CheckerPawn.h"
#include "BoardSlot.generated.h"

USTRUCT(BlueprintType)
struct FBoardSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint tilePosition = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector worldPosition = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool isHoldingPawn = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCheckerPawnData holdedPawn;
};

UCLASS()
class WARCABYONLINE_API ABoardSlot : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY()
	UMaterialInstanceDynamic* dynamicMaterial;

protected:
	virtual void BeginPlay() override;
	
public:	
	ABoardSlot();

	UPROPERTY()
	USceneComponent* rootSlotComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Board Slot")
	UStaticMeshComponent* shadowMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Board Slot")
	UBoxComponent* boxCollider;

	UPROPERTY(Replicated)
	FIntPoint tilePosition;

	UPROPERTY()
	FLinearColor lastRememberedColor;

	void SetHighlight(bool isHiglighted);

	FLinearColor GetColor();

	void SetColor(FLinearColor color);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
