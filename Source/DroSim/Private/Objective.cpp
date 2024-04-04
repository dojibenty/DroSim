#include "Objective.h"

AObjective::AObjective()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creation of a mesh component for the objective
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;
	//static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Meshes/Ship_Mesh"));
	if (MeshAsset.Succeeded()) StaticMesh->SetStaticMesh(MeshAsset.Object);

	// Loading config from .ini file
	LoadConfig();

	// Collision sphere for the objective to be seen by Drones
	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	HitSphere->InitSphereRadius(CollisionCheckRadius);
	HitSphere->SetCollisionProfileName("OverlapAll");
	HitSphere->SetupAttachment(RootComponent);
}


/**
 * Loading configuration from .ini file
 *
 * Sets initial values to variables.
 */
void AObjective::LoadConfig()
{
	FString ConfigFilePath = FPaths::ProjectContentDir() + TEXT("SimConfig.ini");
	ConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(ConfigFilePath);

	GConfig->LoadFile(ConfigFilePath);
	
    GConfig->GetInt(TEXT("sim/global"), TEXT("sim_speed"), SimulationSpeed, ConfigFilePath);
    GConfig->GetFloat(TEXT("sim/global"), TEXT("step"), TickInterval, ConfigFilePath);
	
	GConfig->GetBool(TEXT("sim/objective"), TEXT("is_moving"), IsMoving, ConfigFilePath);
	if (IsMoving)
	{
		GConfig->GetFloat(TEXT("sim/global"), TEXT("environment_Y_length"), YLimit, ConfigFilePath);
		GConfig->GetFloat(TEXT("sim/objective"), TEXT("speed"), MovementSpeed, ConfigFilePath);
	}

	GConfig->GetFloat(TEXT("sim/objective"), TEXT("collision_check_radius"), CollisionCheckRadius, ConfigFilePath);
}


/**
 * First function to be executed after instantiation.
 */
void AObjective::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickInterval(TickInterval);

	SetActorRotation(MoveDirection.Rotation());
	SetActorLocation(FVector(GetActorLocation().X,GetActorLocation().Y,100.0));
}


/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void AObjective::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsMoving) return;
	
	FVector NextLocation = GetActorLocation() + MoveDirection * MovementSpeed * SimulationSpeed;
	
	if (NextLocation.Y >= 0 && NextLocation.Y <= YLimit) SetActorLocation(NextLocation);
	else
	{
		MoveDirection.Y = -MoveDirection.Y;
		SetActorRotation(MoveDirection.Rotation());
	}
}

