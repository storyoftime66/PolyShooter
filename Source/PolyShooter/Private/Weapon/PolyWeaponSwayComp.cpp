// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PolyWeaponSwayComp.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"

UPolyWeaponSwayComp::UPolyWeaponSwayComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	YawLagMod = 3.0f;
	PitchLagMod = 2.0;
	RecoveryRate = 12.0f;
	MaxYawLag = 2.0f;
	MaxPitchLag = 1.2f;

	MaxRollLag = 2.0f;
	RollLagMod = 2.0f;
}


void UPolyWeaponSwayComp::BeginPlay()
{
	Super::BeginPlay();

	// 从Character上尝试获取PlayerCameraManager
	if (const ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (PlayerCharacter->IsLocallyControlled())
		{
			if (auto* PC = PlayerCharacter->GetController<APlayerController>())
			{
				PlayerController = PC;
				SetComponentTickEnabled(true);
			}
		}
	}
}


void UPolyWeaponSwayComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 镜头移动导致的手臂旋转滞后
	if (MaxPitchLag != 0.0f)
	{
		TargetRotation.Pitch = FMath::Clamp(ViewAxis[1] * PitchLagMod, -MaxPitchLag, MaxPitchLag);
	}
	if (MaxYawLag != 0.0f)
	{
		TargetRotation.Yaw = FMath::Clamp(-ViewAxis[0] * YawLagMod, -MaxYawLag, MaxYawLag);
	}

	// 角色左右移动导致的手臂滚动（Roll）
	if (MaxRollLag != 0.0f)
	{
		TargetRotation.Roll = FMath::Clamp(MoveAxis[0] * RollLagMod, -MaxRollLag, MaxRollLag);
	}

	SetRelativeRotation_Direct(FMath::RInterpTo(GetRelativeRotation(), TargetRotation, DeltaTime, RecoveryRate));
}

void UPolyWeaponSwayComp::UpdateViewInput(const FInputActionValue& NewViewAxis)
{
	ViewAxis[0] = NewViewAxis[0];
	ViewAxis[1] = NewViewAxis[1];
}

void UPolyWeaponSwayComp::UpdateMoveInput(const FInputActionValue& NewMoveAxis)
{
	MoveAxis[0] = NewMoveAxis[0];
	MoveAxis[1] = NewMoveAxis[1];
}
