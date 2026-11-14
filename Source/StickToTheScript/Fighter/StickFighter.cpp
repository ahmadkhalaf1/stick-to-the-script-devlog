#include "StickToTheScript/Fighter/StickFighter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StickToTheScript/Core/TakeEvents.h"
#include "StickToTheScript/Fighter/DummyBody.h"
#include "StickToTheScript/Fighter/SwingBar.h"
#include "StickToTheScript/Fighter/ThrownProp.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogStickFighter, Log, All);

namespace
{
	using namespace StickJoint;

	const FName JointBones[JointCount] = {
		"pelvis", "spine_03", "neck_01", "head",
		"upperarm_l", "lowerarm_l", "hand_l",
		"upperarm_r", "lowerarm_r", "hand_r",
		"thigh_l", "calf_l", "foot_l",
		"thigh_r", "calf_r", "foot_r",
	};

	const int32 LimbJoints[][2] = {
		{PelvisJoint, ChestJoint}, {ChestJoint, NeckJoint}, {NeckJoint, SkullJoint},
		{NeckJoint, LeftShoulder}, {LeftShoulder, LeftElbow}, {LeftElbow, LeftHand},
		{NeckJoint, RightShoulder}, {RightShoulder, RightElbow}, {RightElbow, RightHand},
		{PelvisJoint, LeftHip}, {LeftHip, LeftKnee}, {LeftKnee, LeftFoot},
		{PelvisJoint, RightHip}, {RightHip, RightKnee}, {RightKnee, RightFoot},
	};

	const int32 ElbowOfSide[2] = { LeftElbow, RightElbow };
	const int32 HandOfSide[2] = { LeftHand, RightHand };
	// Where each forearm sits in LimbJoints.
	const int32 ForearmLimbOfSide[2] = { 5, 8 };

	const float BreakKink = 13.f;
	const float BoneLength = 11.f;
	const FLinearColor BloodiedColor(0.45f, 0.01f, 0.01f);
	const FLinearColor BoneColor(0.92f, 0.9f, 0.82f);
	const FLinearColor StumpRed(0.3f, 0.f, 0.f);
	// A gun is aimed at the nearest opponent within this distance and this much of straight ahead, and reaches this far.
	const double GunAimDistance = 1100.;
	const double GunAimCosine = 0.5;
	const double GunRange = 1700.;
	const double GunMuzzleAhead = 40.;
	// With nobody to aim at, a shot flies level at this height. A pellet hits whoever it passes this close to.
	const double GunLevelHeight = 120.;
	const double GunBodyRadius = 34.;
	// The end walls a shot comes off stand this far outside where the fighters are held, as they do for a thrown prop.
	const double GunWallBeyond = 150.;
	// The shooter is pushed back with this share of what the shot does to whoever it hits.
	const float GunKickBackShare = 0.2f;
	const double LooseLimbGravity = 980.;
	const double LooseLimbSpinDegrees = 420.;

	void PlaceLimb(UStaticMeshComponent* Limb, const FVector& From, const FVector& To, float Thickness)
	{
		Limb->SetWorldLocationAndRotation((From + To) * 0.5f, FRotationMatrix::MakeFromZ(To - From).Rotator());
		Limb->SetWorldScale3D(FVector(Thickness, Thickness, FVector::Dist(From, To)) / 100.f);
	}

	const FName StrikingBones[] = { "hand_l", "hand_r", "foot_l", "foot_r" };

	// Which arm a strike is thrown with: 0 the left, 1 the right, none for a foot. The mannequin's bones end in _l and _r.
	int32 ArmOfStrikeBone(const FName& Bone)
	{
		const FString Name = Bone.ToString();
		return !Name.StartsWith(TEXT("hand")) ? INDEX_NONE : Name.EndsWith(TEXT("_l")) ? 0 : 1;
	}

	// A heavy blow breaks an arm only on a fighter it leaves this badly hurt. Before, every kick broke one.
	const float ArmBreaksBelowHealthShare = 0.35f;

	const float RecoilSeconds = 0.4f;
	const float RecoilDistance = 28.f;
	// How far from the struck joint a hit is still felt strongly.
	const float RecoilReach = 40.f;
	const float HeavyRecoil = 1.6f;
	const float BlockedRecoil = 0.35f;
	const float KnockoutSpeed = 200.f;
	const float AnimationEndTolerance = 0.02f;
	// A motion-captured move is never played faster than this, and this much of its way back is shown.
	const float MaxClipRate = 2.f;
	const float ClipShareShownComingBack = 0.8f;

	const float LooseArmDamping = 1.2f;
	const float LooseArmMaxSpeed = 1500.f;

	const float TurnSpeed = 14.f;
	const float BlendSeconds = 0.14f;
	// A jog covers about this much ground a second at its natural speed.
	const float RunAnimationSpeed = 400.f;
	const float ArrivedDistance = 10.f;
	// A hand or foot faster than this, in centimetres a second, gets a smear.
	const double SmearSpeed = 1300.;

	const float PoseHoldSeconds = 0.7f;
	const float PoseBlendPerSecond = 9.f;
	const double DuckDrop = 45.;
	const double DuckLeanDegrees = 30.;

	// A free strike lands when its fist or foot gets this close to the middle of a body.
	const double BodyRadius = 18.;
	// A fighter steps up to this far into its strike, to arrive with the fist or foot just touching.
	const double LungeDistance = 75.;
	const double TouchGap = 5.;
	const float KickRecoverSeconds = 0.45f;
	const double KickLeanDegrees = 16.;
	const double KickLegStretch = 0.97;
	// The fist of an uppercut and the edge of a chopping hand land this far ahead of the fighter.
	const double UppercutReach = 42.;
	const double ChopReach = 52.;
	// A bar is caught by a fighter this close under it, whose raised hands are within the slack of it.
	const double BarGrabWidth = 42.;
	const double BarGrabSlack = 14.;
	const float MaxHangSeconds = 3.6f;
	// From the bar to the middle of the body that swings on it.
	const float HangPendulumLength = 100.f;
	// The swing loses this share of its speed a second, and the walk keys push it this hard.
	const float HangSwingDamping = 0.45f;
	const float HangSwingPush = 5.f;
	const float HangSwingLimit = 1.35f;
	// Nobody catches a bar and hangs dead still.
	const float HangSwingAtLeast = 0.9f;
	// The legs follow the body this many seconds late, up to this many degrees.
	const float HangLegTrailSeconds = 0.09f;
	const double HangLegTrailLimit = 26.;
	// The kick off a bar swings back this far first, then through this far, in radians.
	const float SwingKickBackAngle = -0.45f;
	const float SwingKickThroughAngle = 1.12f;
	// The kick off a bar: how long the swing takes, who it reaches, and how far the fighter flies after letting go.
	const float SwingKickContactSeconds = 0.34f;
	const double SwingKickReach = 205.;
	const double SwingKickWidth = 62.;
	const float SwingKickDamageScale = 2.2f;
	const float SwingKickFlySpeed = 1400.f;
	const float SwingKickLiftSpeed = 160.f;
	// A waiting fighter rides its knees a little, and a landing bends them.
	const double StanceBobDepth = 0.05;
	const double StanceBobPerSecond = 5.5;
	const float LandSquashSeconds = 0.16f;
	const double LandSquashDepth = 0.4;

	// How thick each limb of LimbJoints is drawn, as a share of the figure's thickness: a thigh is thicker than a calf
	// and an upper arm than a forearm, which is most of what makes a row of sticks read as a body.
	const float TrunkWidth = 1.6f;
	const float GirdleWidth = 1.25f;
	const float LimbWidths[] = {
		TrunkWidth, TrunkWidth, 0.95f,
		GirdleWidth, 1.25f, 0.95f,
		GirdleWidth, 1.25f, 0.95f,
		GirdleWidth, 1.5f, 1.1f,
		GirdleWidth, 1.5f, 1.1f,
	};
	const float JointBallWidth = 1.3f;
	// The build the model is made for. A stouter or slighter one swells or thins it, within what still looks like a body.
	const float UsualThickness = 7.f;
	const float SlightestGirth = 0.85f;
	const float StoutestGirth = 1.3f;
	const float HandAndFootWidth = 1.6f;
	const int32 FirstBallJoint = LeftShoulder;

	// The stunt dummy's body, in BodyParts.
	enum { ChestPart, WaistPart, HipsPart, LeftBoot, RightBoot, BodyPartCount };
	const FLinearColor MarkYellow(1.f, 0.7f, 0.f);
	const FLinearColor MarkBlack(0.02f, 0.02f, 0.02f);

	// A dummy's joints, fists and boots are black rubber. On a figure that is itself near black they are steel, so they still show.
	FLinearColor RubberFor(const FLinearColor& Body)
	{
		return Body.GetLuminance() > 0.06f ? FLinearColor(0.025f, 0.025f, 0.03f) : FLinearColor(0.3f, 0.31f, 0.34f);
	}

	// A sphere stretched into a body part: Size is its width along the rotation's X, Y and Z, in centimetres.
	void PlaceBlob(UStaticMeshComponent* Part, const FVector& Centre, const FRotator& Rotation, const FVector& Size)
	{
		Part->SetWorldLocationAndRotation(Centre, Rotation);
		Part->SetWorldScale3D(Size / 100.);
	}

	// A flat disc, lying against a surface that faces Out.
	void PlaceDisc(UStaticMeshComponent* Disc, const FVector& Centre, const FVector& Out, double Radius)
	{
		Disc->SetWorldLocationAndRotation(Centre, FRotationMatrix::MakeFromZ(Out).Rotator());
		Disc->SetWorldScale3D(FVector(Radius * 2., Radius * 2., 0.5) / 100.);
	}
	const float StanceBlendPerSecond = 6.f;

	// A legendary prop in the hands: one blow knocks out, reaches further, and throws the body.
	const double LegendaryReach = 45.;
	const float LegendaryLaunchSpeed = 950.f;

	// A backwards walk covers about this much ground a second at its natural speed.
	const float WalkBackAnimationSpeed = 170.f;
	// A struck figure shows as its own skeleton for a moment, like an X-ray: thin pale bones, a skull, ribs.
	// A little brighter than white, so the picture's glow catches it.
	const float FlashSeconds = 0.13f;
	const FLinearColor FlashColor(1.8f, 1.75f, 1.55f);
	const float BoneWidth = 0.32f;
	const float BoneJointWidth = 0.6f;

	struct FLookColor
	{
		const TCHAR* Name;
		FLinearColor Color;
	};
	// Bright, and none of them an opponent's colour or the captive's.
	const FLookColor LookColors[] = {
		{ TEXT("Chalk"), FLinearColor(0.8f, 0.8f, 0.8f) },
		{ TEXT("Sky"), FLinearColor(0.25f, 0.6f, 1.f) },
		{ TEXT("Lime"), FLinearColor(0.45f, 0.9f, 0.15f) },
		{ TEXT("Orange"), FLinearColor(1.f, 0.45f, 0.05f) },
		{ TEXT("Pink"), FLinearColor(1.f, 0.3f, 0.6f) },
		{ TEXT("Lilac"), FLinearColor(0.65f, 0.45f, 1.f) },
		{ TEXT("Mint"), FLinearColor(0.3f, 0.95f, 0.75f) },
		{ TEXT("Bone"), FLinearColor(0.9f, 0.85f, 0.65f) },
		// Sold in the shop. The shop names this entry by its place in the list.
		{ TEXT("Royal purple"), FLinearColor(0.42f, 0.1f, 0.85f) },
	};

	struct FLookBuild
	{
		const TCHAR* Name;
		float Thickness;
		float HeadRadius;
	};
	const FLookBuild LookBuilds[] = {
		{ TEXT("Slim"), 5.5f, 10.f },
		{ TEXT("Regular"), 7.f, 11.f },
		{ TEXT("Heavy"), 9.5f, 12.5f },
	};

	// One shape of a hat. Ahead and Above place it from the middle of the head, and with Size they are in head radii,
	// so a hat fits whatever the build.
	struct FHatPiece
	{
		bool bUsed;
		bool bCube;
		double Ahead;
		double Above;
		FVector Size;
	};
	// The last two are sold in the shop, which names them by their place in this list.
	const TCHAR* HatNames[] = { TEXT("None"), TEXT("Cap"), TEXT("Top hat"), TEXT("Headband"), TEXT("Mohawk"), TEXT("Halo"), TEXT("Crown") };
	const FHatPiece Hats[][2] = {
		{ {}, {} },
		{ { true, false, 0., 0.75, FVector(2.1, 2.1, 0.7) }, { true, true, 1.1, 0.5, FVector(1.2, 1.6, 0.12) } },
		{ { true, false, 0., 1.6, FVector(1.5, 1.5, 1.4) }, { true, false, 0., 0.9, FVector(2.6, 2.6, 0.12) } },
		{ { true, false, 0., 0.25, FVector(2.12, 2.12, 0.32) }, { true, true, -1.4, 0.25, FVector(0.9, 0.25, 0.25) } },
		{ { true, true, 0., 1., FVector(1.9, 0.22, 0.75) }, {} },
		{ { true, false, 0., 1.8, FVector(2.1, 2.1, 0.07) }, {} },
		{ { true, false, 0., 1., FVector(1.9, 1.9, 0.6) }, { true, false, 0., 0.7, FVector(2.25, 2.25, 0.14) } },
	};
	const FLinearColor HatColors[] = {
		FLinearColor::Black,
		FLinearColor(0.55f, 0.03f, 0.03f),
		FLinearColor(0.03f, 0.03f, 0.04f),
		FLinearColor(0.6f, 0.03f, 0.03f),
		FLinearColor(0.75f, 0.3f, 0.02f),
		FLinearColor(2.6f, 2.2f, 0.7f),
		FLinearColor(1.6f, 1.05f, 0.12f),
	};
	// A free strike lands on anyone within about 70 degrees of straight ahead.
	const double StrikeConeCosine = 0.34;
	const float AirStrikeDamageScale = 1.4f;
	const float StunSeconds = 0.35f;
	const float HeavyStunSeconds = 0.6f;
	const float KnockSpeed = 270.f;
	const float HeavyKnockSpeed = 720.f;
	const float BlockedKnockSpeed = 140.f;
	const float KnockDamping = 9.f;
	const float JumpLaunchSpeed = 620.f;
	const float JumpGravity = 1800.f;
	// A fighter higher than this is out of reach of a strike thrown from the floor.
	const float OutOfReachHeight = 60.f;
	const double BodyGap = 45.;
	// How soon after a strike lands the next move may cut its follow-through short.
	const float CancelSeconds = 0.12f;
	const float GuardHoldSeconds = 0.12f;
	// A kick is strong, so its follow-through cannot be cut short as early as a punch's.
	const float KickCancelSeconds = 0.3f;
	// The third quick blow in a row sends its victim flying. That ends the string and gives everyone room.
	const int32 BlowsInAFinisher = 3;
	const float BlowChainSeconds = 1.f;

	const double GrabReach = 130.;
	// Hurled downwards from over the head: slow enough to be seen, and it comes down within about four metres.
	const float ThrowSpeed = 1000.f;
	const float ThrowDrop = -250.f;
	// A throw bends up to about 35 degrees to find someone within this distance.
	const double ThrowAimDistance = 450.;
	const double ThrowAimCosine = 0.82;
	const float ThrowGestureSeconds = 0.3f;
	const double ThrowLeanRadians = 0.42;
	// A strike thrown from too close steps back by up to this much, so the fist does not go through the body.
	const double StepBackDistance = 40.;

	// Where an elbow or knee sits, given both ends of the limb, its two lengths and the way it bends.
	FVector MiddleJoint(const FVector& Root, const FVector& End, double UpperLength, double LowerLength, const FVector& BendDirection)
	{
		const FVector Axis = (End - Root).GetSafeNormal();
		const double Distance = FMath::Clamp(FVector::Dist(Root, End), 1., UpperLength + LowerLength);
		const double Along = (FMath::Square(UpperLength) - FMath::Square(LowerLength) + FMath::Square(Distance)) / (2. * Distance);
		const double Out = FMath::Sqrt(FMath::Max(FMath::Square(UpperLength) - FMath::Square(Along), 0.));
		const FVector Bend = (BendDirection - Axis * (BendDirection | Axis)).GetSafeNormal();
		return Root + Axis * Along + Bend * Out;
	}

