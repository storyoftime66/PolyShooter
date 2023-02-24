// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GKInteractionCompInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UNIVERSAL3DGAMEKIT_API UGKInteractionCompInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 交互组件接口
 */
class UNIVERSAL3DGAMEKIT_API IGKInteractionCompInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** 通用的交互方法，通常由角色（玩家）调用，在角色类中绑定到交互键 */
	virtual void Interact() = 0;
	
	/** 使得 Agent 可与 Object 交互 */
	virtual void EnterInteractiveItems(UObject* InteractiveObject) = 0;
	/** 禁止 Agent 与 Object 交互 */
	virtual void ExitInteractiveItems(UObject* InteractiveObject) = 0;
};
