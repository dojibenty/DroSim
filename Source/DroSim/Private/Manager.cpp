#include "Manager.h"

#include "Misc/ConfigCacheIni.h"
#include "DrawDebugHelpers.h"
#include "Drone.h"
#include "DroneRandom.h"
#include "DroneSweep.h"
#include "DroneSpiral.h"
#include "Objective.h"


AManager::AManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Creation of a mesh component for the manager
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;
	//static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Meshes/Ship_Mesh"));
	if (MeshAsset.Succeeded()) StaticMesh->SetStaticMesh(MeshAsset.Object); 

	// Loading config from .ini file
	LoadConfig();
}


/**
 * Loading configuration from .ini file
 *
 * Sets initial values to variables.
 */
void AManager::LoadConfig()
{
	FString ConfigFilePath = FPaths::ProjectContentDir() + TEXT("SimConfig.ini");
	ConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(ConfigFilePath);

	GConfig->LoadFile(ConfigFilePath);
	
	GConfig->GetDouble(TEXT("sim/global"), TEXT("environment_X_length"), EnvSize.X, ConfigFilePath);
	GConfig->GetDouble(TEXT("sim/global"), TEXT("environment_Y_length"), EnvSize.Y, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/objective"), TEXT("min_distance_ratio"), ObjectiveMinDistanceRatio, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/manager"), TEXT("env_max_columns"), ColMax, ConfigFilePath);

	GConfig->GetInt(TEXT("sim/global"), TEXT("sim_speed"), SimulationSpeed, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/global"), TEXT("step"), TickInterval, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/manager"), TEXT("sim_group_size"), SimGroupSize, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/global"), TEXT("lines_thickness"), LinesThickness, ConfigFilePath);

	GConfig->GetInt(TEXT("sim/drones"), TEXT("strategy"), StrategyID, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("ground_offset"), DronesGroundOffset, ConfigFilePath);

	GConfig->GetFloat(TEXT("sim/drones"), TEXT("min_speed"), MinSpeed, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("max_speed"), MaxSpeed, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/manager"), TEXT("min_drones"), MinNumDrones, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/manager"), TEXT("max_drones"), MaxNumDrones, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("battery_capacity"), BatteryCapacity, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("battery_weight"), BatteryWeight, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/drones"), TEXT("min_battery_count"), MinBatteryCount, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/drones"), TEXT("max_battery_count"), MaxBatteryCount, ConfigFilePath);
	GConfig->GetFloat(TEXT("sim/drones"), TEXT("initial_weight"), InitialWeight, ConfigFilePath);
	
	GConfig->GetFloat(TEXT("sim/manager"), TEXT("speed_increment"), SpeedIncrement, ConfigFilePath);
	GConfig->GetInt(TEXT("sim/manager"), TEXT("drone_increment"), DroneIncrement, ConfigFilePath);
}


/**
 * First function to be executed after instantiation.
 */
void AManager::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickInterval(TickInterval);

	// Initial config for the first group of simulations
	GroupSpeed = MinSpeed;
	GroupNumDrones = MinNumDrones;
	GroupBatteryCount = MinBatteryCount;
	MaxTimePerSim = CalculateMaximumAutonomy();

	PrintSimConfigRecap();
	
	ManageNewSimulation();
}


void AManager::PrintSimConfigRecap() const
{
	UE_LOG(LogTemp, Warning, TEXT("Trying with %d drone%hs of %fkg at %fm/s with %d batter%hs of %fWh"),
		GroupNumDrones, GroupNumDrones > 1 ? "s" : "",
		InitialWeight + GroupBatteryCount * BatteryWeight,
		GroupSpeed,
		GroupBatteryCount, GroupBatteryCount > 1 ? "ies" : "y", BatteryCapacity);
}


/**
 * Update function.
 *
 * This function is called every TickInterval seconds, as set in BeginPlay().
 *
 * @param DeltaTime Time elapsed since last frame.
 */
void AManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CurrentSimulatedTime += DeltaTime * SimulationSpeed;

	if (CurrentSimulatedTime >= MaxTimePerSim) HandleSimulationEnd();
}


