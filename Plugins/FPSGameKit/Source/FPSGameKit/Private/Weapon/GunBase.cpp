// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/GunBase.h"

#include "DrawDebugHelpers.h"
#include "FPSGameKit.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"

#include "Weapon/GunComp.h"
#include "Weapon/GunCompInterface.h"
#include "Weapon/ProjectileInterface.h"

// 初始化 AGunBase::BurstCountMap
TMap<EGunFireMode, int32> AGunBase::BurstCountMap([]()
{
	TMap<EGunFireMode, int32> Temp;
	Temp.Emplace(EGunFireMode::Semi, 1);
	Temp.Emplace(EGunFireMode::Burst, 3);
	Temp.Emplace(EGunFireMode::Auto, INT_MAX);
	return Temp;
}());

AGunBase::AGunBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 武器模型
	GunFPMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunFPMesh"));
	GunTPMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunTPMesh"));
	if (GunFPMesh)
	{
		GunFPMesh->SetupAttachment(RootComponent);
		GunFPMesh->SetOnlyOwnerSee(true);
		GunFPMesh->SetCollisionProfileName(FName("NoCollision"));
		GunFPMesh->CastShadow = false;
		// GunFPMesh->PrimaryComponentTick.bCanEverTick = false;
	}
	if (GunTPMesh)
	{
		GunTPMesh->SetupAttachment(RootComponent);
		GunTPMesh->SetOwnerNoSee(true);
		GunTPMesh->SetCollisionProfileName(FName("NoCollision"));
		// GunTPMesh->PrimaryComponentTick.bCanEverTick = false;
	}
}

void AGunBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 从数据表中获取武器属性
	InitializeGunData();
}

void AGunBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME(AGunBase, GunAttributes);
	DOREPLIFETIME(AGunBase, GunState);
	DOREPLIFETIME(AGunBase, AmmoInClip);
	DOREPLIFETIME(AGunBase, AmmoInBackup);
	DOREPLIFETIME(AGunBase, bADS);
	DOREPLIFETIME(AGunBase, GunComp);
}

void AGunBase::BeginPlay()
{
	Super::BeginPlay();
}

void AGunBase::InitializeGunData()
{
	if (!GunTable or RowName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Gun didnt setup GunAttributesTable."), *GetName());
		return;
	}

	const auto* GunTableRow = GunTable->FindRow<FGunTableRow>(RowName, GetName());

	if (!GunTableRow)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] GunTableRow wasn't found."), *GetName());
		return;
	}

	// [server] 获取并设置初始武器属性，武器属性会复制到客户端
	if (GetLocalRole() == ROLE_Authority)
	{
		GunAttributes = GunTableRow->GunAttributes;
		GunAttributes.UpdateFireInterval();
		BulletClass = GunTableRow->BulletClass;
		BulletType = GunTableRow->BulletType;

		// 内部变量更新
		BurstCount = 0;
		AmmoInClip = GunAttributes.AmmoPerClip;
		AmmoInBackup = GunAttributes.AmmoPerClip * 2; // TODO: 初始子弹数待定
	}

	AdsInfo = GunTableRow->AdsInfo;
	GunAnimPack = GunTableRow->GunAnimPack;
	CharAnimPack = GunTableRow->CharAnimPack;
}

void AGunBase::OnRep_GunComp()
{
	check(GunComp == nullptr or GunComp->Implements<UGunCompInterface>());

	if (GunComp)
	{
		GunHolder = GunComp->GetOwner();
		if (GunFPMesh and IsHolderLocallyControlled())
		{
			FPGunAnimInstance = GunFPMesh->GetAnimInstance();
		}
		if (GunTPMesh)
		{
			TPGunAnimInstance = GunTPMesh->GetAnimInstance();
		}
		IGunCompInterface* IGunComp = Cast<IGunCompInterface>(GunComp);
		AttachToActor(GunHolder, FAttachmentTransformRules::KeepRelativeTransform);
		if (IGunComp)
		{
			if (GunFPMesh)
			{
				GunFPMesh->AttachToComponent(
					IGunComp->GetFPCharMesh(),
					FAttachmentTransformRules::KeepRelativeTransform,
					IGunComp->GetGripPoint());
			}
			if (GunTPMesh)
			{
				GunTPMesh->AttachToComponent(
					IGunComp->GetTPCharMesh(),
					FAttachmentTransformRules::KeepRelativeTransform,
					IGunComp->GetGripPoint());
			}
		}
	}
	else
	{
		GunHolder = nullptr;
		// 回到原根组件下
		GunFPMesh->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		GunTPMesh->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
		DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
	}
}

void AGunBase::OnRep_GunState()
{
	if (GunState == EGunState::Reloading)
	{
		SimulateReloading();
	}

	if (GunState == EGunState::Equipping)
	{
		SimulateEquipping();
	}

	if (GunState == EGunState::Retracted)
	{
		SimulateUnequipping();
	}
}

