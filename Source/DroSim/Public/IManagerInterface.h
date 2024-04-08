#pragma once

class IManagerInterface
{
public:
	virtual void ObjectiveFound() = 0;
	virtual float GetGroupDroneSpeed() = 0;
	virtual bool IsObjectiveNear(const FVector& DronePos) = 0;
	virtual float GetVisionRadius() = 0;
};
