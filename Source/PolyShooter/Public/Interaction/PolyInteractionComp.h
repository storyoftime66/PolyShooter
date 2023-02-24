// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/PolyInteractiveInterface.h"
#include "Components/ActorComponent.h"
#include "PolyInteractionComp.generated.h"

/**
 * 交互组件
 * 角色的拾取、与场景机关的交互功能由此组件实现。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class POLYSHOOTER_API UPolyInteractionComp : public UActorComponent
{
	GENERATED_BODY()

private:
	TArray<IPolyInteractiveInterface*> InteractiveProps;

public:
	// Sets default values for this component's properties
	UPolyInteractionComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 通用的交互方法，由角色（玩家）调用 */
	UFUNCTION()
	void Interact();
	
	UFUNCTION(Server, Reliable)
	void ServerInteract();

	/** [server] 和可交互列表的第一个进行交互 */
	void ServerInteractFirst();

	FORCEINLINE void EnterInteractiveProps(IPolyInteractiveInterface* InteractiveProp)
	{
		InteractiveProps.AddUnique(InteractiveProp);
	}
	FORCEINLINE void ExitInteractiveProps(IPolyInteractiveInterface* InteractiveProp)
	{
		InteractiveProps.Remove(InteractiveProp);
	}
};
