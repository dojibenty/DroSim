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

	if (TicksToWait != 0)
	{
		TicksToWait--;
		return;
	}
	
	// Calculate the Drone's next location
	FVector NextLocation = GetActorLocation() + MoveDirection * MovementSpeed * TickInterval;
	float DistanceToDestination = FVector::Dist(GetActorLocation(), CurrentDestination);
	float NextDistanceToDestination = FVector::Dist(NextLocation, CurrentDestination);
	
	// If the Drone is close enough or has passed its destination, it is considered arrived
	if (DistanceToDestination <= MovementTolerance)
	{
		SetActorLocation(CurrentDestination);
		SetNewDestination();
	}
	else if (NextDistanceToDestination >= DistanceToDestination) NextLocation = CurrentDestination;
	
	SetActorLocation(NextLocation);
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

		// Rotate the Drone to face its destination
        SetActorRotation(MoveDirection.Rotation());
		
        CurrentDestination = GetActorLocation() + MoveDirection * MovementDistance;
	}
	while (IsOutOfBounds(CurrentDestination));
	// Redo if the next destination is outside environment limits
}
