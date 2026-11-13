#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwingBar.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

// A pipe across the set, overhead. A fighter who jumps under it hangs from it, and can kick off it.
// It hangs from above so that nothing stands between it and the camera.
UCLASS()
class ASwingBar : public AActor
{
	GENERATED_BODY()

public:
	ASwingBar();

	// Call once. The actor's place is the middle of the pipe; it runs this far to either side, across the set.
	// With a post height it stands on one post at its far end, that tall, instead of hanging from above.
	void SetUp(double HalfLength, double PostHeight);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Pipe;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Hangers;

	// A set lit from straight above shows the camera only the dark underside of the pipe. This shows it anywhere.
	UPROPERTY()
	TObjectPtr<UPointLightComponent> Glow;
};
