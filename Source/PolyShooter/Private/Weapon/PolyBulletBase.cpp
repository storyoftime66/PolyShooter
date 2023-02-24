// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PolyBulletBase.h"

#include "NiagaraFunctionLibrary.h"
#include "PoolManagerBPLibrary.h"
#include "PolyShooter/PolyShooter.h"


// Sets default values
APolyBulletBase::APolyBulletBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bUsePool = false;

	InitialSpeed = 6000.0f;
}

// Called when the game starts or when spawned
void APolyBulletBase::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(InitialLifeSpan);
	SetActorTickEnabled(true);
	GetProjectileMovement()->SetActive(true, true);
	GetProjectileMovement()->SetComponentTickEnabled(true);
	GetProjectileMovement()->Velocity = GetActorRotation().Vector() * InitialSpeed;
}

void APolyBulletBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetProjectileMovement())
	{
		InitialSpeed = GetProjectileMovement()->InitialSpeed;
		TraceLength = InitialSpeed * PrimaryActorTick.TickInterval;
	}
}

void APolyBulletBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	static FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	// 仅在服务端Tick
	// Note: 尝试过在客户端Tick，让客户端自行确定 SpawnImpactFX(Hit) 的时机，但结果是客户端不能生成 ImpactFX。
	// 推测是因为服务端Actor在客户端生成 ImpactFX 之前被销毁导致。
	if (true or GetLocalRole() == ROLE_Authority)
	{
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(
			Hit, GetActorLocation(), GetActorLocation() + GetActorRotation().Vector() * TraceLength, TRACE_WEAPON, QueryParams);
		if (Hit.IsValidBlockingHit())
		{
			// BroadcastImpact(Hit);
			SpawnImpactFX(Hit);

			// TODO: 造成伤害

			// 释放或销毁
			if (bUsePool)
			{
				PoolRelease();
			}
			else
			{
				// TODO: Destroy() 会导致前面的 BroadcastImpact(Hit) 在客户端不会执行
				// Destroy();
				SetLifeSpan(0.1f);  // 维持特效
			}
		}
	}
}

void APolyBulletBase::OnPoolBegin_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("[%s][%d]OnPoolBegin_Implementation"), *GetName(), GetLocalRole());
	bUsePool = true;
	SetLifeSpan(InitialLifeSpan);

	SetActorTickEnabled(true);
	GetProjectileMovement()->SetActive(true, true);
	GetProjectileMovement()->SetComponentTickEnabled(true);

	GetProjectileMovement()->Velocity = GetActorRotation().Vector() * InitialSpeed;

	if (GetLocalRole() == ROLE_Authority)
	{
		BroadcastFetched();
	}
}

void APolyBulletBase::OnPoolEnd_Implementation()
{
	// TODO: 理解Actor复制
	UE_LOG(LogTemp, Display, TEXT("[%s][%d]OnPoolEnd_Implementation"), *GetName(), GetLocalRole());

	SetActorTickEnabled(false);
	GetProjectileMovement()->SetActive(false);
	GetProjectileMovement()->SetComponentTickEnabled(false);
	GetProjectileMovement()->Velocity = FVector::ZeroVector;
	GetWorldTimerManager().ClearTimer(TimerHandle_LifeSpanExpired);

	if (GetLocalRole() == ROLE_Authority)
	{
		BroadcastReleased();
	}
}

void APolyBulletBase::LifeSpanExpired()
{
	if (bUsePool)
	{
		PoolRelease();
	}
	else
	{
		Destroy();
	}
}

void APolyBulletBase::BroadcastFetched_Implementation()
{
	if (GetLocalRole() != ROLE_Authority)
	{
		// 模拟从Pool中取出
		UPoolManagerBPLibrary::TPoolList& BulletPool = UPoolManagerBPLibrary::Pool.FindOrAdd(GetClass());
		BulletPool.Remove(this);
		Execute_OnPoolBegin(this);
	}
}

void APolyBulletBase::BroadcastReleased_Implementation()
{
	if (GetLocalRole() != ROLE_Authority)
	{
		PoolRelease();
	}
}

void APolyBulletBase::BroadcastImpact_Implementation(const FHitResult& Hit)
{
	UE_LOG(LogTemp, Display, TEXT("[%d]BroadcastImpact_Implementation"), GetLocalRole());
	SpawnImpactFX(Hit);
}

void APolyBulletBase::SpawnImpactFX(const FHitResult& Hit)
{
	if (Hit.IsValidBlockingHit() and ImpactFX_NG)
	{
		if (!(GetNetMode() == NM_DedicatedServer and GetLocalRole() == ROLE_Authority))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, ImpactFX_NG,
				Hit.ImpactPoint + Hit.Normal,
				Hit.ImpactNormal.Rotation());
			// TODO: 音效
		}
	}
}
