#include "StickToTheScript/Core/StickGameInstance.h"

#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* ProgressSection = TEXT("StickToTheScript.Progress");

	struct FShopItem
	{
		const TCHAR* Name;
		int32 Price;
		// An EPropKind for a weapon, or none for a look.
		int32 Weapon;
		// For a look: the wardrobe's row and the entry in it. They are the last entries of the fighter's tables.
		int32 LookRow;
		int32 LookIndex;
	};
	const FShopItem ShopItems[] = {
		{ TEXT("Wooden bat"), 150, static_cast<int32>(EPropKind::WoodBat), INDEX_NONE, INDEX_NONE },
		{ TEXT("Iron bar"), 300, static_cast<int32>(EPropKind::IronBar), INDEX_NONE, INDEX_NONE },
		{ TEXT("Sledgehammer"), 500, static_cast<int32>(EPropKind::Sledge), INDEX_NONE, INDEX_NONE },
		{ TEXT("Halo"), 250, INDEX_NONE, 1, 5 },
		{ TEXT("Crown"), 400, INDEX_NONE, 1, 6 },
		{ TEXT("Royal purple paint"), 300, INDEX_NONE, 0, 8 },
	};
}

void UStickGameInstance::Init()
{
	Super::Init();
	GConfig->GetInt(ProgressSection, TEXT("ScenesWrapped"), ScenesWrapped, GGameUserSettingsIni);
	GConfig->GetInt(ProgressSection, TEXT("Banked"), Banked, GGameUserSettingsIni);
	GConfig->GetInt(ProgressSection, TEXT("OwnedLooks"), OwnedLooks, GGameUserSettingsIni);
	GConfig->GetInt(ProgressSection, TEXT("BoughtWeapon"), BoughtWeapon, GGameUserSettingsIni);
}

