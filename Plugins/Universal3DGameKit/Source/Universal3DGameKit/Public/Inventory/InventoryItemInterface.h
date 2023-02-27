// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Interface.h"
#include "InventoryItemInterface.generated.h"

/**
 * 配置物品描述信息的数据表行结构
 */
USTRUCT(BlueprintType)
struct FInventoryItemData : public FTableRowBase
{
	GENERATED_BODY()

	/////////////////////////////////////////////////////////////////////
	/// 物品描述信息

	/** 物品名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Description")
	FString ItemName;
	/** 物品图标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Description")
	UTexture2D* ItemIcon;
	/** 物品描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Description")
	FText ItemDescription;
	/** 物品二级描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Description")
	FText ItemRemark;

	/////////////////////////////////////////////////////////////////////
	/// 物品玩法数据
	/** 物品堆叠上限 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay", AdvancedDisplay)
	int32 MaxStack = 64;
	/** 物品类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay", AdvancedDisplay)
	TSubclassOf<UObject> ItemClass;
};

// This class does not need to be modified.
UINTERFACE()
class UInventoryItemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 每种背包中的物品都需要实现此接口
 */
class UNIVERSAL3DGAMEKIT_API IInventoryItemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** 获取物品数据 */
	virtual FInventoryItemData GetItemData() const = 0;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Game Kit|Inventory|Item")
	FInventoryItemData BP_GetItemData() const;

	/** 获取物品名称 */
	virtual FName GetItemName() const = 0;
	
	/** 获取拥有该道具的背包组件 */
	virtual UActorComponent* GetOwnedInventoryComp() const = 0;
	/** 获取拥有该道具的Actor */
	virtual AActor* GetOwnedInventoryAgent() const = 0;

	/** 使用物品 */
	virtual void Use() = 0;
};
