#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "FinalCut.generated.h"

class AFightRecorder;

// A stretch of the take that the film keeps.
struct FFilmClip
{
	float StartSeconds = 0.f;
	float EndSeconds = 0.f;
};

// How the player asked for the film to be cut.
enum class EFilmCut : uint8
{
	// The best of the fight, cut between three cameras.
	Action,
	// The same, with every knockout slowed and not only the last.
	SlowMotion,
	// The whole fight from one camera with nothing left out, run a little fast.
	LongTake
};

// The Director's Cut: a clean take replayed as an edited scene. The same fighters are posed from the recording,
// the picture cuts between three cameras, and every strike lands again with its blood, shake and sound.
UCLASS()
class AFinalCut : public AActor
{
	GENERATED_BODY()

public:
	AFinalCut();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Which stretches of a take the film keeps: its first blow, every knockout and thrown prop with room around them,
	// then kicks and stopped blows until the film is long enough. In order, and never overlapping.
	// A long take keeps one stretch: from the opening to the end.
	static TArray<FFilmClip> PlanClips(const TArray<FRecordedEvent>& Events, float TakeSeconds, EFilmCut Cut = EFilmCut::Action);

	static const TCHAR* CutName(EFilmCut Cut);
	EFilmCut GetCut() const { return Cut; }

	// Plays the film of the take that just wrapped. With no such take it says at once that the film is over.
	void Start(EFilmCut InCut);

	// Ends the film now.
	void Skip();

	// The film opens on a title card and ends on a slate over its last frame. The HUD draws both.
	bool IsOnTitleCard() const { return Phase == EPhase::Titles; }
	bool IsOnEndSlate() const { return Phase == EPhase::HoldingLastFrame && !bOuttake; }
	// How long the film itself ran, known once it is over.
	float GetRunningSeconds() const { return RunningSeconds; }

	UPROPERTY()
	TObjectPtr<AFightRecorder> Recorder;

private:
	enum class EPhase : uint8
	{
		Idle,
		Ready,
		Titles,
		Playing,
		HoldingLastFrame
	};
	static constexpr float TitleCardSeconds = 2.2f;

	enum ECamera
	{
		WideCamera,
		CloseCamera,
		LowCamera,
		CameraCount
	};

	struct FShot
	{
		float StartSeconds = 0.f;
		int32 Camera = WideCamera;
		FVector LookAt = FVector::ZeroVector;
		// Who a close shot is of, by their place in the recording: the shot follows them.
		int32 Attacker = INDEX_NONE;
		int32 Victim = INDEX_NONE;
	};

	UFUNCTION()
	void HandleTakeFinished(const FTake& FinishedTake);

	void StartReplay();
	// After the slate, the takes that went wrong, one after another. False when there is none left to show.
	bool StartOuttake();
	void FinishReplay();
	void FindOrPlaceCameras();
	void PlanShots();
	void ShowMoment(float Seconds);
	void ReplayEvent(const FRecordedEvent& Event);
	void CutTo(const FShot& Shot);
	FVector FindSubject(const FShot& Shot) const;
	enum class EInTheWay : uint8 { Nothing, AProp, AFighter };
	EInTheWay FindInTheWay(const FVector& From, const FVector& To, const FShot& Shot) const;
	float PlaybackRate(float Seconds) const;

	struct FBigMoment
	{
		float Seconds = 0.f;
		bool bKnockout = false;
	};

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Cameras;

	FTake Take;
	TArray<FTake> Outtakes;
	int32 NextOuttake = 0;
	bool bOuttake = false;
	EFilmCut Cut = EFilmCut::Action;
	TArray<FShot> Shots;
	EPhase Phase = EPhase::Idle;
	float PhaseSecondsLeft = 0.f;
	double PlayingSinceRealSeconds = 0.;
	float RunningSeconds = 0.f;
	float ReplaySeconds = 0.f;
	int32 FrameCursor = 0;
	int32 NextEvent = 0;
	int32 CurrentShot = INDEX_NONE;

	TArray<FFilmClip> Clips;
	int32 ClipIndex = 0;
	TArray<FBigMoment> BigMoments;
	int32 ShotCamera = WideCamera;
	FVector ShotLookAt = FVector::ZeroVector;
};
