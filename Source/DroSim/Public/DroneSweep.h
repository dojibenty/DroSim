#pragma once

#include "CoreMinimal.h"
#include "Drone.h"
#include "DroneSweep.generated.h"

UCLASS()
class DROSIM_API ADroneSweep : public ADrone
{
	GENERATED_BODY()
	virtual void Tick(float DeltaTime) override;
	virtual void SetNewDestination() override;
	bool GoesUp = true;
	bool LeftToRight = true;
};
