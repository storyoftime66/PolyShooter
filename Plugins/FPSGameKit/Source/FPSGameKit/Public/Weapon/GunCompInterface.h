// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GunCompInterface.generated.h"

class UCameraComponent;
class UCharacterMovementComponent;

UENUM()
enum class EGunEvent :uint8
{
	// 射击结束
	Event_FireEnd,
	// 装弹结束
	Event_ReloadEnd,
	// 模拟射击
	Event_SimulateFiring,
	// 模拟装弹
	Event_SimulateReloading,
	// 模拟进入或退出瞄准状态
	Event_SimulateAdsIn,
	Event_SimulateAdsOut,
	// 模拟装备
	Event_SimulateEquipping,
	// 模拟检视
	Event_SimulateInspecting,
};

// This class does not need to be modified.
UINTERFACE(BlueprintType, meta=(CannotImplementInterfaceInBlueprint="true"))
class UGunCompInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * GunComp接口，用于保存、切换武器和传递GunAgent（通常是Character）的输入。
 */
class FPSGAMEKIT_API IGunCompInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	////////////////////////////////////////////////////
	// GunAgent调用的接口
	/** 初始化组件 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp")
	virtual void InitialGunComp(
		USkeletalMeshComponent* FPMesh,
		USkeletalMeshComponent* TPMesh,
		UCameraComponent* FPCamera,
		UCharacterMovementComponent* CharacterMovement
	);
	/** 设置GunComp是否可以对Camera、CharacterMovement造成影响 */
	virtual void SetEnableCameraEffect(bool);
	virtual void SetEnableMovementEffect(bool);

	/** 获取当前武器 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp")
	virtual AActor* GetCurrentGun() const = 0;
	/** 获取角色动画包 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp")
	virtual FCharAnimPack BP_GetCharAnimPack() const;

	/** 枪械操作，通常直接绑定输入 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void FirePressed() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void FireReleased() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void ReloadPressed() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void AdsPressed() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void AdsReleased() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void NextGun() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void PrevGun() = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void RetractGun() = 0;

	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|Input")
	virtual void EquipGunByIndex(int32 NewGunIndex)
	{
	} // 装备特定位置的枪械，可选实现

	/** 进入、退出枪械背包 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|GunInventory")
	virtual void EnterGunInventory(AActor* NewGun) = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|GunComp|GunInventory")
	virtual void LeaveGunInventory(AActor* Gun) = 0;

	////////////////////////////////////////////////////
	// Gun调用的接口
	/** 获取枪械连接的关节 */
	virtual FName GetGripPoint() const { return NAME_None; }
	/** 获取枪械连接的模型 */
	virtual USkeletalMeshComponent* GetFPCharMesh() const { return nullptr; }
	virtual USkeletalMeshComponent* GetTPCharMesh() const { return nullptr; }
	virtual AController* GetOwnerController() const = 0;
	/** 获取视点，通常直接调用Actor的GetActoeEyesViewPoint() */
	virtual void GetOwnerEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const = 0;
	/** 通知GunComp触发了某些事件 */
	virtual void NotifyCompGunEvent(EGunEvent GunEvent) = 0;

};
