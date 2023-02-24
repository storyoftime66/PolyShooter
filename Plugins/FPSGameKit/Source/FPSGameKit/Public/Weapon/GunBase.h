// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

#include "Weapon/GunInterface.h"

#include "GunBase.generated.h"


class UGunComp;
class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;
class APolyProjectileBase;

/**
 * 武器射击模式
 */
UENUM(BlueprintType)
enum class EGunFireMode : uint8
{
	// 单发
	Semi,
	// 三连发
	Burst,
	// 全自动
	Auto
};

/** 子弹类型，暂未使用 */
UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Default
};

/**
 * 动态属性
 * 方便在游戏运行时动态修改。
 */
USTRUCT(BlueprintType)
struct FGunAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Attributes")
	float BaseValue = 0.0f;
	float CurrentValue = 0.0f;
	float Addition = 0.0f;
	float Subtraction = 0.0f;
	float Multiplier = 0.0f;
	float Divider = 1.0f;

	// Setters
	void AddAddition(float Value)
	{
		Addition += Value;
		RecalculateCurrentValue();
	}

	void AddSubtraction(float Value)
	{
		Subtraction += Value;
		RecalculateCurrentValue();
	}

	void AddMultiplier(float Value)
	{
		Multiplier += Value;
		RecalculateCurrentValue();
	}

	void AddDivider(float Value)
	{
		Divider = FMath::Clamp(Divider + Value, 0.01f, 1.0f);
		RecalculateCurrentValue();
	}

	void ResetDivider()
	{
		Divider = 1.0f;
		RecalculateCurrentValue();
	}

	void RecalculateCurrentValue()
	{
		CurrentValue = (BaseValue + Addition - Subtraction) * (1 + Multiplier) * Divider;
	}

	void ResetModifiers()
	{
		Addition = 0.0f;
		Subtraction = 0.0f;
		Multiplier = 0.0f;
		Divider = 1.0f;
		RecalculateCurrentValue();
	}

	void SetBaseValue(float Value)
	{
		BaseValue = Value;
		RecalculateCurrentValue();
	}

	// Getters
	float GetBaseValue() const
	{
		return BaseValue;
	}

	float GetCurrentValue() const { return CurrentValue; }

	operator float() const { return GetCurrentValue(); }

	FGunAttribute()
	{
	}

	FGunAttribute(float Value): BaseValue(Value)
	{
	}

	FGunAttribute& operator=(const FGunAttribute& rhs)
	{
		BaseValue = rhs.BaseValue;
		Addition = rhs.Addition;
		Subtraction = rhs.Subtraction;
		Multiplier = rhs.Multiplier;
		Divider = rhs.Divider;
		RecalculateCurrentValue();
		return *this;
	}

	FGunAttribute& operator=(const float& rhs)
	{
		BaseValue = rhs;
		RecalculateCurrentValue();
		return *this;
	}
};

/**
 * 武器属性
 * - 仅包含武器属性，如伤害、射速、弹匣上限等。
 * - 可在游戏运行时修改，实现如临时提升伤害、提升射速等功能。
 */
USTRUCT(BlueprintType)
struct FGunAttributeSet
{
	GENERATED_BODY()

	/** 弹匣容量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FPS Game Kit|Gun|Attributes", meta=(ClampMin="1.0"))
	FGunAttribute AmmoPerClip = 30.0f;

	/** 武器伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FPS Game Kit|Gun|Attributes", meta=(ClampMin="0.0"))
	FGunAttribute Damage = 20.0f;

	/** 射击速率，单位：发/分钟*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FPS Game Kit|Gun|Attributes", meta=(ClampMin="1.0", ClampMax="1200.0"))
	FGunAttribute Rate = 600.0f;
	float FireInterval;

	/** 射击模式*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FPS Game Kit|Gun|Attributes")
	EGunFireMode FireMode = EGunFireMode::Auto;

	///////////////////////////////////////////////////////////////
	/// Setters
	void UpdateFireInterval()
	{
		const float RateValue = FMath::Clamp(Rate.GetCurrentValue(), 1.0f, 1200.0f);
		FireInterval = 60.0f / RateValue;
		if (FireInterval > 0.01f)
		{
			FireInterval -= 0.01f; // 修正误差
		}
	}
};

/**
 * 武器动画包
 */
USTRUCT(BlueprintType)
struct FGunAnimPack
{
	GENERATED_BODY()

