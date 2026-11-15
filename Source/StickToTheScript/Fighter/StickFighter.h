#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "StickFighter.generated.h"

class ASwingBar;
class AThrownProp;
class UAnimSequenceBase;
class UStaticMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

// The index of each joint in AStickFighter::GetJointPositions.
namespace StickJoint
{
	enum
	{
		PelvisJoint, ChestJoint, NeckJoint, SkullJoint,
		LeftShoulder, LeftElbow, LeftHand,
		RightShoulder, RightElbow, RightHand,
		LeftHip, LeftKnee, LeftFoot,
		RightHip, RightKnee, RightFoot,
		JointCount
	};
}

class UDummyBody;

UCLASS()
class AStickFighter : public AActor
{
	GENERATED_BODY()

public:
	AStickFighter();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	// Plays a move by itself, to look at it. An attack played this way hits nobody.
	void PlayMove(EMove Move);

	// Starts the animation of a punch or a kick. SecondsUntilContact stretches the wind-up so the strike arrives when
	// wanted; 0 plays it at natural speed. It lands on nobody by itself: Strike is what gives a blow someone to hit.
	// False when nothing was started: the fighter is down, or it is a punch and no arm is sound to throw it.
	bool Attack(EMove Move, float SecondsUntilContact = 0.f);

	// A heavy hit that does not knock out breaks the nearer arm, which hangs loose until the next take.
	void TakeHit(const FVector& StrikePoint, const FVector& StrikeDirection, bool bKnockout, bool bHeavy);

	void ReturnToIdle();

	// Turns on the spot to face a point, over a few frames.
	void FaceTowards(const FVector& Target);

	// Runs to a point in the given time, then turns to face FaceAfter.
	void MoveTo(const FVector& Target, float Seconds, const FVector& FaceAfter);

	// Where the fighter stands at the start of every take.
	void SetHome(const FVector& Location, const FRotator& Rotation);
	void SetColor(const FLinearColor& NewColor);

	// The player's choices: a colour, a hat and a build. Row 0 is colours, 1 hats, 2 builds.
	void ApplyLook(const FCharacterLook& Look);
	// Gives the fighter the body of one of the cast, by its role: Hero, or the name of a kind of opponent. The models
	// are made by Tools/build_cast.py; a role without one keeps the dummy, and false comes back.
	bool WearCast(const FString& Part);
	// Gives the fighter a small light of its own that goes with its head, in the fight and in the film: the hero in
	// dark clothes is otherwise lost on a dark set.
	void LightAsTheStar();
	static int32 LookCount(int32 Row);
	static FString LookName(int32 Row, int32 Index);
	static FLinearColor LookColor(int32 Index);

	// The world positions the stick is drawn from, always in the same order. The take recorder stores these.
	TArray<FVector> GetJointPositions() const { return BuildJoints(true); }

	// Where on this fighter's body the last strike landed. Effects start here.
	FVector GetLastImpactPoint() const { return LastImpactPoint; }

	bool IsDown() const { return bPuppet ? bPuppetDown : State == EFighterState::Down; }

	// Bit 0 is the left arm, bit 1 the right.
	uint8 GetBrokenArms() const;

	// While a pose is set the fighter is a puppet: it shows exactly that pose and ignores everything else. The final cut replays a take this way.
	void SetPuppetPose(const FFighterPose& Pose);
	void EndPuppet();

	// For a replayed strike, so effects can ask a puppet where it was hit and whether it went down.
	void MarkStruck(const FVector& ImpactPoint, bool bKnockedOut);

	// Lets the fighter walk, jump and be knocked about, inside this part of the floor.
	void SetArena(const FVector2D& Min, const FVector2D& Max);
	bool FightsFreely() const { return bFree; }
	FVector2D ClampToArena(const FVector2D& Point) const;

	// A direction on the floor, in world space. Its length, up to 1, scales the speed. It holds until changed.
	void Walk(const FVector2D& Direction) { WalkInput = Direction; }
	void Jump();

	// Block or Duck, held for as long as this is called every frame.
	void Guard(EMove Pose);