	// Tips everything above the legs about the pelvis, forward for a positive angle, and lowers it by Drop.
	void LeanBody(TArray<FVector>& Joints, const FVector& Forward, double Radians, double Drop)
	{
		const FVector Pelvis = Joints[PelvisJoint];
		for (const int32 Index : { PelvisJoint, ChestJoint, NeckJoint, SkullJoint, LeftShoulder, LeftElbow, LeftHand, RightShoulder, RightElbow, RightHand, LeftHip, RightHip })
		{
			const FVector Offset = Joints[Index] - Pelvis;
			const double Ahead = Offset | Forward;
			const double Above = Offset.Z;
			const FVector Sideways = Offset - Forward * Ahead - FVector::UpVector * Above;
			Joints[Index] = Pelvis + Sideways
				+ Forward * (Ahead * FMath::Cos(Radians) + Above * FMath::Sin(Radians))
				+ FVector::UpVector * (Above * FMath::Cos(Radians) - Ahead * FMath::Sin(Radians) - Drop);
		}
	}

	// The engine ships no defensive animations, so block and duck are posed on the joints over whatever is playing.
	void PoseDuck(TArray<FVector>& Joints, const FVector& Forward, double Alpha)
	{
		const double LeftThigh = FVector::Dist(Joints[LeftHip], Joints[LeftKnee]);
		const double LeftCalf = FVector::Dist(Joints[LeftKnee], Joints[LeftFoot]);
		const double RightThigh = FVector::Dist(Joints[RightHip], Joints[RightKnee]);
		const double RightCalf = FVector::Dist(Joints[RightKnee], Joints[RightFoot]);

		LeanBody(Joints, Forward, FMath::DegreesToRadians(DuckLeanDegrees * Alpha), DuckDrop * Alpha);

		Joints[LeftKnee] = MiddleJoint(Joints[LeftHip], Joints[LeftFoot], LeftThigh, LeftCalf, Forward);
		Joints[RightKnee] = MiddleJoint(Joints[RightHip], Joints[RightFoot], RightThigh, RightCalf, Forward);
	}

	// Both hands go to a point given from the head: ahead of it for a guard, above it to carry something.
	void PoseArms(TArray<FVector>& Joints, const FVector& Forward, double Alpha, double Ahead, double Out, double Up)
	{
		const FVector Right = FVector::UpVector ^ Forward;
		const int32 Arms[2][3] = { { LeftShoulder, LeftElbow, LeftHand }, { RightShoulder, RightElbow, RightHand } };
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 Shoulder = Arms[Side][0];
			const int32 Elbow = Arms[Side][1];
			const int32 Hand = Arms[Side][2];
			const FVector Outward = Right * (Side == 0 ? -1. : 1.);
			const double UpperLength = FVector::Dist(Joints[Shoulder], Joints[Elbow]);
			const double LowerLength = FVector::Dist(Joints[Elbow], Joints[Hand]);

			const FVector Guard = Joints[SkullJoint] + Forward * Ahead + Outward * Out + FVector(0., 0., Up);
			Joints[Hand] = FMath::Lerp(Joints[Hand], Guard, Alpha);
			Joints[Elbow] = MiddleJoint(Joints[Shoulder], Joints[Hand], UpperLength, LowerLength, Outward * 0.5 - FVector::UpVector);
		}
	}

	// What follows poses strikes in code. The engine's mannequin has three attack animations and nothing else to fight with.

	const int32 ArmJoints[2][3] = { { LeftShoulder, LeftElbow, LeftHand }, { RightShoulder, RightElbow, RightHand } };
	const int32 LegJoints[2][3] = { { LeftHip, LeftKnee, LeftFoot }, { RightHip, RightKnee, RightFoot } };

	// The lengths of the four limbs as the animation left them. A pose moves shoulders and hips first, and a limb
	// measured after that has changed length.
	struct FLimbs
	{
		explicit FLimbs(const TArray<FVector>& Joints)
		{
			for (int32 Side = 0; Side < 2; ++Side)
			{
				ArmUpper[Side] = FVector::Dist(Joints[ArmJoints[Side][0]], Joints[ArmJoints[Side][1]]);
				ArmLower[Side] = FVector::Dist(Joints[ArmJoints[Side][1]], Joints[ArmJoints[Side][2]]);
				Thigh[Side] = FVector::Dist(Joints[LegJoints[Side][0]], Joints[LegJoints[Side][1]]);
				Calf[Side] = FVector::Dist(Joints[LegJoints[Side][1]], Joints[LegJoints[Side][2]]);
			}
		}

		double Leg(int32 Side) const { return Thigh[Side] + Calf[Side]; }

		double ArmUpper[2];
		double ArmLower[2];
		double Thigh[2];
		double Calf[2];
	};

	// Puts a hand or a foot where it is wanted and bends the limb to get there. A target out of reach is brought in.
	void ReachTo(TArray<FVector>& Joints, const int32* Limb, double UpperLength, double LowerLength, const FVector& Target, const FVector& Bend)
	{
		const FVector Root = Joints[Limb[0]];
		Joints[Limb[2]] = Root + (Target - Root).GetClampedToMaxSize((UpperLength + LowerLength) * 0.99);
		Joints[Limb[1]] = MiddleJoint(Root, Joints[Limb[2]], UpperLength, LowerLength, Bend);
	}

	void ReachWithArm(TArray<FVector>& Joints, const FLimbs& Limbs, int32 Side, const FVector& Target, const FVector& Bend)
	{
		ReachTo(Joints, ArmJoints[Side], Limbs.ArmUpper[Side], Limbs.ArmLower[Side], Target, Bend);
	}

	void ReachWithLeg(TArray<FVector>& Joints, const FLimbs& Limbs, int32 Side, const FVector& Target, const FVector& Bend)
	{
		ReachTo(Joints, LegJoints[Side], Limbs.Thigh[Side], Limbs.Calf[Side], Target, Bend);
	}

	// A strike's phase runs 0 to 1 while it is on its way and 1 to 2 while it is taken back.
	// This is 0 as it starts, 1 as it lands and 0 again when it is over.
	double OutAndBack(float Phase)
	{
		return FMath::SmoothStep(0., 1., static_cast<double>(Phase <= 1.f ? Phase : 2.f - Phase));
	}

	// Turns the whole figure about the upright line through its pelvis.
	void SpinBody(TArray<FVector>& Joints, double Degrees)
	{
		const FVector Pivot = Joints[PelvisJoint];
		for (FVector& Joint : Joints)
		{
			Joint = Pivot + (Joint - Pivot).RotateAngleAxis(Degrees, FVector::UpVector);
		}
	}

	// The knee comes up first and the foot snaps out from it, with the body leaning back to balance.
	// Thrown straight from the hip, as it first was, the kick read as a push.
	void PoseKick(TArray<FVector>& Joints, const FVector& Forward, float Phase)
	{
		const FLimbs Limbs(Joints);
		const double Out = OutAndBack(Phase);
		const FVector Planted = Joints[LeftFoot];
		const FVector Standing = Joints[RightFoot];

		LeanBody(Joints, Forward, -FMath::DegreesToRadians(KickLeanDegrees * Out), 0.);
		ReachWithLeg(Joints, Limbs, 0, Planted, Forward);

		const double Leg = Limbs.Leg(1);
		const FVector Chambered = Joints[RightHip] + Forward * (Leg * 0.3) - FVector::UpVector * (Leg * 0.2);
		const FVector Extended = Joints[RightHip] + Forward * (Leg * KickLegStretch) + FVector(0., 0., 10.);
		const FVector Foot = Out < 0.5 ? FMath::Lerp(Standing, Chambered, Out * 2.) : FMath::Lerp(Chambered, Extended, Out * 2. - 1.);
		ReachWithLeg(Joints, Limbs, 1, Foot, Forward + FVector::UpVector);
	}

	// Driven up out of a crouch: the fist travels from the hip to above the head, close to the body all the way.
	void PoseUppercut(TArray<FVector>& Joints, const FVector& Forward, float Phase, int32 Side)
	{
		const FLimbs Limbs(Joints);
		const FVector Outward = (FVector::UpVector ^ Forward) * (Side == 0 ? -1. : 1.);
		const double Rise = FMath::SmoothStep(0., 1., FMath::Min(static_cast<double>(Phase), 1.));
		const double Back = FMath::SmoothStep(0., 1., FMath::Clamp(static_cast<double>(Phase) - 1., 0., 1.));
		const FVector FistAtRest = Joints[ArmJoints[Side][2]];
		const FVector OtherAtRest = Joints[ArmJoints[1 - Side][2]];
		const FVector LeftPlanted = Joints[LeftFoot];
		const FVector RightPlanted = Joints[RightFoot];

		PoseDuck(Joints, Forward, 1. - Rise);
		LeanBody(Joints, Forward, -FMath::DegreesToRadians(8. * Rise * (1. - Back)), 0.);
		ReachWithLeg(Joints, Limbs, 0, LeftPlanted, Forward);
		ReachWithLeg(Joints, Limbs, 1, RightPlanted, Forward);

		const FVector Low = Joints[PelvisJoint] + Forward * 14. + Outward * 16. - FVector(0., 0., 6.);
		const FVector High = Joints[SkullJoint] + Forward * 46. + Outward * 4. + FVector(0., 0., 22.);
		// The fist swings out ahead of the straight line between the two, as an arm on a shoulder does.
		const FVector Fist = FMath::Lerp(Low, High, Rise) + Forward * (10. * FMath::Sin(UE_PI * Rise));
		ReachWithArm(Joints, Limbs, Side, FMath::Lerp(Fist, FistAtRest, Back), Outward * 0.5 - FVector::UpVector);

		const FVector Chin = Joints[SkullJoint] + Forward * 16. - Outward * 9. - FVector(0., 0., 16.);
		ReachWithArm(Joints, Limbs, 1 - Side, FMath::Lerp(Chin, OtherAtRest, Back), -Outward * 0.5 - FVector::UpVector);
	}

	// Down on one leg with a hand to the floor, and the other leg goes round like a scythe: from behind on the right,
	// straight ahead as it lands, and on past to the left.
	void PoseSweep(TArray<FVector>& Joints, const FVector& Forward, float Phase)
	{
		const FLimbs Limbs(Joints);
		const FVector Right = FVector::UpVector ^ Forward;
		const double Floor = FMath::Min(Joints[LeftFoot].Z, Joints[RightFoot].Z);
		const FVector Planted = Joints[LeftFoot];
		const FVector Standing = Joints[RightFoot];
		const FVector HandAtRest = Joints[LeftHand];
		const double Down = 1. - FMath::SmoothStep(0., 1., FMath::Clamp((static_cast<double>(Phase) - 1.5) / 0.5, 0., 1.));
		const double Stretched = FMath::SmoothStep(0., 1., FMath::Clamp(static_cast<double>(Phase) / 0.3, 0., 1.)) * Down;

		PoseDuck(Joints, Forward, Down);
		LeanBody(Joints, Forward, FMath::DegreesToRadians(8. * Down), 14. * Down);
		ReachWithLeg(Joints, Limbs, 0, Planted, Forward);

		const double Degrees = Phase < 1.f
			? FMath::Lerp(150., 0., static_cast<double>(Phase))
			: FMath::Lerp(0., -100., FMath::Min((static_cast<double>(Phase) - 1.) / 0.5, 1.));
		const FVector Hip = Joints[RightHip];
		const double Leg = Limbs.Leg(1) * 0.97;
		const double Along = FMath::Sqrt(FMath::Max(FMath::Square(Leg) - FMath::Square(Hip.Z - Floor - 6.), FMath::Square(Leg * 0.5)));
		const FVector Swept = FVector(Hip.X, Hip.Y, Floor + 6.) + Forward.RotateAngleAxis(Degrees, FVector::UpVector) * Along;
		ReachWithLeg(Joints, Limbs, 1, FMath::Lerp(Standing, Swept, Stretched), FVector::UpVector);

		const FVector OnTheFloor = FVector(Joints[PelvisJoint].X, Joints[PelvisJoint].Y, Floor + 4.) - Right * 30. + Forward * 18.;
		ReachWithArm(Joints, Limbs, 0, FMath::Lerp(HandAtRest, OnTheFloor, Down), -Right - FVector::UpVector * 0.3);
	}

	// One whole turn on the standing leg. The kicking leg comes out over the second half of the turn, level with a
	// head, and arrives as the turn ends. The turn starts and ends facing the same way, so nothing jumps.
	void PoseRoundhouse(TArray<FVector>& Joints, const FVector& Forward, float Phase)
	{
		const FLimbs Limbs(Joints);
		const double Wind = FMath::Min(static_cast<double>(Phase), 1.);
		const double After = FMath::Clamp(static_cast<double>(Phase) - 1., 0., 1.);
		const double Out = Phase <= 1.f ? FMath::SmoothStep(0., 1., (Wind - 0.45) / 0.55) : 1. - FMath::SmoothStep(0., 1., After);
		const FVector Planted = Joints[LeftFoot];
		const FVector Standing = Joints[RightFoot];

		LeanBody(Joints, Forward, -FMath::DegreesToRadians(24. * Out), 0.);
		ReachWithLeg(Joints, Limbs, 0, Planted, Forward);

		const double Leg = Limbs.Leg(1);
		const FVector Chambered = Joints[RightHip] + Forward * (Leg * 0.25) + FVector(0., 0., Leg * 0.05);
		const FVector High = Joints[RightHip] + Forward * (Leg * 0.9) + FVector(0., 0., Leg * 0.4);
		const FVector Foot = Out < 0.4 ? FMath::Lerp(Standing, Chambered, Out / 0.4) : FMath::Lerp(Chambered, High, (Out - 0.4) / 0.6);
		ReachWithLeg(Joints, Limbs, 1, Foot, Forward + FVector::UpVector);

		// It speeds up into the blow and swings a little past it.
		SpinBody(Joints, 360. * FMath::Square(Wind) + 35. * FMath::Sin(UE_PI * After));
	}

	// The hand goes up over the head and comes down edge-first, with the body's weight behind it.
	// The other fist is pulled back to the hip, as it is taught.
	void PoseChop(TArray<FVector>& Joints, const FVector& Forward, float Phase, int32 Side)
	{
		const FLimbs Limbs(Joints);
		const FVector Outward = (FVector::UpVector ^ Forward) * (Side == 0 ? -1. : 1.);
		const double Wind = FMath::Min(static_cast<double>(Phase), 1.);
		const double Raise = FMath::SmoothStep(0., 1., Wind / 0.6);
		const double Fall = FMath::SmoothStep(0., 1., (Wind - 0.6) / 0.4);
		const double Back = FMath::SmoothStep(0., 1., FMath::Clamp(static_cast<double>(Phase) - 1., 0., 1.));
		const FVector LeftPlanted = Joints[LeftFoot];
		const FVector RightPlanted = Joints[RightFoot];

		LeanBody(Joints, Forward, FMath::DegreesToRadians(-8. * Raise * (1. - Fall) + 14. * Fall * (1. - Back)), 0.);
		ReachWithLeg(Joints, Limbs, 0, LeftPlanted, Forward);
		ReachWithLeg(Joints, Limbs, 1, RightPlanted, Forward);

		const FVector Guard = Joints[SkullJoint] + Forward * 30. + Outward * 10. + FVector(0., 0., 6.);
		const FVector Over = Joints[SkullJoint] + Forward * 2. + Outward * 12. + FVector(0., 0., 40.);
		const FVector Struck = Joints[SkullJoint] + Forward * 50. + Outward * 2. - FVector(0., 0., 16.);
		const FVector Edge = FMath::Lerp(FMath::Lerp(Guard, Over, Raise), Struck, Fall);
		ReachWithArm(Joints, Limbs, Side, FMath::Lerp(Edge, Guard, Back), Outward * 0.7 - FVector::UpVector * 0.3 - Forward * 0.2);

		const FVector OtherGuard = Joints[SkullJoint] + Forward * 30. - Outward * 10. + FVector(0., 0., 6.);
		const FVector AtTheHip = Joints[PelvisJoint] - Outward * 16. + FVector(0., 0., 14.) - Forward * 4.;
		ReachWithArm(Joints, Limbs, 1 - Side, FMath::Lerp(OtherGuard, AtTheHip, Raise * (1. - Back)), -Outward * 0.6 - Forward * 0.8);
	}

	// What sets one strike posed in code apart from another.
	struct FTechnique
	{
		EMove Move;
		// How long it takes to arrive when the caller does not say. The player's plain kick is told to hurry.
		float WindUpSeconds;
		float RecoverSeconds;
		// Of the fighter's kick damage for a leg, of its punch damage for a hand.
		float DamageScale;
		bool bWithALeg;
		// Everyone inside this much of straight ahead is hit: 1 would be nobody, -1 is all round.
		double ConeCosine;
		// A high strike passes over a ducked head.
		bool bHigh;
		bool bThroughGuard;
		// Throws whoever it hits into the air at this speed.
		float PopSpeed;
		// Leaves whoever it hits helpless for this long. 0 is the usual stagger.
		float StunSeconds;
	};
	const FTechnique Techniques[] = {
		// The punch of a fighter with two sound arms and empty hands: a jab that measures the distance and a cross that lands.
		{ EMove::Punch, 0.3f, 0.35f, 1.f, false, StrikeConeCosine, true, false, 0.f, 0.f },
		{ EMove::Kick, 0.26f, KickRecoverSeconds, 1.f, true, StrikeConeCosine, false, false, 0.f, 0.f },
		{ EMove::Uppercut, 0.2f, 0.5f, 1.8f, false, StrikeConeCosine, false, false, 560.f, 0.95f },
		{ EMove::Sweep, 0.24f, 0.5f, 0.7f, true, -1., false, false, 300.f, 1.1f },
		{ EMove::Roundhouse, 0.36f, 0.5f, 1.6f, true, 0., true, true, 0.f, 0.f },
		{ EMove::Chop, 0.28f, 0.42f, 1.3f, false, StrikeConeCosine, false, true, 0.f, 1.2f },
	};

	const FTechnique* FindTechnique(EMove Move)
	{
		for (const FTechnique& Technique : Techniques)
		{
			if (Technique.Move == Move)
			{
				return &Technique;
			}
		}
		return nullptr;
	}

	void PoseTechnique(TArray<FVector>& Joints, const FVector& Forward, EMove Move, float Phase, int32 Side)
	{
		switch (Move)
		{
		case EMove::Uppercut:
			PoseUppercut(Joints, Forward, Phase, Side);
			break;
		case EMove::Sweep:
			PoseSweep(Joints, Forward, Phase);
			break;
		case EMove::Roundhouse:
			PoseRoundhouse(Joints, Forward, Phase);
			break;
		case EMove::Chop:
			PoseChop(Joints, Forward, Phase, Side);
			break;
		default:
			PoseKick(Joints, Forward, Phase);
			break;
		}
	}

	// Hung by both hands from a bar: the shoulders an arm's length under the grip, and the whole body turned about the
	// bar by Swing degrees, towards world +X for a positive angle. LegsOut throws both legs ahead of the body for the kick,
	// and LegTrail is how many degrees ahead of the body the legs are, which is behind it while they catch up.
	void PoseHang(TArray<FVector>& Joints, const FVector& Forward, const FVector& Grip, double Swing, double LegsOut, double LegTrail)
	{
		const FLimbs Limbs(Joints);
		const FVector Right = FVector::UpVector ^ Forward;
		const double Arm = Limbs.ArmUpper[1] + Limbs.ArmLower[1];
		const FVector Shift = Grip - FVector(0., 0., Arm * 0.96) - (Joints[LeftShoulder] + Joints[RightShoulder]) * 0.5;
		for (FVector& Joint : Joints)
		{
			Joint = Grip + (Joint + Shift - Grip).RotateAngleAxis(-Swing, FVector::YAxisVector);
		}

		for (int32 Side = 0; Side < 2; ++Side)
		{
			ReachWithArm(Joints, Limbs, Side, Grip + Right * (Side == 0 ? -9. : 9.), -Forward);
		}

		// The legs hang a little bent, knees forward. For the kick they go out straight, well ahead of the body.
		const FVector Down = (Joints[PelvisJoint] - Joints[ChestJoint]).GetSafeNormal();
		const FVector Along = Down.RotateAngleAxis(-(FMath::Lerp(14., 58., LegsOut) + LegTrail), Right);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector Hip = Joints[LegJoints[Side][0]];
			ReachWithLeg(Joints, Limbs, Side, Hip + Along * (Limbs.Leg(Side) * FMath::Lerp(0.84, 0.97, LegsOut)), Forward + FVector::UpVector * 0.3);
		}
	}
}

