// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryCompInterface.h"
#include "Components/ActorComponent.h"
#include "InventoryComp.generated.h"

class UDataTable;

/**
 * 背包组件示例
 * 背包功能参考Minecraft的背包
 * 目前未实现联机功能 TODO
 *
 * 一些约定：
 * 1. 在ItemArray中的物品就是我的，不存在ItemArray中的物品的InventoryComp不是我的情况
 */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UNIVERSAL3DGAMEKIT_API UInventoryComp : public UActorComponent, public IInventoryCompInterface
{
private:
	GENERATED_BODY()

	/** 背包物品列表 */
	UPROPERTY(ReplicatedUsing=OnRep_ItemArray)
	TArray<FInventoryItemLabel> ItemArray;

	UPROPERTY(EditAnywhere, NoClear, Category="Game Kit|Inventory|InventoryComp")
	UDataTable* ItemTable;

	/** 背包容量，在这个组件的实现中是固定的 */
	UPROPERTY(Replicated)
	int32 Capacity = 20;
	/** 背包剩余容量 */
	UPROPERTY()
	int32 Slack = 20;

public:
	/** 空道具，用来表示位置是空的 */
	static FInventoryItemLabel NONE_ITEM;
	
	UInventoryComp();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ItemArray();

#pragma region HELPER_FUNCTIONS
	/** 查询背包是否已满 */
	bool IsFull() const
	{
		return Slack == 0;
	}
	/** 查找首个空位，返回索引 */
	int32 FindFirstSlack() const
	{
		check(!IsFull());
		
		for(int32 ItemIndex = 0; ItemIndex < Capacity; ++ItemIndex)
		{
			if (ItemArray[ItemIndex].ItemName.IsNone())
			{
				return ItemIndex;
			}
		}
		return -1;  // 实际上不会出现这种情况
	}
#pragma endregion 

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IInventoryCompInterface
	virtual void RestoreFromSave() override;
	virtual TArray<FInventoryItemLabel> GetAllItems() const override { return ItemArray; }
	virtual int32 GetCapacity() const override { return Capacity; }
	virtual int32 GetSize() const override { return Capacity - Slack; }
	virtual int32 GetSlack() const override { return Slack; }
	
	virtual void SwapItem(int32 ItemIndex1, int32 ItemIndex2) override;
	virtual void AddItem(FInventoryItemLabel NewItem, int32 Position) override;
	virtual void RemoveItem(int32 ItemIndex) override;
	virtual void MergeItem(int32 ItemIndex1, int32 ItemIndex2) override;
	virtual void UseItem(int32 ItemIndex) override;
};
