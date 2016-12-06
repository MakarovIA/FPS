// By Polyakov Pavel

#pragma once

#include "Gameplay/Weapons/Weapon.h"
#include "Shotgun.generated.h"

UCLASS()
class PRACTICEPROJECT_API AShotgun : public AWeapon
{
	GENERATED_BODY()
	
protected:
    virtual void OnFire();	
};