AStickFighter::AStickFighter()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	Body = CreateDefaultSubobject<USkeletalMeshComponent>("Body");
	Body->SetupAttachment(RootComponent);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetVisibility(false);
	// The mannequin is never drawn, but its bones drive the stick limbs, so it must keep animating while hidden.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	AddTickPrerequisiteComponent(Body);

#define MANNEQUIN_ANIMATION(Name, Path) static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Name(TEXT("/Game/Characters/Mannequins/Anims/" Path))
	MANNEQUIN_ANIMATION(Idle, "Unarmed/MM_Idle");
	MANNEQUIN_ANIMATION(Punch1, "Unarmed/Attack/MM_Attack_01");
	MANNEQUIN_ANIMATION(Punch2, "Unarmed/Attack/MM_Attack_02");
	MANNEQUIN_ANIMATION(Punch3, "Unarmed/Attack/MM_Attack_03");
	MANNEQUIN_ANIMATION(Charged, "Unarmed/Attack/MM_ChargedAttack");
	MANNEQUIN_ANIMATION(Jog, "Unarmed/Jog/MF_Unarmed_Jog_Fwd");
	MANNEQUIN_ANIMATION(Fall, "Unarmed/Jump/MM_Fall_Loop");
	MANNEQUIN_ANIMATION(WalkBack, "Unarmed/Walk/MF_Unarmed_Walk_Bwd");
#undef MANNEQUIN_ANIMATION

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mannequin(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/M_Figure"));

	Body->SetSkeletalMeshAsset(Mannequin.Object);
	Model = CreateDefaultSubobject<UDummyBody>("Model");
	Model->SetupAttachment(RootComponent);
	IdleAnimation = Idle.Object;
	PunchAnimations = { Punch1.Object, Punch2.Object, Punch3.Object };
	// shortcut: the engine ships no kick, so the charged punch stands in, S2-FIGHT-01 replaces it.
	KickAnimation = Charged.Object;
	RunAnimation = Jog.Object;
	FallAnimation = Fall.Object;
	BackAnimation = WalkBack.Object;

#define FIGHT_CLIP(Name) static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Name(TEXT("/Game/Characters/Fight/A_" #Name))
	FIGHT_CLIP(MmaKick);
	FIGHT_CLIP(Uppercut);
	FIGHT_CLIP(LegSweep);
	FIGHT_CLIP(RoundhouseKick);
	FIGHT_CLIP(HookPunch);
	FIGHT_CLIP(JabCross);
	FIGHT_CLIP(HeadHit);
	FIGHT_CLIP(StomachHit);
	FIGHT_CLIP(FightingIdle_Anim);
#undef FIGHT_CLIP
	auto Learn = [this](EMove Move, UAnimSequenceBase* Clip)
	{
		if (Clip)
		{
			TechniqueClips.Add(Move, Clip);
		}
	};
	Learn(EMove::Kick, MmaKick.Object);
	Learn(EMove::Uppercut, Uppercut.Object);
	Learn(EMove::Sweep, LegSweep.Object);
	Learn(EMove::Roundhouse, RoundhouseKick.Object);
	Learn(EMove::Chop, HookPunch.Object);
	Learn(EMove::Punch, JabCross.Object);
	HitClips[0] = HeadHit.Object;
	HitClips[1] = StomachHit.Object;
	StanceClip = FightingIdle_Anim.Object;

	auto MakePart = [this](const FString& Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Part->SetupAttachment(RootComponent);
		Part->SetUsingAbsoluteLocation(true);
		Part->SetUsingAbsoluteRotation(true);
		Part->SetUsingAbsoluteScale(true);
		Part->SetStaticMesh(Mesh);
		Part->SetMaterial(0, Material.Object);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};

	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		LooseLimbs.Add(MakePart(FString::Printf(TEXT("LooseLimb%d"), Limb), Cylinder.Object));
		LooseLimbs.Add(MakePart(FString::Printf(TEXT("LooseLimbEnd%d"), Limb), Sphere.Object));
		Stumps.Add(MakePart(FString::Printf(TEXT("Stump%d"), Limb), Sphere.Object));
		LooseLimbs[Limb * 2]->SetVisibility(false);
		LooseLimbs[Limb * 2 + 1]->SetVisibility(false);
		Stumps[Limb]->SetVisibility(false);
	}

	for (int32 i = 0; i < UE_ARRAY_COUNT(LimbJoints); ++i)
	{
		Limbs.Add(MakePart(FString::Printf(TEXT("Limb%d"), i), Cylinder.Object));
	}
	Head = MakePart(TEXT("Head"), Sphere.Object);
	Smear = MakePart(TEXT("Smear"), Cylinder.Object);
	Smear->SetVisibility(false);
	Gun = MakePart(TEXT("Gun"), Cube.Object);
	Gun->SetVisibility(false);

	CylinderMesh = Cylinder.Object;
	CubeMesh = Cube.Object;
	for (int32 Piece = 0; Piece < 2; ++Piece)
	{
		HatParts.Add(MakePart(FString::Printf(TEXT("Hat%d"), Piece), Cylinder.Object));
		HatParts[Piece]->SetVisibility(false);
		Eyes.Add(MakePart(FString::Printf(TEXT("Eye%d"), Piece), Sphere.Object));
	}
	for (int32 Joint = FirstBallJoint; Joint < JointCount; ++Joint)
	{
		JointBalls.Add(MakePart(FString::Printf(TEXT("Ball%d"), Joint), Sphere.Object));
	}
	for (int32 Rib = 0; Rib < 3; ++Rib)
	{
		Ribs.Add(MakePart(FString::Printf(TEXT("Rib%d"), Rib), Cylinder.Object));
		Ribs[Rib]->SetVisibility(false);
	}
	for (int32 Part = 0; Part < BodyPartCount; ++Part)
	{
		BodyParts.Add(MakePart(FString::Printf(TEXT("Body%d"), Part), Sphere.Object));
	}
	for (int32 Disc = 0; Disc < 6; ++Disc)
	{
		Marks.Add(MakePart(FString::Printf(TEXT("Mark%d"), Disc), Cylinder.Object));
		Marks[Disc]->SetCastShadow(false);
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		ForearmStumps.Add(MakePart(FString::Printf(TEXT("ForearmStump%d"), Side), Cylinder.Object));
		ForearmStumps[Side]->SetVisibility(false);
		BreakParts.Add(MakePart(FString::Printf(TEXT("Bone%d"), Side), Cylinder.Object));
		BreakParts.Add(MakePart(FString::Printf(TEXT("Wound%d"), Side), Sphere.Object));
		BreakParts[Side * 2]->SetVisibility(false);
		BreakParts[Side * 2 + 1]->SetVisibility(false);
	}
}

void AStickFighter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetColor(Color);
	ReturnToIdle();
#if WITH_EDITOR
	Body->SetUpdateAnimationInEditor(true);
#endif

	Body->TickAnimation(0.f, false);
	Body->RefreshBoneTransforms();
	PoseStick(GetJointPositions(), GetBrokenArms());
}

void AStickFighter::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UTakeEvents>()->OnTakeStarted.AddDynamic(this, &AStickFighter::HandleTakeStarted);
}

void AStickFighter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FlashSecondsLeft > 0.f)
	{
		FlashSecondsLeft -= DeltaSeconds;
		if (FlashSecondsLeft <= 0.f)
		{
			TintBody(Color);
		}
	}

	FlyLooseLimbs(DeltaSeconds);
	if (bPuppet)
	{
		// A knocked-out body flies before it lands, and that is not a fault.
		if (!bPuppetDown)
		{
			// A figure in the film does not know why it is in the air. The longest honest reason is a swing on a bar,
			// so only longer than that is a fault.
			WatchForFloating(PuppetPose.Joints, DeltaSeconds, TEXT("in the film"), MaxHangSeconds + 0.6f);
		}
		PoseStick(PuppetPose.Joints, PuppetPose.BrokenArms);
		return;
	}

	if (State != EFighterState::Busy)
	{
		bKicking = false;
	}
	if (bKicking)
	{
		// A kick is posed in code, so it keeps its own time instead of following an animation.
		KickElapsed += DeltaSeconds;
		if (bStrikeOnItsWay && KickElapsed >= KickContactAt)
		{
			LandStrike();
		}
		if (State == EFighterState::Busy && KickElapsed >= KickContactAt + PosedRecoverSeconds)
		{
			ReturnToIdle();
		}
	}
	else
	{
		const bool bAnimationEnded = Body->GetPosition() >= BusyAnimationSeconds - AnimationEndTolerance;
		if (bStrikeOnItsWay && Body->GetPosition() >= ContactSeconds)
		{
			LandStrike();
		}
		if (State == EFighterState::Busy && bAnimationEnded)
		{
			ReturnToIdle();
		}
	}
	RecoilProgress = FMath::Min(RecoilProgress + DeltaSeconds / RecoilSeconds, 1.f);

	BlendSecondsLeft = FMath::Max(BlendSecondsLeft - DeltaSeconds, 0.f);

	if (bMoving && State != EFighterState::Down)
	{
		MoveElapsed += DeltaSeconds;
		const float Progress = FMath::Clamp(MoveElapsed / MoveSeconds, 0.f, 1.f);
		SetActorLocation(FMath::Lerp(MoveFrom, MoveTarget, FMath::SmoothStep(0.f, 1.f, Progress)));
		if (Progress >= 1.f)
		{
			bMoving = false;
			if (State == EFighterState::Moving)
			{
				ReturnToIdle();
			}
			FaceTowards(FaceAfterMove);
		}
	}

	StunSecondsLeft = FMath::Max(StunSecondsLeft - DeltaSeconds, 0.f);
	if (StunSecondsLeft <= 0.f && State == EFighterState::Idle && Playing && (Playing == HitClips[0] || Playing == HitClips[1]))
	{
		ReturnToIdle();
	}
	LandSquashSecondsLeft = FMath::Max(LandSquashSecondsLeft - DeltaSeconds, 0.f);
	if (State == EFighterState::Busy && !bStrikeOnItsWay)
	{
		SecondsSinceContact += DeltaSeconds;
	}
	if (bFree && State != EFighterState::Down)
	{
		AdvanceFreeMovement(DeltaSeconds);
	}

	if (bTurning && State != EFighterState::Down && State != EFighterState::Busy)
	{
		const FRotator Wanted(0.f, FacingYaw, 0.f);
		const FRotator Turned = FMath::RInterpTo(GetActorRotation(), Wanted, DeltaSeconds, TurnSpeed);
		SetActorRotation(Turned);
		bTurning = !Turned.Equals(Wanted, 0.5f);
	}

	PoseSecondsLeft -= DeltaSeconds;
	PoseAlpha = FMath::FInterpConstantTo(PoseAlpha, PoseSecondsLeft > 0.f ? 1.f : 0.f, DeltaSeconds, PoseBlendPerSecond);
	const bool bStandsReady = bFree && State == EFighterState::Idle && StunSecondsLeft <= 0.f && !Carried.IsValid();
	StanceAlpha = FMath::FInterpConstantTo(StanceAlpha, bStandsReady ? 1.f : 0.f, DeltaSeconds, StanceBlendPerSecond);
	CarryAlpha = FMath::FInterpConstantTo(CarryAlpha, Carried.IsValid() ? 1.f : 0.f, DeltaSeconds, PoseBlendPerSecond);
	ThrowSecondsLeft = FMath::Max(ThrowSecondsLeft - DeltaSeconds, 0.f);

	if (bDemoAllMoves && State != EFighterState::Busy)
	{
		DemoRestSeconds += DeltaSeconds;
		if (DemoRestSeconds > (State == EFighterState::Down ? 2.5f : 0.8f))
		{
			DemoRestSeconds = 0.f;
			DemoNextMove();
		}
	}

	if (DeltaSeconds > UE_KINDA_SMALL_NUMBER)
	{
		MovedPerSecond = (GetActorLocation() - LastTickLocation) / DeltaSeconds;
		LastTickLocation = GetActorLocation();
	}

	SwingBrokenArms(DeltaSeconds);
	const TArray<FVector> Joints = GetJointPositions();
	if (State != EFighterState::Down && JumpHeight <= 0.f)
	{
		WatchForFloating(Joints, DeltaSeconds, TEXT("in play"), 0.3f);
	}
	PoseStick(Joints, GetBrokenArms());
}

