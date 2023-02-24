// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyProjectileBase.h"
#include "PoolActorInterface.h"
#include "PolyBulletBase.generated.h"

UCLASS()
class POLYSHOOTER_API APolyBulletBase : public APolyProjectileBase, public IPoolActorInterface
{
	GENERATED_BODY()
	
private:
	//////////////////////////////////////////////////
	/// 子弹属性
	float InitialSpeed;
	float TraceLength;

	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Projectile")
	UNiagaraSystem* ImpactFX_NG;

	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Projectile")
	bool bUsePool;

public:
	APolyBulletBase();
	virtual void PostInitializeComponents() override;

protected:
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

	/** 通知客户端调用OnPoolBegin和OnPoolEnd */
	UFUNCTION(NetMulticast, Reliable)
	virtual void BroadcastFetched();
	UFUNCTION(NetMulticast, Reliable)
	virtual void BroadcastReleased();


public:
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Projectile")
	FORCEINLINE float GetInitialSpeed() const { return InitialSpeed; }

	// 生成命中特效的临时解决方案
	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastImpact(const FHitResult& Hit);

	/** [local] 生成命中特效 */
	virtual void SpawnImpactFX(const FHitResult& Hit) override;

	virtual void OnPoolBegin_Implementation() override;
	virtual void OnPoolEnd_Implementation() override;
};
