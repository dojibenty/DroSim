#pragma once

#include "CoreMinimal.h"
#include "Drone.h"
#include "DroneRandom.generated.h"

UCLASS()
class DROSIM_API ADroneRandom : public ADrone
{
	GENERATED_BODY()
	virtual void Tick(float DeltaTime) override;
	virtual void SetNewDestination() override;
};
