// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Interaction/GKInteractiveInterface.h"
#include "Kismet/GameplayStatics.h"
#include "PolyWeaponBase.generated.h"


class UPolyWeaponComp;
class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;
class APolyProjectileBase;

/** 武器射击模式。
 *  大部分武器的射击模式固定，小部分可调节。 */
UENUM(BlueprintType)
enum class EPolyFireMode : uint8
{
	Semi,
	// 单发
	Burst,
	// 三连发
	Auto // 全自动
};

/** 子弹类型，暂未使用 */
UENUM(BlueprintType)
enum class EPolyBulletType:uint8
{
	Default
};

/**
 * 武器属性数据行
 * - 将武器属性、所需动画、特效全部集中到数据表中配置。
 * - 武器初始化时，会从数据表中读取相关的数据，分别保存到 FWeaponData、FCharFPSAnimationPack 和其他武器的内部变量中。
 */
USTRUCT(BlueprintType)
struct FWeaponDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	///////////////////////////////////////////////////////////////
	/// 武器属性
	/** 单个弹夹容量 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon", meta=(ClampMin="1"))
	int32 AmmoPerClip;
	/** 基础伤害 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	float Damage;
	/** 射速，单位：发/分钟 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon", meta=(ClampMin="1", ClampMax="1200"))
	float Rate;
	/** 射击模式，有单发、三连发、全自动。部分武器可修改。 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	EPolyFireMode FireMode;
	/** 瞬间击中敌人的距离，并不代表最终射程。 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	float InstantHitRange;
	/** 子弹类 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<APolyProjectileBase> BulletClass;
	/** 子弹类型。
	 *  不方便从子弹类中取，所以单独设置 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	EPolyBulletType BulletType;


	///////////////////////////////////////////////////////////////
	/// 瞄准相关
	/** 切换瞄准的运动曲线 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	UCurveFloat* AdsCurve;
	/** 瞄准时的FOV */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	float AdsFOV;
	/** 瞄准时的镜头移动速度系数 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	float AdsCameraSpeedMod;
	/** 瞄准时的移动速度 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	float AdsMovementSpeed;


	///////////////////////////////////////////////////////////////
	/// 武器动画
	/** 武器射击动画，射击音效、枪口火焰特效放在动画通知中 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon Animations")
	UAnimMontage* FireMontage;
	/** 武器装弹动画，装弹音效放在动画通知中 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon Animations")
	UAnimMontage* ReloadMontage;
	/** 瞄准时武器射击动画，射击音效、枪口火焰特效放在动画通知中 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon Animations")
	UAnimMontage* AdsFireMontage;
	/** 检查武器时，武器播放的蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon Animations")
	UAnimMontage* InspectMontage;

	///////////////////////////////////////////////////////////////
	/// 武器特效
	/// Note: 同时拥有两个版本特效时，优先使用Niagara版本
	/** 子弹轨迹特效，Niagara版本（目前仅支持Niagara） */
	UPROPERTY(EditDefaultsOnly, Category="Weapon VFX")
	UNiagaraSystem* TrailFX_NG;
	// /** 子弹轨迹特效，Cascade版本 */
	// UPROPERTY(EditDefaultsOnly, Category="Weapon VFX")
	// UParticleSystem* TrailFX_CC;
	/** 命中特效，Niagara版本（目前仅支持Niagara） */
	UPROPERTY(EditDefaultsOnly, Category="Weapon VFX")
	UNiagaraSystem* ImpactFX_NG;

	///////////////////////////////////////////////////////////////
	/// 角色动画
	/** 手臂原始相对变换 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	FTransform ArmOriginTransform;
	/** 手臂瞄准时相对变换 */
	UPROPERTY(EditDefaultsOnly, Category="Weapon ADS")
	FTransform ArmAimTransform;
	
	/** 角色行走动画 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimSequence* CharWalk;
	/** 角色冲刺动画 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimSequence* CharSprint;
	/** 角色滑铲动画 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimSequence* CharSlide;
	/** 角色滑铲开始蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharSlideStartMontage;
	/** 角色滑铲结束蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharSlideEndMontage;
	/** 角色起跳蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharJumpStartMontage;
	/** 角色落地蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharJumpEndMontage;

	/** 角色进入瞄准蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharAdsUpMontage;
	/** 角色退出瞄准蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharAdsDownMontage;
	/** 角色瞄准射击蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharAdsFireMontage;
	
	/** 角色射击蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharFireMontage;
	/** 角色装弹蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharReloadMontage;
	/** 角色装备蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharEquipMontage;
	/** 角色检查武器蒙太奇 */
	UPROPERTY(EditDefaultsOnly, Category="Character Animations")
	UAnimMontage* CharInspectMontage;

	FWeaponDataTableRow()
	{
		// 武器属性默认值
		AmmoPerClip = 30;
		Rate = 600.0f;
		Damage = 20.0f;
		InstantHitRange = 1000.0f;
		FireMode = EPolyFireMode::Auto;
		AdsFOV = 75.0f;
	}
};

