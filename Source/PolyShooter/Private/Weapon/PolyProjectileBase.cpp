// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PolyProjectileBase.h"

#include "NiagaraFunctionLibrary.h"


// Sets default values
APolyProjectileBase::APolyProjectileBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	bReplicates = true;
	SetActorEnableCollision(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->SetActive(false);
		ProjectileMovement->SetComponentTickEnabled(false);
	}
}

// Called when the game starts or when spawned
void APolyProjectileBase::BeginPlay()
{
	Super::BeginPlay();
}



// Called every frame
void APolyProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}
