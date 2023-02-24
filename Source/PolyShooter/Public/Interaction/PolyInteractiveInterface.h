// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PolyInteractiveInterface.generated.h"

class IInteractionActorInterface;

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI)
class UPolyInteractiveInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可交互物接口。
 * 可交互物品如：按钮、拾取物等需要实现这个接口。
 * 使用可交互物品的角色需要带InteractionComp，@see UPolyInteractionComp
 */
class POLYSHOOTER_API IPolyInteractiveInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** 被交互时，由交互者调用，通常是由InteractionComp调用
	 *  Note: 需要在实现该接口的类中实现该方法，并调用 Execute_OnInteracted(self, InteractionActor)
	 *  否则无法触发蓝图事件OnInteracted。
	 */
	virtual void Interacted(AActor* InteractionActor) {}

	/** 物体被交互时，执行蓝图事件 */
	UFUNCTION(BlueprintNativeEvent)
	void OnInteracted(AActor* InteractionActor);

	void OnInteracted_Implementation(AActor* InteractionActor);

	/** 使 InteractionActor 可以对此物品发生交互，或使其不能此物品交互（由NewEnable确定）。
	 *  通常在角色进出可交互物附近范围时调用。*/
	virtual void SetEnableInteraction(bool NewEnable, AActor* InteractionActor);
};
