// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Weapon/GunCompInterface.h"
#include "Weapon/GunInterface.h"

#include "GunComp.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Display, All);

class AActor;
class UCameraComponent;

/**
 * 枪械组件
 * 实现和枪械交互、切换枪械、网络复制功能
 * Note: 由于瞄准操作需要改变第一人称Mesh的相对变换，所以需要将第一人称Mesh作为摄像机组件的子组件
 * 使用步骤：
 *		1. 角色实现 IGunAgentInterface
 *		2. 角色添加 UGunComp 组件
 *		3. 角色将输入绑定到 IGunCompInterface 的枪械操作
 */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class FPSGAMEKIT_API UGunComp : public UActorComponent, public IGunCompInterface
{
	GENERATED_BODY()

private:
	////////////////////////////////////////////////
	// 角色相关
	// 角色骨架网格体组件引用，用于播放射击动画
	UPROPERTY()
	USkeletalMeshComponent* CharFPMesh;
	UPROPERTY()
	USkeletalMeshComponent* CharTPMesh;
	UPROPERTY()
	UAnimInstance* CharFPAnimInstance;
	UPROPERTY()
	UAnimInstance* CharTPAnimInstance;
	UPROPERTY(EditAnywhere, Category="PolyShooter|WeaponComp")
	FName GunGripSocket = FName("GripPoint");
	// 相机组件和移动组件引用，用于修改FOV、修改移动速度等
	UPROPERTY()
	UCameraComponent* Camera;
	UPROPERTY()
	UCharacterMovementComponent* CharacterMovement;

	////////////////////////////////////////////////
	/// 武器信息
	/** 拥有的武器列表（背包） */
	UPROPERTY(ReplicatedUsing=OnRep_GunInventory)
	TArray<AActor*> GunInventory;

	/** 当前持有的武器索引，索引无效时为空手 */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentGunIndex)
	int32 CurrentGunIndex = -1;
	IGunInterface* CurrentGunCache;

	/** 当前角色动画（local）*/
	FCharAnimPack AnimPack;

	//////////////////////////////////////////////
	/// 武器瞄准
	/** 转换到瞄准状态的时间轴，时间轴曲线由武器确定 */
	FTimeline AdsTimeline;
	/** 手臂原始相对变换 */
	FTransform ArmOriginTransform;

	////////////////////////////////////////////////
	// 玩家输入信息
	/** 玩家是否按住射击键 */
	bool bHoldingFire = false;
	/** 玩家是否按住瞄准键 */
	bool bHoldingAds = false;

	////////////////////////////////////////////////
	/// 内部变量
	/** 空手时的角色动画 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|WeaponComp")
	FCharAnimPack HandAnimPack;

public:
	UGunComp();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 瞄准操作变换网格体位置 */
	UFUNCTION()
	void AdsTimelineProgress(float Value);

	UPROPERTY()
	FAdsInfo AdsInfoCache;
	FAdsInfo DefaultAdsInfo;

	UFUNCTION()
	void OnRep_GunInventory();
	UFUNCTION()
	void OnRep_CurrentGunIndex();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSwapGunByIndex(int32 GunIndex);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRetractGun();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEnterGunInventory(AActor* NewGun);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLeaveGunInventory(AActor* Gun);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region WEAPON_OPERATIONS
	//////////////////////////////////
	/// 使用武器的操作，供OwnerActor调用，由客户端发起

	/** [server] 拾起武器，由武器调用。
	 *  调用链：InteractionComp::Interact() => [server]WeaponBase::Interacted
	 *			=> [server]WeaponComp::PickUpWeapon =>[multicast]WeaponBase::PickedUp
	 *  Note: 由武器调用有些奇怪 */
	// void PickUpWeapon(AActor* NewWeapon);

	void StartInspecting();
#pragma endregion

#pragma region SIMULATE
	/** [local] 模拟射击 */
	void CharSimulateFiring();
	/** [local] 模拟装弹，WeaponState变更后由OnRep_ArmState调用 */
	void CharSimulateReloading();

	/** [local] 模拟装备武器比较特殊，玩家在装备武器时会修改角色的动画数据
		所以需要在PrimaryWeapon或SecondaryWeapon复制完成后，由OnRep_ArmState调用 */
	void CharSimulateEquipping();

	////////////////////////////////////////////////
	/// 武器瞄准（仅动作）
	void CharSimulateAdsIn();
	void CharSimulateAdsOut();
