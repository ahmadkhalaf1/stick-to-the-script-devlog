#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "StickToTheScript/Core/TakeTypes.h"
#include "StickGameInstance.generated.h"

// What survives from one scene's level to the next: which scene is being shot and what the performer has earned.
UCLASS()
class UStickGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Scenes are text files, Content/Scripts/Scene*.json, shot in the order of their file names.
	static TArray<FString> FindSceneFiles();
	static bool LoadSceneFile(const FString& Path, FSceneScript& OutScript);

	int32 GetSceneCount() const;
	bool LoadScene(int32 Index, FSceneScript& OutScript) const;

	// The scene written for the given level (a package path), or INDEX_NONE.
	int32 FindSceneForStage(const FString& StagePackage) const;

	// Back to the title: nothing earned and no scene chosen. The look and what is kept between sessions stay.
	void StartOver();

	virtual void Init() override;

	// Adds a wrapped scene's pay to what is kept between sessions, and unlocks the scene after it.
	void Bank(int32 Dollars);
	void SaveProgress() const;

	// Kept between sessions, in the player's settings file: how many scenes are open, and all the money ever made.
	int32 ScenesWrapped = 0;
	int32 Banked = 0;

	// The shop. An item is a weapon for the next scene shot, or a hat or a colour that stays bought.
	static int32 ShopItemCount();
	static FString ShopItemName(int32 Item);
	static bool IsWeapon(int32 Item);
	static bool CanBuy(int32 InTheBank, int32 Price, bool bOwned) { return !bOwned && InTheBank >= Price; }
	bool Owns(int32 Item) const;
	// What the shop writes next to an item: its price, or that it is already the player's.
	FString ShopItemState(int32 Item) const;
	bool Buy(int32 Item);
	// False for a hat or a colour that has to be bought first.
	bool OwnsLook(int32 Row, int32 Index) const;
	void UseUpWeapon();

	// One bit for each look in the shop, and the weapon waiting for the next scene as an EPropKind, or none.
	int32 OwnedLooks = 0;
	int32 BoughtWeapon = INDEX_NONE;

	int32 SceneIndex = 0;
	int32 Earned = 0;
	int32 Offered = 0;
	// The part of Earned that the audience paid, on top of the fees.
	int32 BoxOffice = 0;
	// How many takes each scene of this run needed, by scene number. 0 for a scene that was not shot.
	TArray<int32> TakesByScene;

	// Chosen in the wardrobe, worn in every scene and so in every film. Starting over keeps it.
	FCharacterLook Look;

	// False until the title card has been answered, so it is shown once and not between scenes.
	bool bRunStarted = false;

	// False until the wardrobe has been answered, so it opens once in a run.
	bool bDressed = false;
};