void AGunBase::OnRep_ADS()
{
	check(GunComp->Implements<UGunCompInterface>());

	if (bADS)
	{
		Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_SimulateAdsIn);
	}
	else
	{
		Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_SimulateAdsOut);
	}
}

void AGunBase::LineTrace(FHitResult& Hit) const
{
	check(GunComp->Implements<UGunCompInterface>());

	FVector CamLoc;
	FRotator CamRot;
	Cast<IGunCompInterface>(GunComp)->GetOwnerEyesViewPoint(CamLoc, CamRot);
	const FVector TraceEnd = CamLoc + CamRot.Vector() * InstantHitRange;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(Cast<AActor>(GunHolder));
	GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, TRACE_FPS_GUN, QueryParams);
}

void AGunBase::SetGunState(EGunState NewGunState)
{
	check(GetLocalRole() == ROLE_Authority);

	const EGunState OldGunState = GunState;
	GunState = NewGunState;

	// 监听服务器上的玩家需要手动触发OnRep函数
	if (OldGunState != GunState and GetNetMode() == NM_ListenServer and IsHolderLocallyControlled())
	{
		OnRep_GunState();
	}
}

void AGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 模拟射击
	if (GunState == EGunState::Firing)
	{
		// 小于0表示是首次射击
		if (DeltaTime_SimulateFiring < 0.0f)
		{
			DeltaTime_SimulateFiring = 0.0f;
			SimulateFiring();
		}
		else
		{
			DeltaTime_SimulateFiring += DeltaTime;
			if (DeltaTime_SimulateFiring > GunAttributes.FireInterval)
			{
				DeltaTime_SimulateFiring -= GunAttributes.FireInterval;
				SimulateFiring();
			}
		}
	}
	else
	{
		DeltaTime_SimulateFiring = -1.0f;
	}

	// 更新 FireInterval
}

void AGunBase::StartFiring()
{
	if (CanFire())
	{
		ServerStartFiring();
	}
}

void AGunBase::StopFiring()
{
	ServerStopFiring();
}

void AGunBase::StartReloading()
{
	if (CanReload())
	{
		ServerStartReloading();
	}
}

void AGunBase::StopReloading()
{
	ServerStopReloading();
}

void AGunBase::BeEquipped()
{
	if (GunState == EGunState::Retracted)
	{
		ServerBeEquipped();
	}
}

void AGunBase::BeUnequipped()
{
	if (CanRetract())
	{
		ServerBeUnequipped();
	}
}

void AGunBase::SetGunComp(UActorComponent* NewGunComp)
{
	if ((NewGunComp == nullptr or NewGunComp->Implements<UGunCompInterface>()) and NewGunComp != GunComp)
	{
		ServerSetGunComp(NewGunComp);
	}
}

void AGunBase::ServerStartFiring_Implementation()
{
	SetGunState(EGunState::Firing);
	HandleFiring();
}

bool AGunBase::ServerStartFiring_Validate()
{
	return CanFire();
}

void AGunBase::ServerStopFiring_Implementation()
{
	check(GetLocalRole() == ROLE_Authority);

	BurstCount = 0;
	GetWorldTimerManager().ClearTimer(TimerHandle_Fire);
	if (GunState == EGunState::Firing)
	{
		SetGunState(EGunState::Idle);
	}
}

void AGunBase::ServerStartReloading_Implementation()
{
	check(GetLocalRole() == ROLE_Authority);

	const EGunState PrevGunState = GunState;
	SetGunState(EGunState::Reloading);

	// 装弹会停止瞄准和射击
	ServerSetAds_Implementation(false);
	if (PrevGunState == EGunState::Firing)
	{
		ServerStopFiring_Implementation();
	}

	HandleReloading();
}

bool AGunBase::ServerStartReloading_Validate()
{
	return CanReload();
}

void AGunBase::ServerStopReloading_Implementation()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Reload);
	if (GunState == EGunState::Reloading)
	{
		SetGunState(EGunState::Idle);
	}
}

void AGunBase::ServerBeEquipped_Implementation()
{
	check(GetLocalRole() == ROLE_Authority);
	check(GunComp);

	SetGunState(EGunState::Equipping);
	GetWorldTimerManager().SetTimer(TimerHandle_Equip, this, &AGunBase::EquipCompleted, 0.6f);
}

bool AGunBase::ServerBeEquipped_Validate()
{
	return GunState == EGunState::Retracted;
}

void AGunBase::ServerBeUnequipped_Implementation()
{
	check(GetLocalRole() == ROLE_Authority);
	check(GunComp);

	const EGunState OldGunState = GunState;
	SetGunState(EGunState::Retracted);

	if (OldGunState == EGunState::Firing)
	{
		ServerStopFiring_Implementation();
	}
	ServerSetAds_Implementation(false);
}