void AStickFighter::PlayMove(EMove Move)
{
	if (State == EFighterState::Down)
	{
		return;
	}

	if (Move == EMove::Punch || Move == EMove::Kick)
	{
		Attack(Move);
	}
	else if (Move == EMove::Block || Move == EMove::Duck)
	{
		HeldPose = Move;
		PoseSecondsLeft = PoseHoldSeconds;
	}
}

bool AStickFighter::Attack(EMove Move, float SecondsUntilContact)
{
	if (State == EFighterState::Down)
	{
		return false;
	}

	CaptureBlendFrom();
	UAnimSequenceBase* Strike = Move == EMove::Kick ? KickAnimation.Get() : NextPunch();
	if (!Strike)
	{
		return false;
	}
	ContactSeconds = FindContactSeconds(Strike);
	StrikeMove = Move;
	bFreeStrike = false;
	bKicking = false;
	bStrikeOnItsWay = true;

	const float Rate = SecondsUntilContact > 0.f ? FMath::Max(ContactSeconds / SecondsUntilContact, 0.1f) : 1.f;
	Play(Strike, false, Rate);
	State = EFighterState::Busy;
	return true;
}

UAnimSequenceBase* AStickFighter::NextPunch()
{
	for (int32 Tries = 0; Tries < PunchAnimations.Num(); ++Tries)
	{
		UAnimSequenceBase* Punch = PunchAnimations[PunchCount++ % PunchAnimations.Num()].Get();
		FindContactSeconds(Punch);
		const int32 Arm = ArmOfStrikeBone(StrikeBone);
		if (Arm == INDEX_NONE || !LooseArms[Arm].bBroken)
		{
			UE_CLOG((GetBrokenArms() & 3) != 0, LogStickFighter, Verbose, TEXT("%s strikes with %s, broken arms %d"), *GetName(), *StrikeBone.ToString(), GetBrokenArms() & 3);
			return Punch;
		}
	}
	return nullptr;
}

void AStickFighter::TakeHit(const FVector& StrikePoint, const FVector& StrikeDirection, bool bKnockout, bool bHeavy)
{
	if (State == EFighterState::Down)
	{
		return;
	}

	bStrikeOnItsWay = false;
	TintBody(FlashColor);
	FlashSecondsLeft = FlashSeconds;
	StartRecoil(StrikePoint, StrikeDirection, bHeavy ? HeavyRecoil : 1.f);

	if (bKnockout)
	{
		// The engine's death animations end on their feet and expect a ragdoll to finish the fall, so the fall is a ragdoll from the start.
		Body->SetCollisionProfileName(TEXT("Ragdoll"));
		Body->SetSimulatePhysics(true);
		Body->SetAllPhysicsLinearVelocity((RecoilDirection + FVector(0.f, 0.f, 0.4f)) * KnockoutSpeed * (bHeavy ? 1.3f : 1.f));
		State = EFighterState::Down;
	}
	// The demo shows the break on every heavy hit, whatever the health.
	else if (bHeavy && (bDemoAllMoves || Health <= MaxHealth * ArmBreaksBelowHealthShare))
	{
		const TArray<FVector> Joints = BuildJoints(true);
		const int32 Side = FVector::DistSquared(Joints[LeftElbow], LastImpactPoint) < FVector::DistSquared(Joints[RightElbow], LastImpactPoint) ? 0 : 1;
		FLooseArm& Arm = LooseArms[Side];
		if (!Arm.bBroken)
		{
			Arm.bBroken = true;
			UE_LOG(LogStickFighter, Verbose, TEXT("%s: the %s arm breaks at %.0f of %.0f health"), *GetName(), Side == 0 ? TEXT("left") : TEXT("right"), Health, MaxHealth);
			Arm.Hand = Joints[HandOfSide[Side]];
			Arm.Velocity = RecoilDirection * 350.f + FVector(0.f, 0.f, 150.f);
		}
	}
}

void AStickFighter::ReturnToIdle()
{
	if (State == EFighterState::Down)
	{
		Body->SetSimulatePhysics(false);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		// The snap does nothing when the body never detached, which would leave it where the ragdoll came to rest.
		Body->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, -90.f, 0.f));
	}

	CaptureBlendFrom();
	bStrikeOnItsWay = false;
	bKicking = false;
	PlayRest();
	State = EFighterState::Idle;
}

void AStickFighter::PlayRest()
{
	const bool bStance = bFree && StanceClip;
	Play(bStance ? StanceClip : IdleAnimation, true);
	if (bStance)
	{
		// Each fighter rides its stance from its own moment, so a row of them does not bounce as one.
		Body->SetPosition(StanceClip->GetPlayLength() * (GetUniqueID() % 7) / 7.f, false);
	}
}

void AStickFighter::FaceTowards(const FVector& Target)
{
	FacingYaw = (Target - GetActorLocation()).GetSafeNormal2D().Rotation().Yaw;
	bTurning = true;
}

void AStickFighter::MoveTo(const FVector& Target, float Seconds, const FVector& FaceAfter)
{
	FaceAfterMove = FaceAfter;
	const float Distance = FVector::Dist2D(GetActorLocation(), Target);
	if (State == EFighterState::Down || Distance < ArrivedDistance || Seconds <= 0.f)
	{
		FaceTowards(FaceAfter);
		return;
	}

	MoveFrom = GetActorLocation();
	MoveTarget = FVector(Target.X, Target.Y, MoveFrom.Z);
	MoveSeconds = Seconds;
	MoveElapsed = 0.f;
	bMoving = true;
	FaceTowards(MoveTarget);

	CaptureBlendFrom();
	bStrikeOnItsWay = false;
	// The legs keep pace with the ground covered, so a short dash and a long run both look right.
	Play(RunAnimation, true, FMath::Clamp(Distance / Seconds / RunAnimationSpeed, 0.8f, 2.2f));
	State = EFighterState::Moving;
}

void AStickFighter::SetHome(const FVector& Location, const FRotator& Rotation)
{
	bHasHome = true;
	HomeLocation = Location;
	HomeRotation = Rotation;
}

void AStickFighter::CaptureBlendFrom()
{
	const TArray<FVector> Joints = BuildJoints(true);
	const FTransform ToWorld = GetActorTransform();

	BlendFromLocal.Reset(Joints.Num());
	for (const FVector& Joint : Joints)
	{
		BlendFromLocal.Add(ToWorld.InverseTransformPosition(Joint));
	}
	BlendSecondsLeft = BlendSeconds;
}

void AStickFighter::SetColor(const FLinearColor& NewColor)
{
	Color = NewColor;
	for (UStaticMeshComponent* Part : Limbs)
	{
		Part->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color);
	}
	Head->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color);
	Smear->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color);
	Gun->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", FLinearColor(0.01f, 0.01f, 0.01f));
	for (int32 Disc = 0; Disc < Marks.Num(); ++Disc)
	{
		Marks[Disc]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Disc % 2 == 0 ? MarkYellow : MarkBlack);
	}
	TintBody(Color);
	for (UStaticMeshComponent* Rib : Ribs)
	{
		Rib->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", FlashColor);
	}
	for (int32 Index = 0; Index < BreakParts.Num(); ++Index)
	{
		BreakParts[Index]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Index % 2 == 0 ? BoneColor : BloodiedColor);
	}
	for (UStaticMeshComponent* Eye : Eyes)
	{
		// Dark on a light figure and light on a dark one, so they always show.
		Eye->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color.GetLuminance() > 0.25f ? FLinearColor(0.01f, 0.01f, 0.01f) : FLinearColor(0.9f, 0.9f, 0.9f));
	}
	TintedBrokenArms = 0;
}

void AStickFighter::HandleTakeStarted(int32 TakeNumber)
{
	LooseArms[0].bBroken = false;
	LooseArms[1].bBroken = false;
	Severed = 0;
	Health = MaxHealth;
	StunSecondsLeft = 0.f;
	KnockVelocity = FVector::ZeroVector;
	JumpHeight = 0.f;
	JumpSpeed = 0.f;
	WalkInput = FVector2D::ZeroVector;
	bWalking = false;
	bWalkingBackwards = false;
	Carried = nullptr;
	Hanging = nullptr;
	SwingKickSeconds = -1.f;
	bMayGrabABar = false;
	CarryAlpha = 0.f;
	ThrowSecondsLeft = 0.f;
	BlowsInARow = 0;
	bMoving = false;
	bTurning = false;
	ReturnToIdle();

	if (bHasHome)
	{
		SetActorLocationAndRotation(HomeLocation, HomeRotation);
		// Going back to one is a cut, not a move: nothing fades or smears across it.
		BlendSecondsLeft = 0.f;
		LastDrawnJoints.Reset();
	}
}

void AStickFighter::Play(UAnimSequenceBase* Animation, bool bLoop, float Rate)
{
	Body->PlayAnimation(Animation, bLoop);
	// The attacks carry root motion. Ignoring it keeps every fighter on its mark.
	Body->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	Body->SetPosition(0.f, false);
	Body->SetPlayRate(Rate);
	Playing = Animation;
	BusyAnimationSeconds = Animation->GetPlayLength();
}

// Contact is the moment a hand or foot reaches furthest forward, found by stepping through the animation.
float AStickFighter::FindContactSeconds(UAnimSequenceBase* Strike, int32 FirstBone, int32 BoneCount)
{
	if (const FContact* Known = Contacts.Find(Strike))
	{
		StrikeBone = Known->Bone;
		StrikeReach = Known->Reach;
		return Known->Seconds;
	}

	const FVector Forward = GetActorForwardVector();
	float BestSeconds = 0.f;
	double FurthestReach = -UE_BIG_NUMBER;

	Body->SetAnimation(Strike);
	for (float Seconds = 0.f; Seconds < Strike->GetPlayLength(); Seconds += 1.f / 30.f)
	{
		Body->SetPosition(Seconds, false);
		Body->TickAnimation(0.f, false);
		Body->RefreshBoneTransforms();
		for (const FName& Bone : MakeArrayView(StrikingBones + FirstBone, BoneCount))
		{
			const double Reach = (Body->GetBoneLocation(Bone) - GetActorLocation()) | Forward;
			if (Reach > FurthestReach)
			{
				FurthestReach = Reach;
				BestSeconds = Seconds;
				StrikeBone = Bone;
			}
		}
	}
	UE_LOG(LogStickFighter, Verbose, TEXT("%s: contact at %.2f s, reaching %.0f cm with %s"), *Strike->GetName(), BestSeconds, FurthestReach, *StrikeBone.ToString());
	StrikeReach = FurthestReach;
	Contacts.Add(Strike, { BestSeconds, StrikeBone, FurthestReach });
	return BestSeconds;
}

void AStickFighter::LandStrike()
{
	bStrikeOnItsWay = false;

	SecondsSinceContact = 0.f;
	const FTechnique* Posed = bKicking ? FindTechnique(StrikeMove) : nullptr;
	const bool bClip = Posed && bPosedByClip;
	// A clip takes its time coming back. It is hurried, so that most of the way back fits in the time the move has to recover.
	Body->SetPlayRate(bClip ? FMath::Clamp((BusyAnimationSeconds - Body->GetPosition()) * ClipShareShownComingBack / PosedRecoverSeconds, 1.f, MaxClipRate) : 1.f);
	const FVector StrikePoint = Posed && !bClip ? BuildJoints(true)[Posed->bWithALeg ? RightFoot : HandOfSide[PosedSide]] : Body->GetBoneLocation(StrikeBone);
	// A move played only to be looked at has nobody to land on.
	if (bFreeStrike)
	{
		LandFreeStrike(StrikePoint);
	}
}

void AStickFighter::StartRecoil(const FVector& StrikePoint, const FVector& StrikeDirection, float Strength)
{
	const TArray<FVector> Joints = BuildJoints(true);

	int32 Struck = 0;
	for (int32 Index = 1; Index < JointCount; ++Index)
	{
		if (FVector::DistSquared(Joints[Index], StrikePoint) < FVector::DistSquared(Joints[Struck], StrikePoint))
		{
			Struck = Index;
		}
	}

	LastImpactPoint = Joints[Struck];
	RecoilDirection = StrikeDirection.GetSafeNormal2D();
	RecoilStrength = Strength;
	RecoilProgress = 0.f;

	const double PelvisHeight = Joints[PelvisJoint].Z;
	const double Height = FMath::Max(Joints[SkullJoint].Z - PelvisHeight, 1.);
	for (int32 Index = 0; Index < JointCount; ++Index)
	{
		const float Near = static_cast<float>(FMath::Exp(-FMath::Square(FVector::Dist(Joints[Index], Joints[Struck]) / RecoilReach)));
		const float Lift = static_cast<float>(FMath::Clamp((Joints[Index].Z - PelvisHeight) / Height, 0., 1.));
		// The whole upper body gives a little, the struck part gives a lot, and the legs stay planted.
		RecoilWeights[Index] = Index >= LeftHip ? 0.25f * Near : 0.45f * Lift + Near;
	}
}

// A broken forearm is a pendulum hung from the elbow: gravity pulls the hand down and the elbow's own motion swings it.
void AStickFighter::SwingBrokenArms(float DeltaSeconds)
{
	if (State == EFighterState::Down || DeltaSeconds < UE_KINDA_SMALL_NUMBER || (!LooseArms[0].bBroken && !LooseArms[1].bBroken))
	{
		return;
	}

	const TArray<FVector> Joints = BuildJoints(false);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FLooseArm& Arm = LooseArms[Side];
		if (!Arm.bBroken)
		{
			continue;
		}

		const FVector Elbow = Joints[ElbowOfSide[Side]];
		const double Length = FVector::Dist(Elbow, Joints[HandOfSide[Side]]);

		Arm.Velocity += FVector(0., 0., -980.) * DeltaSeconds;
		Arm.Velocity *= FMath::Exp(-LooseArmDamping * DeltaSeconds);
		const FVector Swung = Elbow + (Arm.Hand + Arm.Velocity * DeltaSeconds - Elbow).GetSafeNormal() * Length;
		Arm.Velocity = ((Swung - Arm.Hand) / DeltaSeconds).GetClampedToMaxSize(LooseArmMaxSpeed);
		Arm.Hand = Swung;
	}
}

void AStickFighter::DemoNextMove()
{
	if (State == EFighterState::Down)
	{
		HandleTakeStarted(0);
		return;
	}

	const FVector InFront = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 140.f);
	const FVector TowardsMe = -GetActorForwardVector();
	switch (DemoStep++ % 9)
	{
	case 0:
	case 1:
	case 2:
		PlayMove(EMove::Punch);
		break;
	case 3:
		PlayMove(EMove::Kick);
		break;
	case 4:
		PlayMove(EMove::Duck);
		break;
	case 5:
		PlayMove(EMove::Block);
		break;
	case 6:
		TakeHit(InFront, TowardsMe, false, false);
		break;
	case 7:
		TakeHit(InFront, TowardsMe, false, true);
		break;
	default:
		TakeHit(InFront, TowardsMe, true, false);
		break;
	}
}

