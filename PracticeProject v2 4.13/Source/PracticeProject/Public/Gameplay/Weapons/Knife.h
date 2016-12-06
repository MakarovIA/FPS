// By Polyakov Pavel

#pragma once

#include "Gameplay/Weapons/Weapon.h"
#include "Knife.generated.h"

UCLASS()
class PRACTICEPROJECT_API AKnife : public AWeapon
{
	GENERATED_BODY()
	
protected:
    virtual void OnFire();
};
