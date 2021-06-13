#pragma once

#include "Components/ActorComponent.h"
#include "NEWFALCON.h"
#include "Weapon.h"
#include "FalconComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PRACTICEPROJECT_API UFalconComponent : public UActorComponent
{
	GENERATED_BODY()

	const static size_t numStates = 2;
	const static size_t numWeapons = 4;

	NEWFALCON *myFALCON;

	float reward;

public:	
	// Sets default values for this component's properties
	UFalconComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;


	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = "Reward Settings")
	float MaxReward;

	void IncreaseReward(float r);

	UFUNCTION(BlueprintCallable, Category = "FALCON")
	EWeaponType PerformFalcon(TArray<float> states);

	UFUNCTION(BlueprintCallable, Category = "FALCON")
	void LearnFalcon();

	UFUNCTION(BlueprintCallable, Category = "FALCON")
	void SaveFalcon();
	
	UFUNCTION(BlueprintCallable, Category = "FALCON")
	void LoadFalcon();
	
	UFUNCTION(BlueprintCallable, Category = "FALCON")
	void PrintFalcon();

	UFUNCTION(BlueprintCallable, Category = "FALCON")
	void PrintStat();
};