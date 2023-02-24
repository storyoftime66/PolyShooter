// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/PolyInteractiveInterface.h"

#include "Interaction/PolyInteractionActorInterface.h"

void IPolyInteractiveInterface::OnInteracted_Implementation(AActor* InteractionActor)
{
}

// Add default functionality here for any IInteractiveInterface functions that are not pure virtual.
void IPolyInteractiveInterface::SetEnableInteraction(bool NewEnable, AActor* InteractionActor)
{
	if (const IPolyInteractionActorInterface* InteractionAgent = Cast<IPolyInteractionActorInterface>(InteractionActor))
	{
		if (InteractionAgent->GetInteractionComp())
		{
			if (NewEnable)
			{
				InteractionAgent->GetInteractionComp()->EnterInteractiveProps(this);
			}
			else
			{
				InteractionAgent->GetInteractionComp()->ExitInteractiveProps(this);
			}
		}
	}
}
