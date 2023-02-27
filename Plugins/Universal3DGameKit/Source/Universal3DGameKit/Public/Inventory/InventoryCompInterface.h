// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryCompInterface.generated.h"

/** 背包物品标识符，用于指代背包存放的物品 */
USTRUCT()
struct FInventoryItemLabel
{
	GENERATED_BODY()

	/** 物品名称（唯一标识符） */
	UPROPERTY()
	FName ItemName;
	/** 物品堆叠数量 */
	UPROPERTY()
	int32 Stack;

	FInventoryItemLabel(): ItemName(NAME_None), Stack(0)
	{
	}
	FInventoryItemLabel(const FInventoryItemLabel& Other): ItemName(Other.ItemName), Stack(Other.Stack)
	{
	}

	FInventoryItemLabel& operator=(const FInventoryItemLabel& Other)
	{
		ItemName = Other.ItemName;
		Stack = Other.Stack;
		return *this;
	}
	bool operator==(const FInventoryItemLabel& Other) const
	{
		return ItemName == Other.ItemName;
	}
	bool operator==(const FName OtherName) const
	{
		return ItemName == OtherName;
	}
	bool operator!=(const FInventoryItemLabel& Other) const
	{
		return ItemName != Other.ItemName;
	}
	bool operator!=(const FName OtherName) const
	{
		return ItemName != OtherName;
	}
	bool IsNone() const { return ItemName.IsNone(); }
	bool IsValid() const { return !ItemName.IsNone(); }
};

// This class does not need to be modified.
UINTERFACE()
class UInventoryCompInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNIVERSAL3DGAMEKIT_API IInventoryCompInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** 从存档中恢复背包，TODO */
	virtual void RestoreFromSave();

	/////////////////////////////////////////////
	/// 背包状态
	/** 获取背包内全部道具 */
	virtual TArray<FInventoryItemLabel> GetAllItems() const = 0;
	/** 获取背包容量上限 */
	virtual int32 GetCapacity() const = 0;
	/** 获取当前物品数量 */
	virtual int32 GetSize() const = 0;
	/** 获取空位数量 */
	virtual int32 GetSlack() const = 0;
	
	/////////////////////////////////////////////
	/// 对道具的操作
	/** 交换两个物品的位置 */
	virtual void SwapItem(int32 ItemIndex1, int32 ItemIndex2);
	/** 合并物品2到物品1上（数量叠加） */
	virtual void MergeItem(int32 ItemIndex1, int32 ItemIndex2);
	/** 添加Item到背包的Position位置（冲突情况由实现决定） */
	virtual void AddItem(FInventoryItemLabel NewItem, int32 Position);
	/** 移除Item */
	virtual void RemoveItem(int32 ItemIndex);
	/** 使用道具 */
	virtual void UseItem(int32 ItemIndex);
};
