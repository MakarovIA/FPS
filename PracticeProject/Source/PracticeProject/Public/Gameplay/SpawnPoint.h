// By Polyakov Pavel

#pragma once

#include "VoronoiGraph.h"
#include "GameFramework/Actor.h"
#include "SpawnPoint.generated.h"

UCLASS()
class PRACTICEPROJECT_API ASpawnPoint : public AActor
{
	GENERATED_BODY()
	
	APawn* lastSpawnedBot;
public:	
	// Sets default values for this actor's properties
	ASpawnPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Bot")
		void OnBotDeath();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Bot Pawn"), Category = Bot)
		TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bot)
		EStatisticKey statKey;

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Spawn Bot", CompactNodeTitle = "Spawn Bot", Keywords = "Bot"), Category = Bot)
		void spawnBot(UBehaviorTree * behaviorTree);


	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get last spawned bot", CompactNodeTitle = "getLastSpawnedBot", Keywords = "Bot"), Category = Bot)
		APawn* getLastSpawnedBot() { return lastSpawnedBot; }
	
};
