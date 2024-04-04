#include "DroneSweep.h"

/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void ADroneSweep::Tick(float DeltaTime)
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
	if (FVector::Dist(GetActorLocation(), CurrentDestination) <= MovementTolerance)
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
 * This one alternates between going vertically or horizontally based on the current position of the Drone.
 */
void ADroneSweep::SetNewDestination()
{
	if (GoesUp && abs(GetActorLocation().X) >= SweepHeight)
	{
		if (LeftToRight) MoveDirection = FVector(0,1.0f,0);
		else MoveDirection = FVector(0,-1.0f,0);
		GoesUp = false;
		LeftToRight = !LeftToRight;
	}
	else if (GetActorLocation().Y <= LeftYBound
		|| GetActorLocation().Y >= SweepLength + LeftYBound)
	{
		MoveDirection = FVector(1.0f,0,0);
		GoesUp = true;
	}
	
	SetActorRotation(MoveDirection.Rotation());
	CurrentDestination = GetActorLocation() + MoveDirection * MovementDistance;
}
