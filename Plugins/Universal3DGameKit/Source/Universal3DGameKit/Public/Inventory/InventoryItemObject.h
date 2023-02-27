// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInterface.h"
#include "UObject/Object.h"
#include "InventoryItemObject.generated.h"

/**
 * 背包物品示例
 * 功能参考Minecraft
 */
UCLASS()
class UNIVERSAL3DGAMEKIT_API UInventoryItemObject : public UObject, public IInventoryItemInterface
{
private:
	GENERATED_BODY()

	UPROPERTY()
	UActorComponent* InventoryComp = nullptr;
	UPROPERTY()
	AActor* InventoryAgent = nullptr;
	UPROPERTY()
	FName ItemName;

public:
	virtual FName GetItemName() const override { return ItemName; }
	virtual void Use() override {}
	virtual FInventoryItemData GetItemData() const override;
	virtual UActorComponent* GetOwnedInventoryComp() const override { return InventoryComp; }
	virtual AActor* GetOwnedInventoryAgent() const override { return InventoryAgent; }
};
