// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GunInterface.generated.h"

enum class EGunState : uint8;

// This class does not need to be modified.
UINTERFACE(BlueprintType, meta=(CannotImplementInterfaceInBlueprint="true"))
class UGunInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 枪械状态
 */
UENUM(BlueprintType)
enum class EGunState : uint8
{
	None,
	Idle,
	Firing,
	Reloading,
	Equipping,
	Retracted,
	Max,
};

/**
 * 瞄准参数
 */
USTRUCT(BlueprintType)
struct FAdsInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Ads")
	float AdsFOV = 75.0f;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Ads")
	UCurveFloat* AdsCurve;
	/** 瞄准时的镜头移动速度系数 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Ads")
	float AdsCameraSpeedMod = 0.6f;
	/** 瞄准时的移动速度 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Ads")
	float AdsMovementSpeed = 400.0f;
};

/**
 * 枪械相关的角色动画包，被装备时会被WeaponComp获取使用。
 */
USTRUCT(BlueprintType)
struct FCharAnimPack
{
	GENERATED_BODY()

	/** 第一人称模型正常状态下的枪械位置 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	FTransform FPOriginalTransform;
	/** 第一人称下，瞄准状态下的枪械位置 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	FTransform FPAimingTransform;

	////////////////////////////////
	/// 武器使用相关动画蒙太奇
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* FireMontage;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* ReloadMontage_Empty;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* EquipMontage;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* InspectMontage;

	////////////////////////////////
	/// 武器瞄准相关动画和蒙太奇
	/** 非瞄准时基础姿势，通常只有一帧，需要根据动画蓝图进行混入和混出 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* BasePose;
	/** 瞄准时的基础姿势，通常只有一帧，需要根据动画蓝图进行混入和混出 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* AdsBasePose;
	/** 抬起武器蒙太奇，基础姿势为BasePose */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* AdsUpMontage;
	/** 放下武器蒙太奇，基础姿势为AdsBasePose */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* AdsDownMontage;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimMontage* AdsFireMontage;

	////////////////////////////////
	/// 角色部分移动相关动画（通常只用上半身）
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimSequence* WalkAnimation;
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	UAnimSequence* SprintAnimation;
	
	// /** 滑行持续时的动画 TODO: 这部分应该放到移动组件里面的 */
	// UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	// UAnimSequence* SlideAnimation;
	// UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	// UAnimMontage* SlideStartMontage;
	// UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	// UAnimMontage* SlideEndMontage;
	// UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	// UAnimMontage* JumpStartMontage;
	// UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Char Animations")
	// UAnimMontage* JumpEndMontage;
};

/**
 * 枪械类武器需要实现的接口
 */
class FPSGAMEKIT_API IGunInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/////////////////////////////////////////////
	/// 枪械操作
	/** 开始射击和停止射击 */
	virtual void StartFiring() = 0;
	virtual void StopFiring() = 0;
	/** 开始装弹和停止装弹 */
	virtual void StartReloading() = 0;
	virtual void StopReloading() = 0;
	/** 装备和卸装（主要用于播放动画） */
	virtual void BeEquipped() = 0;
	virtual void BeUnequipped() = 0;

	/** 设置持有者 */
	virtual void SetGunComp(UActorComponent*) = 0;

	//////////////////////////////////////////////
	/// 查询状态
	/** 查询当前枪械状态 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual EGunState GetGunState() const = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual int32 GetAmmoInClip() const = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual int32 GetAmmoInBackup() const = 0;
	/** 是否可以开火、装弹、收回 */
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual bool CanFire() const = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual bool CanReload() const = 0;
	UFUNCTION(BlueprintCallable, Category="FPS Game Kit|Gun")
	virtual bool CanRetract() const = 0;

	/////////////////////////////////////////////
	/// 瞄准操作
	virtual void SetAds(bool) = 0;
	virtual bool GetAds() const = 0;
	virtual FAdsInfo GetAdsInfo() const = 0;

	/////////////////////////////////////////////
	/// 使用者动画
	virtual FCharAnimPack GetCharAnimPack() const = 0;

	/** 仅在玩家本地播放动画，用于实现一些非玩法功能，如检视武器 */
	virtual void PlayAnimMontageOnlyLocally(UAnimMontage*);
};
