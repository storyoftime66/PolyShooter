// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyWeaponBase.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PolyProjectileBase.generated.h"

UCLASS()
class POLYSHOOTER_API APolyProjectileBase : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Projectile", meta=(AllowPrivateAccess="true"))
	UProjectileMovementComponent* ProjectileMovement;

public:
	APolyProjectileBase();

protected:
	virtual void BeginPlay() override;
	
	virtual void SpawnImpactFX(const FHitResult& Hit) {}

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
};
