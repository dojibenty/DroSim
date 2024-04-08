#include "DroneRandom.h"
#include "Manager.h"

/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void ADroneRandom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CalculatedPosition = GetActorLocation();
	
	for (int i = 0; i < SimulationSpeed; i++)
	{
		// Calculate the Drone's next location
		FVector NextLocation = CalculatedPosition + MoveDirection * MovementSpeed * TickInterval;
		float DistanceToDestination = FVector::Dist(CalculatedPosition, CurrentDestination);
		float NextDistanceToDestination = FVector::Dist(NextLocation, CurrentDestination);
	
		// If the Drone is close enough or has passed its destination, it is considered arrived
		if (DistanceToDestination <= MovementTolerance)
		{
			CalculatedPosition = CurrentDestination;
			SetNewDestination();
		}
		else if (NextDistanceToDestination >= DistanceToDestination) NextLocation = CurrentDestination;
	
		CalculatedPosition = NextLocation;

		if (Manager->IsObjectiveNear(CalculatedPosition)) Manager->ObjectiveFound();
	}

	SetActorLocation(CalculatedPosition);
	SetActorRotation(MoveDirection.Rotation());
	DrawDebugCircle(GetWorld(),CalculatedPosition,Manager->GetVisionRadius(),32,FColor::Yellow,false,TickInterval,0,10,FVector(0,1,0),FVector(1,0,0),false);
}


/**
 * Set a new destination point for the Drone to move towards.
 *
 * This one picks a random point in a cone in front of the Drone.
 */
void ADroneRandom::SetNewDestination()
{
	do
	{
		// Random direction vector in a cone
		FVector NewMoveDirection = FVector(
                			MoveDirection.X + FMath::RandRange(-1.0f,1.0f),
                			MoveDirection.Y + FMath::RandRange(-1.0f,1.0f),
                			0);

		// Normalize the direction vector
        float Magnitude = NewMoveDirection.Size();
        if (Magnitude > SMALL_NUMBER) MoveDirection = NewMoveDirection / Magnitude;
        else MoveDirection = NewMoveDirection;
		
        CurrentDestination = CalculatedPosition + MoveDirection * MovementDistance;
	}
	while (IsOutOfBounds(CurrentDestination));
	// Redo if the next destination is outside environment limits
}
