#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Objective.generated.h"

UCLASS()
class DROSIM_API AObjective : public AActor
{
	GENERATED_BODY()
	
public:	
	AObjective();

protected:
	virtual void BeginPlay() override;
	void LoadConfig();

public:	
	virtual void Tick(float DeltaTime) override;

private:
	float MovementSpeed;
	float YLimit;
	float CollisionCheckRadius;
	float TickInterval;
	bool IsMoving;
	int SimulationSpeed;
	
	FVector MoveDirection = FVector(0,1.0f,0);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* HitSphere;
};
