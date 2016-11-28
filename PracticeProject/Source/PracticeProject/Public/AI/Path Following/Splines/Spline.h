// By Polyakov Pavel

#pragma once

/** 
 * A numeric function that is piecewise-defined by polynomial functions, and which possesses a high degree of smoothness at the places where the polynomial pieces connect.
 * Defines an interface so bots can move along this spline
 */
class PRACTICEPROJECT_API FSpline
{
protected:
    double CurrentParameter;
    int32 CurrentPiece;

public:
    FSpline() : CurrentParameter(0), CurrentPiece(0) {}

    /** Calculate value of a spline in a given piece with a given parameter */
    virtual FVector GetValue(int32 InPiece, double InParameter) const { check(0); return FVector(); }

    /** Get number of pieces in a peacewise spline */
    virtual int32 GetPieceNumber() const { check(0);  return 0; }

    /** Get the next target to move to based on current location and possible distance */
    FVector GetNextTargetLocation(const FVector& CurrentLocation, double PossibleDistance);
};