/**
 * 武器属性
 * 仅包含武器属性，如伤害、射速、弹匣上限等。
 * 可在游戏运行时修改，实现如临时提升伤害、提升射速等功能。
 */
USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 AmmoPerClip;

	/** 武器基础伤害，该值在初始化后保持不变。 */
	UPROPERTY(BlueprintReadOnly)
	float BaseDamage;
	/** 武器实际伤害，TODO: Setter */
	UPROPERTY(BlueprintReadOnly)
	float Damage;

	/** 射击速率，单位：发/分钟，TODO: Setter */
	UPROPERTY(BlueprintReadOnly)
	float Rate;
	/** 射击间隔，值为 60/Rate */
	UPROPERTY(BlueprintReadOnly)
	float FireInterval;

	/** 射击模式*/
	UPROPERTY(BlueprintReadOnly)
	EPolyFireMode FireMode;

	///////////////////////////////////////////////////////////////
	/// Setters
	void SetRate(float NewRate)
	{
		Rate = FMath::Clamp(NewRate, 1.0f, 1200.0f);
		FireInterval = 60.0f / Rate;
		if (FireInterval > 0.01f)
		{
			FireInterval -= 0.01f; // 修正误差
		}
	}
};

/** 武器相关的角色动画包
 *  被装备时会被WeaponComp获取使用。
 */
USTRUCT(BlueprintType)
struct FCharFPSAnimationPack
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform OriginTransform;
	UPROPERTY()
	FTransform AimTransform;
	
	////////////////////////////////
	/// 武器使用相关
	UPROPERTY()
	UAnimMontage* FireMontage;
	UPROPERTY()
	UAnimMontage* ReloadMontage;
	UPROPERTY()
	UAnimMontage* EquipMontage;
	UPROPERTY()
	UAnimMontage* InspectMontage;

	UPROPERTY()
	UAnimMontage* AdsUpMontage;
	UPROPERTY()
	UAnimMontage* AdsDownMontage;
	UPROPERTY()
	UAnimMontage* AdsFireMontage;

	////////////////////////////////
	/// 角色移动相关
	UPROPERTY()
	UAnimSequence* WalkAnimation;
	UPROPERTY()
	UAnimSequence* SprintAnimation;
	UPROPERTY()
	UAnimSequence* SlideAnimation;
	UPROPERTY()
	UAnimMontage* SlideStartMontage;
	UPROPERTY()
	UAnimMontage* SlideEndMontage;
	UPROPERTY()
	UAnimMontage* JumpStartMontage;
	UPROPERTY()
	UAnimMontage* JumpEndMontage;
};

/**
 * 武器基类
 * - 武器类的大部分状态修改仅发生服务端，不进行同步
 * - 武器保存了射击、装弹、装备时的角色动画和武器动画，以及武器伤害、射程、射速等属性。 TODO: 使用数据结构收纳
 * - 武器会处理射击、装弹、装备等逻辑，并通知WeaponComp播放角色动画。
 * - 武器在被拾起前会自行处理交互功能相关逻辑。@see IInteractiveInterface
 */
