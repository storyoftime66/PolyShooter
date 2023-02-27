// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComp.h"
#include "Inventory/InventoryItemInterface.h"

#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

FInventoryItemLabel UInventoryComp::NONE_ITEM = FInventoryItemLabel();

UInventoryComp::UInventoryComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	ItemArray.Init(NONE_ITEM, Capacity);
}

void UInventoryComp::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

void UInventoryComp::OnRep_ItemArray()
{
	Slack = 0;
	for (const auto Item : ItemArray)
	{
		if (Item.ItemName.IsNone())
		{
			++Slack;
		}
	}
}

void UInventoryComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UInventoryComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComp, ItemArray);
	DOREPLIFETIME(UInventoryComp, Capacity);
}

void UInventoryComp::RestoreFromSave()
{
}

void UInventoryComp::MergeItem(int32 ItemIndex1, int32 ItemIndex2)
{
	if (!ItemArray.IsValidIndex(ItemIndex1) or !ItemArray.IsValidIndex(ItemIndex2))
	{
		return;
	}

	FInventoryItemLabel& Item1 = ItemArray[ItemIndex1];
	FInventoryItemLabel& Item2 = ItemArray[ItemIndex2];

	if (Item1.IsNone() or Item1 != Item2)
	{
		return;
	}

	const FInventoryItemData* ItemData = ItemTable->FindRow<FInventoryItemData>(ItemArray[ItemIndex1].ItemName, "MergeItem");

	if (ItemData->MaxStack > Item1.Stack)
	{
		// 确保物品堆叠数不会溢出
		const int32 MoveNum = FMath::Min(ItemData->MaxStack - Item1.Stack, Item2.Stack);
		Item1.Stack += MoveNum;
		Item2.Stack -= MoveNum;
		if (Item2.Stack == 0)
		{
			ItemArray[ItemIndex2] = NONE_ITEM;
		}
	}
}

void UInventoryComp::SwapItem(int32 ItemIndex1, int32 ItemIndex2)
{
	if (ItemArray.IsValidIndex(ItemIndex1) and ItemArray.IsValidIndex(ItemIndex2))
	{
		ItemArray.Swap(ItemIndex1, ItemIndex2);
	}
}

void UInventoryComp::AddItem(FInventoryItemLabel NewItem, int32 NewItemPos)
{
	if (!ItemArray.IsValidIndex(NewItemPos) or IsFull())
	{
		return;
	}
	
	if (ItemArray[NewItemPos].IsNone())
	{
		ItemArray[NewItemPos] = NewItem;
	}
	else
	{
		ItemArray[FindFirstSlack()] = NewItem;
	}
}

void UInventoryComp::RemoveItem(int32 ItemIndex)
{
	if (ItemArray.IsValidIndex(ItemIndex))
	{
		ItemArray[ItemIndex] = NONE_ITEM;
	}
}

void UInventoryComp::UseItem(int32 ItemIndex)
{
	if (!ItemArray.IsValidIndex(ItemIndex) or ItemArray[ItemIndex].IsNone())
	{
		return;
	}

	const FInventoryItemData* ItemData = ItemTable->FindRow<FInventoryItemData>(ItemArray[ItemIndex].ItemName, "UseItem");
	check(ItemData);
	// 临时创建一个UItemObject，调用这个Item的Use()，TODO: 需要改进
	if (IsValid(ItemData->ItemClass))
	{
		UObject* ItemObject = NewObject<UObject>(this, ItemData->ItemClass.Get());
		if (ItemObject->Implements<UInventoryItemInterface>())
		{
			Cast<IInventoryItemInterface>(ItemObject)->Use();
		}
		ItemObject->MarkPendingKill();
	}
}
