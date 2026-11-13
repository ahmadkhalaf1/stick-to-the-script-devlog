#include "StickToTheScript/Core/StickGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StickToTheScript/Core/GameFlow.h"
#include "StickToTheScript/Core/StickGameInstance.h"
#include "StickToTheScript/Core/StickPlayerController.h"
#include "StickToTheScript/Cam/BodyCam.h"
#include "StickToTheScript/Cam/FightCamera.h"
#include "StickToTheScript/Cam/FinalCut.h"
#include "StickToTheScript/Cam/FightRecorder.h"
#include "StickToTheScript/Core/TakeDirector.h"
#include "StickToTheScript/Fighter/Captive.h"
#include "StickToTheScript/Fighter/StuntPartner.h"
#include "StickToTheScript/Fighter/SwingBar.h"
#include "StickToTheScript/Fighter/ThrownProp.h"
#include "StickToTheScript/UI/FilmScore.h"
#include "StickToTheScript/UI/ImpactEffects.h"
#include "StickToTheScript/UI/StickHUD.h"

namespace
{
	struct FMark
	{
		FName Tag;
		FVector FallbackLocation;
		FLinearColor Color;
	};

	// A stage places actors carrying these tags. Without them the fight is laid out around the origin, so any level is playable.
	// Not pure white: under stage lights that burns out and loses the figure's shape.
	const FMark PlayerMark = { "PlayerMark", FVector::ZeroVector, FLinearColor(0.8f, 0.8f, 0.8f) };
	const FMark PartnerMarks[] = {
		{ "PartnerMark_A", FVector(100.f, 0.f, 0.f), FLinearColor(0.02f, 0.02f, 0.02f) },
		{ "PartnerMark_B", FVector(70.f, 130.f, 0.f), FLinearColor(0.35f, 0.02f, 0.02f) },
		{ "PartnerMark_C", FVector(70.f, -130.f, 0.f), FLinearColor(0.02f, 0.05f, 0.3f) },
	};
	const FName WideCameraTag = "Cam_Wide";

	AActor* FindTagged(const UObject* WorldContext, FName Tag)
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(WorldContext, Tag, Tagged);
		return Tagged.IsEmpty() ? nullptr : Tagged[0];
	}

	FVector MarkLocation(const UObject* WorldContext, const FMark& Mark)
	{
		const AActor* Placed = FindTagged(WorldContext, Mark.Tag);
		return Placed ? Placed->GetActorLocation() : Mark.FallbackLocation;
	}

	FRotator Facing(const FVector& From, const FVector& To)
	{
		return (To - From).GetSafeNormal2D().Rotation();
	}
}

AStickGameMode::AStickGameMode()
{
	PlayerControllerClass = AStickPlayerController::StaticClass();
	HUDClass = AStickHUD::StaticClass();
	// The engine's default pawn flies on WASD, which are move keys here.
	DefaultPawnClass = nullptr;
}

