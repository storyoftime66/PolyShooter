// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PolyWeaponComp.h"
#include "Weapon/PolyWeaponBase.h"
#include "PolyCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY(LogWeapon);

// Sets default values for this component's properties
UPolyWeaponComp::UPolyWeaponComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	WeaponState = EPolyWeaponState::Idle;
	ArmState = EPolyArmState::Hands;
	WeaponSocket = FName("WeaponSocket");
	
	// 玩家输入、状态
	bHoldingFire = false;
	bIsAiming = false;
}

void UPolyWeaponComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPolyWeaponComp, WeaponState);
	DOREPLIFETIME(UPolyWeaponComp, ArmState);

	DOREPLIFETIME(UPolyWeaponComp, PrimaryWeapon);
	DOREPLIFETIME(UPolyWeaponComp, SecondaryWeapon);
}

void UPolyWeaponComp::OnRep_WeaponState()
{
	if (WeaponState == EPolyWeaponState::Reloading)
	{
		SimulateReloading();
	}
}

void UPolyWeaponComp::OnRep_PrimaryWeapon()
{
	// 捡起武器后自动进行装备
	if (PrimaryWeapon and IsFirstPerson())
	{
		EquipPrimary();
	}
}

void UPolyWeaponComp::OnRep_SecondaryWeapon()
{
	// 捡起武器后自动进行装备
	if (SecondaryWeapon and IsFirstPerson())
	{
		EquipSecondary();
	}
}

void UPolyWeaponComp::OnRep_ArmState()
{
	// 切换武器后更新本地WeaponComp数据

	if (ArmState == EPolyArmState::Hands)
	{
		AnimPack = HandAnimPack;
	}
	else
	{
		// 更新角色动画包
		AnimPack = GetCurrentWeapon()->GetCharFPSAnimPack();

		// 设置新的AdsTimeline
		AdsTimeline = FTimeline();
		if (GetCurrentWeapon() and GetCurrentWeapon()->AdsCurve)
		{
			AdsTimeline = FTimeline();
			FOnTimelineFloat OnAdsTimeline;
			OnAdsTimeline.BindUFunction(this, FName("AdsTimelineProgress"));
			AdsTimeline.AddInterpFloat(GetCurrentWeapon()->GetAdsCurve(), OnAdsTimeline);
		}
	}

	SimulateEquipping();
}

void UPolyWeaponComp::BeginPlay()
{
	Super::BeginPlay();

	// 初始化设置，获取角色的骨骼网格体组件、摄像机组件、移动组件
	// 需要等到角色的骨骼网格体都初始化完成后才能进行
	if (AActor* Owner = GetOwner())
	{
		// 获取模拟射击动作使用的骨架网格体组件、摄像机组件、移动组件
		if (const APolyCharacterBase* PolyChar = Cast<APolyCharacterBase>(Owner))
		{
			// Note: 对 APolyCharacterBase 特殊处理
			FirstPersonMesh = PolyChar->GetFirstPersonMesh();
			ThirdPersonMesh = PolyChar->GetThirdPersonMesh();
			Camera = PolyChar->GetCamera();
			CharacterMovement = PolyChar->GetCharacterMovement();
		}
		else if (const ACharacter* Character = Cast<ACharacter>(Owner))
		{
			// 对一般 Character 的处理
			TArray<UActorComponent*> SkeletalMeshes;
			Character->GetComponents(USkeletalMeshComponent::StaticClass(), SkeletalMeshes);
			if (SkeletalMeshes.Num() >= 1)
			{
				ThirdPersonMesh = Cast<USkeletalMeshComponent>(SkeletalMeshes[0]);
				if (SkeletalMeshes.Num() >= 2)
				{
					FirstPersonMesh = Cast<USkeletalMeshComponent>(SkeletalMeshes[1]);
				}
			}
			else
			{
				UE_LOG(LogWeapon, Warning, TEXT("[%s] WeaponComp's SkeletalMeshComps haven't been set."), *GetName());
			}
			Camera = Owner->FindComponentByClass<UCameraComponent>();
			CharacterMovement = Owner->FindComponentByClass<UCharacterMovementComponent>();
		}
		else
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] WeaponComp's Owner needs to be a Character."), *GetName());
		}

		// 设置 CharAnimInstance
		if (IsFirstPerson())
		{
			if (FirstPersonMesh)
			{
				CharAnimInstance = FirstPersonMesh->GetAnimInstance();
			}
		}
		else
		{
			if (ThirdPersonMesh)
			{
				CharAnimInstance = ThirdPersonMesh->GetAnimInstance();
			}
		}
	}
}

