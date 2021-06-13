#pragma once

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

#include "VoronoiQuerier.h"
#include "FALCON/FalconComponent.h"
#include "BotAIController.generated.h"

/** Bot AI Controller */
UCLASS(HideCategories = ("Actor Tick", Replication, Input, Actor, Controller, Transform))
class PRACTICEPROJECT_API ABotAIController : public AAIController, public IVoronoiQuerier
{
    GENERATED_BODY()

    /** Values to use when making queries to VoronoiNavData */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Voronoi, meta = (AllowPrivateAccess = "true"))
    FVoronoiQuerierParameters VoronoiQuerierParams;

	UPROPERTY(EditDefaultsOnly, Category = "FALCON")
	UFalconComponent *FalconComponent;

    UPROPERTY()
    UBlackboardComponent* BlackboardComponent;

    UPROPERTY()
    UBehaviorTreeComponent* BehaviorTreeComponent;

    /** Behaviour tree to run */
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

public:
    ABotAIController(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "FALCON")
	UFalconComponent* getFalconComponent();

	UFUNCTION(BlueprintCallable, Category = kek)
		void setFocusActor(AActor* actor);

    /** Overrided to run behavior tree */
    virtual void Possess(APawn* InPawn) override;

    /** Overrided to stop behavior tree */
    virtual void UnPossess() override;

    /** Overrided to allow pitch rotation and smooth quick rotations */
    virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

    /** Check line of sight with respect to angle of view */
    virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;

    virtual void InstigatedAnyDamage(float Damage, const UDamageType *DamageType, AActor *DamagedActor, AActor *DamageCauser) override;

    /** Assigns Team Agent to given TeamID */
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;

    /** Retrieve team identifier in form of FGenericTeamId */
    virtual FGenericTeamId GetGenericTeamId() const override;

    // -------------------------------------------------------------------------------------------------------------------------------------
    // IVoronoiQuerier interface begin
    // -------------------------------------------------------------------------------------------------------------------------------------

    virtual const FVoronoiQuerierParameters& GetVoronoiQuerierParameters() const override;
    virtual float GetAdditionalPenalty(const FVoronoiFace *From, const FVoronoiFace *To) const override;
    virtual FVector2D GetInitialRotation() const override;
    virtual bool IsVoronoiQuerier() const override;
};