	///////////////////////////////////////////////////////////////
	/// 武器动画
	/** 武器射击动画，射击音效、枪口火焰特效放动画通知 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* FireMontage;
	/** 武器装弹动画，音效放动画通知 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* ReloadMontage;
	/** 武器空仓装弹动画，音效放动画通知 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* ReloadMontage_Empty;
	/** 武器装备动画，音效放动画通知 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* EquipMontage;
	/** 瞄准时武器射击动画，射击音效、枪口火焰特效放在动画通知中 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* AdsFireMontage;
	/** 检查武器动画 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UAnimMontage* InspectMontage;

	///////////////////////////////////////////////////////////////
	/// 射击特效
	/// Note: 枪口火焰特效需要放在 FireMontage 中。
	/** 子弹轨迹特效，Niagara版本 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UNiagaraSystem* TrailFX_NG;
	/** 击中特效，Niagara版本 */
	UPROPERTY(EditAnywhere, Category="FPS Game Kit|Gun|Animation")
	UNiagaraSystem* ImpactFX_NG;
};

/**
 * 武器属性数据行
 * - 将武器属性、所需动画、特效全部集中到数据表中配置。
 * - 武器初始化时，会从数据表中读取相关的数据，分别保存到 FWeaponData、FCharFPSAnimationPack 和其他武器的内部变量中。
 */
USTRUCT(BlueprintType)
struct FGunTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 武器初始属性 */
	UPROPERTY(EditAnywhere)
	FGunAttributeSet GunAttributes;
	/** 子弹类，需要实现IProjectileInterface。 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> BulletClass;
	/** 子弹类型。
	 *  不方便从子弹类中取，所以单独设置 */
	UPROPERTY(EditAnywhere)
	EBulletType BulletType;

	/** 瞄准相关 */
	UPROPERTY(EditAnywhere)
	FAdsInfo AdsInfo;

	UPROPERTY(EditAnywhere)
	FGunAnimPack GunAnimPack;

	UPROPERTY(EditAnywhere)
	FCharAnimPack CharAnimPack;
};

/**
 * 枪械Actor类示例
 * - 枪械保存了射击、装弹、装备时的角色动画和武器动画，以及武器伤害、射程、射速等属性。
 * - 枪械会处理射击、装弹、装备等逻辑，并通知GunComp播放角色动画。
 */
UCLASS(Abstract)
class FPSGAMEKIT_API AGunBase : public AActor, public IGunInterface
{
	GENERATED_BODY()

public:
	static TMap<EGunFireMode, int32> BurstCountMap; // 各模式最多连射次数

private:
	// TODO: 准心

	/////////////////////////////////////////////
	/// 持有者信息
	UPROPERTY()
	AActor* GunHolder;
	UPROPERTY(ReplicatedUsing=OnRep_GunComp)
	UActorComponent* GunComp;

#pragma region WEAPON_CONFIG
	UPROPERTY()
	USceneComponent* SceneRoot;
	////////////////////////////////////////////
	/// 武器模型
	UPROPERTY(VisibleAnywhere, Category="FPS Game Kit|Gun")
	USkeletalMeshComponent* GunFPMesh;
	UPROPERTY(VisibleAnywhere, Category="FPS Game Kit|Gun")
	USkeletalMeshComponent* GunTPMesh;
	/** 枪口插槽名称，用于生成发射物 */
	UPROPERTY(EditDefaultsOnly, Category="FPS Game Kit|Gun")
	FName MuzzleSocketName = FName("MuzzleSocket");

	/** 动画相关 */
	UPROPERTY()
	UAnimInstance* GunAnimInstance;
	UPROPERTY()
	FGunAnimPack GunAnimPack;
	UPROPERTY()
	FCharAnimPack CharAnimPack;

	////////////////////////////////////////////
	// 枪械属性
	/** 枪械数据所在的表格 */
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	UDataTable* GunTable;
	UPROPERTY(EditDefaultsOnly, Category="PolyShooter|Weapon")
	FName RowName;

	/** 枪械状态 */
	UPROPERTY(ReplicatedUsing=OnRep_GunState)
	EGunState GunState;
	/** 枪械数值属性 */
	UPROPERTY()
	FGunAttributeSet GunAttributes;
	/** 子弹类，需要实现 IProjectileInterface */
	TSubclassOf<AActor> BulletClass;
	EBulletType BulletType;

	////////////////////////////////////////////
	/// 瞄准相关 TODO

	UPROPERTY(ReplicatedUsing=OnRep_ADS)
	bool bADS = false;
	UPROPERTY()
	FAdsInfo AdsInfo;

#pragma endregion

#pragma region SERVER_ONLY_VARIABLES
	// 控制射击间隔相关的变量
	float LastShotTime = 0.0f;
	int32 BurstCount = 0;
	float FireInterval;
	FTimerHandle TimerHandle_Fire;

	/** 控制装弹、装备枪械的定时器 */
	FTimerHandle TimerHandle_Reload;
	FTimerHandle TimerHandle_Equip;

	/** 瞬间击中敌人的距离，并不代表最终射程。 */
	float InstantHitRange = 1000.0f;
#pragma endregion

public:
	AGunBase();
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** 初始化武器数据，会在 PostInitializeComponents() 中调用 */
	void InitializeGunData();

	UFUNCTION()
	void OnRep_GunComp();

