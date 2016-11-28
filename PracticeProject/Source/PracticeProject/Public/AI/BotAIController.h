// By Polyakov Pavel

#pragma once

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

#include "VoronoiQuerier.h"
#include "BotAIController.generated.h"

/** Voronoi querier parameters */
USTRUCT()
struct PRACTICEPROJECT_API FVoronoiQuerierParams
{
    GENERATED_USTRUCT_BODY()

    /** Penalty value for rotation used in pathfinding */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (ClampMin = "0.0"))
    float PenaltyForRotation;

    /** Penalty mutiplier for crouching on the way */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (ClampMin = "1.0"))
    float PenaltyMultiplierForCrouch;

    /** Penalty value for jump */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voronoi, meta = (ClampMin = "0.0"))
    float PenaltyForJump;

    /** Whether to use these parameters */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi)
    bool bIsVoronoiQuerier;

    FVoronoiQuerierParams() : PenaltyForRotation(200), PenaltyMultiplierForCrouch(1), PenaltyForJump(0), bIsVoronoiQuerier(true) {}
};

/** Bot AI Controller */
UCLASS(HideCategories = ("Actor Tick", Replication, Input, Actor, Controller, Transform))
class PRACTICEPROJECT_API ABotAIController : public AAIController, public IVoronoiQuerier
{
    GENERATED_BODY()

    /** Values to use when making queries to VoronoiNavData */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVoronoiQuerierParams VoronoiQuerierParams;

    UPROPERTY()
    UBlackboardComponent* BlackboardComponent;

    UPROPERTY()
    UBehaviorTreeComponent* BehaviorTreeComponent;

    /** Behaviour tree to run */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

public:
    ABotAIController(const FObjectInitializer& ObjectInitializer);

    /** Overrided to run behavior tree */
    virtual void Possess(APawn* InPawn) override;

    /** Overrided to stop behavior tree */
    virtual void UnPossess() override;

    /** Overrided to allow pitch rotation and smooth quick rotations */
    virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

    /** Check line of sight with respect to angle of view */
    virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;

    /** Assigns Team Agent to given TeamID */
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;

    /** Retrieve team identifier in form of FGenericTeamId */
    virtual FGenericTeamId GetGenericTeamId() const override;

    // -------------------------------------------------------------------------------------------------------------------------------------
    // IVoronoiQuerier interface begin
    // -------------------------------------------------------------------------------------------------------------------------------------

    virtual float GetPenaltyForRotation() const override;
    virtual float GetPenaltyMultiplierForCrouch() const override;
    virtual float GetPenaltyForJump() const override;
    virtual float GetAdditionalPenalty(FVoronoiFace *From, FVoronoiFace *To) const override;
    virtual FVector2D GetInitialRotation() const override;
    virtual bool IsVoronoiQuerier() const override;

    // -------------------------------------------------------------------------------------------------------------------------------------
    // IVoronoiQuerier interface end
    // -------------------------------------------------------------------------------------------------------------------------------------

    /** Returns Voronoi Querier Params */
    FORCEINLINE const FVoronoiQuerierParams& GetVoronoiQuerierParams() const { return VoronoiQuerierParams; }
};
