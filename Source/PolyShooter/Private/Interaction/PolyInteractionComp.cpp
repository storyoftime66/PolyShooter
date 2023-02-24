// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/PolyInteractionComp.h"

// Sets default values for this component's properties
UPolyInteractionComp::UPolyInteractionComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UPolyInteractionComp::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPolyInteractionComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
}

void UPolyInteractionComp::Interact()
{
	ServerInteract();
}

void UPolyInteractionComp::ServerInteract_Implementation()
{
	ServerInteractFirst();
}

void UPolyInteractionComp::ServerInteractFirst()
{
	if (InteractiveProps.Num() > 0)
	{
		InteractiveProps[0]->Interacted(GetOwner());
		// Note: 多数可拾取物被拾取后会消失或禁用碰撞，会自动移除InteractiveProps列表中的引用
		// InteractiveProps.RemoveAt(0);
	}
}

