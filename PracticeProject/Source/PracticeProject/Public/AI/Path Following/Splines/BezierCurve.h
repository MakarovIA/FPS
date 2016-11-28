// By Polyakov Pavel

#pragma once

#include "Spline.h"
#include "BezierCurve.generated.h"

/** Class representing a bezier curve to move along */
class FBezierCurve final : public FSpline
{
    TArray<TArray<FVector>> ControlPoints;

    /** Calculate combination */
    static int32 Combination(int32 n, int32 k);

public:
    FBezierCurve(const TArray<TArray<FVector>> &InControlPoints) : ControlPoints(InControlPoints) {}

    /** Calculate value of a Bezier curve in a given piece with a given parameter */
    virtual FVector GetValue(int32 InPiece, double InParameter) const override;

    /** Get number of pieces in a bezier curve */
    virtual int32 GetPieceNumber() const override;
};

/** Bezier curve building parameters */
USTRUCT()
struct FBezierCurveBuildParams
{
    GENERATED_USTRUCT_BODY()

    /** Minimal distance between control points in a single part of a Bezier curve */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig, meta = (ClampMin = "0.0"))
    float MinDistanceBetweenControlPoints;

    /** Additional number of points that should be added on both sides of existing ones before calculating bezier curve parts */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig, meta = (ClampMin = "0", ClampMax = "10"))
    int32 AdditionalPathPointsNumber;

    /** This the offset used to place aditional path points around existing ones */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig, meta = (ClampMin = "0.0"))
    float AdditionalPathPointsOffset;

    /** For performance reasons there should be some limit on bezier curve degree */
    UPROPERTY(EditDefaultsOnly, Category = BotPathFollowingConfig, meta = (ClampMin = "0", ClampMax = "20"))
    int32 CurveDergeeLimit;

    /** Whatever is specified in blueprint, Bezier curve degree can't be greater than this value */
    static const int32 CurveDergeeTrueLimit = 20;

    FBezierCurveBuildParams() : MinDistanceBetweenControlPoints(100), AdditionalPathPointsNumber(2), AdditionalPathPointsOffset(75), CurveDergeeLimit(15) {};
};

/** Class responsible for building a bezier curve */
class FBezierCurveBuilder
{
    struct ETuple
    {
        float Value;
        int32 End, Parts;
    };

    /** Convert path points from FNavPathPoint to FVector. Also paste some additional points */
    static void RetrivePathPoints(const TArray<FNavPathPoint>& NavPathPoints, const FBezierCurveBuildParams &BuildParams,
        const FNavAgentProperties& AgentProperties, const UWorld* World, TArray<FVector> &PathPoints);

    /** Split PathPoints into segments where Bezier curves can be built */
    static void CalculateSubsequencesArray(const TArray<FVector>& PathPoints, const FNavAgentProperties& AgentProperties, const UWorld* World, TArray<int32> &SubsequencesArray);

    /** Recursive part of previous method */
    static void Backtracking(const TArray<TArray<ETuple>> &E, int32 i, int32 j, TArray<int32> &SubsequencesArray);

public:
    /** Builds a bezier curve based on given path points and current move direction */
    static TUniquePtr<FBezierCurve> BuildBezierCurve(const TArray<FNavPathPoint>& NavPathPoints, const FVector& CurrentMoveDirection,
        const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World);
};
