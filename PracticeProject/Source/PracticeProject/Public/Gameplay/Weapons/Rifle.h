#pragma once

#include "Gameplay/Weapons/Weapon.h"
#include "Rifle.generated.h"

UCLASS()
class PRACTICEPROJECT_API ARifle : public AWeapon
{
	GENERATED_BODY()
	
protected:
    virtual void OnFire();
};