void UPolyWeaponComp::AdsTimelineProgress(float Value)
{
	if (ArmState != EPolyArmState::Hands)
	{
		FTransform TargetTransform;
		TargetTransform.Blend(AnimPack.OriginTransform, AnimPack.AimTransform, Value);

		FirstPersonMesh->SetRelativeTransform(TargetTransform);
	}
}

void UPolyWeaponComp::RequestMulticastPlayMontage(UAnimMontage* MontageToPlay, bool OtherReliable) const
{
	LocalPlayMontage(MontageToPlay);
	ServerPlayMontage(MontageToPlay, OtherReliable);
}

void UPolyWeaponComp::ServerPlayMontage_Implementation(UAnimMontage* MontageToPlay, bool OtherReliable) const
{
	if (OtherReliable)
	{
		MulticastPlayMontageReliable(MontageToPlay);
	}
	else
	{
		MulticastPlayMontageUnreliable(MontageToPlay);
	}
}

void UPolyWeaponComp::MulticastPlayMontageUnreliable_Implementation(UAnimMontage* MontageToPlay) const
{
	if (CharAnimInstance and (GetOwnerRole() == ROLE_AutonomousProxy or GetOwnerRole() == ROLE_Authority))
	{
		CharAnimInstance->Montage_Play(MontageToPlay);
	}
}

void UPolyWeaponComp::MulticastPlayMontageReliable_Implementation(UAnimMontage* MontageToPlay) const
{
	if (CharAnimInstance and (GetOwnerRole() == ROLE_AutonomousProxy or GetOwnerRole() == ROLE_Authority))
	{
		CharAnimInstance->Montage_Play(MontageToPlay);
	}
}

void UPolyWeaponComp::SimulateFiring()
{
	if (bIsAiming)
	{
		LocalPlayMontage(AnimPack.AdsFireMontage);
	}
	else
	{
		LocalPlayMontage(AnimPack.FireMontage);
	}

	if (IsFirstPerson())
	{
		// TODO: CameraShake、Recal
	}
}

void UPolyWeaponComp::SimulateReloading()
{
	LocalPlayMontage(AnimPack.ReloadMontage);
	GetCurrentWeapon()->SimulateReloading();
}

void UPolyWeaponComp::SimulateEquipping()
{
	LocalPlayMontage(AnimPack.EquipMontage);
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetRelativeTransform(AnimPack.OriginTransform);
	}
}

void UPolyWeaponComp::SimulateAds_Implementation()
{
	// Do Simulate
	if (IsFirstPerson())
	{
		LogDebugMessage("SimulateAds");
	}
}

void UPolyWeaponComp::ServerSimulateAds_Implementation()
{
	SimulateAds();
}

void UPolyWeaponComp::StopSimulateAds_Implementation()
{
	// Do Simulate
	if (IsFirstPerson())
	{
	}
}

void UPolyWeaponComp::ServerStopSimulateAds_Implementation()
{
	StopSimulateAds();
}

void UPolyWeaponComp::NotifyFireFinished()
{
	if (WeaponState == EPolyWeaponState::Firing)
	{
		UpdateWeaponState(EPolyWeaponState::Idle);
	}
}

void UPolyWeaponComp::NotifyReloadFinished()
{
	if (WeaponState == EPolyWeaponState::Reloading)
	{
		UpdateWeaponState(EPolyWeaponState::Idle);
	}
}

void UPolyWeaponComp::NotifyEquipFinished()
{
	UpdateWeaponState(EPolyWeaponState::Idle);
}

void UPolyWeaponComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsFirstPerson())
	{
		AdsTimeline.TickTimeline(DeltaTime);
	}
}

void UPolyWeaponComp::StartFiring()
{
	bHoldingFire = true;

	if (CanFire())
	{
		ServerStartFiring();
	}
}

void UPolyWeaponComp::ServerStartFiring_Implementation()
{
	if (CanFire())
	{
		ServerUpdateWeaponState(EPolyWeaponState::Firing);
		GetCurrentWeapon()->HandleFiring();
	}
}

