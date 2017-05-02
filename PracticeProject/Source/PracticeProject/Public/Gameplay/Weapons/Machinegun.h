// By Polyakov Pavel

#pragma once

#include "Gameplay/Weapons/Weapon.h"
#include "Machinegun.generated.h"

UCLASS()
class PRACTICEPROJECT_API AMachinegun : public AWeapon
{
	GENERATED_BODY()
	
protected:
    virtual void OnFire();
};