	// A punch or a kick with no chosen victim: it lands on whoever of the other team is in reach when it arrives.
	// What the fighter is holding turns it into another move. Ducked, a punch is an uppercut and a kick a sweep.
	// Behind a raised guard, a punch is a chop and a kick a roundhouse.
	// Held is the key the player has down, Duck or Block: it counts at once, before the body has got into that pose.
	bool Strike(EMove Move, float SecondsUntilContact, EMove Held = EMove::None);

	// Two keys pressed together land a moment apart, in either order. A plain strike that has only just begun becomes
	// the move it makes with the key that followed it. False when there is no such strike to change.
	bool TurnStrikeInto(EMove Held);

	// One move asked for by name: Punch, Kick, Uppercut, Sweep, Roundhouse or Chop. 0 seconds means its own time.
	bool StrikeWith(EMove Move, float SecondsUntilContact);

	// Thrown into the air by a blow, and helpless until a little after landing.
	void PopUp(float Speed, float HelplessSeconds);

	// Picks up the nearest loose prop in reach, or throws the one it carries. False when there was nothing to do.
	bool GrabOrThrow();
	// The loose prop a grab would pick up now: the nearest one in reach, or none.
	AThrownProp* FindPropInReach() const;
	bool IsCarrying() const { return Carried.IsValid(); }
	const AThrownProp* GetCarried() const { return Carried.Get(); }
	FVector2D GetArenaMin() const { return ArenaMin; }
	FVector2D GetArenaMax() const { return ArenaMax; }
	// Takes off the forearm or the shin nearest to a point: a gun does that. Someone still on their feet keeps both legs.
	void Sever(const FVector& Near);
	// For tests in the editor: the first call breaks the right arm, the second the left, the third takes the left forearm off, the fourth every limb.
	void HurtForTests();
	FVector GetCarryPoint() const;
	// 0 while a carried weapon is held up, 1 as its swing lands.
	float GetWeaponSwing() const;

	// The big finish: three blows on the nearest opponent, a hook that stops him, an uppercut that lifts him and a
	// spinning kick that takes him and everyone near apart. Nothing touches the fighter until the last has landed.
	// False when it cannot be started now, or nobody is near enough to be finished.
	bool Finish();
	// 0 the left hand, 1 the right.
	FVector GetHandPoint(int32 Side) const;
	// Where a hand grips: the middle of its palm, a little past the wrist that is its joint.
	FVector GetGripPoint(int32 Side) const;
	// A broken arm holds nothing, and a fighter with one holds nothing in the other either: what he had falls, and
	// he picks nothing up. One good hand on a gun or a crate looked wrong whichever way it was drawn.
	bool CanHold() const { return !LooseArms[0].bBroken && !LooseArms[1].bBroken; }

	// A blow from anything, a strike or a thrown prop: it hurts, staggers or knocks out, and the take hears about it.
	void SufferBlow(AStickFighter* Attacker, float Damage, const FVector& Point, const FVector& Direction, bool bHeavy, EMove Move);

	// Throws a knocked-out fighter's body that way, up and along.
	void Launch(const FVector& Direction, float Speed);

	// True when a new move would be accepted: standing, walking, or in the tail of a strike that has already landed.
	bool CanAct() const;

	// True a little sooner than CanAct, for the player's fighter: walking off, a guard or a jump may cut a strike's
	// follow-through short before another strike may.
	bool CanBreakOff() const;
	bool IsStriking() const { return bStrikeOnItsWay || SwingKickSeconds >= 0.f; }
	bool IsHanging() const { return Hanging.IsValid(); }
	bool IsStunned() const { return StunSecondsLeft > 0.f; }
	bool IsAirborne() const { return JumpHeight > 0.f; }
	bool IsAiming() const { return bAiming; }
	bool IsTravelling() const { return bMoving; }
	bool IsGuarding() const { return HeldPose == EMove::Block && PoseAlpha > 0.4f; }
	bool IsDucking() const { return HeldPose == EMove::Duck && PoseAlpha > 0.5f; }
	float GetHealthFraction() const { return FMath::Clamp(Health / MaxHealth, 0.f, 1.f); }
	// Back to full strength, for a fight that starts in the middle of a take.
	void CatchBreath() { Health = MaxHealth; }

