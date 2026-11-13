#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Cam/CameraJolt.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "BodyCam.generated.h"

class AStickFighter;
class UCameraComponent;

// A camera worn at a fighter's eye level: it turns a little with the torso, lags, and jolts when a strike lands.
UCLASS()
class ABodyCam : public AActor
{
	GENERATED_BODY()

public:
	ABodyCam();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Wear(AStickFighter* Fighter);

	UPROPERTY(VisibleAnywhere, Category = "Body cam")
	TObjectPtr<UCameraComponent> Camera;

private:
	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UPROPERTY()
	TObjectPtr<AStickFighter> Wearer;

	bool bJustPutOn = true;
	FCameraJolt Jolt;
};
