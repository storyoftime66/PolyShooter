// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PolyWeaponBase.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PolyWeaponComp.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Display, All);

class APolyWeaponBase;
class UCameraComponent;

/**
 * 武器状态，由WeaponComp维护
 */
UENUM(BlueprintType)
enum class EPolyWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Equipping
};

/**
 * 武器持有状态，表示当前装备哪一把武器。
 */
UENUM()
enum class EPolyArmState :uint8
{
	Hands,
	Primary,
	Secondary
};

/**
 * 武器组件
 * 实现和武器交互、切换武器、拾取武器（？）、网络复制功能
 * Note:
 *	1. 需要角色类手动调用来实现功能的方法，主要在 WEAPON_OPERATIONS 中：
 *		- 瞄准、停止瞄准（ADS）: StartADS(), StopADS()
 *		- 射击、装弹：StartFiring(), StopFiring(), StartReloading(), StopReloading()
 *		- 更换武器：EquipPrimary(), EquipSecondary(), EquipHands()
 */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class POLYSHOOTER_API UPolyWeaponComp : public UActorComponent
{
	GENERATED_BODY()

private:
	////////////////////////////////////////////////
	// 角色相关
	// 角色骨架网格体组件引用，用于播放射击动画
	UPROPERTY()
	USkeletalMeshComponent* FirstPersonMesh;
	UPROPERTY()
	USkeletalMeshComponent* ThirdPersonMesh;
	UPROPERTY()
	UAnimInstance* CharAnimInstance;
	UPROPERTY(EditAnywhere, Category="PolyShooter|WeaponComp")
	FName WeaponSocket;
	// 相机组件和移动组件引用，用于修改FOV、修改移动速度等
	UPROPERTY()
	UCameraComponent* Camera;
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovement;

	////////////////////////////////////////////////
	/// 武器信息
	/** 持有的武器 */
	UPROPERTY(ReplicatedUsing=OnRep_PrimaryWeapon)
	APolyWeaponBase* PrimaryWeapon;
	UPROPERTY(ReplicatedUsing=OnRep_SecondaryWeapon)
	APolyWeaponBase* SecondaryWeapon;
	/* 持有状态，持有状态需要请求服务端更新。
	   在拾取新武器时，会先更新PrimaryWeapon或SecondaryWeapon，再由OnRep来请求更新持有状态。*/
	UPROPERTY(ReplicatedUsing=OnRep_ArmState)
	EPolyArmState ArmState;
	/** 当前角色动画（local）*/
	FCharFPSAnimationPack AnimPack;
	/** 武器使用状态 */
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState)
	EPolyWeaponState WeaponState;

	//////////////////////////////////////////////
	/// 武器瞄准
	/** 瞄准状态 */
	UPROPERTY()
	bool bIsAiming;
	/** 转换到瞄准状态的时间轴，时间轴曲线由武器确定 */
	FTimeline AdsTimeline;

	////////////////////////////////////////////////
	// 玩家输入信息
	/** 玩家是否按住射击键 */
	bool bHoldingFire;

	////////////////////////////////////////////////
	/// 内部变量
	/** 空手时的角色动画 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|WeaponComp")
	FCharFPSAnimationPack HandAnimPack;

public:
	UPolyWeaponComp();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** 瞄准操作变换网格体位置 */
	UFUNCTION()
	void AdsTimelineProgress(float Value);

#pragma region ONREP_METHODS
	UFUNCTION()
	void OnRep_WeaponState();

	/** OnRep_PrimaryWeapon 和 OnRep_SecondaryWeapon 用于确保PrimaryWeapon和SecondaryWeapon成功复制到客户端后
	 *  再进行装备武器操作，保证角色可以播放的装备武器动画。 */
	UFUNCTION()
	void OnRep_PrimaryWeapon();
	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UFUNCTION()
	void OnRep_ArmState();
#pragma endregion

#pragma region ANIMATION
	/** Helper function，仅本地播放Montage */
	FORCEINLINE void LocalPlayMontage(UAnimMontage* MontageToPlay) const
	{
		if (CharAnimInstance)
		{
			CharAnimInstance->Montage_Play(MontageToPlay);
		}
	}

	/** Helper function，广播播放Montage */
	void RequestMulticastPlayMontage(UAnimMontage* MontageToPlay, bool OtherReliable = false) const;

	UFUNCTION(Server, Reliable)
	void ServerPlayMontage(UAnimMontage* MontageToPlay, bool OtherReliable) const;
	UFUNCTION(Server, Unreliable)
	void MulticastPlayMontageUnreliable(UAnimMontage* MontageToPlay) const;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMontageReliable(UAnimMontage* MontageToPlay) const;
#pragma endregion

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region WEAPON_OPERATIONS
	//////////////////////////////////
	/// 使用武器的操作，供OwnerActor调用，由客户端发起

	// 射击、装弹、瞄准
	UFUNCTION()
	void StartFiring();
	UFUNCTION(Server, Reliable)
	void ServerStartFiring();
	UFUNCTION()
	void StopFiring();
	UFUNCTION(Server, Reliable)
	void ServerStopFiring();
	UFUNCTION()
	void StartReloading();
	UFUNCTION(Server, Reliable)
	void ServerStartReloading();
	UFUNCTION()
	void StopReloading();
	UFUNCTION(Server, Reliable)
	void ServerStopReloading();
	UFUNCTION()
	void StartADS();
	UFUNCTION()
	void StopADS();

	// 更换武器。
	// 更换武器的目的是更新WeaponComp中所有与当前武器相关的变量，如ArmState, AnimPack, 
	UFUNCTION()
	void EquipPrimary();
	UFUNCTION(Server, Reliable)
	void ServerEquipPrimary();
	UFUNCTION()
	void EquipSecondary();
	UFUNCTION(Server, Reliable)
	void ServerEquipSecondary();
	UFUNCTION()
	void EquipHands();
	UFUNCTION(Server, Reliable)
	void ServerEquipHands();

	/** [server] 拾起武器，由武器调用。
	 *  调用链：InteractionComp::Interact() => [server]WeaponBase::Interacted
	 *			=> [server]WeaponComp::PickUpWeapon =>[multicast]WeaponBase::PickedUp
	 *  Note: 由武器调用有些奇怪 */
	void PickUpWeapon(APolyWeaponBase* NewWeapon);

	void StartInspecting();
