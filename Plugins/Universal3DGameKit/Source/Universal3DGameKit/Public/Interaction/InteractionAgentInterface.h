// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionAgentInterface.generated.h"

class IInteractionCompInterface;

// This class does not need to be modified.
UINTERFACE()
class UNIVERSAL3DGAMEKIT_API UInteractionAgentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 交互代理接口
 * 可以使用交互功能的角色应该实现此接口
 */
class UNIVERSAL3DGAMEKIT_API IInteractionAgentInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual IInteractionCompInterface* GetInteractionComp() const = 0;
};
