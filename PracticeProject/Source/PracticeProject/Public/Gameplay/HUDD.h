#pragma once

#include "GameFramework/HUD.h"
#include "HUDD.generated.h"

/**
*
*/
UCLASS()
class PRACTICEPROJECT_API AHUDD : public AHUD
{
	GENERATED_BODY()

protected:
	// This will be drawn at the center of the screen.
	UPROPERTY(EditDefaultsOnly)
	UTexture2D* CrosshairTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFont* Font;

public:
	AHUDD();
	// Primary draw call for the HUD.
	virtual void DrawHUD() override;
	void DrawHealthbar();
	void DrawWeaponName();

};