/**
 * Management function.
 *
 * The execution flow always comes back to this function unless the simulation is over.
 */
void AManager::ManageNewSimulation()
{
	if (AutoSuccess)
	{
		AutoSuccess = false;
		MutateSimulationParameters(true);
	}
	else if (AutoFail)
	{
		AutoFail = false;
		MutateSimulationParameters(false);
	}
	else
	{
		if (CurrentGroupSim++ == SimGroupSize) // End of current group
        	{
				// If >50% of simulations result in a success, the configuration is successful
				if (SimGroupSize == 1) MutateSimulationParameters(SuccessfulSim == 1);
        		else MutateSimulationParameters(SuccessfulSim >= SimGroupSize/2);
        		SuccessfulSim = 0;
        		CurrentGroupSim = 1;
        	}
	}
	UE_LOG(LogTemp,Warning,TEXT("Autonomy : %f, Group n°%d"), MaxTimePerSim, CurrentGroupSim);
	InitSimulation();
}


/**
 * Mutate configuration of the current group of simulations, based on the outcome it gave.
 * 
 * @param IsGroupSuccessful True if the current group configuration is successful, false otherwise.
 */
void AManager::MutateSimulationParameters(const bool IsGroupSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("%hs"), IsGroupSuccessful ? "Success" : "Fail");
	
	if (!IsCurveFound || IsGroupSuccessful)
	{
		// We overwrite the saved config as we don't need it right now
        PreviousSpeed = GroupSpeed;
        PreviousBatteryCount = GroupBatteryCount;
	}
	
	if (IsCurveFound)
	{
		if (IsGroupSuccessful && GroupSpeed - SpeedIncrement >= MinSpeed) GroupSpeed -= SpeedIncrement;
		else
		{
			SlowConfigs.Add({
				PreviousSpeed,
				(float)GroupNumDrones,
				(float)PreviousBatteryCount,
				InitialWeight + PreviousBatteryCount * BatteryWeight});
			GroupNumDrones += DroneIncrement;
			
			// We save the current config for later comparison
			PreviousSpeed = GroupSpeed;
			PreviousBatteryCount = GroupBatteryCount;
		}
	}
	else
	{
		if (IsGroupSuccessful)
		{
			SlowConfigs.Add({
				GroupSpeed,
				(float)GroupNumDrones,
				(float)GroupBatteryCount,
				InitialWeight + GroupBatteryCount * BatteryWeight});
			IsCurveFound = true;
			GroupNumDrones += DroneIncrement;
			GroupSpeed -= SpeedIncrement;
		}
		else if (GroupSpeed + SpeedIncrement <= MaxSpeed)
			GroupSpeed += SpeedIncrement;
		else
		{
			GroupNumDrones += DroneIncrement;
			GroupSpeed = MinSpeed;
		}
	}

	// Calculate the least amount of batteries required
	if (IsGroupSuccessful) AutoSetMinBatteryCountForGroup();

	SummedTimesToFind = 0;
	
	// Calculate the maximum autonomy of the battery in minutes
	
	MaxTimePerSim = CalculateMaximumAutonomy();
	
	UE_LOG(LogTemp, Warning, TEXT("----------------------------"));
	PrintSimConfigRecap();
}


float AManager::CalculateMaximumAutonomy() const
{
	return 60 * BatteryCapacity * MaxBatteryCount / (pow(GroupSpeed,2) * DRONEWEIGHT(MaxBatteryCount)/2.0);
}


void AManager::AutoSetMinBatteryCountForGroup()
{
	for (int i = MaxBatteryCount; i > 0; i--)
		if (SummedTimesToFind/SuccessfulSim/60.0 * pow(GroupSpeed,2) * DRONEWEIGHT(i)/2.0 <= BatteryCapacity * i)
		{
			GroupBatteryCount = i;
			return;
		}
	GroupBatteryCount = MaxBatteryCount;
	ThrowUnexpectedWarning(TEXT("Selected config requires more batteries than allowed !"));
}


void AManager::ThrowUnexpectedWarning(const wchar_t* Text)
{
	UE_LOG(LogTemp,Warning,TEXT("UNEXPECTED: %s"), Text);
}


