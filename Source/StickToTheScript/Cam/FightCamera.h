#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Cam/CameraJolt.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "FightCamera.generated.h"

class AStickFighter;
class UCameraComponent;

// The outside view of a fight. It shakes and pushes in when a strike lands. Told to follow a fighter, it also travels
// sideways to keep that fighter in the middle of the picture. While that fighter has a gun in its hands, the camera
// makes room for whoever the gun points at: it moves to the middle of the two and pulls back until both are seen.
UCLASS()
class AFightCamera : public AActor
{
	GENERATED_BODY()

public:
	AFightCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Fight camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Fight camera")
	float BaseFieldOfView = 80.f;

	void Follow(AStickFighter* Fighter);

	// A film lens: sharp at this point, soft in front of it and behind. The fight itself is filmed sharp all the way through.
	void FocusOn(const FVector& Point);

private:
	UFUNCTION()
	void HandleHit(AActor* Attacker, AActor* Victim, EMove Move);

	UFUNCTION()
	void HandleBlocked(AActor* Attacker, AActor* Defender);

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	UPROPERTY()
	TObjectPtr<AStickFighter> Subject;

	// Where the fighter it follows points a gun: the nearest opponent on its feet roughly ahead. False with no gun in
	// the hands or nobody there. Worked out from joints alone, so it is the same in the film, where fighters are puppets.
	bool FindAimedAt(FVector& OutPoint) const;

	FVector BaseLocation = FVector::ZeroVector;
	float Along = 0.f;
	float PullBack = 0.f;
	FVector AimedAt = FVector::ZeroVector;
	// The picture stays on whoever was aimed at for a moment after, so a shot is seen to arrive.
	float AimedUntilSeconds = 0.f;
	bool bSnapToSubject = true;

	FCameraJolt Jolt;
};
