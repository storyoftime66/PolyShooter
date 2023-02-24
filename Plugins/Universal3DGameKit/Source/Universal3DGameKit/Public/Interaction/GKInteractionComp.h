// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Interaction/GKInteractionCompInterface.h"
#include "GKInteractionComp.generated.h"

class IGKInteractiveInterface;

/**
 * 交互能力组件
 * 角色的拾取、与场景机关的交互功能由此组件完成。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNIVERSAL3DGAMEKIT_API UGKInteractionComp : public USphereComponent, public IGKInteractionCompInterface
{
	GENERATED_BODY()

private:
	TArray<IGKInteractiveInterface*> InteractiveItems;

protected:
	UFUNCTION(Server, Reliable)
	void ServerInteract();

	/** [server] 和可交互列表的第一个进行交互 */
	bool InteractFirstItem();

public:
	// Sets default values for this component's properties
	UGKInteractionComp();
	
	UFUNCTION()
	virtual void Interact() override;

	virtual void EnterInteractiveItems(UObject* InteractiveObject) override;
	
	virtual void ExitInteractiveItems(UObject* InteractiveObject) override;
};