	// 0 is the player, 1 the opponents. A strike only lands on the other team.
	int32 Team = 1;
	float MaxHealth = 40.f;
	float WalkSpeed = 300.f;
	float PunchDamage = 10.f;
	float KickDamage = 16.f;
	// The share of a blow's damage this fighter takes. The flow lowers it for the player after several retakes.
	float DamageTakenScale = 1.f;
	// Punches hurt an armoured fighter but do not stop what it is doing.
	bool bArmored = false;
	// Draws a gun in the right hand.
	bool bArmed = false;

	UPROPERTY(VisibleAnywhere, Category = "Stick")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(EditAnywhere, Category = "Stick")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Stick")
	float Thickness = 7.f;

	UPROPERTY(EditAnywhere, Category = "Stick")
	float HeadRadius = 11.f;

	// Cycles through every move forever, to check the animations by eye.
	UPROPERTY(EditAnywhere, Category = "Stick")
	bool bDemoAllMoves = false;

protected:
	// While set, the right arm points straight at this place: someone taking aim.
	bool bAiming = false;
	FVector AimPoint = FVector::ZeroVector;

private:
	enum class EFighterState : uint8
	{
		Idle,
		Moving,
		Busy,
		Down
	};

	struct FLooseArm
	{
		bool bBroken = false;
		FVector Hand = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
	};

	UFUNCTION()
	void HandleTakeStarted(int32 TakeNumber);

	void Play(UAnimSequenceBase* Animation, bool bLoop, float Rate = 1.f);
	// Of the four limbs that can strike, only BoneCount from FirstBone on are looked at: the hands come first, then the feet.
	float FindContactSeconds(UAnimSequenceBase* Strike, int32 FirstBone = 0, int32 BoneCount = 4);
	// Standing ready: the fighting stance for someone who fights freely, the plain idle for anyone else.
	void PlayRest();
	// The next punch that is thrown with an arm that is not broken, or none.
	UAnimSequenceBase* NextPunch();
	void LandStrike();
	void StartRecoil(const FVector& StrikePoint, const FVector& StrikeDirection, float Strength);
	void CaptureBlendFrom();
	void SwingBrokenArms(float DeltaSeconds);
	void DemoNextMove();
	TArray<FVector> BuildJoints(bool bWithBrokenArms) const;
	// Brings a hand or a foot that has gone into an opponent's body back to its surface, and bends the limb to suit.
	void KeepOutOfBodies(TArray<FVector>& Joints) const;
	void PoseStick(const TArray<FVector>& Joints, uint8 BrokenArms);
	void AdvanceFreeMovement(float DeltaSeconds);
	void LandFreeStrike(const FVector& StrikePoint);
	void TakeBlow(float Damage, const FVector& StrikePoint, const FVector& StrikeDirection, bool bHeavy);
	void StartPosed(EMove Technique, float SecondsUntilContact);
	void TryToGrabABar();
	void AdvanceHang(float DeltaSeconds);
	void LetGoOfBar(const FVector& Velocity, float UpSpeed);
	// Lets go and flies off the way the swing was going.
	void SwingOffBar();
	bool StartSwingKick();
	bool FireGun();
	void FlyLooseLimbs(float DeltaSeconds);

	// Bit 0 and 1: the left and the right forearm are gone. Bit 2 and 3: the left and the right shin.
	uint8 Severed = 0;
	uint8 DrawnSevered = 0;
	// For each limb that can come off, a length of it and its hand or foot, to fly and lie where they land.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> LooseLimbs;
	// The wound where each one was: a flat cut, and the end of bone that shows in the middle of it.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Stumps;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StumpBones;
	// The same cut on the end of the limb that flew off, when the limb is one of the cast's own.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> LimbCuts;
	struct FFlyingLimb
	{
		FVector Middle = FVector::ZeroVector;
		FVector Along = FVector::UpVector;
		FVector Velocity = FVector::ZeroVector;
		double Length = 0.;
		bool bFlying = false;
	};
	FFlyingLimb FlyingLimbs[4];
	void LandSwingKick();
	FVector WantedForward() const;
	void SnapToWantedFacing();
	void TintBody(const FLinearColor& Tint);
	void WatchForFloating(const TArray<FVector>& Joints, float DeltaSeconds, const TCHAR* When, float AllowedSeconds);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Limbs;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Head;

