// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/GunComp.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY(LogWeapon);

// Sets default values for this component's properties
UGunComp::UGunComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// 玩家输入、状态
	bHoldingAds = false;
}

void UGunComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGunComp, GunInventory);
	DOREPLIFETIME(UGunComp, CurrentGunIndex);
}

void UGunComp::AdsTimelineProgress(float Value)
{
	if (IsArmed())
	{
		FTransform TargetTransform;
		TargetTransform.Blend(AnimPack.FPOriginalTransform, AnimPack.FPAimingTransform, Value);

		CharFPMesh->SetRelativeTransform(TargetTransform);
	}
}

void UGunComp::OnRep_GunInventory()
{
	OnRep_CurrentGunIndex();
}

void UGunComp::OnRep_CurrentGunIndex()
{
	// 更新 CurrentGunCache
	if (GunInventory.IsValidIndex(CurrentGunIndex))
	{
		CurrentGunCache = Cast<IGunInterface>(GunInventory[CurrentGunIndex]);
	}
	else
	{
		CurrentGunCache = nullptr;
	}


	// 更新角色在本地的动画包和瞄准信息
	if (!IsArmed())
	{
		AnimPack = HandAnimPack;
	}
	else
	{
		if (CurrentGunCache)
		{
			AnimPack = CurrentGunCache->GetCharAnimPack();
			AdsInfoCache = CurrentGunCache->GetAdsInfo();
		}

		if (AdsInfoCache.AdsCurve)
		{
			AdsTimeline = FTimeline();
			FOnTimelineFloat OnAdsTimeline;
			OnAdsTimeline.BindUFunction(this, FName("AdsTimelineProgress"));
			AdsTimeline.AddInterpFloat(AdsInfoCache.AdsCurve, OnAdsTimeline);
		}
	}
}

void UGunComp::ServerSwapGunByIndex_Implementation(int32 GunIndex)
{
	if (IsArmed())
	{
		CurrentGunCache->BeUnequipped();
	}

	SetCurrentGunIndex(GunIndex);
	CurrentGunCache->BeEquipped();
}

bool UGunComp::ServerSwapGunByIndex_Validate(int32 GunIndex)
{
	return (!IsArmed() or CurrentGunCache->CanRetract()) and GunInventory.IsValidIndex(GunIndex);
}

void UGunComp::ServerRetractGun_Implementation()
{
	CurrentGunCache->BeUnequipped();
	SetCurrentGunIndex(-1);
}

bool UGunComp::ServerRetractGun_Validate()
{
	return IsArmed() and CurrentGunCache->CanRetract();
}

void UGunComp::ServerEnterGunInventory_Implementation(AActor* NewGun)
{
	check(GetOwnerRole() == ROLE_Authority);

	GunInventory.AddUnique(NewGun);
	Cast<IGunInterface>(NewGun)->SetGunComp(this);
}

bool UGunComp::ServerEnterGunInventory_Validate(AActor* NewGun)
{
	return NewGun and NewGun->Implements<UGunInterface>() and !GunInventory.Contains(NewGun);
}

void UGunComp::ServerLeaveGunInventory_Implementation(AActor* Gun)
{
	GunInventory.Remove(Gun);
	Cast<IGunInterface>(Gun)->SetGunComp(nullptr);
}

bool UGunComp::ServerLeaveGunInventory_Validate(AActor* Gun)
{
	return Gun and Gun->Implements<UGunInterface>() and GunInventory.Contains(Gun);
}

void UGunComp::CharSimulateFiring()
{
	check(IsArmed());

	UAnimMontage* MontageToPlay = AnimPack.FireMontage;
	if (CurrentGunCache->GetAds() and AnimPack.AdsFireMontage)
	{
		MontageToPlay = AnimPack.AdsFireMontage;
	}
	LocalPlayMontage(MontageToPlay);

	if (IsOwnerLocallyControlled())
	{
		// TODO: CameraShake、Recal
	}
}

void UGunComp::CharSimulateReloading()
{
	check(IsArmed());
	UAnimMontage* MontageToPlay = AnimPack.ReloadMontage;
	if (CurrentGunCache->GetAmmoInClip() == 0 and AnimPack.ReloadMontage_Empty)
	{
		MontageToPlay = AnimPack.ReloadMontage_Empty;
	}
	LocalPlayMontage(MontageToPlay);
}

void UGunComp::CharSimulateEquipping()
{
	LocalPlayMontage(AnimPack.EquipMontage);
	if (CharFPMesh)
	{
		CharFPMesh->SetRelativeTransform(AnimPack.FPOriginalTransform);
	}
}

void UGunComp::CharSimulateAdsIn()
{
	if (IsOwnerLocallyControlled())
	{
		AdsTimeline.Play();
	}
	LocalPlayMontage(AnimPack.AdsUpMontage);
}

void UGunComp::CharSimulateAdsOut()
{
	if (IsOwnerLocallyControlled())
	{
		AdsTimeline.Reverse();
	}
	LocalPlayMontage(AnimPack.AdsDownMontage);
}

void UGunComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsOwnerLocallyControlled())
	{
		AdsTimeline.TickTimeline(DeltaTime);

		if (bHoldingFire and IsArmed() and CurrentGunCache->CanFire())
		{
			CurrentGunCache->StartFiring();
		}
	}
}

void UGunComp::StartInspecting()
{
	if (IsArmed() and GetGunState() == EGunState::Idle and !bHoldingAds and !CurrentGunCache->GetAds())
	{
		LocalPlayMontage(AnimPack.InspectMontage);
	}
}