/**
 * Performs the initialization for a new simulation.
 *
 * It spawns an Objective and Drones and draws visuals.
 */
void AManager::InitSimulation()
{
	// Objective
	ObjectiveSpawnPoint = FVector(
		FMath::RandRange(EnvSize.X*ObjectiveMinDistanceRatio,EnvSize.X),
		FMath::RandRange(0.,EnvSize.Y),
		GetActorLocation().Z);
	CurrentSimulatedObjective = GetWorld()->SpawnActor<AObjective>(AObjective::StaticClass(), ObjectiveSpawnPoint, GetActorRotation());

	// Drones
	switch (StrategyID)
	{
	case 1:
		SpawnDrones(ADroneRandom::StaticClass());
		break;
	case 2:
		SpawnDrones(ADroneSweep::StaticClass());
		break;
	case 3:
		SpawnDrones(ADroneSpiral::StaticClass());
		break;
	default: break;
	}

	// Research environment limits visuals
	FVector BoxCenter = FVector(EnvSize.X/2,EnvSize.Y/2,0);
	FVector BoxExtent = FVector(EnvSize.X/2,EnvSize.Y/2,200);
	DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, FQuat::Identity, FColor::Green, true, -1, 0, LinesThickness);

	SimID++;
}


/**
 * Spawns a certain number of a given class of Drones in the environment.
 * 
 * @param DroneStrategy The derived class of Drone to spawn.
 */
void AManager::SpawnDrones(const TSubclassOf<ADrone> DroneStrategy)
{
	TArray<std::vector<FVector2D>> Zones = AssignZones();
	for (int i = 0; i < GroupNumDrones; i++)
	{
		ADrone* d = GetWorld()->SpawnActor<ADrone>(DroneStrategy, FVector(0,200*(i+1),DronesGroundOffset), GetActorRotation());
		d->ID = i + 1; // A unique ID
		d->Manager = this; // A reference to the manager
		d->AssignedZone = Zones[i];
		CurrentSimulatedDrones.Add(d);
	}
}


TArray<std::vector<FVector2D>> AManager::AssignZones()
{
	int FilledLines = floor((float)GroupNumDrones/(float)ColMax);
	int TotalLines = ceil((float)GroupNumDrones/(float)ColMax);
	int ZonesLastLine = GroupNumDrones - FilledLines * ColMax;
	
	std::vector<std::vector<std::vector<FVector2D>>> Zones;
	
	// Filled lines
	for (int line = 0; line < FilledLines; line++)
	{
		std::vector<std::vector<FVector2D>> Line;
		for (int zone = 0; zone < ColMax; zone++)
		{
			std::vector<FVector2D> Zone;
        	
			// Top left point
			Zone.push_back(FVector2D(
				EnvSize.X - (EnvSize.X / TotalLines) * line,
				(EnvSize.Y / ColMax) * zone));

			// Bottom right point
			Zone.push_back(FVector2D(
				EnvSize.X - (EnvSize.X / TotalLines) * (line + 1),
				(EnvSize.Y / ColMax) * (zone + 1)));

			Line.push_back(Zone);
		}
		Zones.push_back(Line);
	}

	// Last line if excess zones
	if (ZonesLastLine != 0)
	{
		std::vector<std::vector<FVector2D>> Line;
		for (int zone = 0; zone < ZonesLastLine; zone++)
		{
			std::vector<FVector2D> Zone;
			
			// Top left point
			Zone.push_back(FVector2D(
				EnvSize.X - (EnvSize.X / TotalLines) * FilledLines,
				(EnvSize.Y / ZonesLastLine) * zone));

			// Bottom right point
			Zone.push_back(FVector2D(
				0,
				(EnvSize.Y / ZonesLastLine) * (zone + 1)));

			Line.push_back(Zone);
		}
		Zones.push_back(Line);
	}

	TArray<std::vector<FVector2D>> ZonesArray;

	for (const auto& line : Zones)
		for (const std::vector<FVector2D>& zone : line)
		{
			DrawDebugBoxFromDiagonalPoints(zone[0],zone[1],FColor::Red,LinesThickness);
			ZonesArray.Add(zone);
		}

	return ZonesArray;
}