	// The second piece of a forearm, shown only while that arm is broken.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> ForearmStumps;

	// What shows at a break, two per arm: a pale spike of bone past it, and a wound around it.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BreakParts;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	UPROPERTY()
	TArray<TObjectPtr<UAnimSequenceBase>> PunchAnimations;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> KickAnimation;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> FallAnimation;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> BackAnimation;

	// Motion-captured clips for the moves that are otherwise poses worked out in code. A move with no clip keeps its pose.
	UPROPERTY()
	TMap<EMove, TObjectPtr<UAnimSequenceBase>> TechniqueClips;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> StanceClip;

	// How a blow that leaves the fighter standing is taken: to the head, and to the body.
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> HitClips[2];

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> Playing;

	// The body the fighter is seen as. Without its model in the project, the fighter is drawn from plain shapes instead.
	UPROPERTY()
	TObjectPtr<UDummyBody> Model;

	// Drawn for one frame behind a hand or foot that moves very fast, like a smear frame in hand-drawn animation.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Smear;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Gun;
	UPROPERTY()
	TObjectPtr<class UPointLightComponent> StarLight;

	// The gun in an opponent's hand is the pistol prop's own model, looked for the first time one is held.
	bool bLookedForAPistol = false;
	bool bHoldsAPistol = false;

	// A hat is a few plain shapes set on the head. It is placed from the joints, so the film shows it too.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> HatParts;

	// The plain shapes a hat is made of: a cylinder, a cube, a cone and a ball.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> HatMeshes;

	int32 Hat = 0;
	// A limb that comes off one of the cast is that limb: a copy of the model with everything else closed down.
	UPROPERTY()
	TArray<TObjectPtr<class UDummyBody>> LimbBodies;

	// A ball at every joint rounds the limbs off and gives the figure hands and feet. Eyes show which way it faces.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> JointBalls;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Eyes;

	// What makes the stick a stunt dummy: a chest, a waist and hips round the spine, and a boot on each foot.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	// A dummy's yellow and black marks: one on the chest and one on the outside of each shoulder, two discs each.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Marks;

	// Shown only for the instant of a hit, when the figure is drawn as its own skeleton.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ribs;

	// How far the hands are up in a fighter's ready stance.
	float StanceAlpha = 0.f;

	EFighterState State = EFighterState::Idle;
	float BusyAnimationSeconds = 0.f;
	int32 PunchCount = 0;

	EMove HeldPose = EMove::None;
	float PoseSecondsLeft = 0.f;
	float PoseAlpha = 0.f;

	bool bStrikeOnItsWay = false;
	float ContactSeconds = 0.f;
	FName StrikeBone;
	EMove StrikeMove = EMove::None;

	FVector LastImpactPoint = FVector::ZeroVector;
	FVector RecoilDirection = FVector::ZeroVector;
	float RecoilProgress = 1.f;
	float RecoilStrength = 1.f;
	float RecoilWeights[StickJoint::JointCount] = {};

	FLooseArm LooseArms[2];
	// Which forearms are currently drawn broken, so the tint only changes when the state does.
	uint8 TintedBrokenArms = 0;

	bool bPuppet = false;
	bool bPuppetDown = false;
	FFighterPose PuppetPose;

	bool bTurning = false;
	float FacingYaw = 0.f;

	bool bHasHome = false;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;

	bool bMoving = false;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveTarget = FVector::ZeroVector;
	FVector FaceAfterMove = FVector::ZeroVector;
	float MoveSeconds = 0.f;
	float MoveElapsed = 0.f;

