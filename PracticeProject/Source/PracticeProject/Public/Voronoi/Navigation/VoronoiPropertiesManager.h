// By Polyakov Pavel

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoronoiPropertiesManager.generated.h"

class AVoronoiNavData;

/** Voronoi properties manager */
UCLASS()
class PRACTICEPROJECT_API UVoronoiPropertiesManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 
     * Retrieves AVoronoiNavData from Navigation System
     * @return VoronoiNavData or NULL if it is not present
     */
    UFUNCTION(BlueprintPure, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static AVoronoiNavData* GetVoronoiNavData(const UObject* WorldContextObject);

    /** 
     * Get Voronoi properties for a given location
     * @return True on success or false if a given location is outside navigable area or there is no VoronoiNavData
     */
    UFUNCTION(BlueprintPure, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static bool GetVoronoiPropertiesForLocation(const UObject* WorldContextObject, const FVector& Location, float &BaseCost, float &BaseEnterCost, bool &bNoWay);

    /** 
     * Set Voronoi properties for a given location
     * @return True on success or false if a given location is outside navigable area or there is no VoronoiNavData
     */
    UFUNCTION(BlueprintCallable, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static bool SetVoronoiPropertiesForLocation(const UObject* WorldContextObject, const FVector& Location, float BaseCost, float BaseEnterCost, bool bNoWay);

    /** 
     * Set Voronoi properties for a given area
     * @return True on success or false if a given location is outside navigable area or there is no VoronoiNavData
     */
    UFUNCTION(BlueprintCallable, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static bool SetVoronoiPropertiesForArea(const UObject* WorldContextObject, const FVector& Origin, float Radius, float BaseCost, float BaseEnterCost, bool bNoWay);

    /**
     * @return False a given location is outside navigable area or there is no VoronoiNavData, True otherwise
     */
    UFUNCTION(BlueprintPure, Category = VoronoiPropertiesManager, meta = (WorldContext = "WorldContextObject"))
    static bool IsLocationNavigable(const UObject* WorldContextObject, const FVector& Location);
};
