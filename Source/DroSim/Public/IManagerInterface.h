#pragma once

class IManagerInterface
{
public:
	virtual void ObjectiveFound() = 0;
	virtual float GetGroupDroneSpeed() = 0;
};