TArray<FVector> AStickFighter::BuildJoints(bool bWithBrokenArms) const
{
	if (bPuppet)
	{
		return PuppetPose.Joints;
	}

	TArray<FVector> Joints;
	Joints.Reserve(JointCount);
	for (const FName& Bone : JointBones)
	{
		Joints.Add(Body->GetBoneLocation(Bone));
	}

	// Knocked out, the ragdoll alone decides where every joint is.
	if (State == EFighterState::Down)
	{
		return Joints;
	}

	if (const ASwingBar* Bar = Hanging.Get())
	{
		const FVector Grip(Bar->GetActorLocation().X, GetActorLocation().Y, Bar->GetActorLocation().Z);
		const bool bKickingOff = SwingKickSeconds >= 0.f;
		const double LegsOut = bKickingOff ? FMath::SmoothStep(0., 1., (FMath::Clamp(static_cast<double>(SwingKickSeconds / SwingKickContactSeconds), 0., 1.) - 0.3) / 0.6) : 0.;
		// Swinging one way, the legs are still on their way from the other.
		const double LegTrail = bKickingOff ? 0. : FMath::Clamp(-FMath::RadiansToDegrees(SwingSpeed) * HangLegTrailSeconds * GetActorForwardVector().X, -HangLegTrailLimit, HangLegTrailLimit);
		PoseHang(Joints, GetActorForwardVector(), Grip, FMath::RadiansToDegrees(SwingAngle), LegsOut, LegTrail);
	}

	if (BlendSecondsLeft > 0.f && BlendFromLocal.Num() == Joints.Num())
	{
		const float Alpha = FMath::SmoothStep(0.f, 1.f, 1.f - BlendSecondsLeft / BlendSeconds);
		const FTransform ToWorld = GetActorTransform();
		for (int32 Index = 0; Index < JointCount; ++Index)
		{
			Joints[Index] = FMath::Lerp(ToWorld.TransformPosition(BlendFromLocal[Index]), Joints[Index], Alpha);
		}
	}

	// A fighter who is standing still keeps its hands up, instead of letting them hang as the idle animation does.
	if (StanceAlpha > 0.f && Playing != StanceClip)
	{
		// A fighter waiting for an opening is never quite still: it rides its knees, each fighter at its own moment.
		const double Bob = 0.5 + 0.5 * FMath::Sin(GetWorld()->GetTimeSeconds() * StanceBobPerSecond + GetUniqueID() % 7);
		PoseDuck(Joints, GetActorForwardVector(), StanceBobDepth * StanceAlpha * Bob);
		PoseArms(Joints, GetActorForwardVector(), 0.8 * StanceAlpha, 24., 11., -8.);
	}

	if (LandSquashSecondsLeft > 0.f)
	{
		// Coming down from a jump, the knees take the weight for a moment.
		PoseDuck(Joints, GetActorForwardVector(), LandSquashDepth * FMath::Sin(UE_PI * LandSquashSecondsLeft / LandSquashSeconds));
	}

	if (PoseAlpha > 0.f)
	{
		if (HeldPose == EMove::Duck)
		{
			PoseDuck(Joints, GetActorForwardVector(), PoseAlpha);
		}
		else
		{
			PoseArms(Joints, GetActorForwardVector(), PoseAlpha, 30., 10., 6.);
		}
	}

	// Not during a strike: a carried prop then follows the fists, which is what makes a swing of it.
	if (CarryAlpha > 0.f && State != EFighterState::Busy)
	{
		// A crate is carried overhead. A weapon is held ready in front of the chest.
		const EPropHold Hold = Carried.IsValid() ? Carried->GetHold() : EPropHold::Overhead;
		const FVector Ahead = GetActorForwardVector();
		if (Hold == EPropHold::LongGun || Hold == EPropHold::OneHand)
		{
			// Each arm has its own place here, so they are posed on a copy and eased in like the other holds.
			TArray<FVector> Held = Joints;
			const FLimbs Reach(Held);
			const FVector Outward = FVector::UpVector ^ Ahead;
			if (Hold == EPropHold::LongGun)
			{
				// Shouldered: the right hand at the grip by the chest, the left forward under the barrel.
				const FVector Grip = Held[RightShoulder] + Ahead * 16. - FVector::UpVector * 15. - Outward * 5.;
				ReachWithArm(Held, Reach, 1, Grip, Outward * 0.7 - FVector::UpVector);
				ReachWithArm(Held, Reach, 0, Grip + Ahead * 36. + FVector::UpVector * 2., -Outward * 0.3 - FVector::UpVector);
			}
			else
			{
				// In one hand, raised beside the head, ready to throw. The other arm is left to the stance.
				ReachWithArm(Held, Reach, 1, Held[RightShoulder] + Ahead * 4. + FVector::UpVector * 20. + Outward * 12., Outward * 0.5 - Ahead - FVector::UpVector * 0.5);
			}
			for (const int32 Joint : { LeftElbow, LeftHand, RightElbow, RightHand })
			{
				Joints[Joint] = FMath::Lerp(Joints[Joint], Held[Joint], static_cast<double>(CarryAlpha));
			}
		}
		else if (Hold == EPropHold::Pistol)
		{
			// In both hands, out in front and a little under the eye, with the elbows bent.
			PoseArms(Joints, Ahead, CarryAlpha, 38., 2.5, -12.);
		}
		else if (Hold == EPropHold::Swung)
		{
			PoseArms(Joints, Ahead, CarryAlpha, 24., 5., -20.);
		}
		else
		{
			PoseArms(Joints, Ahead, CarryAlpha, 6., 15., 32.);
		}
	}

	if (ThrowSecondsLeft > 0.f && State != EFighterState::Busy)
	{
		// The arms that held the prop overhead come down and forward to hurl it, then let the pose go.
		const double Done = 1. - ThrowSecondsLeft / ThrowGestureSeconds;
		const double Swing = FMath::Min(Done * 2.5, 1.);
		// The body goes with the throw: it leans into it as the arms come over, and straightens again.
		LeanBody(Joints, GetActorForwardVector(), ThrowLeanRadians * FMath::Sin(UE_PI * FMath::Min(Done * 1.3, 1.)), 0.);
		PoseArms(Joints, GetActorForwardVector(), Done < 0.6 ? 1. : (1. - Done) / 0.4, FMath::Lerp(4., 55., Swing), 14., FMath::Lerp(36., -25., Swing));
	}

	if (bKicking && !bPosedByClip)
	{
		// 0 to 1 while the strike is on its way, 1 to 2 while it is taken back.
		const float Phase = KickElapsed < KickContactAt ? KickElapsed / KickContactAt : 1.f + FMath::Min((KickElapsed - KickContactAt) / PosedRecoverSeconds, 1.f);
		PoseTechnique(Joints, GetActorForwardVector(), StrikeMove, Phase, PosedSide);
	}

	if (bAiming)
	{
		const double Upper = FVector::Dist(Joints[RightShoulder], Joints[RightElbow]);
		const double Lower = FVector::Dist(Joints[RightElbow], Joints[RightHand]);
		const FVector Towards = (AimPoint - Joints[RightShoulder]).GetSafeNormal();
		Joints[RightHand] = Joints[RightShoulder] + Towards * ((Upper + Lower) * 0.96);
		Joints[RightElbow] = MiddleJoint(Joints[RightShoulder], Joints[RightHand], Upper, Lower, -FVector::UpVector);
	}

	if (RecoilProgress < 1.f)
	{
		// The engine's hit reactions are additive and cannot play alone, so the flinch is done here, joint by joint.
		const FVector Knock = RecoilDirection * RecoilDistance * RecoilStrength * FMath::Sin(UE_PI * FMath::Sqrt(RecoilProgress));
		for (int32 Index = 0; Index < JointCount; ++Index)
		{
			Joints[Index] += Knock * RecoilWeights[Index];
		}
	}

	if (bWithBrokenArms)
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			if (LooseArms[Side].bBroken)
			{
				Joints[HandOfSide[Side]] = LooseArms[Side].Hand;
			}
		}
	}
	return Joints;
}

uint8 AStickFighter::GetBrokenArms() const
{
	if (bPuppet)
	{
		return PuppetPose.BrokenArms;
	}
	// What a gun took off stays off, on a body that is down too. The rest is only told of someone on their feet.
	const uint8 Gone = static_cast<uint8>(Severed << 3);
	if (State == EFighterState::Down)
	{
		return Gone;
	}
	return static_cast<uint8>((LooseArms[0].bBroken ? 1 : 0) | (LooseArms[1].bBroken ? 2 : 0) | (bArmed ? 4 : 0) | Gone);
}

void AStickFighter::SetPuppetPose(const FFighterPose& Pose)
{
	bPuppet = true;
	PuppetPose = Pose;
}

void AStickFighter::EndPuppet()
{
	bPuppet = false;
	bPuppetDown = false;
}

void AStickFighter::MarkStruck(const FVector& ImpactPoint, bool bKnockedOut)
{
	LastImpactPoint = ImpactPoint;
	bPuppetDown = bKnockedOut;
	TintBody(FlashColor);
	FlashSecondsLeft = FlashSeconds;
}

void AStickFighter::PoseStick(const TArray<FVector>& Joints, uint8 BrokenArms)
{
	const bool bBones = FlashSecondsLeft > 0.f;
	// Limbs a gun took off. They ride in the same byte as the broken arms, so the film has them too.
	const uint8 SeveredNow = static_cast<uint8>(BrokenArms >> 3);
	const int32 SeveredJoints[4][2] = { { LeftElbow, LeftHand }, { RightElbow, RightHand }, { LeftKnee, LeftFoot }, { RightKnee, RightFoot } };
	bool bJustLost = false;
	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		const bool bGone = (SeveredNow & (1 << Limb)) != 0;
		if (bGone != ((DrawnSevered & (1 << Limb)) != 0))
		{
			// It has just come off, or a new take or the film has put it back.
			FFlyingLimb& Flying = FlyingLimbs[Limb];
			Flying.bFlying = bGone;
			LooseLimbs[Limb * 2]->SetVisibility(bGone);
			LooseLimbs[Limb * 2 + 1]->SetVisibility(bGone);
			if (bGone)
			{
				const FVector From = Joints[SeveredJoints[Limb][0]];
				const FVector To = Joints[SeveredJoints[Limb][1]];
				Flying.Middle = (From + To) * 0.5;
				Flying.Along = (To - From).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				Flying.Length = FVector::Dist(From, To);
				Flying.Velocity = FMath::VRand() * 160. + FVector(0., 0., 330.) + (Flying.Middle - Joints[PelvisJoint]).GetSafeNormal2D() * 260.;
				LooseLimbs[Limb * 2]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Color);
				LooseLimbs[Limb * 2 + 1]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", RubberFor(Color));
				Stumps[Limb]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", StumpRed);
				bJustLost = true;
			}
		}
		Stumps[Limb]->SetVisibility(bGone);
		if (bGone)
		{
			Stumps[Limb]->SetWorldLocation(Joints[SeveredJoints[Limb][0]]);
			Stumps[Limb]->SetWorldScale3D(FVector(Thickness * 1.25f / 100.f));
		}
	}
	DrawnSevered = SeveredNow;
	if (bJustLost)
	{
		// Put where they start at once, not where they lay after the take before.
		FlyLooseLimbs(0.f);
	}

	// With a model for a body, the plain shapes are only the skeleton that shows through it at a blow.
	const bool bModel = Model->HasBody();
	const bool bShapes = bBones || !bModel;
	Model->SetVisibility(bModel && !bBones);
	if (bModel && !bBones)
	{
		Model->PoseOn(Joints, FloorHeight, FMath::Clamp(Thickness / UsualThickness, SlightestGirth, StoutestGirth), SeveredNow);
	}
	for (int32 i = 0; i < Limbs.Num(); ++i)
	{
		Limbs[i]->SetVisibility(bShapes);
		PlaceLimb(Limbs[i], Joints[LimbJoints[i][0]], Joints[LimbJoints[i][1]], Thickness * (bBones ? BoneWidth : LimbWidths[i]));
	}

	// The dummy's body goes round the spine. The skeleton flash shows what is under it, so it steps aside for that.
	const FVector Spine = (Joints[NeckJoint] - Joints[PelvisJoint]).GetSafeNormal();
	const FVector ShoulderSpan = Joints[RightShoulder] - Joints[LeftShoulder];
	const FVector HipSpan = Joints[RightHip] - Joints[LeftHip];
	const FVector Front = (ShoulderSpan ^ Spine).GetSafeNormal();
	const FRotator Upright = FRotationMatrix::MakeFromZY(Spine, ShoulderSpan).Rotator();
	const double Deep = Thickness * 2.3;
	PlaceBlob(BodyParts[ChestPart], FMath::Lerp(Joints[ChestJoint], Joints[NeckJoint], 0.4), Upright,
		FVector(Deep, ShoulderSpan.Size() + Thickness * 1.6, FVector::Dist(Joints[ChestJoint], Joints[NeckJoint]) * 1.5 + Thickness));
	PlaceBlob(BodyParts[WaistPart], FMath::Lerp(Joints[PelvisJoint], Joints[ChestJoint], 0.55), Upright,
		FVector(Deep * 0.85, ShoulderSpan.Size() * 0.6 + Thickness * 1.2, FVector::Dist(Joints[PelvisJoint], Joints[ChestJoint]) * 1.25));
	PlaceBlob(BodyParts[HipsPart], Joints[PelvisJoint], FRotationMatrix::MakeFromZY(Spine, HipSpan).Rotator(),
		FVector(Deep * 0.95, HipSpan.Size() + Thickness * 2.4, Thickness * 2.6));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		// A boot stands square to its shin and points the way the body faces, so it follows the leg through a kick.
		const FVector Foot = Joints[Side == 0 ? LeftFoot : RightFoot];
		const FVector Shin = (Foot - Joints[Side == 0 ? LeftKnee : RightKnee]).GetSafeNormal();
		const FVector Toe = (Front - Shin * (Front | Shin)).GetSafeNormal(UE_SMALL_NUMBER, Front);
		PlaceBlob(BodyParts[LeftBoot + Side], Foot + Toe * Thickness * 0.9, FRotationMatrix::MakeFromXZ(Toe, -Shin).Rotator(),
			FVector(Thickness * 3.2, Thickness * 1.6, Thickness * 1.5));
	}
	for (UStaticMeshComponent* Part : BodyParts)
	{
		Part->SetVisibility(!bBones && !bModel);
	}
	for (int32 Ball = 0; Ball < JointBalls.Num(); ++Ball)
	{
		const int32 Joint = FirstBallJoint + Ball;
		const bool bEnd = Joint == LeftHand || Joint == RightHand || Joint == LeftFoot || Joint == RightFoot;
		JointBalls[Ball]->SetVisibility(bShapes);
		JointBalls[Ball]->SetWorldLocation(Joints[Joint]);
		JointBalls[Ball]->SetWorldScale3D(FVector(Thickness * (bBones ? BoneJointWidth : bEnd ? HandAndFootWidth : JointBallWidth) / 100.f));
	}

	const FVector ShoulderLine = (Joints[RightShoulder] - Joints[LeftShoulder]).GetSafeNormal();
	for (int32 Rib = 0; Rib < Ribs.Num(); ++Rib)
	{
		Ribs[Rib]->SetVisibility(bBones);
		if (bBones)
		{
			const FVector At = FMath::Lerp(Joints[ChestJoint], Joints[PelvisJoint], 0.12 + Rib * 0.24);
			const FVector Half = ShoulderLine * (12. - Rib * 2.);
			PlaceLimb(Ribs[Rib], At - Half, At + Half, Thickness * BoneWidth);
		}
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		// An arm that is gone has no break to show.
		const bool bBroken = (BrokenArms & (1 << Side)) != 0 && (SeveredNow & (1 << Side)) == 0;
		const bool bDrawnBroken = (TintedBrokenArms & (1 << Side)) != 0;
		UStaticMeshComponent* Forearm = Limbs[ForearmLimbOfSide[Side]];
		UStaticMeshComponent* Stump = ForearmStumps[Side];

		// Set every time, not only on a change: a knocked-out fighter stops reporting its breaks, and what was
		// showing a moment ago must not be left hanging in the air.
		Stump->SetVisibility(bBroken && bShapes);
		BreakParts[Side * 2]->SetVisibility(bBroken);
		BreakParts[Side * 2 + 1]->SetVisibility(bBroken);

		if (bBroken != bDrawnBroken)
		{
			// The arm keeps its colour: the break is shown by the bone and the wound, not by painting the arm red.
			const FLinearColor Tint = Color;
			Forearm->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Tint);
			Stump->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Tint);
		}

		if (bBroken)
		{
			// A broken forearm is drawn in two pieces that meet at an angle.
			const FVector Elbow = Joints[ElbowOfSide[Side]];
			const FVector Hand = Joints[HandOfSide[Side]];
			const FVector Sideways = ((Hand - Elbow) ^ GetActorForwardVector()).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
			const FVector Break = (Elbow + Hand) * 0.5f + Sideways * BreakKink;
			PlaceLimb(Forearm, Elbow, Break, Thickness);
			PlaceLimb(Stump, Break, Hand, Thickness);
			PlaceLimb(BreakParts[Side * 2], Break, Break + (Break - Elbow).GetSafeNormal() * BoneLength, Thickness * 0.42f);
			BreakParts[Side * 2 + 1]->SetWorldLocation(Break);
			BreakParts[Side * 2 + 1]->SetWorldScale3D(FVector(Thickness * 1.3f / 100.f));
		}
	}
	TintedBrokenArms = BrokenArms;

	const bool bHoldsGun = (BrokenArms & 4) != 0;
	Gun->SetVisibility(bHoldsGun);
	if (bHoldsGun)
	{
		const FVector Barrel = (Joints[RightHand] - Joints[RightElbow]).GetSafeNormal();
		Gun->SetWorldLocationAndRotation(Joints[RightHand] + Barrel * 9., Barrel.Rotation());
		Gun->SetWorldScale3D(FVector(0.2, 0.05, 0.11));
	}

	// The smear joins where the fastest hand or foot was last frame to where it is now.
	const float Now = GetWorld()->GetTimeSeconds();
	const float FrameSeconds = Now - LastDrawnSeconds;
	int32 Fastest = INDEX_NONE;
	if (LastDrawnJoints.Num() == Joints.Num() && FrameSeconds > UE_KINDA_SMALL_NUMBER && FrameSeconds < 0.1f)
	{
		double FastestSpeed = SmearSpeed;
		for (const int32 Index : { LeftHand, RightHand, LeftFoot, RightFoot })
		{
			const double Speed = FVector::Dist(Joints[Index], LastDrawnJoints[Index]) / FrameSeconds;
			if (Speed > FastestSpeed)
			{
				FastestSpeed = Speed;
				Fastest = Index;
			}
		}
	}
	Smear->SetVisibility(Fastest != INDEX_NONE);
	if (Fastest != INDEX_NONE)
	{
		PlaceLimb(Smear, LastDrawnJoints[Fastest], Joints[Fastest], Thickness * 1.2f);
	}
	LastDrawnJoints = Joints;
	LastDrawnSeconds = Now;

	const FVector Up = (Joints[SkullJoint] - Joints[NeckJoint]).GetSafeNormal();
	Head->SetVisibility(bShapes);
	Head->SetWorldLocation(Joints[SkullJoint] + Up * HeadRadius);
	Head->SetWorldScale3D(FVector(HeadRadius / 50.f));

	// Which way the head faces comes from the shoulders, not from the actor, so a puppet in the film faces right too.
	const FVector Ahead = ((Joints[RightShoulder] - Joints[LeftShoulder]) ^ Up).GetSafeNormal();
	const FVector Middle = Joints[SkullJoint] + Up * HeadRadius;
	const FVector Across = Up ^ Ahead;
	for (int32 Eye = 0; Eye < Eyes.Num(); ++Eye)
	{
		Eyes[Eye]->SetVisibility(bShapes);
		Eyes[Eye]->SetWorldLocation(Middle + (Ahead * 0.86 + Across * (Eye == 0 ? -0.36 : 0.36) + Up * 0.1) * HeadRadius);
		// In the skeleton flash the eyes are the skull's sockets.
		Eyes[Eye]->SetWorldScale3D(FVector(HeadRadius * (bBones ? 0.62 : 0.32) / 100.));
	}

	// The marks every crash dummy wears: yellow with a black centre, on the chest and on the outside of each shoulder.
	// Not on the head: the fight is watched from the side, and there a mark on the temple reads as a huge eye.
	const FVector ChestMiddle = FMath::Lerp(Joints[ChestJoint], Joints[NeckJoint], 0.4) + Front * Deep * 0.5;
	const FVector Outward = ShoulderSpan.GetSafeNormal();
	const double ShoulderBall = Thickness * JointBallWidth * 0.5;
	const FVector MarkAt[] = { ChestMiddle, Joints[RightShoulder] + Outward * ShoulderBall, Joints[LeftShoulder] - Outward * ShoulderBall };
	const FVector MarkFaces[] = { Front, Outward, -Outward };
	const double MarkRadius[] = { Thickness * 0.85, Thickness * 0.5, Thickness * 0.5 };
	for (int32 Mark = 0; Mark < 3; ++Mark)
	{
		PlaceDisc(Marks[Mark * 2], MarkAt[Mark], MarkFaces[Mark], MarkRadius[Mark]);
		PlaceDisc(Marks[Mark * 2 + 1], MarkAt[Mark] + MarkFaces[Mark] * 0.3, MarkFaces[Mark], MarkRadius[Mark] * 0.5);
		Marks[Mark * 2]->SetVisibility(!bBones && !bModel);
		Marks[Mark * 2 + 1]->SetVisibility(!bBones && !bModel);
	}

	if (Hat != 0)
	{
		const FRotator Tilt = FRotationMatrix::MakeFromZX(Up, Ahead).Rotator();
		for (int32 Piece = 0; Piece < 2; ++Piece)
		{
			const FHatPiece& Shape = Hats[Hat][Piece];
			if (Shape.bUsed)
			{
				HatParts[Piece]->SetWorldLocationAndRotation(Middle + (Ahead * Shape.Ahead + Up * Shape.Above) * HeadRadius, Tilt);
				HatParts[Piece]->SetWorldScale3D(Shape.Size * (HeadRadius / 100.));
			}
		}
	}
}