void AStickGameMode::StartPlay()
{
	Super::StartPlay();

	UWorld* World = GetWorld();

	// The scene to shoot is the one the run has reached. On a fresh start it is the one written for this level,
	// so pressing Play on any stage in the editor shoots that stage's scene. With no scene files, the built-in scene plays.
	FSceneScript Script = GetDefault<ATakeDirector>()->Script;
	if (UStickGameInstance* Game = GetGameInstance<UStickGameInstance>())
	{
		if (!Game->bRunStarted)
		{
			Game->SceneIndex = FMath::Max(Game->FindSceneForStage(UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())), 0);
		}
		Game->LoadScene(Game->SceneIndex, Script);
	}

	// A scene says where its opponents wait and where they fight. One that does not gets the level's tagged marks.
	const float FloorHeight = MarkLocation(this, PlayerMark).Z;
	TArray<FOpponent> Opponents = Script.Opponents;
	FVector PlayerLocation(Script.PlayerStart.X, Script.PlayerStart.Y, FloorHeight);
	if (Opponents.IsEmpty())
	{
		PlayerLocation = MarkLocation(this, PlayerMark);
		for (const FMark& Mark : PartnerMarks)
		{
			const FVector Location = MarkLocation(this, Mark);
			FOpponent& Opponent = Opponents.AddDefaulted_GetRef();
			Opponent.Enter = FVector2D(Location);
			Opponent.Spot = FVector2D(Location);
			Opponent.Color = Mark.Color;
		}
	}

	const FVector FirstPartnerLocation(Opponents[0].Spot.X, Opponents[0].Spot.Y, FloorHeight);
	const FRotator PlayerFacing = Facing(PlayerLocation, FirstPartnerLocation);
	AStickFighter* PlayerFighter = World->SpawnActor<AStickFighter>(PlayerLocation, PlayerFacing);
	PlayerFighter->SetColor(PlayerMark.Color);
	if (const UStickGameInstance* Wardrobe = GetGameInstance<UStickGameInstance>())
	{
		PlayerFighter->ApplyLook(Wardrobe->Look);
	}
	PlayerFighter->SetHome(PlayerLocation, PlayerFacing);

	PlayerFighter->Team = 0;
	PlayerFighter->MaxHealth = 100.f;
	PlayerFighter->WalkSpeed = 330.f;
	PlayerFighter->PunchDamage = 9.f;
	PlayerFighter->KickDamage = 15.f;
	PlayerFighter->SetArena(Script.ArenaMin, Script.ArenaMax);

	// The recorder keeps the fighters in this order: the player first, then the opponents by index.
	TArray<AStickFighter*> FightersInOrder = { PlayerFighter };
	for (int32 Index = 0; Index < Opponents.Num(); ++Index)
	{
		const FOpponent& Opponent = Opponents[Index];
		const FVector Enter(Opponent.Enter.X, Opponent.Enter.Y, FloorHeight);
		const FVector Spot(Opponent.Spot.X, Opponent.Spot.Y, FloorHeight);
		const FRotator Rotation = Facing(Enter, Enter.Equals(Spot, 1.) ? PlayerLocation : Spot);

		AStuntPartner* Partner = World->SpawnActor<AStuntPartner>(Enter, Rotation);
		Partner->SetColor(Opponent.Color);
		Partner->SetHome(Enter, Rotation);
		Partner->SpotLocation = Spot;
		Partner->PartnerIndex = Index;
		Partner->PlayerFighter = PlayerFighter;
		Partner->StartThinking(Opponent.Kind, Opponent.WakeDistance);
		Partner->SetArena(Script.ArenaMin, Script.ArenaMax);
		FightersInOrder.Add(Partner);
	}

	// The recorder keeps a captive too, after the opponents, so the film shows it being freed.
	const int32 OpponentCount = FightersInOrder.Num() - 1;
	for (const FVector2D& Place : Script.Captives)
	{
		const FVector Location(Place.X, Place.Y, FloorHeight);
		ACaptive* Captive = World->SpawnActor<ACaptive>(Location, Facing(Location, PlayerLocation));
		Captive->SetActorScale3D(FVector(0.72));
		Captive->Thickness = 5.5f;
		Captive->HeadRadius = 10.f;
		Captive->SetColor(FLinearColor(0.9f, 0.7f, 0.1f));
		Captive->SetHome(Location, Captive->GetActorRotation());
		Captive->Team = 0;
		Captive->OpponentsToBeat = OpponentCount;
		FightersInOrder.Add(Captive);
	}

	TArray<AThrownProp*> Props;
	for (const FSceneProp& Placed : Script.Props)
	{
		AThrownProp* Prop = World->SpawnActor<AThrownProp>(FVector(Placed.Location.X, Placed.Location.Y, FloorHeight + Placed.Height), FRotator::ZeroRotator);
		Prop->SetUp(Placed.Kind, FloorHeight, Script.ArenaMin, Script.ArenaMax);
		Props.Add(Prop);
	}

	for (const FSceneBar& Placed : Script.Bars)
	{
		const double Depth = Script.ArenaMax.Y - Script.ArenaMin.Y;
		const FVector Middle(Placed.X, (Script.ArenaMin.Y + Script.ArenaMax.Y) * 0.5, FloorHeight + Placed.Height);
		World->SpawnActor<ASwingBar>(Middle, FRotator::ZeroRotator)->SetUp(Depth * 0.5 + 45., Placed.bOnAPost ? Placed.Height : 0.);
	}

	// One legendary prop a scene, of a kind picked at random. It is made now and hidden, not made when it drops, so the
	// recorder has it from the first frame and the film can show it. It drops on a knockout chosen afresh each take,
	// never the last one, so there is still someone to use it on.
	const EPropKind Legends[] = { EPropKind::Hammer, EPropKind::Bat, EPropKind::Fish };
	AThrownProp* Legend = World->SpawnActor<AThrownProp>(FVector(PlayerLocation.X, PlayerLocation.Y, FloorHeight), FRotator::ZeroRotator);
	Legend->SetUp(Legends[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Legends)) - 1)], FloorHeight, Script.ArenaMin, Script.ArenaMax);
	Legend->HoldInReserve(OpponentCount - 1);
	Props.Add(Legend);

	AThrownProp* Bought = World->SpawnActor<AThrownProp>(FVector(PlayerLocation.X, PlayerLocation.Y, FloorHeight), FRotator::ZeroRotator);
	Bought->SetUp(EPropKind::WoodBat, FloorHeight, Script.ArenaMin, Script.ArenaMax);
	Bought->bShopSlot = true;
	Bought->KeepOffTheSet();
	Props.Add(Bought);

	AActor* WideCamera = FindTagged(this, WideCameraTag);
	if (!WideCamera)
	{
		const FVector Middle = (PlayerLocation + FirstPartnerLocation) * 0.5f + FVector(0.f, 0.f, 100.f);
		const FVector Side = FVector::CrossProduct(FVector::UpVector, (FirstPartnerLocation - PlayerLocation).GetSafeNormal2D());
		const FVector CameraLocation = Middle + Side * 380.f;
		WideCamera = World->SpawnActor<AFightCamera>(CameraLocation, (Middle - CameraLocation).Rotation());
		WideCamera->Tags.Add(WideCameraTag);
	}

	if (AFightCamera* Travelling = Cast<AFightCamera>(WideCamera))
	{
		Travelling->Follow(PlayerFighter);
	}

	// The fight is played from outside, where both fighters are seen whole. V switches to the body cam.
	World->SpawnActor<ABodyCam>()->Wear(PlayerFighter);
	if (APlayerController* Player = World->GetFirstPlayerController())
	{
		Player->SetViewTarget(WideCamera);
		if (AStickPlayerController* Controller = Cast<AStickPlayerController>(Player))
		{
			Controller->Fighter = PlayerFighter;
		}
	}

	World->SpawnActor<AImpactEffects>();
	World->SpawnActor<AFilmScore>();

	AFightRecorder* Recorder = World->SpawnActor<AFightRecorder>();
	Recorder->Watch(FightersInOrder);
	Recorder->WatchProps(Props);
	World->SpawnActor<AFinalCut>()->Recorder = Recorder;

	// The director gets its script before the flow exists, because the flow reads it the moment it is spawned.
	ATakeDirector* Director = World->SpawnActor<ATakeDirector>();
	Director->Script = Script;
	Director->OpponentCount = OpponentCount;
	World->SpawnActor<AGameFlow>();
}