UCLASS(Abstract)
class POLYSHOOTER_API APolyWeaponBase : public AActor, public IGKInteractiveInterface
{
	GENERATED_BODY()

	friend class UPolyWeaponComp;

private:
	// 人物动作
	// 开火动画、装弹动画、装备动画
	// 瞄准动画
	// 行走动画

	// 准心

	////////////////////////////////////////////
	// 交互范围
	UPROPERTY()
	USphereComponent* InteractiveSphere;

	/////////////////////////////////////////////
	/// 持有者信息
	/** 持有者和持有者的WeaponComp，通过PickedUp和ThrewDown向客户端同步 */
	UPROPERTY()
	AActor* Holder;
	UPROPERTY()
	UPolyWeaponComp* WeaponComp;

	/////////////////////////////////////////////
	/// 弹药信息
	int32 AmmoInClip;
	int32 AmmoInBackup;

#pragma region WEAPON_CONFIG
	////////////////////////////////////////////
	/// 武器模型
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	USkeletalMeshComponent* FirstPersonMesh;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	USkeletalMeshComponent* ThirdPersonMesh;
	/** 武器枪口插槽名称，用于生成发射物*/
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	FName MuzzleSocketName;

	/** 动画实例。*/
	// UPROPERTY()
	// UAnimInstance* FPAnimInstance;
	// UPROPERTY()
	// UAnimInstance* TPAnimInstance;
	UPROPERTY()
	UAnimInstance* WeaponAnimInstance;

	////////////////////////////////////////////
	// 武器属性
	/** 武器数据所在的表格 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	UDataTable* WeaponDataTable;
	/** 武器数据所在的行 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	FName RowName;

	/** 武器数值属性 */
	UPROPERTY(ReplicatedUsing=OnRep_WeaponData)
	FWeaponData WeaponData;

	/** 子弹类型和子弹类 */
	TSubclassOf<APolyProjectileBase> BulletClass;
	EPolyBulletType BulletType;

	/** 优化相关，是否使用子弹池 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	bool bUsePoolBullet;

	////////////////////////////////////////////
	/// 瞄准相关
	/** [owner only] 瞄准时的FOV */
	UPROPERTY()
	UCurveFloat* AdsCurve;
	/** [owner only] 瞄准时的FOV */
	float AdsFOV;
	/** [owner only] 瞄准时的镜头移动速度系数 */
	float AdsCameraSpeedMod;
	/** [owner only] 瞄准时的移动速度 */
	float AdsMovementSpeed;
	/** [owner only] 手臂原始相对变换 */
	FTransform ArmOriginTransform;
	/** [owner only] 手臂瞄准相对变换 */
	FTransform ArmAimTransform;

#pragma endregion

#pragma region SERVER_ONLY_VARIABLES
	// 控制射击间隔
	float LastShotTime;
	float FireInterval;
	FTimerHandle TimerHandle_Fire;
	FTimerHandle TimerHandle_Reload;
	FTimerHandle TimerHandle_Equip;
	int32 BurstCount;
	TMap<EPolyFireMode, int32> BurstCountMap; // 各模式最多连射次数

	/** 瞬间击中敌人的距离，并不代表最终射程。 */
	float InstantHitRange;
#pragma endregion

#pragma region SIMULATE_CONFIG
	//////////////////////////////////////////////
	/// FX
	UPROPERTY()
	UNiagaraSystem* TrailFX_NG;
	UPROPERTY()
	UNiagaraSystem* ImpactFX_NG;

	//////////////////////////////////////////////
	/// Animation
	UPROPERTY()
	UAnimMontage* FireMontage;
	UPROPERTY()
	UAnimMontage* ReloadMontage;
	UPROPERTY()
	UAnimMontage* InspectMontage;

	UPROPERTY()
	FCharFPSAnimationPack CharFPSAnimPack;
#pragma endregion

public:
	// Sets default values for this actor's properties
	APolyWeaponBase();
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** 初始化武器数据，会在 PostInitializeComponents() 中调用 */
	void InitializeWeaponData();