void UPolyWeaponComp::StopFiring()
{
	bHoldingFire = false;

	if (WeaponState == EPolyWeaponState::Firing)
	{
		ServerStopFiring();
	}
}

void UPolyWeaponComp::ServerStopFiring_Implementation()
{
	if (WeaponState == EPolyWeaponState::Firing and GetCurrentWeapon())
	{
		ServerUpdateWeaponState(EPolyWeaponState::Idle);
		GetCurrentWeapon()->StopFiring();
	}
}

void UPolyWeaponComp::StartReloading()
{
	if (CanReload())
	{
		if (bIsAiming)
		{
			StopADS();
		}
		ServerStartReloading();
	}
}

void UPolyWeaponComp::StopReloading()
{
	if (WeaponState == EPolyWeaponState::Reloading and GetCurrentWeapon())
	{
		GetCurrentWeapon()->StopReloading();
	}
}

void UPolyWeaponComp::ServerStartReloading_Implementation()
{
	if (CanReload())
	{
		UpdateWeaponState(EPolyWeaponState::Reloading);
		GetCurrentWeapon()->HandleReloading();
	}
}

void UPolyWeaponComp::ServerStopReloading_Implementation()
{
	if (WeaponState == EPolyWeaponState::Reloading and GetCurrentWeapon())
	{
		GetCurrentWeapon()->StopReloading();
		UpdateWeaponState(EPolyWeaponState::Idle);
	}
}

void UPolyWeaponComp::StartADS()
{
	if (WeaponState == EPolyWeaponState::Idle or WeaponState == EPolyWeaponState::Firing)
	{
		bIsAiming = true;

		AdsTimeline.Play();
		LocalPlayMontage(AnimPack.AdsUpMontage);

		ServerSimulateAds();
	}
}

void UPolyWeaponComp::StopADS()
{
	bIsAiming = false;

	AdsTimeline.Reverse();
	LocalPlayMontage(AnimPack.AdsDownMontage);

	ServerStopSimulateAds();
}

void UPolyWeaponComp::EquipPrimary()
{
	if (PrimaryWeapon and ArmState != EPolyArmState::Primary)
	{
		if (bIsAiming)
		{
			StopADS();
		}
		ServerEquipPrimary();
	}
}

void UPolyWeaponComp::ServerEquipPrimary_Implementation()
{
	if (ArmState == EPolyArmState::Secondary and SecondaryWeapon)
	{
		SecondaryWeapon->UnEquipped();
	}

	if (ArmState != EPolyArmState::Primary and PrimaryWeapon)
	{
		UpdateWeaponState(EPolyWeaponState::Equipping);
		PrimaryWeapon->Equipped();
		// Note: 玩家在客户端时，OnRep_ArmState()会调用SimulateEquipping()
		UpdateArmState(EPolyArmState::Primary);

		if (IsFirstPerson())
		{
			// 若玩家在服务器，则需要手动调用SimulateEquipping()
			SimulateEquipping();
		}
	}
}

void UPolyWeaponComp::EquipSecondary()
{
	if (SecondaryWeapon and ArmState != EPolyArmState::Secondary)
	{
		if (bIsAiming)
		{
			StopADS();
		}
		ServerEquipSecondary();
	}
}

void UPolyWeaponComp::ServerEquipSecondary_Implementation()
{
	if (ArmState == EPolyArmState::Primary and PrimaryWeapon)
	{
		PrimaryWeapon->UnEquipped();
	}

	if (ArmState != EPolyArmState::Secondary)
	{
		UpdateWeaponState(EPolyWeaponState::Equipping);
		SecondaryWeapon->Equipped();
		UpdateArmState(EPolyArmState::Secondary);

		if (IsFirstPerson())
		{
			// 玩家角色在服务器上运行，需要手动调用会调用SimulateEquipping()
			SimulateEquipping();
		}
	}
}

void UPolyWeaponComp::EquipHands()
{
}

void UPolyWeaponComp::ServerEquipHands_Implementation()
{
}

