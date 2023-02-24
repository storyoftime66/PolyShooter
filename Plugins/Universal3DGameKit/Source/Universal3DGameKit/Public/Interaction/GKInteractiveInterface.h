// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GKInteractiveInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGKInteractiveInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可交互物接口。
 * 可交互物品如：按钮、拾取物等需要实现这个接口。
 * 使用可交互物品的角色需要带InteractionComp，@see UGKInteractionComp
 */
class UNIVERSAL3DGAMEKIT_API IGKInteractiveInterface
{
	GENERATED_BODY()

public:
	/** 被交互时，由InteractionComp调用
	 *  Note: 在实现中使用以下代码
	 *		const bool bCanInteract = IGKInteractiveInterface::Interacted(InteractionCompOwner);
	 *  @return 是否交互成功
	 */
	virtual bool Interacted(AActor* InteractionCompOwner)
	{
		const bool bCanInteract = CanBeInteracted(InteractionCompOwner);
		if (bCanInteract)
		{
			Execute_BP_OnInteracted(Cast<UObject>(this), InteractionCompOwner);
		}
		return bCanInteract;
	}

	virtual bool CanBeInteracted(const AActor* InteractionCompOwner) const { return true; }

	UFUNCTION(BlueprintNativeEvent)
	void BP_OnInteracted(AActor* InteractionCompOwner);
	void BP_OnInteracted_Implementation(AActor* InteractionCompOwner);
};
