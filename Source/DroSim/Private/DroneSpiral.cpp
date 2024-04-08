#include "DroneSpiral.h"

/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void ADroneSpiral::Tick(float DeltaTime)
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
			if (--Wander == 0) SetCircle();
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
 * This one picks a random point to wander or calculate the next spiral point
 */
void ADroneSpiral::SetNewDestination()
{
	if (Wander > 0) GetRandomDirection();
	else // Making spiral
	{
		CurrentCirclePointId = CurrentCirclePointId % NbCirclePoints + 1;
		FVector CurrentCirclePoint = CirclePoints[CurrentCirclePointId-1];

		float DistX = CurrentCirclePoint.X - CurrentCircleCenter.X;
		float DistY = CurrentCirclePoint.Y - CurrentCircleCenter.Y;

		// Pick an intermediate point placed between the current circle center and the current circle point selected
		FVector IntermediatePoint = FVector(
			CurrentCircleCenter.X + ((DistX / NbCirclePoints) * CurrentSpiralIncrementFactor),
			CurrentCircleCenter.Y + ((DistY / NbCirclePoints) * CurrentSpiralIncrementFactor),
			CurrentCircleCenter.Z);

		if (!DrawsConcentricCircles) CurrentSpiralIncrementFactor += SpiralIncrementFactor / NbCirclePoints;
		else if (CurrentCirclePointId == NbCirclePoints) CurrentSpiralIncrementFactor += SpiralIncrementFactor;

		// Can be translated as "if the current intermediate point is equal or is greater than the selected circle point"
		if (CurrentSpiralIncrementFactor >= NbCirclePoints
			|| IsOutOfBounds(IntermediatePoint))
		{
			// Go back at the center of the circle
			MoveDirection = CurrentCircleCenter - CalculatedPosition;
			CurrentDestination = CurrentCircleCenter;
			Wander = WanderSteps + 1;
		}
		else
		{
			MoveDirection = IntermediatePoint - CalculatedPosition;
			CurrentDestination = IntermediatePoint;
		}
		
		MoveDirection.Normalize();
	}
}


/**
 * Set a circle with the Drone's location as the center for spiral movement.
 */
void ADroneSpiral::SetCircle()
{
	CirclePoints.Empty();
	
	const float AngleStep = 2.0f * PI / NbCirclePoints;
	
	for (int i = 0; i < NbCirclePoints; ++i)
	{
		float Angle = i * AngleStep;
		FVector Point(
			SpiralRadius * FMath::Cos(Angle),
			SpiralRadius * FMath::Sin(Angle),
			0);
		Point = Point + CalculatedPosition;
		CirclePoints.Add(Point);
	}
	
	CurrentCircleCenter = CalculatedPosition;
	CurrentSpiralIncrementFactor = 1;
}


/**
 * Calculate a random destination for the Drone, effectively making it wander.
 */
void ADroneSpiral::GetRandomDirection()
{
	do
	{
		FVector NewMoveDirection = FVector(
		MoveDirection.X + FMath::RandRange(-1.0f,1.0f),
		MoveDirection.Y + FMath::RandRange(-1.0f,1.0f),
		0);
		float Magnitude = NewMoveDirection.Size();
		if (Magnitude > UE_SMALL_NUMBER) MoveDirection = NewMoveDirection / Magnitude;
		else MoveDirection = NewMoveDirection;
		CurrentDestination = CalculatedPosition + MoveDirection * WanderDistance;
	}
	while (IsOutOfBounds(CurrentDestination));
}