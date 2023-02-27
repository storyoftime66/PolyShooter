// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryAgentInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInventoryAgentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 拥有背包组件的Actor需要实现此接口（通常为角色、箱子Actor等）
 */
class UNIVERSAL3DGAMEKIT_API IInventoryAgentInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** C++获取背包组件 */
	virtual UActorComponent* GetInventoryComp() const = 0;

	/** 蓝图获取背包组件 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Game Kit|Inventory|InventoryAgent")
	UActorComponent* BP_GetInventoryComp() const;
};
