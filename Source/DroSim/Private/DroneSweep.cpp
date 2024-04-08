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
 * This one alternates between going vertically or horizontally based on the current position of the Drone.
 */
void ADroneSweep::SetNewDestination()
{
	if (GoesUp && CalculatedPosition.X >= SweepHeight * HeightCount)
	{
		if (LeftToRight) MoveDirection = FVector(0,1.0f,0);
		else MoveDirection = FVector(0,-1.0f,0);
		GoesUp = false;
		LeftToRight = !LeftToRight;
	}
	else if (CalculatedPosition.Y - MovementDistance < LeftYBound
		|| CalculatedPosition.Y + MovementDistance > SweepLength + LeftYBound)
	{
		if (TopToBottom) MoveDirection = FVector(-1.0f,0,0);
		else MoveDirection = FVector(1.0f,0,0);
		if (!GoesUp) HeightCount++;
		GoesUp = true;
	}
	
	CurrentDestination = CalculatedPosition + MoveDirection * MovementDistance;
	if (IsOutOfBounds(CurrentDestination))
	{
		if (GoesUp)
		{
			CurrentDestination = CalculatedPosition - MoveDirection * MovementDistance;
			TopToBottom = !TopToBottom;
		}
		else if (LeftToRight) CurrentDestination = CalculatedPosition + MoveDirection * (AssignedZone[1].Y - CalculatedPosition.Y);
        else CurrentDestination = CalculatedPosition + MoveDirection * (CalculatedPosition.Y - AssignedZone[0].Y);
	}
	
}
