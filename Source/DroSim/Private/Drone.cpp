#include "Drone.h"

#include "DroneSpiral.h"
#include "DroneSweep.h"

ADrone::ADrone()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creation of a mesh component for the drones
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;
	//static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Meshes/Drone_Mesh"));
	if (MeshAsset.Succeeded()) StaticMesh->SetStaticMesh(MeshAsset.Object);

	// Loading config from .ini file
	LoadConfig();
}


/**
 * Loading configuration from .ini file
 *
 * Sets initial values to variables.
 */
void ADrone::LoadConfig()
{
	FString ConfigFilePath = FPaths::ProjectContentDir() + TEXT("SimConfig.ini");
	ConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(ConfigFilePath);

	GConfig->LoadFile(ConfigFilePath);
	
	GConfig->GetInt(TEXT("sim/global"), TEXT("sim_speed"), SimulationSpeed, ConfigFilePath);
	
	GConfig->GetFloat(TEXT("sim/global"), TEXT("step"), TickInterval, ConfigFilePath);
	
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("movement_tolerance"), MovementTolerance, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("movement_distance"), MovementDistance, ConfigFilePath);
	
	GConfig->GetDouble(TEXT("sim/global"), TEXT("environment_X_length"), EnvSize.X, ConfigFilePath);
	GConfig->GetDouble(TEXT("sim/global"), TEXT("environment_Y_length"), EnvSize.Y, ConfigFilePath);

	GConfig->GetFloat(TEXT("sim/drones"), TEXT("ground_offset"), GroundOffset, ConfigFilePath);

	if (this->GetClass() == ADroneSpiral::StaticClass())
	{
		GConfig->GetFloat(TEXT("sim/drones/spiral"), TEXT("spiral_radius"), SpiralRadius, ConfigFilePath);
		GConfig->GetFloat(TEXT("sim/drones/spiral"), TEXT("wander_distance"), WanderDistance, ConfigFilePath);
		GConfig->GetInt(TEXT("sim/drones/spiral"), TEXT("wander_steps"), WanderSteps, ConfigFilePath);
		GConfig->GetFloat(TEXT("sim/drones/spiral"), TEXT("spiral_increment_factor"), SpiralIncrementFactor, ConfigFilePath);
		GConfig->GetBool(TEXT("sim/drones/spiral"), TEXT("concentric_circles"), DrawsConcentricCircles, ConfigFilePath);
		GConfig->GetInt(TEXT("sim/drones/spiral"), TEXT("circle_points"), NbCirclePoints, ConfigFilePath);
	}
	else if (this->GetClass() == ADroneSweep::StaticClass())
		GConfig->GetFloat(TEXT("sim/drones/sweep"), TEXT("sweep_height"), SweepHeight, ConfigFilePath);
}


/**
 * First function to be executed after instantiation.
 */
void ADrone::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickInterval(TickInterval);
}


/**
 * Set a new destination point for a drone to move towards.
 *
 * Overridden by derived Drone classes.
 */
void ADrone::SetNewDestination() {}


/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void ADrone::Tick(float DeltaTime)
{
	if (Init)
	{
		// Set the speed of the Drone as the Manager reference is only resolved after BeginPlay()
		MovementSpeed = Manager->GetGroupDroneSpeed();
		
		if (this->GetClass() == ADroneSweep::StaticClass())
		{
			if (AssignedZone[1].X + AssignedZone[0].Y != 0)
				SetDestinationManual(FVector(AssignedZone[1].X,AssignedZone[0].Y,GroundOffset));
			SweepLength = AssignedZone[1].Y - AssignedZone[0].Y;
			LeftYBound = AssignedZone[0].Y;
		}
		else
		{
			SetDestinationManual(FVector(
		(AssignedZone[0].X + AssignedZone[1].X) / 2.0,
		(AssignedZone[0].Y + AssignedZone[1].Y) / 2.0,
		GroundOffset));
		}
		
		Init = false;
	}
	//DrawDebugSphere(GetWorld(), CurrentDestination, 50, 8, FColor::Yellow, false, TickInterval*SimulationSpeed); // Current destination tracker
	Super::Tick(DeltaTime);
}


/**
 * Manually sets the destination point.
 */
void ADrone::SetDestinationManual(const FVector& NewDestination)
{
	CurrentDestination = MoveDirection = NewDestination;
	MoveDirection.Normalize();
	SetActorRotation(MoveDirection.Rotation());
}


/**
 * Checks whether the given point is outside the boundaries of the drone's assigned zone.
 */
bool ADrone::IsOutOfBounds(const FVector& Point) const
{
	return Point.X < AssignedZone[1].X
		|| Point.Y < AssignedZone[0].Y
		|| Point.X > AssignedZone[0].X
		|| Point.Y > AssignedZone[1].Y;
}
