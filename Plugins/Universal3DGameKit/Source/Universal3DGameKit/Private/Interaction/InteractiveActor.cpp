// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/InteractiveActor.h"
#include "Interaction/InteractionAgentInterface.h"
#include "Interaction/InteractionCompInterface.h"


// Sets default values
AInteractiveActor::AInteractiveActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

bool AInteractiveActor::Interacted(AActor* InteractionCompOwner)
{
	check(GetLocalRole() == ROLE_Authority);

	const bool bCanInteract = IInteractiveInterface::Interacted(InteractionCompOwner);

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

bool AInteractiveActor::CanBeInteracted(const AActor* InteractionCompOwner) const
{
	return bActive;
}

void AInteractiveActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	SetEnableInteraction(true, OtherActor);
}

void AInteractiveActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	SetEnableInteraction(false, OtherActor);
}

void AInteractiveActor::SetEnableInteraction(bool NewEnable, AActor* InteractionCompOwner)
{
	if (const IInteractionAgentInterface* InteractionAgent = Cast<IInteractionAgentInterface>(InteractionCompOwner))
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
