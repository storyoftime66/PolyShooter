// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/GKInteractiveActor.h"
#include "Interaction/GKInteractionAgentInterface.h"


// Sets default values
AGKInteractiveActor::AGKInteractiveActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

bool AGKInteractiveActor::Interacted(AActor* InteractionCompOwner)
{
	check(GetLocalRole() == ROLE_Authority);

	const bool bCanInteract = IGKInteractiveInterface::Interacted(InteractionCompOwner);

	if (bCanInteract)
	{
		if (bCanOnlyInteractOnce)
		{
			bActive = false;
			SetActorEnableCollision(false); // Note: 设置禁用碰撞后会触发一次 NotifyActorEndOverlap()
		}
	}

	return bCanInteract;
}

bool AGKInteractiveActor::CanBeInteracted(const AActor* InteractionCompOwner) const
{
	return bActive;
}

void AGKInteractiveActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	SetEnableInteraction(true, OtherActor);
}

void AGKInteractiveActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	SetEnableInteraction(false, OtherActor);
}

void AGKInteractiveActor::SetEnableInteraction(bool NewEnable, AActor* InteractionCompOwner)
{
	if (const IGKInteractionAgentInterface* InteractionAgent = Cast<IGKInteractionAgentInterface>(InteractionCompOwner))
	{
		if (InteractionAgent->GetInteractionComp())
		{
			if (NewEnable)
			{
				InteractionAgent->GetInteractionComp()->EnterInteractiveItems(this);
			}
			else
			{
				InteractionAgent->GetInteractionComp()->ExitInteractiveItems(this);
			}
		}
	}
}
