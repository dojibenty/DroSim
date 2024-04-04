#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Drone.h"
#include "IManagerInterface.h"
#include "Objective.h"
#include "Manager.generated.h"

UCLASS()
class DROSIM_API AManager : public AActor, public IManagerInterface
{
	GENERATED_BODY()
	
public:
	AManager();

protected:
	virtual void BeginPlay() override;
	void LoadConfig();
	void InitSimulation();
	bool DroneDestroyedEvent();
	void HandleSimulationEnd();
	void MutateSimulationParameters(const bool IsSimSuccessful);
	void ManageNewSimulation();
	void SpawnDrones(const TSubclassOf<ADrone> DroneStrategy);
	void WriteResultsToFile();
	void DrawDebugBoxFromDiagonalPoints(const FVector2D& TopLeft, const FVector2D& BottomRight, const FColor& Color, const int& Thickness);
	TArray<std::vector<FVector2D>> AssignZones();
	void PrintSimConfigRecap() const;
	
public:
	virtual void Tick(float DeltaTime) override;
	virtual void ObjectiveFound() override;
	virtual float GetGroupDroneSpeed() override;

private:
	FVector2D EnvSize;
	FVector ObjectiveSpawnPoint;
	float ObjectiveMinDistanceRatio;
	int ColMax;

	int StrategyID;
	float DronesGroundOffset;

	float MinSpeed;
	float MaxSpeed;
	int MinNumDrones;
	int MaxNumDrones;
	float GroupSpeed;
	int GroupNumDrones;
	float BatteryCapacity;
	float BatteryWeight;
	int MinBatteryCount;
	int GroupBatteryCount;
	float InitialWeight;

	float SpeedIncrement;
	int DroneIncrement;
	
	float TickInterval;
	float CurrentSimulatedTime;
	int SimulationSpeed;
	int LinesThickness;
	
	int SimID = 0;
	int ReportedSimID = 0;
	int SimGroupSize;
	int CurrentGroupSim = 0;
	int SuccessfulSim = 0;
	bool IsCurveFound = false;
	bool AutoSuccess = false;
	bool AutoFail = false;

	float PreviousSpeed = -1;
	int PreviousNumDrones = -1;
	int PreviousBatteryCount = -1;
	bool WasPreviousConfigSuccessful = false;

	bool SimulationHasEnded = false;

	TArray<std::vector<float>> SuccessfulConfigs;

	AObjective* CurrentSimulatedObjective;
	TArray<ADrone*> CurrentSimulatedDrones;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMesh;
};