void UStickGameInstance::SaveProgress() const
{
	GConfig->SetInt(ProgressSection, TEXT("ScenesWrapped"), ScenesWrapped, GGameUserSettingsIni);
	GConfig->SetInt(ProgressSection, TEXT("Banked"), Banked, GGameUserSettingsIni);
	GConfig->SetInt(ProgressSection, TEXT("OwnedLooks"), OwnedLooks, GGameUserSettingsIni);
	GConfig->SetInt(ProgressSection, TEXT("BoughtWeapon"), BoughtWeapon, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

int32 UStickGameInstance::ShopItemCount()
{
	return UE_ARRAY_COUNT(ShopItems);
}

FString UStickGameInstance::ShopItemName(int32 Item)
{
	return ShopItems[Item].Name;
}

bool UStickGameInstance::IsWeapon(int32 Item)
{
	return ShopItems[Item].Weapon != INDEX_NONE;
}

bool UStickGameInstance::Owns(int32 Item) const
{
	return IsWeapon(Item) ? BoughtWeapon == ShopItems[Item].Weapon : (OwnedLooks & (1 << Item)) != 0;
}

FString UStickGameInstance::ShopItemState(int32 Item) const
{
	return !Owns(Item) ? FString::Printf(TEXT("$%d"), ShopItems[Item].Price) : IsWeapon(Item) ? TEXT("waiting for your next scene") : TEXT("yours");
}

bool UStickGameInstance::Buy(int32 Item)
{
	// One weapon waits at a time: a second would throw the first away unused.
	if (!CanBuy(Banked, ShopItems[Item].Price, Owns(Item)) || (IsWeapon(Item) && BoughtWeapon != INDEX_NONE))
	{
		return false;
	}

	Banked -= ShopItems[Item].Price;
	if (IsWeapon(Item))
	{
		BoughtWeapon = ShopItems[Item].Weapon;
	}
	else
	{
		OwnedLooks |= 1 << Item;
	}
	SaveProgress();
	return true;
}

bool UStickGameInstance::OwnsLook(int32 Row, int32 Index) const
{
	for (int32 Item = 0; Item < ShopItemCount(); ++Item)
	{
		if (ShopItems[Item].LookRow == Row && ShopItems[Item].LookIndex == Index)
		{
			return Owns(Item);
		}
	}
	return true;
}

void UStickGameInstance::UseUpWeapon()
{
	BoughtWeapon = INDEX_NONE;
	SaveProgress();
}

void UStickGameInstance::Bank(int32 Dollars)
{
	ScenesWrapped = FMath::Max(ScenesWrapped, SceneIndex + 1);
	Banked += Dollars;
	SaveProgress();
}

TArray<FString> UStickGameInstance::FindSceneFiles()
{
	const FString Folder = FPaths::ProjectContentDir() / TEXT("Scripts");

	TArray<FString> Names;
	IFileManager::Get().FindFiles(Names, *(Folder / TEXT("Scene*.json")), true, false);
	Names.Sort();

	TArray<FString> Paths;
	for (const FString& Name : Names)
	{
		Paths.Add(Folder / Name);
	}
	return Paths;
}

bool UStickGameInstance::LoadSceneFile(const FString& Path, FSceneScript& OutScript)
{
	// The reader keeps whatever is already in the struct for any field the file leaves out, so it must start empty.
	OutScript = FSceneScript();

	FString Json;
	return FFileHelper::LoadFileToString(Json, *Path) && FJsonObjectConverter::JsonObjectStringToUStruct(Json, &OutScript);
}

int32 UStickGameInstance::GetSceneCount() const
{
	return FindSceneFiles().Num();
}

bool UStickGameInstance::LoadScene(int32 Index, FSceneScript& OutScript) const
{
	const TArray<FString> Files = FindSceneFiles();
	return Files.IsValidIndex(Index) && LoadSceneFile(Files[Index], OutScript);
}

int32 UStickGameInstance::FindSceneForStage(const FString& StagePackage) const
{
	const TArray<FString> Files = FindSceneFiles();
	for (int32 Index = 0; Index < Files.Num(); ++Index)
	{
		FSceneScript Script;
		if (LoadSceneFile(Files[Index], Script) && Script.Stage == StagePackage)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UStickGameInstance::StartOver()
{
	SceneIndex = 0;
	Earned = 0;
	Offered = 0;
	BoxOffice = 0;
	TakesByScene.Reset();
	bRunStarted = false;
	bDressed = false;
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneFilesTest, "StickToTheScript.Core.Scenes.EverySceneFileIsPlayable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSceneFilesTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Files = UStickGameInstance::FindSceneFiles();
	TestTrue(TEXT("There is at least one scene file in Content/Scripts"), Files.Num() > 0);

	for (const FString& File : Files)
	{
		const FString Name = FPaths::GetCleanFilename(File);

		FSceneScript Script;
		if (!TestTrue(FString::Printf(TEXT("%s parses"), *Name), UStickGameInstance::LoadSceneFile(File, Script)))
		{
			continue;
		}

		TestTrue(FString::Printf(TEXT("%s has opponents"), *Name), Script.Opponents.Num() > 0);
		TestTrue(FString::Printf(TEXT("%s gives the director lines of his own to shout"), *Name), Script.DirectorLines.Num() >= 4);
		const FBox2D Arena(Script.ArenaMin, Script.ArenaMax);
		TestTrue(FString::Printf(TEXT("%s has an arena with some floor"), *Name), Script.ArenaMin.X < Script.ArenaMax.X && Script.ArenaMin.Y < Script.ArenaMax.Y);
		TestTrue(FString::Printf(TEXT("%s starts the player inside the arena"), *Name), Arena.IsInside(Script.PlayerStart));
		for (int32 Index = 0; Index < Script.Opponents.Num(); ++Index)
		{
			TestTrue(FString::Printf(TEXT("%s opponent %d comes to a spot inside the arena"), *Name, Index + 1), Arena.IsInside(Script.Opponents[Index].Spot));
		}
		for (int32 Index = 0; Index < Script.Captives.Num(); ++Index)
		{
			const FVector2D At = Script.Captives[Index];
			TestTrue(FString::Printf(TEXT("%s captive %d is out of every opponent's reach, outside the arena"), *Name, Index + 1), FVector2D::Distance(At, Arena.GetClosestPointTo(At)) > 150.);
		}
		for (int32 Index = 0; Index < Script.Props.Num(); ++Index)
		{
			const FVector2D At = Script.Props[Index].Location;
			TestTrue(FString::Printf(TEXT("%s prop %d can be reached from inside the arena"), *Name, Index + 1), FVector2D::Distance(At, Arena.GetClosestPointTo(At)) < 125.);
		}
		for (int32 Index = 0; Index < Script.Bars.Num(); ++Index)
		{
			const FSceneBar& Bar = Script.Bars[Index];
			TestTrue(FString::Printf(TEXT("%s bar %d crosses the arena, where a jump can reach it"), *Name, Index + 1),
				Bar.X > Script.ArenaMin.X && Bar.X < Script.ArenaMax.X && Bar.Height > 215.f && Bar.Height < 295.f);
		}
		TestTrue(FString::Printf(TEXT("%s pays something"), *Name), Script.Fee > 0);

		// Loading over a scene that already has content must give the same result as loading into an empty one.
		FSceneScript Reloaded;
		Reloaded.Opponents.SetNum(12);
		UStickGameInstance::LoadSceneFile(File, Reloaded);
		TestEqual(FString::Printf(TEXT("%s reloads with the same number of opponents"), *Name), Reloaded.Opponents.Num(), Script.Opponents.Num());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopTest, "StickToTheScript.Core.Shop.MoneyBuysOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FShopTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The exact price buys"), UStickGameInstance::CanBuy(150, 150, false));
	TestFalse(TEXT("A dollar short does not"), UStickGameInstance::CanBuy(149, 150, false));
	TestFalse(TEXT("What is owned is not sold again"), UStickGameInstance::CanBuy(1000, 150, true));
	TestTrue(TEXT("The shop has weapons and looks"), UStickGameInstance::IsWeapon(0) && !UStickGameInstance::IsWeapon(UStickGameInstance::ShopItemCount() - 1));
	return true;
}

#endif