void UPolyWeaponComp::PickUpWeapon(APolyWeaponBase* NewWeapon)
{
	check(GetOwnerRole() == ROLE_Authority);
	
	// TODO: 考虑在装弹、开火、装备武器的时候PickUp
	if (NewWeapon and !IsValid(NewWeapon->Holder))
	{
		NewWeapon->PickedUp(this); // 设置武器Holder

		if (ArmState == EPolyArmState::Secondary)
		{
			// 丢弃原副武器
			if (SecondaryWeapon)
			{
				SecondaryWeapon->ThrewDown();
			}
			// 仅设置武器，由OnRep_SecondaryWeapon回调调用EquipSecondary
			SecondaryWeapon = NewWeapon;
			if (IsFirstPerson())
			{
				// 单人游戏不会复制，需要手动调用
				EquipPrimary();
			}
		}

		if (ArmState == EPolyArmState::Primary or ArmState == EPolyArmState::Hands)
		{
			// 丢弃原主武器，空手或手持主武器时拾取新武器均替换主武器
			if (PrimaryWeapon)
			{
				PrimaryWeapon->ThrewDown();
			}

			// 仅设置武器，由OnRep_PrimaryWeapon回调调用EquipPrimary
			PrimaryWeapon = NewWeapon;
			if (IsFirstPerson())
			{
				// 单人游戏不会复制，需要手动调用
				EquipPrimary();
			}
		}
	}
}

void UPolyWeaponComp::StartInspecting()
{
	if (WeaponState == EPolyWeaponState::Idle)
	{
		if (bIsAiming)
		{
			StopADS();
		}
		LocalPlayMontage(AnimPack.InspectMontage);
		if (GetCurrentWeapon())
		{
			GetCurrentWeapon()->SimulateInspecting();
		}
	}
}

bool UPolyWeaponComp::CanFire() const
{
	return WeaponState != EPolyWeaponState::Equipping and WeaponState != EPolyWeaponState::Reloading
		and GetCurrentWeapon() and GetCurrentWeapon()->CanFire();
}

bool UPolyWeaponComp::CanReload() const
{
	return WeaponState == EPolyWeaponState::Idle or WeaponState == EPolyWeaponState::Firing
		and GetCurrentWeapon() and GetCurrentWeapon()->CanReload();
}

AController* UPolyWeaponComp::GetOwnerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Pawn->GetController() : nullptr;
}

void UPolyWeaponComp::GetCameraLocationAndRotation(FVector& CameraLocation, FRotator& CameraRotation)
{
	// TODO: 兼容无Camera的Character（如AI控制的）
	check(Camera);

	CameraLocation = Camera->GetComponentLocation();
	CameraRotation = Camera->GetComponentRotation();
}

FTransform UPolyWeaponComp::GetLeftHandIKTransform() const
{
	if (GetCurrentWeapon() and FirstPersonMesh)
	{
		const FTransform IKSocketTransfrom = GetCurrentWeapon()->GetFirstPersonMesh()->GetSocketTransform(FName("ik_hand_l"));
		const FMatrix WorldCoord = FRotationTranslationMatrix(IKSocketTransfrom.Rotator(), IKSocketTransfrom.GetLocation());
		const FMatrix WorldToComponent = FirstPersonMesh->GetComponentToWorld().ToMatrixWithScale().Inverse();
		const FMatrix ComponentCoord = WorldCoord * WorldToComponent;
		return FTransform(ComponentCoord.ToQuat(), ComponentCoord.GetOrigin());
	}
	return FTransform();
}

FTransform UPolyWeaponComp::GetLeftHandIKWorldTransform() const
{
	if (GetCurrentWeapon() and FirstPersonMesh)
	{
		return GetCurrentWeapon()->GetFirstPersonMesh()->GetSocketTransform(FName("ik_hand_l"));
	}
	return FTransform();
}

void UPolyWeaponComp::UpdateWeaponState(EPolyWeaponState NewWeaponState)
{
	WeaponState = NewWeaponState;

	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerUpdateWeaponState(NewWeaponState);
	}
	else
	{
		OnRep_WeaponState();
	}
}

void UPolyWeaponComp::ServerUpdateWeaponState_Implementation(EPolyWeaponState NewWeaponState)
{
	UpdateWeaponState(NewWeaponState);
}

void UPolyWeaponComp::UpdateArmState(EPolyArmState NewArmState)
{
	ArmState = NewArmState;

	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerUpdateArmState(NewArmState);
	}
	else
	{
		OnRep_ArmState();
	}
}

void UPolyWeaponComp::ServerUpdateArmState_Implementation(EPolyArmState NewArmState)
{
	UpdateArmState(NewArmState);
}
