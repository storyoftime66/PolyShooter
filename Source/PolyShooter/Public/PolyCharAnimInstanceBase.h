// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyCharacterBase.h"
#include "PolyCharAnimInstanceBase.generated.h"

/**
 * 角色蓝图基类
 */
UCLASS()
class POLYSHOOTER_API UPolyCharAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Category="PolyShooter|AnimInstance")
	APolyCharacterBase* PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category="PolyShooter|AnimInstance")
	UPolyWeaponComp* PlayerWeaponComp;
	
public:
	///////////////////////////////////
	/// 角色状态
	float Speed;
	EPolyMovementState MovementState;
	EPolyWeaponState WeaponState;
	bool InAds;
	FCharFPSAnimationPack CharAnimPack;

public:
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