void UGunComp::SetCurrentGunIndex(int32 NewIndex)
{
	const int32 OldGunIndex = CurrentGunIndex;
	CurrentGunIndex = NewIndex;
	if (OldGunIndex != CurrentGunIndex and GetNetMode() == NM_ListenServer and IsOwnerLocallyControlled())
	{
		OnRep_CurrentGunIndex();
	}
}

void UGunComp::InitialGunComp(USkeletalMeshComponent* FPMesh, USkeletalMeshComponent* TPMesh, UCameraComponent* FPCamera, UCharacterMovementComponent* InCharacterMovement)
{
	CharFPMesh = FPMesh;
	CharTPMesh = TPMesh;
	Camera = FPCamera;
	CharacterMovement = InCharacterMovement;

	// 设置动画实例
	if (IsOwnerLocallyControlled())
	{
		if (CharFPMesh)
		{
			CharFPAnimInstance = CharFPMesh->GetAnimInstance();
		}
	}
	if (CharTPMesh)
	{
		CharTPAnimInstance = CharTPMesh->GetAnimInstance();
	}
}

void UGunComp::SetEnableCameraEffect(bool NewEnable)
{
	// TODO
}

void UGunComp::SetEnableMovementEffect(bool NewEnable)
{
	// TODO
}

void UGunComp::FirePressed()
{
	bHoldingFire = true;

	if (IsArmed())
	{
		CurrentGunCache->StartFiring();
	}
}

void UGunComp::FireReleased()
{
	bHoldingFire = false;

	if (IsArmed())
	{
		CurrentGunCache->StopFiring();
	}
}

void UGunComp::ReloadPressed()
{
	if (IsArmed())
	{
		CurrentGunCache->StartReloading();
	}
}

void UGunComp::AdsPressed()
{
	bHoldingAds = true;

	if (IsArmed())
	{
		CurrentGunCache->SetAds(true);
	}
}

void UGunComp::AdsReleased()
{
	bHoldingAds = false;
	if (IsArmed())
	{
		CurrentGunCache->SetAds(false);
	}
}

void UGunComp::NextGun()
{
	if (GunInventory.IsValidIndex(CurrentGunIndex) and GunInventory.IsValidIndex(CurrentGunIndex + 1))
	{
		EquipGunByIndex(CurrentGunIndex + 1);
	}
	else if (GunInventory.Num() > 0)
	{
		EquipGunByIndex(0);
	}
}

void UGunComp::PrevGun()
{
	if (GunInventory.IsValidIndex(CurrentGunIndex) and GunInventory.IsValidIndex(CurrentGunIndex - 1))
	{
		EquipGunByIndex(CurrentGunIndex - 1);
	}
	else if (GunInventory.Num() > 0)
	{
		EquipGunByIndex(GunInventory.Num() - 1);
	}
}

void UGunComp::RetractGun()
{
	if (IsArmed())
	{
		ServerRetractGun();
	}
}

void UGunComp::EquipGunByIndex(int32 GunIndex)
{
	if (GunInventory.IsValidIndex(GunIndex))
	{
		ServerSwapGunByIndex(GunIndex);
	}
	else
	{
		ServerRetractGun();
	}
}

void UGunComp::EnterGunInventory(AActor* NewGun)
{
	if (NewGun and NewGun->Implements<UGunInterface>() and !GunInventory.Contains(NewGun))
	{
		ServerEnterGunInventory(NewGun);
	}
}

void UGunComp::LeaveGunInventory(AActor* Gun)
{
	if (Gun and Gun->Implements<UGunInterface>() and GunInventory.Contains(Gun))
	{
		ServerLeaveGunInventory(Gun);
	}
}

void UGunComp::NotifyCompGunEvent(EGunEvent GunEvent)
{
	check(IsArmed());

	switch (GunEvent)
	{
	case EGunEvent::Event_SimulateFiring:
		CharSimulateFiring();
		break;
	case EGunEvent::Event_SimulateReloading:
		CharSimulateReloading();
		break;
	case EGunEvent::Event_SimulateEquipping:
		CharSimulateEquipping();
		break;
	case EGunEvent::Event_SimulateAdsIn:
		CharSimulateAdsIn();
		break;
	case EGunEvent::Event_SimulateAdsOut:
		CharSimulateAdsOut();
		break;
	default:
		break;
	}
}

AController* UGunComp::GetOwnerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Pawn->GetController() : nullptr;
}

void UGunComp::GetOwnerEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	check(GetOwner());
	GetOwner()->GetActorEyesViewPoint(OutLocation, OutRotation);
}

FTransform UGunComp::GetLeftHandIKTransform() const
{
	// TODO
	// if (GetCurrentGun() and CharFPMesh)
	// {
	// 	const FTransform IKSocketTransfrom = GetCurrentGun()->GetFirstPersonMesh()->GetSocketTransform(FName("ik_hand_l"));
	// 	const FMatrix WorldCoord = FRotationTranslationMatrix(IKSocketTransfrom.Rotator(), IKSocketTransfrom.GetLocation());
	// 	const FMatrix WorldToComponent = CharFPMesh->GetComponentToWorld().ToMatrixWithScale().Inverse();
	// 	const FMatrix ComponentCoord = WorldCoord * WorldToComponent;
	// 	return FTransform(ComponentCoord.ToQuat(), ComponentCoord.GetOrigin());
	// }
	return FTransform();
}

FTransform UGunComp::GetLeftHandIKWorldTransform() const
{
	// TODO
	// if (GetCurrentGun() and CharFPMesh)
	// {
	// 	return GetCurrentGun()->GetFirstPersonMesh()->GetSocketTransform(FName("ik_hand_l"));
	// }
	return FTransform();
}
