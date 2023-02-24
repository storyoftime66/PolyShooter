// Fill out your copyright notice in the Description page of Project Settings.


#include "PolyCharAnimInstanceBase.h"

void UPolyCharAnimInstanceBase::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	PlayerCharacter = Cast<APolyCharacterBase>(TryGetPawnOwner());
	if (PlayerCharacter)
	{
		PlayerWeaponComp = PlayerCharacter->GetWeaponComp();
	}
}

void UPolyCharAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(PlayerCharacter) or !IsValid(PlayerWeaponComp))
	{
		return;
	}

	Speed = PlayerCharacter->GetVelocity().Size2D();
	MovementState = PlayerCharacter->GetMovementState();
	WeaponState = PlayerWeaponComp->GetWeaponState();
	InAds = PlayerWeaponComp->IsAiming();
	if (WeaponState == EPolyWeaponState::Equipping)
	{
		CharAnimPack = PlayerWeaponComp->GetAnimPack();
	}
}