void AStickFighter::SetArena(const FVector2D& Min, const FVector2D& Max)
{
	bFree = true;
	ArenaMin = Min;
	ArenaMax = Max;
	FloorHeight = GetActorLocation().Z;
	Health = MaxHealth;
}

FVector2D AStickFighter::ClampToArena(const FVector2D& Point) const
{
	return bFree ? FVector2D(FMath::Clamp(Point.X, ArenaMin.X, ArenaMax.X), FMath::Clamp(Point.Y, ArenaMin.Y, ArenaMax.Y)) : Point;
}

void AStickFighter::Jump()
{
	if (Hanging.IsValid())
	{
		SwingOffBar();
		return;
	}

	// A jump may come out of the tail of a strike, at the moment another strike could.
	if (State == EFighterState::Busy && CanAct())
	{
		ReturnToIdle();
	}
	const bool bOnItsFeet = State == EFighterState::Idle || State == EFighterState::Moving;
	if (!bFree || !bOnItsFeet || bMoving || JumpHeight > 0.f || StunSecondsLeft > 0.f)
	{
		return;
	}

	UE_LOG(LogStickFighter, Verbose, TEXT("%s jumps"), *GetName());
	JumpSpeed = JumpLaunchSpeed;
	bMayGrabABar = true;
	JumpHeight = UE_KINDA_SMALL_NUMBER;
	bWalking = false;
	CaptureBlendFrom();
	Play(FallAnimation, true);
	State = EFighterState::Moving;
}

void AStickFighter::Guard(EMove Pose)
{
	if (Hanging.IsValid())
	{
		return;
	}

	if (State == EFighterState::Idle || (State == EFighterState::Moving && !bMoving))
	{
		HeldPose = Pose;
		PoseSecondsLeft = StunSecondsLeft > 0.f ? 0.f : GuardHoldSeconds;
	}
}

bool AStickFighter::CanAct() const
{
	if (bPuppet || bMoving || StunSecondsLeft > 0.f)
	{
		return false;
	}
	if (State == EFighterState::Idle || State == EFighterState::Moving)
	{
		return true;
	}
	return State == EFighterState::Busy && !bStrikeOnItsWay && SecondsSinceContact >= (StrikeMove == EMove::Punch ? CancelSeconds : KickCancelSeconds);
}

bool AStickFighter::Strike(EMove Move, float SecondsUntilContact)
{
	if (Move != EMove::Punch && Move != EMove::Kick)
	{
		return false;
	}
	// On its feet and empty-handed, what the fighter is holding turns the plain strike into another.
	if (JumpHeight <= 0.f && !Carried.IsValid())
	{
		if (IsDucking())
		{
			return StrikeWith(Move == EMove::Punch ? EMove::Uppercut : EMove::Sweep, 0.f);
		}
		if (IsGuarding())
		{
			return StrikeWith(Move == EMove::Punch ? EMove::Chop : EMove::Roundhouse, 0.f);
		}
	}
	return StrikeWith(Move, SecondsUntilContact);
}

bool AStickFighter::StrikeWith(EMove Move, float SecondsUntilContact)
{
	// From a bar there is one strike, whatever was asked for.
	if (Hanging.IsValid())
	{
		return StartSwingKick();
	}

	const FTechnique* Posed = FindTechnique(Move);
	if (!CanAct() || (Move != EMove::Punch && !Posed))
	{
		return false;
	}
	// A gun in the hands is fired by the punch. A kick with one in the hands is still a kick.
	if (Carried.IsValid() && Carried->IsGun())
	{
		if (Move == EMove::Punch)
		{
			return FireGun();
		}
	}
	// With anything else in its hands, a strike is a throw.
	// A legendary prop is kept in the hands and swung. E still throws it.
	else if (Carried.IsValid() && !Carried->IsSwung())
	{
		return GrabOrThrow();
	}

	// An arm that hangs loose throws no uppercut and no chop. The other arm does, and with neither it is a plain strike.
	if (Posed && !Posed->bWithALeg)
	{
		PosedSide = LooseArms[1].bBroken ? 0 : 1;
		if (LooseArms[PosedSide].bBroken)
		{
			Move = EMove::Punch;
			Posed = nullptr;
		}
	}

	// The jab and cross take both arms. With one hanging loose, or a weapon in the hands, the punch is the plain one.
	if (Move == EMove::Punch && (LooseArms[0].bBroken || LooseArms[1].bBroken || Carried.IsValid() || !TechniqueClips.Contains(EMove::Punch)))
	{
		Posed = nullptr;
	}

	PoseSecondsLeft = 0.f;
	SnapToWantedFacing();
	// With no sound arm to punch with, a punch comes out as a kick.
	if (Move == EMove::Punch && !Posed && !Attack(Move, SecondsUntilContact))
	{
		UE_LOG(LogStickFighter, Verbose, TEXT("%s has no sound arm to punch with, and kicks"), *GetName());
		Move = EMove::Kick;
		Posed = FindTechnique(Move);
	}
	if (Posed)
	{
		SecondsUntilContact = SecondsUntilContact > 0.f ? SecondsUntilContact : Posed->WindUpSeconds;
		StartPosed(Move, SecondsUntilContact);
	}
	bFreeStrike = true;
	bStrikeFromAir = JumpHeight > OutOfReachHeight * 0.5f;

	// The fighter steps into its strike, or back from a target that is too close, so the blow arrives on a body:
	// not in the air in front of one, and not through it.
	LungeVelocity = FVector::ZeroVector;
	const FVector Facing = WantedForward();
	double Nearest = StrikeReach + BodyRadius + LungeDistance;
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		const FVector To = (It->GetActorLocation() - GetActorLocation()) * FVector(1., 1., 0.);
		const double Distance = To.Size();
		if (It->Team != Team && !It->IsDown() && Distance < Nearest && (To.GetSafeNormal() | Facing) > StrikeConeCosine)
		{
			Nearest = Distance;
			LungeVelocity = To.GetSafeNormal() * (FMath::Max(Distance - StrikeReach - TouchGap, -StepBackDistance) / FMath::Max(SecondsUntilContact, 0.05f));
		}
	}
	GetWorld()->GetSubsystem<UTakeEvents>()->OnSwing.Broadcast(this, Move);
	return true;
}

void AStickFighter::LandFreeStrike(const FVector& StrikePoint)
{
	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	const FTechnique* Posed = bKicking ? FindTechnique(StrikeMove) : nullptr;
	const bool bWithALeg = Posed && Posed->bWithALeg;
	const double ConeCosine = Posed ? Posed->ConeCosine : StrikeConeCosine;
	const FVector Forward = GetActorForwardVector();
	const FVector From = GetActorLocation();
	AThrownProp* Weapon = Carried.Get();
	const bool bLegendary = Weapon && Weapon->IsLegendary();
	const bool bWithAWeapon = Weapon && Weapon->IsSwung();
	// For the log, when the strike lands on nobody: how near it came.
	double NearestMiss = UE_BIG_NUMBER;
	bool bReachedSomeone = false;

	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		AStickFighter* Other = *It;
		if (Other == this || Other->Team == Team || Other->IsDown())
		{
			continue;
		}

		const FVector To = (Other->GetActorLocation() - From) * FVector(1., 1., 0.);
		NearestMiss = FMath::Min(NearestMiss, To.Size());
		if (To.Size() > StrikeReach + BodyRadius + (bWithAWeapon ? LegendaryReach : 0.) || (To.GetSafeNormal() | Forward) < ConeCosine)
		{
			continue;
		}
		// A punch and a high kick pass over a ducked head, and a fighter in the air is only reached by a strike thrown from the air.
		const bool bPassesOverADuck = Posed ? Posed->bHigh : true;
		if ((bPassesOverADuck && Other->IsDucking()) || (Other->JumpHeight > OutOfReachHeight && !bStrikeFromAir))
		{
			continue;
		}

		if (Other->IsGuarding() && (Other->GetActorForwardVector() | Forward) < 0. && !(Posed && Posed->bThroughGuard))
		{
			Other->StartRecoil(StrikePoint, Forward, BlockedRecoil);
			Other->KnockVelocity = Forward * BlockedKnockSpeed;
			Events->OnBlocked.Broadcast(this, Other);
		}
		else
		{
			const float Damage = bWithAWeapon ? Weapon->GetSwingDamage() : (bWithALeg ? KickDamage : PunchDamage) * (Posed ? Posed->DamageScale : 1.f) * (bStrikeFromAir ? AirStrikeDamageScale : 1.f);

			// What the rules decided against what the picture shows: how far the fist or foot is from the body it is about to hurt.
			double Gap = UE_BIG_NUMBER;
			for (const FVector& Joint : Other->GetJointPositions())
			{
				Gap = FMath::Min(Gap, FVector::Dist(Joint, StrikePoint));
			}
			UE_LOG(LogStickFighter, Verbose, TEXT("Blow: %s lands %s on %s, bodies %.0f cm apart, reach %.0f, limb stopped %.0f cm from the body"),
				*GetName(), *UEnum::GetValueAsString(StrikeMove), *Other->GetName(), To.Size(), StrikeReach, Gap);
			Other->SufferBlow(this, Damage, StrikePoint, Forward, IsHeavyMove(StrikeMove) || bStrikeFromAir || bWithAWeapon, StrikeMove);
			if (Posed && !Other->IsDown())
			{
				// The crew are stuntmen and pull their blows: the player is held helpless for half as long as they are.
				const float Helpless = Posed->StunSeconds * (Other->Team == 0 ? 0.5f : 1.f);
				if (Posed->PopSpeed > 0.f)
				{
					Other->PopUp(Posed->PopSpeed, Helpless);
				}
				Other->StunSecondsLeft = FMath::Max(Other->StunSecondsLeft, Helpless);
			}
			if (bLegendary)
			{
				Other->Launch(Forward, LegendaryLaunchSpeed);
			}
			if (bWithAWeapon && Carried.IsValid() && Weapon->SpendUse())
			{
				Carried = nullptr;
			}
		}

		bReachedSomeone = true;
		// A hand has one target. A leg goes through everyone in its way.
		if (!bWithALeg && !bWithAWeapon)
		{
			break;
		}
	}

	UE_CLOG(!bReachedSomeone, LogStickFighter, Verbose, TEXT("Miss: %s lands %s on nobody. Nearest opponent %.0f cm away, reach %.0f plus %.0f"),
		*GetName(), *UEnum::GetValueAsString(StrikeMove), NearestMiss, StrikeReach, BodyRadius);
}

