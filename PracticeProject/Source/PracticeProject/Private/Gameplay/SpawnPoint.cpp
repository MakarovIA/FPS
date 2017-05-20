// By Polyakov Pavel

#include "PracticeProject.h"
#include "SpawnPoint.h"
#include "Runtime/AIModule/Classes/Blueprint/AIBlueprintHelperLibrary.h"
#include "Bot.h"

void ASpawnPoint::spawnBot(UBehaviorTree * behaviorTree)
{
	APawn * bot = UAIBlueprintHelperLibrary::SpawnAIFromClass(GetWorld(), PawnClass, behaviorTree, this->GetActorLocation(), FRotator(0, 0, 0), false);
	bot->SpawnDefaultController();

	lastSpawnedBot = bot;

	ABot * botWithStatistic = dynamic_cast<ABot*>(bot);
	if (botWithStatistic)
	{
		botWithStatistic->setStatKey(statKey);
		botWithStatistic->setBotSpawn(this);
	}
}


// Sets default values
ASpawnPoint::ASpawnPoint()
{
	lastSpawnedBot = nullptr;
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

