#include "Drone.h"

#include "AsyncTreeDifferences.h"
#include "DroneSpiral.h"
#include "DroneSweep.h"
#include "Objective.h"

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

	// Collision sphere for long distance vision of the drone
	VisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("VisionSphere"));
	VisionSphere->InitSphereRadius(VisionSphereRadius);
	VisionSphere->SetCollisionProfileName("OverlapAll");
	VisionSphere->SetGenerateOverlapEvents(true);
	VisionSphere->OnComponentBeginOverlap.AddDynamic(this,&ADrone::OnObjectVisible);
	VisionSphere->SetupAttachment(RootComponent);

	// Collision sphere for near collisions with other drones
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->InitSphereRadius(CollisionCheckRadius);
	DetectionSphere->SetCollisionProfileName("OverlapAll");
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this,&ADrone::OnObjectNear);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this,&ADrone::OnObjectAway);
	DetectionSphere->SetupAttachment(RootComponent);
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
	TickInterval /= SimulationSpeed;
	
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("movement_tolerance"), MovementTolerance, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("movement_distance"), MovementDistance, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("collision_check_radius"), CollisionCheckRadius, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("vision_sphere_radius"), VisionSphereRadius, ConfigFilePath);
	
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
		MovementSpeed *= SimulationSpeed;
		
		if (this->GetClass() == ADroneSweep::StaticClass())
		{
			if (AssignedZone[1].X + AssignedZone[0].Y != 0)
				SetDestinationManual(FVector(AssignedZone[1].X,AssignedZone[0].Y,GroundOffset));
			SweepLength = AssignedZone[1].Y - AssignedZone[0].Y;
			LeftYBound = AssignedZone[0].Y;
			UE_LOG(LogTemp,Warning,TEXT("%f - %f = %f"), AssignedZone[1].Y, AssignedZone[0].Y, SweepLength);
		}
		else
		{
			SetDestinationManual(FVector(
		(AssignedZone[0].X + AssignedZone[1].X) / 2.0,
		(AssignedZone[0].Y + AssignedZone[1].Y) / 2.0,
		GroundOffset));
			DrawDebugSphere(GetWorld(), CurrentDestination, 10, 8, FColor::Blue, true, -1);
		}
		
		Init = false;
	}
	//DrawDebugSphere(GetWorld(), CurrentDestination, 10, 8, FColor::Blue, false, TickInterval); // Current destination tracker
	//DrawDebugSphere(GetWorld(), GetActorLocation(), VisionSphereRadius, 32, FColor::Red, false, TickInterval); // Vision tracker
	Super::Tick(DeltaTime);
}

void ADrone::SetDestinationManual(const FVector& NewDestination)
{
	CurrentDestination = MoveDirection = NewDestination;
	MoveDirection.Normalize();
	SetActorRotation(MoveDirection.Rotation());
}



/**
 * Thrown when an object enters the DetectionSphere of this Drone.
 *
 * @param OtherActor Object entering the sphere.
 */
void ADrone::OnObjectNear(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ADrone* OtherDrone = Cast<ADrone>(OtherActor);
	if (!OtherDrone || OtherDrone == this) return;
	if (OverlappingDrones.Contains(OtherDrone)) return;
	//DrawDebugSphere(GetWorld(), GetActorLocation(), CollisionCheckRadius, 32, FColor::Red, false, TickInterval);
	FVector OtherDroneLocation = OtherDrone->GetActorLocation();
	MoveDirection = (OtherDroneLocation - GetActorLocation()).GetSafeNormal();
	if (OtherDrone->ID > ID) TicksToWait = 4;
	OverlappingDrones.Add(OtherDrone);
}


/**
 * Thrown when an object exits any collision sphere of this Drone.
 *
 * @param OtherActor Object exiting the sphere.
 */
void ADrone::OnObjectAway(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex)
{
	const ADrone* OtherDrone = Cast<ADrone>(OtherActor);
	if (!OtherDrone || OtherDrone == this) return;
	MoveDirection = CurrentDestination.GetSafeNormal();
	OverlappingDrones.Remove(OtherDrone);
}


/**
 * Thrown when an object enters the VisionSphere of this Drone.
 *
 * @param OtherActor Object entering the sphere.
 */
void ADrone::OnObjectVisible(UPrimitiveComponent* ThisSphere, AActor* OtherActor, UPrimitiveComponent* OtherSphere, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AObjective* Objective = Cast<AObjective>(OtherActor);
	if (!Objective) return;
	Manager->ObjectiveFound();
}

bool ADrone::IsOutOfBounds(const FVector& Point) const
{
	return Point.X < AssignedZone[1].X
		|| Point.Y < AssignedZone[0].Y
		|| Point.X > AssignedZone[0].X
		|| Point.Y > AssignedZone[1].Y;
}