bool AGunBase::ServerBeUnequipped_Validate()
{
	return CanRetract();
}

void AGunBase::ServerSetAds_Implementation(bool NewAds)
{
	check(GetLocalRole() == ROLE_Authority);

	const bool OldAds = bADS;
	if (NewAds == true)
	{
		if (GunState == EGunState::Idle or GunState == EGunState::Firing)
		{
			bADS = true;
		}
	}
	else
	{
		bADS = false;
	}

	// 监听服务器上的玩家单独调用OnRep
	if (OldAds != bADS and GetNetMode() == NM_ListenServer and IsHolderLocallyControlled())
	{
		OnRep_ADS();
	}
}

void AGunBase::ServerSetGunComp_Implementation(UActorComponent* NewGunComp)
{
	check(GetLocalRole() == ROLE_Authority);

	GunComp = NewGunComp;
	if (NewGunComp)
	{
		SetOwner(NewGunComp->GetOwner());
		SetGunState(EGunState::Retracted);
	}

	if (GetNetMode() == NM_ListenServer and IsHolderLocallyControlled())
	{
		OnRep_GunComp();
	}
}

bool AGunBase::ServerSetGunComp_Validate(UActorComponent* NewGunComp)
{
	return (NewGunComp == nullptr or NewGunComp->Implements<UGunCompInterface>()) and NewGunComp != GunComp;
}

void AGunBase::HandleFiring()
{
	check(GetLocalRole() == ROLE_Authority);
	check(GunComp and GunComp->Implements<UGunCompInterface>() and GetWorld());

	++BurstCount;
	--AmmoInClip;
	LastShotTime = UGameplayStatics::GetTimeSeconds(this);

	// 直接击中近距离的目标
	FHitResult Hit;
	LineTrace(Hit);

	if (Hit.IsValidBlockingHit())
	{
		if (Hit.GetActor())
		{
			// 造成伤害
			FPointDamageEvent DamageEvent;
			DamageEvent.Damage = GunAttributes.Damage;
			DamageEvent.HitInfo = Hit;
			DamageEvent.DamageTypeClass = UDamageType::StaticClass();
			// 使用内置的伤害系统造成伤害
			Hit.GetActor()->TakeDamage(GunAttributes.Damage, DamageEvent,
			                           Cast<IGunCompInterface>(GunComp)->GetOwnerController(), this);
		}
	}
	else
	{
		// 近距离未击中时生成子弹
		SpawnProjectile(Hit);
	}

	// 判断是否继续射击
	if (BurstCount < BurstCountMap[GunAttributes.FireMode] and AmmoInClip > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_Fire, this, &AGunBase::HandleFiring, GunAttributes.FireInterval);
	}
	else
	{
		ServerStopFiring_Implementation();
		Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_FireEnd);
	}
}

void AGunBase::SpawnProjectile(const FHitResult& Hit) const
{
	check(GetLocalRole() == ROLE_Authority);
	check(GetWorld());

	if (BulletClass->ImplementsInterface(UProjectileInterface::StaticClass()))
	{
		FTransform SpawnTransform(Hit.TraceStart);
		SpawnTransform.SetRotation((Hit.TraceEnd - Hit.TraceStart).ToOrientationQuat());
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AActor>(BulletClass, SpawnTransform, SpawnParameters);
	}
}

void AGunBase::HandleReloading()
{
	check(GetLocalRole() == ROLE_Authority);
	check(GunComp);

	float ReloadDuration = 0.6f; // Note: 未设置装备动画时的默认值

	if (GunAnimPack.ReloadMontage)
	{
		ReloadDuration = GunAnimPack.ReloadMontage->CalculateSequenceLength();

		// Note: 让上弹动画未播放完也可进行其他操作，让操作更顺畅
		if (ReloadDuration > 0.5f)
		{
			ReloadDuration -= 0.1f;
		}
	}

	GetWorldTimerManager().SetTimer(TimerHandle_Reload, this, &AGunBase::ReloadCompleted, ReloadDuration);
}

void AGunBase::ReloadCompleted()
{
	check(GetLocalRole() == ROLE_Authority);
	check(GunComp);

	const int32 AmmoReloaded = FMath::Min(AmmoInBackup, static_cast<int32>(GunAttributes.AmmoPerClip - AmmoInClip));
	if (AmmoReloaded > 0)
	{
		AmmoInClip += AmmoReloaded;
		AmmoInBackup -= AmmoReloaded;
	}
	SetGunState(EGunState::Idle);
}

void AGunBase::EquipCompleted()
{
	check(GetLocalRole() == ROLE_Authority);

	SetGunState(EGunState::Idle);
}

