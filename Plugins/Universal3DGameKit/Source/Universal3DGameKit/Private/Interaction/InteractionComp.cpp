// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/InteractionComp.h"
#include "Interaction/InteractiveInterface.h"

// Sets default values for this component's properties
UInteractionComp::UInteractionComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	SetIsReplicatedByDefault(true);

	// 默认交互范围
	SphereRadius = 200.0f;
}

void UInteractionComp::InitializeComponent()
{
	Super::InitializeComponent();

	if (bUseTraceMode)
	{
		SetComponentTickEnabled(true);
		SetCollisionProfileName(FName("NoCollision"));
	}
	else
	{
		SphereRadius = InteractionRange;
		SetComponentTickEnabled(false);
	}

	const UObject* CurrentObject = GetOuter();
	while (CurrentObject)
	{
		const AActor* IgnoreActor = Cast<AActor>(CurrentObject);
		if (IgnoreActor)
		{
			IgnoredActors.Add(IgnoreActor);
			break;
		}
		CurrentObject = CurrentObject->GetOuter();
	}
}

void UInteractionComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UInteractionComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bUseTraceMode)
	{
		TraceInteractiveItem();
	}
}

void UInteractionComp::Interact()
{
	ServerInteract();
}

void UInteractionComp::ServerInteract_Implementation()
{
	InteractFirstItem();
}

bool UInteractionComp::InteractFirstItem()
{
	check(GetOwnerRole() == ROLE_Authority);

	if (InteractiveItems.Num() > 0)
	{
		AActor* FirstItem = InteractiveItems[0];
		IInteractiveInterface* FirstInteractiveItem = Cast<IInteractiveInterface>(FirstItem);
		FirstInteractiveItem->Interacted(GetOwner());
		// Note: 多数可拾取物被拾取后会消失或禁用碰撞，会自动移除InteractiveItems列表中的引用
		InteractiveItems.Remove(FirstItem);
		return true;
	}

	return false;
}

void UInteractionComp::TraceInteractiveItem()
{
	check(GetOwner());
	check(GetWorld());

	FVector CamLoc;
	FRotator CamRot;
	GetOwner()->GetActorEyesViewPoint(CamLoc, CamRot);
	const FVector TraceEnd = CamLoc + CamRot.Vector() * InteractionRange;

	FHitResult OutHit;

	// 下面代码修改自 UKismetSystemLibrary::SphereTraceSingle
	FCollisionQueryParams Params(true);
	Params.bReturnPhysicalMaterial = false;
	Params.bReturnFaceIndex = false;
	Params.AddIgnoredActors(IgnoredActors);

	bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit, CamLoc, TraceEnd, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceScale), Params);
	if (bHit)
	{
		HitActorCache = Cast<IInteractiveInterface>(OutHit.GetActor());
	}
}

void UInteractionComp::EnterInteractiveItems(AActor* InteractiveActor)
{
	if (InteractiveActor and InteractiveActor->Implements<UInteractiveInterface>())
	{
		InteractiveItems.AddUnique(InteractiveActor);
	}
}

void UInteractionComp::ExitInteractiveItems(AActor* InteractiveActor)
{
	if (InteractiveActor and InteractiveItems.Contains(InteractiveActor))
	{
		InteractiveItems.Remove(InteractiveActor);
	}
}