#pragma endregion

#pragma region HELPER_FUNCTIONS
	FORCEINLINE bool IsOwnerLocallyControlled() const
	{
		check(GetOwner());

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

	FORCEINLINE bool IsArmed() const
	{
		return GunInventory.IsValidIndex(CurrentGunIndex) and CurrentGunCache;
	}

	FORCEINLINE void LocalPlayMontage(UAnimMontage* MontageToPlay) const
	{
		if (CharFPAnimInstance and IsOwnerLocallyControlled())
		{
			CharFPAnimInstance->Montage_Play(MontageToPlay);
		}
		if (CharTPAnimInstance)
		{
			CharTPAnimInstance->Montage_Play(MontageToPlay);
		}
	}

	void SetCurrentGunIndex(int32 NewIndex);

	USkeletalMeshComponent* GetMesh() const
	{
		check(CharFPMesh and CharTPMesh);
		return IsOwnerLocallyControlled() ? CharFPMesh : CharTPMesh;
	}
#pragma endregion


	//~ IGunCompInterface
	virtual void InitialGunComp(
		USkeletalMeshComponent* FPMesh,
		USkeletalMeshComponent* TPMesh,
		UCameraComponent* FPCamera,
		UCharacterMovementComponent* InCharacterMovement
	) override;
	virtual void SetEnableCameraEffect(bool NewEnable) override;
	virtual void SetEnableMovementEffect(bool NewEnable) override;

	virtual AActor* GetCurrentGun() const override
	{
		if (!GunInventory.IsValidIndex(CurrentGunIndex))
		{
			return nullptr;
		}

		return GunInventory[CurrentGunIndex];
	}

	/** 武器操作，通常直接绑定输入 */
	virtual void FirePressed() override;
	virtual void FireReleased() override;
	virtual void ReloadPressed() override;
	virtual void AdsPressed() override;
	virtual void AdsReleased() override;
	virtual void NextGun() override;
	virtual void PrevGun() override;
	virtual void RetractGun() override;
	virtual void EquipGunByIndex(int32) override;

	virtual void EnterGunInventory(AActor* NewGun) override;
	virtual void LeaveGunInventory(AActor* Gun) override;

	virtual FName GetGripPoint() const override { return GunGripSocket; }

	virtual AController* GetOwnerController() const override;
	virtual void GetOwnerEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	virtual void NotifyCompGunEvent(EGunEvent GunEvent) override;
	//~ End IGunCompInterface


	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp")
	FORCEINLINE bool IsAiming() const { return bHoldingAds; }

	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp")
	FORCEINLINE EGunState GetGunState() const
	{
		if (GetCurrentGun())
		{
			return Cast<IGunInterface>(GetCurrentGun())->GetGunState();
		}

		return EGunState::None;
	}

	FORCEINLINE float GetAdsFOV() const
	{
		return bHoldingAds and IsArmed() ? AdsInfoCache.AdsFOV : DefaultAdsInfo.AdsFOV;
	}

	FORCEINLINE float GetAdsCameraSpeedMod() const
	{
		return bHoldingAds and IsArmed() ? AdsInfoCache.AdsCameraSpeedMod : DefaultAdsInfo.AdsCameraSpeedMod;
	}

	FORCEINLINE float GetAdsMovementSpeed() const
	{
		return bHoldingAds and IsArmed() ? AdsInfoCache.AdsMovementSpeed : 660.0f;
	}

	virtual USkeletalMeshComponent* GetFPCharMesh() const override { return CharFPMesh; }
	virtual USkeletalMeshComponent* GetTPCharMesh() const override { return CharTPMesh; }
	FORCEINLINE FName GetWeaponSocketName() const { return GunGripSocket; }

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
	FCharAnimPack GetAnimPack() const { return AnimPack; }

#if WITH_EDITOR
	void LogDebugMessage(FString DebugMessage) const
	{
		UE_LOG(LogTemp, Display, TEXT("[%s] %s"), *GetName(), *DebugMessage);
	}
#endif
};
