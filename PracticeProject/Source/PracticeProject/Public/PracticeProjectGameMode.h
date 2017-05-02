// By Polyakov Pavel

#pragma once

#include "GameFramework/GameMode.h"
#include "PracticeProjectGameMode.generated.h"

UCLASS()
class PRACTICEPROJECT_API APracticeProjectGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    APracticeProjectGameMode();

    virtual void InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage) override;
};
