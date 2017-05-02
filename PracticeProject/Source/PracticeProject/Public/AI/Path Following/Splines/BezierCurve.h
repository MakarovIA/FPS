// By Polyakov Pavel

#pragma once

#include "Spline.h"
#include "BezierCurve.generated.h"

/** A Bezier curve to move along */
class PRACTICEPROJECT_API FBezierCurve : public FSpline
{
    TArray<TArray<FVector>> ControlPoints;

public:
    FORCEINLINE FBezierCurve(TArray<TArray<FVector>> InControlPoints)
        : ControlPoints(MoveTemp(InControlPoints)) {}

    virtual FVector GetValue(int32 InPiece, double InParameter) const override;
    virtual int32 GetPieceNumber() const override;

    FORCEINLINE const TArray<TArray<FVector>>& GetControlPoints() const { return ControlPoints; }
};

/** Bezier curve building parameters */
USTRUCT()
struct PRACTICEPROJECT_API FBezierCurveBuildParams
{
    GENERATED_USTRUCT_BODY()

    /** Minimal distance between control points in a single part of a Bezier curve */
    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
    float MinDistanceBetweenControlPoints;

    /** Additional number of points that should be added on both sides of existing ones before calculating bezier curve parts */
    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0", ClampMax = "5"))
    int32 AdditionalPathPointsNumber;

    /** This the offset used to place aditional path points around existing ones */
    UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", ClampMax = "1000.0"))
    float AdditionalPathPointsOffset;

    FORCEINLINE FBezierCurveBuildParams()
        : MinDistanceBetweenControlPoints(200), AdditionalPathPointsNumber(2), AdditionalPathPointsOffset(75) {};
};

/** Class responsible for building a bezier curve */
class FBezierCurveBuilder
{
    static TArray<FVector> RetrivePathPoints(const TArray<FNavPathPoint>& NavPathPoints, const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World);
    static TArray<int32> CalculateSubsequencesArray(const TArray<FVector>& PathPoints, const FNavAgentProperties& AgentProperties, const UWorld* World);

public:
    static TUniquePtr<FBezierCurve> BuildBezierCurve(const TArray<FNavPathPoint>& NavPathPoints, const FVector& CurrentMoveDirection, const FBezierCurveBuildParams &BuildParams, const FNavAgentProperties& AgentProperties, const UWorld* World);
};