void AStickFighter::SufferBlow(AStickFighter* Attacker, float Damage, const FVector& Point, const FVector& Direction, bool bHeavy, EMove Move)
{
	if (IsDown())
	{
		return;
	}

	TakeBlow(Damage, Point, Direction, bHeavy);
	UTakeEvents* Events = GetWorld()->GetSubsystem<UTakeEvents>();
	Events->OnHit.Broadcast(Attacker, this, Move);
	if (IsDown())
	{
		Events->OnFighterDown.Broadcast(this, Team == 0);
	}
}

void AStickFighter::TakeBlow(float Damage, const FVector& StrikePoint, const FVector& StrikeDirection, bool bHeavy)
{
	if (Hanging.IsValid())
	{
		LetGoOfBar(FVector::ZeroVector, 0.f);
	}

	if (AThrownProp* Dropped = Carried.Get())
	{
		Carried = nullptr;
		Dropped->Release(FVector::ZeroVector);
	}

	Health -= Damage * DamageTakenScale;
	const bool bKnockout = Health <= 0.f;
	const bool bShrugsItOff = bArmored && !bHeavy && !bKnockout;
	const bool bWasStriking = bStrikeOnItsWay;

	TakeHit(StrikePoint, StrikeDirection, bKnockout, bHeavy);
	if (bKnockout)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	BlowsInARow = Now - LastBlowSeconds < BlowChainSeconds ? BlowsInARow + 1 : 1;
	LastBlowSeconds = Now;
	const bool bSentFlying = bHeavy || BlowsInARow >= BlowsInAFinisher;
	if (bSentFlying)
	{
		BlowsInARow = 0;
	}

	KnockVelocity = StrikeDirection.GetSafeNormal2D() * (bSentFlying ? HeavyKnockSpeed : KnockSpeed) * (bShrugsItOff ? 0.3f : 1.f);
	if (bShrugsItOff)
	{
		bStrikeOnItsWay = bWasStriking;
		return;
	}

	StunSecondsLeft = bSentFlying ? HeavyStunSeconds : StunSeconds;
	PoseSecondsLeft = 0.f;
	UAnimSequenceBase* Reaction = HitClips[StrikePoint.Z > Body->GetBoneLocation(JointBones[ChestJoint]).Z ? 0 : 1];
	// In the air the fall is the picture. On its feet the fighter takes the blow where it landed.
	if (Reaction && bFree && JumpHeight <= 0.f)
	{
		CaptureBlendFrom();
		bKicking = false;
		Play(Reaction, false);
		State = EFighterState::Idle;
	}
	else if (State == EFighterState::Busy)
	{
		ReturnToIdle();
	}
}

bool AStickFighter::GrabOrThrow()
{
	if (Hanging.IsValid())
	{
		return false;
	}

	if (!bFree || !CanAct())
	{
		return false;
	}

	if (AThrownProp* Thrown = Carried.Get())
	{
		// A throw goes the way the fighter means to face, and bends a little to find someone to hit.
		const FVector Facing = WantedForward();
		FVector Aim = Facing;
		double Nearest = ThrowAimDistance;
		for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
		{
			const FVector To = (It->GetActorLocation() - GetActorLocation()) * FVector(1., 1., 0.);
			if (It->Team != Team && !It->IsDown() && To.Size() < Nearest && (To.GetSafeNormal() | Facing) > ThrowAimCosine)
			{
				Nearest = To.Size();
				// Thrown at where a moving target will be, not at where it is.
				Aim = ((To + It->MovedPerSecond * (To.Size() / ThrowSpeed)) * FVector(1., 1., 0.)).GetSafeNormal();
			}
		}

		Carried = nullptr;
		FaceTowards(GetActorLocation() + Aim * 100.);
		SnapToWantedFacing();
		Thrown->Release(Aim * ThrowSpeed + FVector(0.f, 0.f, ThrowDrop));
		ThrowSecondsLeft = ThrowGestureSeconds;
		GetWorld()->GetSubsystem<UTakeEvents>()->OnSwing.Broadcast(this, EMove::Throw);
		return true;
	}

	AThrownProp* Found = nullptr;
	double Reach = GrabReach;
	for (TActorIterator<AThrownProp> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (It->IsLoose() && Distance < Reach)
		{
			Reach = Distance;
			Found = *It;
		}
	}
	if (!Found)
	{
		return false;
	}

	Found->PickUp(this);
	Carried = Found;
	return true;
}

// The punch, with a gun in the hands. Every pellet is a straight line: it stops in the first opponent it meets, or comes
// off the end wall of the set once and tries again with half its sting.
bool AStickFighter::FireGun()
{
	AThrownProp* Firearm = Carried.Get();
	int32 Pellets = 0;
	float PelletDamage = 0.f;
	float SpreadDegrees = 0.f;
	float ShotKnockSpeed = 0.f;
	EMove Move = EMove::Shot;
	if (!Firearm->TakeShot(Pellets, PelletDamage, SpreadDegrees, ShotKnockSpeed, Move))
	{
		// Empty, it is one more thing to throw. Between two shots it is nothing yet.
		return Firearm->GetShotsLeft() <= 0 && GrabOrThrow();
	}

	// It is aimed like a throw: the way the fighter means to face, bent a little to find someone.
	const FVector Facing = WantedForward();
	FVector Aim = Facing;
	double ChestHeight = GunLevelHeight;
	double Nearest = GunAimDistance;
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		const FVector To = (It->GetActorLocation() - GetActorLocation()) * FVector(1., 1., 0.);
		if (It->Team != Team && !It->IsDown() && To.Size() < Nearest && (To.GetSafeNormal() | Facing) > GunAimCosine)
		{
			Nearest = To.Size();
			Aim = To.GetSafeNormal();
			ChestHeight = It->GetActorLocation().Z - FloorHeight + (It->IsDucking() ? 70. : 120.);
		}
	}
	FaceTowards(GetActorLocation() + Aim * 100.);
	SnapToWantedFacing();

	const FVector Muzzle = GetCarryPoint() + Aim * GunMuzzleAhead;
	// Tilted so that the shot passes through the chest of whoever it was aimed at: standing, ducking or in the air.
	const FVector Level = (Aim * Nearest + FVector(0., 0., FloorHeight + ChestHeight - Muzzle.Z)).GetSafeNormal();

	struct FStruck
	{
		AStickFighter* Who;
		float Damage;
		FVector Point;
		FVector Direction;
	};
	TArray<FStruck, TInlineAllocator<4>> StruckOnes;
	for (int32 Pellet = 0; Pellet < Pellets; ++Pellet)
	{
		FVector From = Muzzle;
		FVector Direction = (Level.RotateAngleAxis(FMath::FRandRange(-1.f, 1.f) * SpreadDegrees, FVector::UpVector)
			+ FVector(0., 0., FMath::Tan(FMath::DegreesToRadians(FMath::FRandRange(-0.4f, 0.4f) * SpreadDegrees)))).GetSafeNormal();
		float Damage = PelletDamage;
		double Range = GunRange;
		// Straight, and then once more off a wall.
		for (int32 Flight = 0; Flight < 2; ++Flight)
		{
			AStickFighter* Hit = nullptr;
			double HitAt = Range;
			const FVector2D Flat(Direction);
			for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
			{
				AStickFighter* Other = *It;
				if (Other == this || Other->Team == Team || Other->IsDown())
				{
					continue;
				}
				const FVector Feet = Other->GetActorLocation();
				const double Along = (FVector2D(Feet - From) | Flat) / FMath::Max(Flat.SizeSquared(), UE_SMALL_NUMBER);
				if (Along <= 0. || Along >= HitAt)
				{
					continue;
				}
				const FVector Passing = From + Direction * Along;
				const double Height = Passing.Z - Feet.Z;
				if (FVector::Dist2D(Passing, Feet) < GunBodyRadius && Height > 8. && Height < (Other->IsDucking() ? 105. : 185.))
				{
					Hit = Other;
					HitAt = Along;
				}
			}
			if (Hit)
			{
				FStruck* Known = StruckOnes.FindByPredicate([Hit](const FStruck& One) { return One.Who == Hit; });
				if (Known)
				{
					Known->Damage += Damage;
				}
				else
				{
					StruckOnes.Add({ Hit, Damage, From + Direction * HitAt, Direction });
				}
				break;
			}

			const double Wall = Direction.X > 0. ? ArenaMax.X + GunWallBeyond : ArenaMin.X - GunWallBeyond;
			const double ToWall = FMath::Abs(Direction.X) > 0.05 ? (Wall - From.X) / Direction.X : UE_BIG_NUMBER;
			if (ToWall <= 0. || ToWall >= Range)
			{
				break;
			}
			From += Direction * ToWall;
			Direction.X = -Direction.X;
			Range -= ToWall;
			Damage *= 0.5f;
		}
	}

	// The noise and the flash first, then what it did.
	GetWorld()->GetSubsystem<UTakeEvents>()->OnSwing.Broadcast(this, Move);
	const float FullBlast = PelletDamage * Pellets;
	for (const FStruck& One : StruckOnes)
	{
		const FVector Away = One.Direction.GetSafeNormal2D();
		One.Who->SufferBlow(this, One.Damage, One.Point, One.Direction, true, Move);
		One.Who->KnockVelocity += Away * (ShotKnockSpeed * FMath::Min(One.Damage / FullBlast + 0.35f, 1.f));
		if (One.Who->IsDown() && Move == EMove::Blast)
		{
			// A shotgun does not let a body fall where it stood.
			One.Who->Body->SetAllPhysicsLinearVelocity(Away * 520. + FVector(0., 0., 260.));
		}
		// A gun takes something off whoever it puts down, and a shotgun off anyone it catches squarely.
		if (One.Who->IsDown() || (Move == EMove::Blast && One.Damage >= FullBlast * 0.5f))
		{
			One.Who->Sever(One.Point);
		}
	}
	KnockVelocity -= Aim * (ShotKnockSpeed * GunKickBackShare);
	return true;
}

void AStickFighter::Sever(const FVector& Near)
{
	const TArray<FVector> Joints = BuildJoints(true);
	const int32 Ends[4][2] = { { LeftElbow, LeftHand }, { RightElbow, RightHand }, { LeftKnee, LeftFoot }, { RightKnee, RightFoot } };
	int32 Nearest = INDEX_NONE;
	double NearestDistance = UE_BIG_NUMBER;
	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		// Someone still standing keeps both legs: there is no way to fight on one.
		if ((Severed & (1 << Limb)) || (Limb >= 2 && !IsDown()))
		{
			continue;
		}
		const double Distance = FVector::DistSquared((Joints[Ends[Limb][0]] + Joints[Ends[Limb][1]]) * 0.5, Near);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Limb;
		}
	}
	if (Nearest == INDEX_NONE)
	{
		return;
	}

	Severed |= static_cast<uint8>(1 << Nearest);
	if (Nearest < 2 && !LooseArms[Nearest].bBroken)
	{
		// It throws no more punches, which is what a broken arm already means to the rest of the fighter.
		LooseArms[Nearest].bBroken = true;
		LooseArms[Nearest].Hand = Joints[HandOfSide[Nearest]];
		LooseArms[Nearest].Velocity = FVector::ZeroVector;
	}
	UE_LOG(LogStickFighter, Verbose, TEXT("%s loses a limb: %d"), *GetName(), Nearest);
}

void AStickFighter::FlyLooseLimbs(float DeltaSeconds)
{
	for (int32 Limb = 0; Limb < 4; ++Limb)
	{
		FFlyingLimb& Flying = FlyingLimbs[Limb];
		if (!Flying.bFlying)
		{
			continue;
		}

		Flying.Velocity.Z -= LooseLimbGravity * DeltaSeconds;
		Flying.Middle += Flying.Velocity * DeltaSeconds;
		Flying.Along = Flying.Along.RotateAngleAxis(LooseLimbSpinDegrees * DeltaSeconds, FVector::RightVector);
		if (Flying.Middle.Z <= FloorHeight + Thickness)
		{
			// It lies where it lands, flat.
			Flying.Middle.Z = FloorHeight + Thickness * 0.7;
			Flying.Along = Flying.Along.GetSafeNormal2D(UE_SMALL_NUMBER, FVector::ForwardVector);
			Flying.bFlying = false;
		}
		const FVector Half = Flying.Along * (Flying.Length * 0.5);
		PlaceLimb(LooseLimbs[Limb * 2], Flying.Middle - Half, Flying.Middle + Half, Thickness * 1.1f);
		LooseLimbs[Limb * 2 + 1]->SetWorldLocation(Flying.Middle + Half);
		LooseLimbs[Limb * 2 + 1]->SetWorldScale3D(FVector(Thickness * HandAndFootWidth / 100.f));
	}
}

FVector AStickFighter::GetHandPoint(int32 Side) const
{
	return BuildJoints(true)[HandOfSide[Side]];
}

FVector AStickFighter::GetCarryPoint() const
{
	const TArray<FVector> Joints = BuildJoints(true);
	return (Joints[LeftHand] + Joints[RightHand]) * 0.5;
}

void AStickFighter::AdvanceFreeMovement(float DeltaSeconds)
{
	if (!Hanging.IsValid() && bMayGrabABar && JumpHeight > 0.f)
	{
		TryToGrabABar();
	}
	if (Hanging.IsValid())
	{
		AdvanceHang(DeltaSeconds);
		return;
	}

	const bool bGrounded = JumpHeight <= 0.f;

	// Walking away cuts short the tail of a strike, so a fighter is never stuck watching its own follow-through.
	if (!WalkInput.IsNearlyZero() && State == EFighterState::Busy && CanAct())
	{
		ReturnToIdle();
	}

	const bool bOnItsFeet = State == EFighterState::Idle || State == EFighterState::Moving;
	const bool bMayWalk = bOnItsFeet && !bMoving && StunSecondsLeft <= 0.f && PoseAlpha < 0.3f;
	const double InputLength = WalkInput.Size();
	const FVector2D Velocity = bMayWalk && InputLength > 0.05 ? WalkInput / FMath::Max(InputLength, 1.) * WalkSpeed : FVector2D::ZeroVector;
	const bool bWantsToWalk = !Velocity.IsNearlyZero(1.);

	// Someone backing away from what it faces walks backwards, instead of jogging on the spot in reverse.
	// The two thresholds keep a fighter that is turning from flipping between the two walks.
	const double Heading = FVector(Velocity, 0.).GetSafeNormal() | GetActorForwardVector();
	const bool bBackwards = bWantsToWalk && Heading < (bWalkingBackwards ? -0.1 : -0.4);

	if (bGrounded && (bWantsToWalk != bWalking || (bWalking && bBackwards != bWalkingBackwards)))
	{
		bWalking = bWantsToWalk;
		bWalkingBackwards = bBackwards;
		if (bWalking)
		{
			CaptureBlendFrom();
			const float Speed = static_cast<float>(Velocity.Size());
			Play(bBackwards ? BackAnimation : RunAnimation, true, FMath::Clamp(Speed / (bBackwards ? WalkBackAnimationSpeed : RunAnimationSpeed), 0.7f, 2.2f));
			State = EFighterState::Moving;
		}
		else if (State == EFighterState::Moving && !bMoving)
		{
			ReturnToIdle();
		}
	}

	FVector Location = GetActorLocation();
	// A fighter waiting outside the arena for its entrance is left where it is. Once inside, the walls hold it.
	const bool bInside = ClampToArena(FVector2D(Location)).Equals(FVector2D(Location), 1.);

	Location += (FVector(Velocity, 0.) + KnockVelocity + (bStrikeOnItsWay ? LungeVelocity : FVector::ZeroVector)) * DeltaSeconds;
	KnockVelocity *= FMath::Exp(-KnockDamping * DeltaSeconds);

	if (!bGrounded)
	{
		JumpSpeed -= JumpGravity * DeltaSeconds;
		JumpHeight = FMath::Max(JumpHeight + JumpSpeed * DeltaSeconds, 0.f);
		if (JumpHeight <= 0.f)
		{
			JumpSpeed = 0.f;
			LandSquashSecondsLeft = LandSquashSeconds;
			bMayGrabABar = false;
			if (State == EFighterState::Moving && !bMoving)
			{
				ReturnToIdle();
			}
		}
	}

	// Two standing fighters never share a spot: each steps half way out of the other's.
	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		const AStickFighter* Other = *It;
		if (Other == this || Other->IsDown())
		{
			continue;
		}
		const FVector Apart = (Location - Other->GetActorLocation()) * FVector(1., 1., 0.);
		const double Gap = Apart.Size();
		if (Gap < BodyGap)
		{
			Location += (Gap > 1. ? Apart / Gap : -GetActorForwardVector()) * (BodyGap - Gap) * 0.5;
		}
	}

	if (bMoving)
	{
		return;
	}
	if (bInside)
	{
		const FVector2D Held = ClampToArena(FVector2D(Location));
		Location.X = Held.X;
		Location.Y = Held.Y;
	}
	Location.Z = FloorHeight + JumpHeight;
	SetActorLocation(Location);
}

