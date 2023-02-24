// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyInteractionComp.h"
#include "UObject/Interface.h"
#include "PolyInteractionActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UPolyInteractionActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 交互接口
 * 可以使用交互功能的角色应该实现此接口
 */
class POLYSHOOTER_API IPolyInteractionActorInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UPolyInteractionComp* GetInteractionComp() const
	{
		return nullptr;
	}
};
