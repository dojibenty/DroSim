#pragma once

#include "CoreMinimal.h"
#include "Drone.h"
#include "DroneSpiral.generated.h"

UCLASS()
class DROSIM_API ADroneSpiral : public ADrone
{
	GENERATED_BODY()
	virtual void Tick(float DeltaTime) override;
	virtual void SetNewDestination() override;
	void SetCircle();
	void GetRandomDirection();
	int Wander = WanderSteps;
	int Precision = 0;

	FVector HotSpot;
	float SpotCertainty = -1;
	
	TArray<FVector> CirclePoints;
	int CurrentCirclePointId = 0;
	FVector CurrentIntermediatePoint;
	FVector CurrentCircleCenter;
	int CurrentCircleCount;
	float CurrentSpiralIncrementFactor;
};