	// The pose being left, relative to the actor, so a new animation fades in instead of snapping.
	TArray<FVector> BlendFromLocal;
	float BlendSecondsLeft = 0.f;

	TArray<FVector> LastDrawnJoints;
	float LastDrawnSeconds = 0.f;

	struct FContact
	{
		float Seconds = 0.f;
		FName Bone;
		double Reach = 0.;
	};
	// When each strike animation makes contact, worked out once.
	TMap<const UAnimSequenceBase*, FContact> Contacts;

	TWeakObjectPtr<AThrownProp> Carried;
	float CarryAlpha = 0.f;
	float ThrowSecondsLeft = 0.f;
	// Since a swing of the carried weapon began, and when it lands. Negative while nothing is swung.
	float WeaponSwingElapsed = -1.f;
	float WeaponSwingContactAt = 0.f;
	// The big finish is under way: which of its blows is on its way, who it is for, when the next one starts, and how
	// long the whole of it may still take before it is given up, so nothing can leave a fighter untouchable for good.
	bool bFinishing = false;
	int32 FinishBeat = 0;
	TWeakObjectPtr<AStickFighter> FinishVictim;
	float FinishNextBeatSecondsLeft = -1.f;
	float FinishSecondsLeft = 0.f;
	void StartFinishBeat();
	void LandFinish();
	int32 BlowsInARow = 0;
	float LastBlowSeconds = 0.f;

	// How far in front of the fighter the strike on its way gets: measured from its animation, or the length of the kicking leg.
	double StrikeReach = 100.;
	FVector LungeVelocity = FVector::ZeroVector;
	// How the fighter moved over the last tick, so a throw can be aimed ahead of it.
	FVector MovedPerSecond = FVector::ZeroVector;
	FVector LastTickLocation = FVector::ZeroVector;
	bool bKicking = false;
	// The technique on its way is a clip, so no pose is laid over the joints for it.
	bool bPosedByClip = false;
	float KickElapsed = 0.f;
	float KickContactAt = 0.f;
	float PosedRecoverSeconds = 0.45f;
	// The arm an uppercut or a chop is thrown with: 0 the left, 1 the right.
	int32 PosedSide = 1;
	float LandSquashSecondsLeft = 0.f;

	TWeakObjectPtr<ASwingBar> Hanging;
	float HangSeconds = 0.f;
	// Under 0 while the fighter only hangs. From 0 up, how long its kick off the bar has been going.
	float SwingKickSeconds = -1.f;
	// Hanging, the body is a pendulum on the bar. Its angle from straight down, in radians, is positive towards world +X:
	// a bar runs along Y, so that is the one way a body on it can swing, whichever way the fighter faces.
	float SwingAngle = 0.f;
	float SwingSpeed = 0.f;
	float KickSwingFrom = 0.f;
	// Only a jump the fighter chose can catch a bar. One thrown into the air by a blow does not.
	bool bMayGrabABar = false;

	float FloatingSeconds = 0.f;

	bool bFree = false;
	FVector2D ArenaMin = FVector2D::ZeroVector;
	FVector2D ArenaMax = FVector2D::ZeroVector;
	double FloorHeight = 0.;
	FVector2D WalkInput = FVector2D::ZeroVector;
	bool bWalking = false;
	bool bWalkingBackwards = false;
	float FlashSecondsLeft = 0.f;
	float Health = 40.f;
	float StunSecondsLeft = 0.f;
	FVector KnockVelocity = FVector::ZeroVector;
	float JumpHeight = 0.f;
	float JumpSpeed = 0.f;
	bool bFreeStrike = false;
	bool bStrikeFromAir = false;
	float SecondsSinceContact = 0.f;
	// After a follow-through was left early: how long until the next strike may start.
	float StrikeAgainSecondsLeft = 0.f;
	bool IsFreeOnceLanded(float AfterAPunch, float AfterAnyOther) const;
	void BreakOff();
	void StepIntoStrike(float SecondsUntilContact);

	int32 DemoStep = 0;
	float DemoRestSeconds = 0.f;
};