/**
 * Event function for a Drone loss.
 */
bool AManager::DroneDestroyedEvent()
{
	const int RdID = FMath::RandRange(1,GroupNumDrones);
	for (ADrone* d : TArray(CurrentSimulatedDrones))
		if (d->ID == RdID)
		{
			d->Destroy();
			CurrentSimulatedDrones.Remove(d);
			UE_LOG(LogTemp, Warning, TEXT("Lost communication with drone %d"), RdID);
			return true;
		}
		
	return false;
}


/**
 * Thrown by any Drone whenever it finds the Objective.
 */
void AManager::ObjectiveFound()
{
	if (ReportedSimID == SimID) return;
	ReportedSimID = SimID;
	SuccessfulSim++;
	SummedTimesToFind += CurrentSimulatedTime;
	HandleSimulationEnd();
}


/**
 * Reset the environment to its initial state.
 *
 * This function destroys every Drones and Objective currently playing.
 *
 * If the stopping condition is met, results are produced and the simulation comes to an end.
 */
void AManager::HandleSimulationEnd()
{
	// Destroy all drones
	TArray<ADrone*> Copy = CurrentSimulatedDrones;
	for (ADrone* d : Copy) if (d) d->Destroy();
	CurrentSimulatedDrones.Empty();

	// Destroy objective
	CurrentSimulatedObjective->Destroy();

	// Clear debug shapes
	FlushPersistentDebugLines(GetWorld());

	// Reset time
	CurrentSimulatedTime = 0;

	// Set up new simulations or print results
	if (SimulationHasEnded) return;
	if (GroupNumDrones < MaxNumDrones) ManageNewSimulation();
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("----------------------------"));
		UE_LOG(LogTemp, Warning, TEXT("%d drones reached : End of simulations"), GroupNumDrones);
		for (const auto& sc : SlowConfigs)
			UE_LOG(LogTemp, Warning, TEXT("vitesse:%f,drones:%d,batteries:%d,(weight:%f)"), sc[0], (int)sc[1], (int)sc[2], sc[3]);
		SimulationHasEnded = true;
		WriteResultsToFile();
	}
}


/**
 * Allows Drones to set their speed properly.
 * 
 * @returns The current speed the Drones are set to.
 */
float AManager::GetGroupDroneSpeed()
{
	return GroupSpeed;
}


/**
 * Appends successful configs to a result file.
 */
void AManager::WriteResultsToFile()
{
	FString ResultsFile = FPaths::ProjectConfigDir();
	ResultsFile.Append(TEXT("../results.txt"));
	
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> ConfigsToWrite;
	for (const auto& sc : SlowConfigs) {
		FString FormattedString = FString::Printf(
		TEXT("vitesse:%f,drones:%d,batteries:%d,(weight:%f)"),
			sc[0], (int)sc[1], (int)sc[2], sc[3]);
		ConfigsToWrite.Add(FormattedString);
	}
	
	if (FileManager.FileExists(*ResultsFile))
	{
		if (FFileHelper::SaveStringArrayToFile(ConfigsToWrite,*ResultsFile))
		{
			UE_LOG(LogTemp, Warning, TEXT("Successfully written \"%d\" strings to the text file"),ConfigsToWrite.Num());
		}
		else UE_LOG(LogTemp, Warning, TEXT("Failed to write to file."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("File not found."));
		UE_LOG(LogTemp, Warning, TEXT("Expected location: %s"),*ResultsFile);
	}
}

void AManager::DrawDebugBoxFromDiagonalPoints(const FVector2D& TopLeft, const FVector2D& BottomRight, const FColor& Color, const int& Thickness)
{
	FVector v1 = FVector(TopLeft.X,TopLeft.Y,10);
	FVector v2 = FVector(BottomRight.X,BottomRight.Y,10);
	FVector Center = (v1 + v2) / 2.0f;
	FVector Extent = (v1 - v2) / 2.0f;
	DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, true, -1, 0, Thickness);
}