#pragma endregion

#pragma region WEAPON_SIMULATE
	////////////////////////////////////////////////
	/// 武器进行射击、装弹等动作时，角色播放的动画、特效
	/// Note: 通常由武器调用

	/** [local] 模拟射击，由WeaponBase每次开火时调用 */
	void SimulateFiring();

	/** [local] 模拟装弹，WeaponState变更后由OnRep_ArmState调用 */
	void SimulateReloading();
	
	/** [local] 模拟装备武器比较特殊，玩家在装备武器时会修改角色的动画数据
		所以需要在PrimaryWeapon或SecondaryWeapon复制完成后，由OnRep_ArmState调用 */
	void SimulateEquipping();

	////////////////////////////////////////////////
	/// 武器瞄准（仅动作），由玩家发起调用
	/// 先玩家本地调用，然后通知服务器进行广播
	UFUNCTION(NetMulticast, Unreliable)
	void SimulateAds();
	UFUNCTION(Server, Reliable)
	void ServerSimulateAds();
	UFUNCTION(NetMulticast, Unreliable)
	void StopSimulateAds();
	UFUNCTION(Server, Reliable)
	void ServerStopSimulateAds();
#pragma endregion

#pragma region WEAPON_NOTIFY
	////////////////////////////////////////////////
	/// 武器状态同步
	/// [server] 由武器调用

	void NotifyFireFinished();
	void NotifyReloadFinished();
	void NotifyEquipFinished();
#pragma endregion

	//////////////////////////////////////////////////////////////
	/// 状态查询
	bool CanFire() const;
	bool CanReload() const;
	FORCEINLINE APolyWeaponBase* GetCurrentWeapon() const
	{
		if (ArmState == EPolyArmState::Primary)
		{
			return PrimaryWeapon;
		}
		if (ArmState == EPolyArmState::Secondary)
		{
			return SecondaryWeapon;
		}
		return nullptr;
	}

	AController* GetOwnerController() const;
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Weapon")
	void GetCameraLocationAndRotation(FVector& CameraLocation, FRotator& CameraRotation);
	FORCEINLINE bool IsFirstPerson() const
	{
		const ENetMode NetMode = GetNetMode();

		if (NetMode == NM_Standalone)
		{
			// Not networked.
			return true;
		}

		if (NetMode == NM_Client and GetOwnerRole() == ROLE_AutonomousProxy)
		{
			// Networked client in control.
			return true;
		}

		if (GetOwner() and GetOwner()->GetRemoteRole() != ROLE_AutonomousProxy and GetOwnerRole() == ROLE_Authority)
		{
			// Local authority in control.
			return true;
		}

		return false;
	}

	UFUNCTION(BlueprintCallable, Category="PolyShooter|Weapon")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Weapon")
	FORCEINLINE EPolyWeaponState GetWeaponState() const { return WeaponState; }

	FORCEINLINE float GetAdsFOV() const
	{
		return bIsAiming and GetCurrentWeapon() ? GetCurrentWeapon()->AdsFOV : 90.0f;
	}

	FORCEINLINE float GetAdsCameraSpeedMod() const
	{
		return bIsAiming and GetCurrentWeapon() ? GetCurrentWeapon()->AdsCameraSpeedMod : 1.0f;
	}

	FORCEINLINE float GetAdsMovementSpeed() const
	{
		return bIsAiming and GetCurrentWeapon() ? GetCurrentWeapon()->AdsMovementSpeed : 660.0f;
	}

	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }
	FORCEINLINE FName GetWeaponSocketName() const { return WeaponSocket; }

	////////////////////////////////////////////////
	// 动画蓝图相关
	/** 获取左手IK骨骼控件位置 */
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Animation")
	FTransform GetLeftHandIKTransform() const;
	/** 获取左手IK世界场景位置 */
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Animation")
	FTransform GetLeftHandIKWorldTransform() const;

	/** 获取角色动画包 */
	UFUNCTION(BlueprintCallable, Category="PolyShooter|Animation")
	FORCEINLINE FCharFPSAnimationPack GetAnimPack() const { return AnimPack; }

	//////////////////////////////////////////////////////////////
	/// 主动更新状态
	/** 客户端和服务端均可 UpdateWeaponState */
	UFUNCTION()
	void UpdateWeaponState(EPolyWeaponState NewWeaponState);
	UFUNCTION(Server, Reliable)
	void ServerUpdateWeaponState(EPolyWeaponState NewWeaponState);

	/** 客户端和服务端均可 UpdateWeaponState */
	UFUNCTION()
	void UpdateArmState(EPolyArmState NewArmState);
	UFUNCTION(Server, Reliable)
	void ServerUpdateArmState(EPolyArmState NewArmState);

#if WITH_EDITOR
	void LogDebugMessage(FString DebugMessage) const
	{
		UE_LOG(LogTemp, Display, TEXT("[%s] %s"), *GetName(), *DebugMessage);
	}
#endif
};