	UFUNCTION()
	void OnRep_GunState();

	UFUNCTION()
	void OnRep_ADS();

	/////////////////////////////////////////////
	/// 当前弹药信息，在客户端上主要用于UI显示
	UPROPERTY(Replicated)
	int32 AmmoInClip;
	UPROPERTY(Replicated)
	int32 AmmoInBackup;

#pragma region HELPER_FUNCTIONS
	bool IsHolderLocallyControlled() const
	{
		if (GunComp and GetOwner())
		{
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

	USkeletalMeshComponent* GetMesh() const
	{
		return IsHolderLocallyControlled() ? GunFPMesh : GunTPMesh;
	}

	void LineTrace(FHitResult& Hit) const;

	void SetGunState(EGunState NewGunState);
#pragma endregion

#pragma region GUN_ACTION
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartFiring();
	UFUNCTION(Server, Reliable)
	void ServerStopFiring();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartReloading();
	UFUNCTION(Server, Reliable)
	void ServerStopReloading();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeEquipped();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeUnequipped();
	UFUNCTION(Server, Reliable)
	void ServerSetAds(bool NewAds);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetGunComp(UActorComponent* NewGunComp);

	/** [server] 射击逻辑 */
	virtual void HandleFiring();
	/** [server] 生成子弹，Hit用于确定子弹生成位置 */
	void SpawnProjectile(const FHitResult& Hit) const;
	/** [server] 装弹逻辑 */
	virtual void HandleReloading();
	/** [server] 装弹完成回调 */
	void ReloadCompleted();
	/** [server] 装备完成回调 */
	void EquipCompleted();

	/** 被捡起 和 被丢弃时调用 */
	// UFUNCTION(NetMulticast, Reliable)
	// void PickedUp(IGunCompInterface* NewHolder);
	// UFUNCTION(NetMulticast, Reliable)
	// void ThrewDown();
	// /** 被捡起和被丢弃时，
	//  *  需要通知每个端更新Holder、WeaponComp和WeaponAnimInstance。
	//  *  之所以不用Replicate说明符，是因为Holder修改频率极低。
	//  */
#pragma endregion

#pragma region SIMULATE
	// 距上次模拟射击的时间，为负则表示是第一次射击
	float DeltaTime_SimulateFiring = -1.0f;

	/** [local] 模拟射击 */
	void SimulateFiring() const;
	/** [local] 模拟装弹 */
	void SimulateReloading() const;
	/** [local] 模拟装备枪械 */
	void SimulateEquipping();
	/** [local] 模拟收起枪械 */
	void SimulateUnequipping();
	/** [local] 模拟检查武器，其他人看不到 */
	void SimulateInspecting() const;
#pragma endregion

public:
	virtual void Tick(float DeltaTime) override;

	//~ IGunInterface
	virtual void StartFiring() override;
	virtual void StopFiring() override;
	virtual void StartReloading() override;
	virtual void StopReloading() override;
	virtual void BeEquipped() override;
	virtual void BeUnequipped() override;

	virtual void SetGunComp(UActorComponent*) override;

	virtual EGunState GetGunState() const override { return GunState; }
	virtual int32 GetAmmoInClip() const override { return AmmoInClip; }
	virtual int32 GetAmmoInBackup() const override { return AmmoInBackup; }

	virtual bool CanFire() const override
	{
		return GunState == EGunState::Idle and
			AmmoInClip > 0 and UGameplayStatics::GetTimeSeconds(this) > LastShotTime + FireInterval;
	}

	virtual bool CanReload() const override
	{
		return (GunState == EGunState::Idle or GunState == EGunState::Firing) and
			AmmoInBackup > 0 and AmmoInClip < GunAttributes.AmmoPerClip;
	}

	virtual bool CanRetract() const override
	{
		return GunState != EGunState::Reloading or GunState != EGunState::Retracted;
	}

	virtual void SetAds(bool NewADS) override
	{
		if (bADS != NewADS)
		{
			if (NewADS)
			{
				if (GunState == EGunState::Idle or GunState == EGunState::Firing)
				{
					ServerSetAds(NewADS);
				}
			}
			else
			{
				ServerSetAds(NewADS);
			}
		}
	}

	virtual bool GetAds() const override { return bADS; }
	virtual FAdsInfo GetAdsInfo() const override { return AdsInfo; }

	virtual FCharAnimPack GetCharAnimPack() const override { return CharAnimPack; }

	virtual void PlayAnimMontageOnlyLocally(UAnimMontage*) override;

	// 读取属性，TODO：移除部分方法
	FORCEINLINE EGunFireMode GetFireMode() const { return GunAttributes.FireMode; }

	UFUNCTION(BlueprintCallable, Category="Weapon")
	FORCEINLINE FCharAnimPack GetCharFPSAnimPack() const { return CharAnimPack; }
};
