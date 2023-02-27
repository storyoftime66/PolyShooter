// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/InteractionComp.h"
#include "Interaction/InteractiveInterface.h"

// Sets default values for this component's properties
UGKInteractionComp::UGKInteractionComp()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 默认交互范围
	SphereRadius = 200.0f;
}

// void UGKInteractionComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// {
// 	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
// }

void UGKInteractionComp::Interact()
{
	ServerInteract();
}

void UGKInteractionComp::ServerInteract_Implementation()
{
	InteractFirstItem();
}

bool UGKInteractionComp::InteractFirstItem()
{
	check(GetOwnerRole() == ROLE_Authority);

	if (InteractiveItems.Num() > 0)
	{
		IInteractiveInterface* FirstItem = InteractiveItems[0];
		InteractiveItems[0]->Interacted(GetOwner());
		// Note: 多数可拾取物被拾取后会消失或禁用碰撞，会自动移除InteractiveProps列表中的引用
		InteractiveItems.Remove(FirstItem);

		return true;
	}

	return false;
}

void UGKInteractionComp::EnterInteractiveItems(UObject* InteractiveObject)
{
	IInteractiveInterface* InteractiveItem = Cast<IInteractiveInterface>(InteractiveObject);
	if (InteractiveItem)
	{
		InteractiveItems.AddUnique(InteractiveItem);
	}
}

void UGKInteractionComp::ExitInteractiveItems(UObject* InteractiveObject)
{
	IInteractiveInterface* InteractiveItem = Cast<IInteractiveInterface>(InteractiveObject);
	if (InteractiveItem)
	{
		InteractiveItems.Remove(InteractiveItem);
	}
}