// Where the fighter is turning to, which is where it means its next move to go.
FVector AStickFighter::WantedForward() const
{
	return FRotator(0.f, bTurning ? FacingYaw : static_cast<float>(GetActorRotation().Yaw), 0.f).Vector();
}

void AStickFighter::StartPosed(EMove Technique, float SecondsUntilContact)
{
	CaptureBlendFrom();
	const FTechnique* Rules = FindTechnique(Technique);
	UAnimSequenceBase* Clip = TechniqueClips.FindRef(Technique);
	float ClipContactSeconds = 0.f;
	if (Clip)
	{
		ClipContactSeconds = FindContactSeconds(Clip, Rules->bWithALeg ? 2 : 0, 2);
		// A clip is thrown with the arm it was recorded with. When that one hangs loose, the pose worked out in code uses the other.
		const int32 Arm = ArmOfStrikeBone(StrikeBone);
		if (Arm != INDEX_NONE && LooseArms[Arm].bBroken)
		{
			Clip = nullptr;
		}
	}
	bPosedByClip = Clip != nullptr;
	State = EFighterState::Busy;
	// The move starts from the held pose and carries it on itself, so the held pose lets go at once.
	PoseAlpha = 0.f;

	StrikeMove = Technique;
	bStrikeOnItsWay = true;
	bKicking = true;
	KickElapsed = 0.f;
	KickContactAt = FMath::Max(SecondsUntilContact, 0.08f);
	PosedRecoverSeconds = Rules->RecoverSeconds;
	if (Clip)
	{
		// Only the last of the wind-up is shown: a strike that has to arrive soon would otherwise play at many times its speed.
		// Its reach is the clip's own, found together with its contact.
		const float Shown = FMath::Min(ClipContactSeconds, KickContactAt * MaxClipRate);
		Play(Clip, false, Shown / KickContactAt);
		Body->SetPosition(ClipContactSeconds - Shown, false);
		return;
	}
	PlayRest();

	const FVector Hip = Body->GetBoneLocation(JointBones[RightHip]);
	const FVector Knee = Body->GetBoneLocation(JointBones[RightKnee]);
	const FVector Foot = Body->GetBoneLocation(JointBones[RightFoot]);
	const double Leg = FVector::Dist(Hip, Knee) + FVector::Dist(Knee, Foot);
	StrikeReach = Technique == EMove::Uppercut ? UppercutReach
		: Technique == EMove::Chop ? ChopReach
		: Technique == EMove::Sweep ? Leg * 0.9
		: Technique == EMove::Roundhouse ? Leg * 0.9
		: Leg * KickLegStretch;
}

// A strike or a throw goes where the fighter means to face, at once. It holds that facing until the move is over.
void AStickFighter::SnapToWantedFacing()
{
	if (bTurning)
	{
		SetActorRotation(FRotator(0.f, FacingYaw, 0.f));
		bTurning = false;
	}
}

// A self-check for something the developer saw once: a fighter walking above the floor with no jump to explain it.
// It says what it knows the moment that happens, so the cause does not have to be guessed from a picture.
void AStickFighter::WatchForFloating(const TArray<FVector>& Joints, float DeltaSeconds, const TCHAR* When, float AllowedSeconds)
{
	// At a few frames a second a whole jump fits inside one frame, and its landing reads as floating. Seen with another
	// game holding the processor; it says nothing about this one.
	if (!bFree || Joints.Num() != JointCount || DeltaSeconds > 0.1f)
	{
		return;
	}

	const double FeetAbove = FMath::Min(Joints[LeftFoot].Z, Joints[RightFoot].Z) - FloorHeight;
	FloatingSeconds = FeetAbove > 30. ? FloatingSeconds + DeltaSeconds : FMath::Min(FloatingSeconds, 0.f);
	if (FloatingSeconds > AllowedSeconds)
	{
		UE_LOG(LogStickFighter, Warning, TEXT("Floating %s: %s has both feet %.0f cm above the floor. Actor z %.0f, floor %.0f, pelvis z %.0f, body offset %s, state %d, jump %.0f, playing %s, stunned %.2f s"),
			When, *GetName(), FeetAbove, GetActorLocation().Z, FloorHeight, Joints[PelvisJoint].Z, *Body->GetRelativeLocation().ToCompactString(), static_cast<int32>(State), JumpHeight,
			Playing ? *Playing->GetName() : TEXT("nothing"), StunSecondsLeft);
		// Once a second is enough.
		FloatingSeconds = -1.f;
	}
}

void AStickFighter::ApplyLook(const FCharacterLook& Look)
{
	const FLookBuild& Build = LookBuilds[FMath::Clamp(Look.Build, 0, LookCount(2) - 1)];
	Thickness = Build.Thickness;
	HeadRadius = Build.HeadRadius;
	SetColor(LookColors[FMath::Clamp(Look.Color, 0, LookCount(0) - 1)].Color);

	Hat = FMath::Clamp(Look.Hat, 0, LookCount(1) - 1);
	for (int32 Piece = 0; Piece < 2; ++Piece)
	{
		const FHatPiece& Shape = Hats[Hat][Piece];
		HatParts[Piece]->SetVisibility(Shape.bUsed);
		HatParts[Piece]->SetStaticMesh(Shape.bCube ? CubeMesh : CylinderMesh);
		HatParts[Piece]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", HatColors[Hat]);
	}
}

int32 AStickFighter::LookCount(int32 Row)
{
	return static_cast<int32>(Row == 0 ? UE_ARRAY_COUNT(LookColors) : Row == 1 ? UE_ARRAY_COUNT(HatNames) : UE_ARRAY_COUNT(LookBuilds));
}

FLinearColor AStickFighter::LookColor(int32 Index)
{
	return LookColors[FMath::Clamp(Index, 0, LookCount(0) - 1)].Color;
}

FString AStickFighter::LookName(int32 Row, int32 Index)
{
	Index = FMath::Clamp(Index, 0, LookCount(Row) - 1);
	return Row == 0 ? LookColors[Index].Name : Row == 1 ? HatNames[Index] : LookBuilds[Index].Name;
}

// Colours the limbs, joints and head without changing what colour the fighter is. The hat, eyes and gun keep theirs.
void AStickFighter::TintBody(const FLinearColor& Tint)
{
	for (UStaticMeshComponent* Part : Limbs)
	{
		Part->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Tint);
	}
	// In its own colour the figure has its rubber joints, fists and boots. In any other tint it is all that tint.
	const FLinearColor Rubber = Tint.Equals(Color) ? RubberFor(Color) : Tint;
	for (UStaticMeshComponent* Ball : JointBalls)
	{
		Ball->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Rubber);
	}
	for (int32 Part = 0; Part < BodyParts.Num(); ++Part)
	{
		BodyParts[Part]->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Part >= LeftBoot ? Rubber : Tint);
	}
	Head->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", Tint);
	Model->Paint(Tint, Rubber);
	const bool bDarkEyes = !Tint.Equals(Color) || Color.GetLuminance() > 0.25f;
	for (UStaticMeshComponent* Eye : Eyes)
	{
		Eye->CreateDynamicMaterialInstance(0)->SetVectorParameterValue("Color", bDarkEyes ? FLinearColor(0.01f, 0.01f, 0.01f) : FLinearColor(0.9f, 0.9f, 0.9f));
	}
	// A broken forearm gets its own tint back the next time the figure is drawn.
	TintedBrokenArms = 0;
}

void AStickFighter::Launch(const FVector& Direction, float Speed)
{
	if (State == EFighterState::Down)
	{
		Body->SetAllPhysicsLinearVelocity((Direction.GetSafeNormal2D() + FVector(0., 0., 0.55)) * Speed);
	}
}

void AStickFighter::PopUp(float Speed, float HelplessSeconds)
{
	if (State == EFighterState::Down || !bFree)
	{
		return;
	}

	JumpSpeed = Speed;
	bMayGrabABar = false;
	JumpHeight = FMath::Max(JumpHeight, UE_KINDA_SMALL_NUMBER);
	StunSecondsLeft = FMath::Max(StunSecondsLeft, HelplessSeconds);
	bWalking = false;
	CaptureBlendFrom();
	Play(FallAnimation, true);
	State = EFighterState::Moving;
}

void AStickFighter::TryToGrabABar()
{
	if (StunSecondsLeft > 0.f || Carried.IsValid() || State != EFighterState::Moving || LooseArms[0].bBroken || LooseArms[1].bBroken)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	for (TActorIterator<ASwingBar> It(GetWorld()); It; ++It)
	{
		const FVector Bar = It->GetActorLocation();
		if (FMath::Abs(Bar.X - Location.X) > BarGrabWidth)
		{
			continue;
		}

		// How high the fighter has to be for its raised hands to close on the bar.
		const TArray<FVector> Joints = BuildJoints(false);
		const double Shoulder = FMath::Max(Joints[LeftShoulder].Z, Joints[RightShoulder].Z) - Location.Z;
		const double Arm = FVector::Dist(Joints[RightShoulder], Joints[RightElbow]) + FVector::Dist(Joints[RightElbow], Joints[RightHand]);
		const double HangHeight = Bar.Z - FloorHeight - Shoulder - Arm * 0.96;
		if (HangHeight < 0. || JumpHeight < HangHeight - BarGrabSlack)
		{
			continue;
		}

		CaptureBlendFrom();
		Hanging = *It;
		HangSeconds = 0.f;
		SwingKickSeconds = -1.f;
		// The body arrives with the speed of the jump that caught the bar, and that speed is its first swing.
		SwingAngle = 0.f;
		SwingSpeed = static_cast<float>(MovedPerSecond.X) / HangPendulumLength;
		if (FMath::Abs(SwingSpeed) < HangSwingAtLeast)
		{
			SwingSpeed = GetActorForwardVector().X >= 0. ? HangSwingAtLeast : -HangSwingAtLeast;
		}
		JumpHeight = static_cast<float>(HangHeight);
		JumpSpeed = 0.f;
		KnockVelocity = FVector::ZeroVector;
		bWalking = false;
		SetActorLocation(FVector(Bar.X, Location.Y, FloorHeight + HangHeight));
		GetWorld()->GetSubsystem<UTakeEvents>()->OnBarGrabbed.Broadcast(this);
		return;
	}
}

void AStickFighter::AdvanceHang(float DeltaSeconds)
{
	HangSeconds += DeltaSeconds;
	if (SwingKickSeconds >= 0.f)
	{
		SwingKickSeconds += DeltaSeconds;
		// The kick takes the swing over: back first, then through and up, towards where the fighter faces.
		const float Facing = GetActorForwardVector().X >= 0. ? 1.f : -1.f;
		const float Done = FMath::Clamp(SwingKickSeconds / SwingKickContactSeconds, 0.f, 1.f);
		SwingAngle = Done < 0.35f ? FMath::Lerp(KickSwingFrom, Facing * SwingKickBackAngle, FMath::SmoothStep(0.f, 1.f, Done / 0.35f))
			: Facing * FMath::Lerp(SwingKickBackAngle, SwingKickThroughAngle, FMath::Square((Done - 0.35f) / 0.65f));
		if (SwingKickSeconds >= SwingKickContactSeconds)
		{
			LandSwingKick();
		}
		return;
	}

	// A pendulum: gravity pulls it back to straight down, it loses a little speed, and walking left or right pushes it.
	const float Push = FMath::Clamp(static_cast<float>(WalkInput.X), -1.f, 1.f);
	SwingSpeed += (-(JumpGravity / HangPendulumLength) * FMath::Sin(SwingAngle) - HangSwingDamping * SwingSpeed + HangSwingPush * Push) * DeltaSeconds;
	SwingAngle += SwingSpeed * DeltaSeconds;
	if (FMath::Abs(SwingAngle) > HangSwingLimit)
	{
		SwingAngle = FMath::Clamp(SwingAngle, -HangSwingLimit, HangSwingLimit);
		SwingSpeed = 0.f;
	}

	// Nobody waits out a fight up there.
	if (HangSeconds > MaxHangSeconds)
	{
		SwingOffBar();
	}
}

void AStickFighter::SwingOffBar()
{
	const float Speed = SwingSpeed * HangPendulumLength;
	LetGoOfBar(FVector(Speed * FMath::Cos(SwingAngle), 0., 0.), Speed * FMath::Sin(SwingAngle));
}

bool AStickFighter::StartSwingKick()
{
	if (SwingKickSeconds >= 0.f)
	{
		return false;
	}

	SnapToWantedFacing();
	KickSwingFrom = SwingAngle;
	SwingKickSeconds = 0.f;
	StrikeMove = EMove::SwingKick;
	GetWorld()->GetSubsystem<UTakeEvents>()->OnSwing.Broadcast(this, EMove::SwingKick);
	return true;
}

void AStickFighter::LandSwingKick()
{
	const FVector Forward = GetActorForwardVector();
	const FVector From = GetActorLocation();
	const double FeetHeight = BuildJoints(true)[RightFoot].Z;

	for (TActorIterator<AStickFighter> It(GetWorld()); It; ++It)
	{
		AStickFighter* Other = *It;
		// The feet come through at chest height, over anyone who has ducked.
		if (Other == this || Other->Team == Team || Other->IsDown() || Other->IsDucking())
		{
			continue;
		}

		const FVector To = (Other->GetActorLocation() - From) * FVector(1., 1., 0.);
		const double Ahead = To | Forward;
		if (Ahead < 30. || Ahead > SwingKickReach || (To - Forward * Ahead).Size() > SwingKickWidth)
		{
			continue;
		}

		const FVector Where(Other->GetActorLocation().X, Other->GetActorLocation().Y, FeetHeight);
		Other->SufferBlow(this, KickDamage * SwingKickDamageScale, Where, Forward, true, EMove::SwingKick);
		Other->Launch(Forward, HeavyKnockSpeed);
		Other->KnockVelocity = Forward * (HeavyKnockSpeed * 1.4f);
	}

	LetGoOfBar(Forward * SwingKickFlySpeed, SwingKickLiftSpeed);
}

void AStickFighter::LetGoOfBar(const FVector& Velocity, float UpSpeed)
{
	// The body leaves the bar from where the swing has carried it, not from straight under it.
	SetActorLocation(GetActorLocation() + FVector(HangPendulumLength * FMath::Sin(SwingAngle), 0., 0.));
	JumpHeight += HangPendulumLength * (1.f - FMath::Cos(SwingAngle));
	CaptureBlendFrom();
	Hanging = nullptr;
	SwingKickSeconds = -1.f;
	bMayGrabABar = false;
	KnockVelocity = Velocity;
	JumpSpeed = UpSpeed;
}
