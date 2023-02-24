// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/GKInteractiveInterface.h"
#include "GameFramework/Actor.h"
#include "GKInteractiveActor.generated.h"

/**
 * 可交互Actor示例。
 * 可交互物品可以继承这个类（图方便的话）。
 */
UCLASS(BlueprintType)
class UNIVERSAL3DGAMEKIT_API AGKInteractiveActor : public AActor, public IGKInteractiveInterface
{
	GENERATED_BODY()

	// UPROPERTY()
	// TSet<AActor*> InteractionAgentsInRange;

	/** 是否只能交互一次 */
	UPROPERTY(EditAnywhere, Category="Game Kit|Interaction")
	bool bCanOnlyInteractOnce = true;

	/** 当前是否可交互 */
	UPROPERTY(EditAnywhere, Category="Game Kit|Interaction")
	bool bActive = true;


protected:
	/** 设置 InteractionCompOwner 是否可与该物品进行交互。
	 *  通常在角色进出可交互物附近范围时调用。*/
	void SetEnableInteraction(bool NewEnable, AActor* InteractionComp);
	
public:
	// Sets default values for this actor's properties
	AGKInteractiveActor();
	
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	//~ IGKInteractiveInterface
	virtual bool Interacted(AActor* InteractionCompOwner) override;
	virtual bool CanBeInteracted(const AActor* InteractionCompOwner) const override;
};
