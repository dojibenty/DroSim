#pragma once

#include "CoreMinimal.h"
#include "IManagerInterface.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Drone.generated.h"

UCLASS()
class DROSIM_API ADrone : public APawn
{
	GENERATED_BODY()

public:
	ADrone();

protected:
	virtual void BeginPlay() override;
	virtual void SetNewDestination();
	
	void SetDestinationManual(const FVector& NewDestination);
	bool IsOutOfBounds(const FVector& Point) const;
	void LoadConfig();

	int SimulationSpeed;
	float TickInterval;
	float GroundOffset;
	
	float MovementTolerance;
	float MovementSpeed;
	float MovementDistance;
	
	FVector CurrentDestination;
	FVector MoveDirection = FVector(1.0f,0,0);
	FVector2D EnvSize;
	
	float WanderDistance;
	int WanderSteps;
	float SpiralRadius;
	float SpiralMovementDistance;
	float SpiralIncrementFactor;
	bool DrawsConcentricCircles;
	int NbCirclePoints;
	
	float SweepHeight;
	float SweepLength;
	float LeftYBound;
	
	int TicksToWait = 0;
	
public:
	virtual void Tick(float DeltaTime) override;
	int ID = -1;
	IManagerInterface* Manager;
	std::vector<FVector2D> AssignedZone;
	
	UFUNCTION()
	void OnObjectNear(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnObjectAway(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex);

	UFUNCTION()
	void OnObjectVisible(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	float CollisionCheckRadius;
	float VisionSphereRadius;

	bool Init = true;

	TSet<ADrone*> OverlappingDrones;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* DetectionSphere;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* VisionSphere;
};