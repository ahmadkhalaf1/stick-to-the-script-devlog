#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StickGameMode.generated.h"

UCLASS()
class AStickGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStickGameMode();

	virtual void StartPlay() override;
};