void AGunBase::SimulateFiring() const
{
	check(GunComp and GunComp->Implements<UGunCompInterface>());

	// 生成 MuzzleFlash 和 Trail
	if (GunAnimPack.TrailFX_NG)
	{
		const FTransform MuzzleTransform = GetMesh()->GetSocketTransform(MuzzleSocketName);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, GunAnimPack.TrailFX_NG, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		// Note: 需要设置Trail参数
	}

	// 播放Montage（音效、MuzzleFlash特效在动画通知中设置）
	LocallyPlayMontage(GunAnimPack.FireMontage);
	// 通知GunComp播放动画
	Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_SimulateFiring);

	// 短距离击中目标时的命中特效
	FHitResult Hit;
	LineTrace(Hit);
	if (Hit.IsValidBlockingHit())
	{
		// Debug
		// DrawDebugBox(GetWorld(), Hit.ImpactPoint, FVector(5.0f), FColor::Red, true, -1.0f, 0, 3.0f);
		if (GunAnimPack.ImpactFX_NG)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, GunAnimPack.ImpactFX_NG, Hit.ImpactPoint + Hit.Normal,
				Hit.Normal.Rotation(), FVector::OneVector, true);
		}
		// Note: 还需要生成Recal和击中音效，但暂时没有素材
	}
}

void AGunBase::SimulateReloading() const
{
	check(GunComp and GunComp->GetClass()->ImplementsInterface(UGunCompInterface::StaticClass()));


	UAnimMontage* MontageToPlay = GunAnimPack.ReloadMontage;
	if (AmmoInClip == 0 and GunAnimPack.ReloadMontage_Empty)
	{
		MontageToPlay = GunAnimPack.ReloadMontage_Empty;
	}
	LocallyPlayMontage(MontageToPlay);

	Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_SimulateReloading);
}

void AGunBase::SimulateEquipping()
{
	check(GunComp and GunComp->GetClass()->ImplementsInterface(UGunCompInterface::StaticClass()));

	SetActorHiddenInGame(false);

	LocallyPlayMontage(GunAnimPack.EquipMontage);
	Cast<IGunCompInterface>(GunComp)->NotifyCompGunEvent(EGunEvent::Event_SimulateEquipping);
}

void AGunBase::SimulateUnequipping()
{
	SetActorHiddenInGame(true);
}

void AGunBase::SimulateInspecting() const
{
	check(GunComp);

	LocallyPlayMontage(GunAnimPack.InspectMontage);
}

// void AGunBase::PickedUp_Implementation(UGunComp* NewGunComp)
// {
// 	// Note: 附加模型的逻辑在GunComp::PickUpWeapon方法中
//
// 	// TODO: 重构这个PickUp到GunComp中
//
// 	if (NewGunComp)
// 	{
// 		AActor* NewOwner = NewGunComp->GetOwner();
// 		GunHolder = NewOwner;
// 		GunComp = NewGunComp;
//
// 		SetOwner(NewOwner);
// 		GunMesh->SetHiddenInGame(false);
// 		SetActorEnableCollision(false); // 不可再拾取
//
// 		// [local] 更新 GunAnimInstance
// 		if (IsHolderLocallyControlled())
// 		{
// 			if (GunMesh)
// 			{
// 				GunAnimInstance = GunMesh->GetAnimInstance();
// 			}
// 		}
// 		else
// 		{
// 			if (ThirdPersonMesh)
// 			{
// 				GunAnimInstance = ThirdPersonMesh->GetAnimInstance();
// 			}
// 		}
//
// 		// [local] 连接武器到角色模型
// 		if (GetGunMesh())
// 		{
// 			GetGunMesh()->AttachToComponent(
// 				NewGunComp->GetGunMesh(),
// 				FAttachmentTransformRules::KeepRelativeTransform,
// 				NewGunComp->GetWeaponSocketName());
// 		}
// 		if (GetThirdPersonMesh())
// 		{
// 			GetThirdPersonMesh()->AttachToComponent(
// 				NewGunComp->GetThirdPersonMesh(),
// 				FAttachmentTransformRules::KeepRelativeTransform,
// 				NewGunComp->GetWeaponSocketName());
// 		}
// 	}
// }

// void AGunBase::ThrewDown_Implementation()
// {
// 	GunHolder = nullptr;
//
// 	SetOwner(nullptr);
// 	GunMesh->SetHiddenInGame(true);
// 	SetActorEnableCollision(true); // 可拾取
//
// 	GunAnimInstance = nullptr;
//
// 	// [local] 分离武器模型
// 	if (GetGunMesh())
// 	{
// 		GetGunMesh()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
// 	}
// 	if (GetThirdPersonMesh())
// 	{
// 		GetThirdPersonMesh()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
// 	}
// }
//
// void AGunBase::PickedUp_Implementation(IGunCompInterface* NewHolder)
// {
// }
