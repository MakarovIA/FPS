// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTService_CollectData.h"

UBTService_CollectData::UBTService_CollectData()
{
	NodeName = "Collect Data";

	bNotifyBecomeRelevant = false;
	bNotifyCeaseRelevant = false;
	bCallTickOnSearchStart = true;

	Interval = 0.5f;
	RandomDeviation = 0.f;

	BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CollectData, BotData), UBotBlackBoardData::StaticClass());
}

void UBTService_CollectData::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	UBotBlackBoardData* Data = static_cast<UBotBlackBoardData*>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID()));

	const ABot* Pawn = Cast<ABot>(OwnerComp.GetAIOwner()->GetPawn());
	AAIController* Controller = OwnerComp.GetAIOwner();

	const AVoronoiNavData* VoronoiNavData = AVoronoiNavData::GetVoronoiNavData(GetWorld());
	const FVoronoiGraph* VoronoiGraph = VoronoiNavData->GetVoronoiGraph();

	if (!Pawn || !Controller)
		return;

	if (!Data)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID(), Data = NewObject<UBotBlackBoardData>(Controller, TEXT("Bot Data"), RF_Transient));
		Data->Target = Pawn->GetActorLocation();
		Data->StartHiding = false;
		Data->FollowTarget = nullptr;
		Data->StartFollowing = false;

		int32 Count = 0;
		for (const auto& i : VoronoiGraph->Surfaces)
			Count += i->Faces.Num();

		Data->MapData.Reserve(Count);
		for (const auto& i : VoronoiGraph->Surfaces)
			for (const auto& Face : i->Faces)
				Data->MapData.Add(Face);
	}

	Data->VisibleEnemies.Reset();
	Data->VisibleAllies.Reset();

	for (TActorIterator<ABot> It(GetWorld()); It; ++It)
	{
		if (*It != Pawn && Controller->LineOfSightTo(*It))
		{
			if (const AAIPlayerState* State = Cast<AAIPlayerState>(It->PlayerState))
			{
				if (FGenericTeamId::GetAttitude(State->GetGenericTeamId(), Controller->GetGenericTeamId()) == ETeamAttitude::Hostile)
					Data->VisibleEnemies.Add(*It);
				else
					Data->VisibleAllies.Add(*It);
			}
		}
	}

	// TODO: Voronoi online properties

	for (auto &i : Data->MapData)
		i.Value.EnemyOverwatch = -1;

	for (auto &i : VoronoiNavData->GetReachableFacesInRadius(Pawn->GetActorLocation(), Data->OverwatchRange))
	{
		Data->MapData[i].EnemyOverwatch = 0;
		if (!SimpleCollisionCheck(Pawn->GetActorLocation(), i->Location, GetWorld()))
			++Data->MapData[i].EnemyOverwatch;
	}
}

void UBTService_CollectData::InitializeFromAsset(UBehaviorTree &Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
		BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTService_CollectData::GetStaticDescription() const
{
	return FString("Find enemies & update Voronoi properties");
}