	UFUNCTION()
	void OnRep_WeaponData();

	/** [server] 预热子弹池，向全局子弹池中添加子弹 */
	virtual void WarmUpBulletPool() const;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

#pragma region WEAPON_ACTION
	/// 武器动作，武器的所有动作都需要从服务端发起
	/** [server] 开始射击 */
	void StartFiring();
	/** [server] 射击逻辑 */
	virtual void HandleFiring();
	/** [server] 主动停止射击 */
	void StopFiring();
	UFUNCTION(NetMulticast, Unreliable)
	void SimulateFiring(const FHitResult& Hit);
	UFUNCTION(NetMulticast, Reliable)
	void StopSimulateFiring();
	/** [server] 生成子弹，Hit用于确定子弹生成位置 */
	void SpawnProjectile(const FHitResult& Hit);

	/** [local] 模拟检查武器 */
	void SimulateInspecting();

	/** [server] 装弹逻辑 */
	virtual void HandleReloading();
	/** [local] 模拟装弹 */
	void SimulateReloading();
	/** [server] 主动停止装弹 */
	void StopReloading();
	/** [server] 装弹完成回调 */
	void ReloadCompleted();

	/** 被捡起 和 被丢弃时调用 */
	UFUNCTION(NetMulticast, Reliable)
	void PickedUp(UPolyWeaponComp* NewHolder);
	UFUNCTION(NetMulticast, Reliable)
	void ThrewDown();
	// /** 被捡起和被丢弃时，
	//  *  需要通知每个端更新Holder、WeaponComp和WeaponAnimInstance。
	//  *  之所以不用Replicate说明符，是因为Holder修改频率极低。
	//  */
	// UFUNCTION(NetMulticast, Reliable)
	// void ReplicateWeaponComp(UPolyWeaponComp* NewWeaponComp);

	/** [server] 被装备与被收起时调用，TODO: 可能客户端不会隐藏/显示 */
	void Equipped();
	void UnEquipped();
#pragma endregion

	// 状态查询
	FORCEINLINE bool CanFire() const
	{
		return AmmoInClip > 0 and UGameplayStatics::GetTimeSeconds(this) > LastShotTime + FireInterval;
	}

	FORCEINLINE bool CanReload() const
	{
		return AmmoInBackup > 0 and AmmoInClip < WeaponData.AmmoPerClip;
	}

	// 读取属性
	FORCEINLINE EPolyFireMode GetFireMode() const { return WeaponData.FireMode; }
	FORCEINLINE UPolyWeaponComp* GetWeaponComp() const { return WeaponComp; }
	FORCEINLINE USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	FORCEINLINE bool IsFirstPerson() const
	{
		if (Holder)
		{
			check(GetOwner());

			const ENetMode NetMode = GetNetMode();
			if (NetMode == NM_Standalone)
			{
				// Not networked.
				return true;
			}

			if (NetMode == NM_Client && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
			{
				// Networked client in control.
				return true;
			}

			if (GetOwner()->GetRemoteRole() != ROLE_AutonomousProxy && GetOwner()->GetLocalRole() == ROLE_Authority)
			{
				// Local authority in control.
				return true;
			}
		}
		return false;
	}

	FORCEINLINE USkeletalMeshComponent* GetMeshByRole() const
	{
		return IsFirstPerson() ? FirstPersonMesh : ThirdPersonMesh;
	}

	FORCEINLINE UCurveFloat* GetAdsCurve() const
	{
		return AdsCurve;
	}

	UFUNCTION(BlueprintCallable, Category="Weapon")
	FORCEINLINE FCharFPSAnimationPack GetCharFPSAnimPack() const { return CharFPSAnimPack; }

	FORCEINLINE float GetAdsFOV() const { return AdsFOV; }

	//~ IInteractiveInterface
	/** [server] 作为可交互物时，被交互 */
	virtual bool Interacted(AActor* InteractionCompOwner) override;
};
