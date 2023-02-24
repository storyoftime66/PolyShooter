// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PolyWeaponBase.h"

#include "DrawDebugHelpers.h"
#include "PoolManagerBPLibrary.h"
#include "Weapon/PolyWeaponHolderInterface.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PolyShooter/PolyShooter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Interaction/GKInteractionAgentInterface.h"
#include "Interaction/GKInteractionCompInterface.h"
#include "Weapon/PolyWeaponComp.h"
#include "Weapon/PolyProjectileBase.h"

APolyWeaponBase::APolyWeaponBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bUsePoolBullet = false;

	// 交互范围
	InteractiveSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractiveSphere"));
	if (InteractiveSphere)
	{
		RootComponent = InteractiveSphere;
		InteractiveSphere->InitSphereRadius(120.0f);
		InteractiveSphere->SetCollisionProfileName(FName("OverlapOnlyPawn"));
	}

	// 武器模型
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonMesh"));
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetupAttachment(RootComponent);
		FirstPersonMesh->SetHiddenInGame(true); // 作为拾取物时，第一人称模型不可见
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	}
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetupAttachment(RootComponent);
		ThirdPersonMesh->SetOwnerNoSee(true);
		ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	}
	MuzzleSocketName = FName("MuzzleSocket");

	BurstCountMap.Emplace(EPolyFireMode::Semi, 1);
	BurstCountMap.Emplace(EPolyFireMode::Burst, 3);
	BurstCountMap.Emplace(EPolyFireMode::Auto, 10000);
}

void APolyWeaponBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 从数据表中获取武器属性
	InitializeWeaponData();

	// 初始化服务端武器属性
	OnRep_WeaponData();
}

void APolyWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APolyWeaponBase, WeaponData);
}

void APolyWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void APolyWeaponBase::InitializeWeaponData()
{
	if (WeaponDataTable and !RowName.IsNone())
	{
		const FString ContextString = GetName();
		if (const auto* WeaponDataRow = WeaponDataTable->FindRow<FWeaponDataTableRow>(RowName, ContextString))
		{
			// [server] 获取并设置初始武器属性，武器属性会复制到客户端
			if (GetLocalRole() == ROLE_Authority)
			{
				WeaponData.SetRate(WeaponDataRow->Rate);
				WeaponData.BaseDamage = WeaponDataRow->Damage;
				WeaponData.Damage = WeaponDataRow->Damage;
				WeaponData.AmmoPerClip = WeaponDataRow->AmmoPerClip;
				WeaponData.FireMode = WeaponDataRow->FireMode;

				InstantHitRange = WeaponDataRow->InstantHitRange;
				BulletClass = WeaponDataRow->BulletClass;
				BulletType = WeaponDataRow->BulletType;

				if (BulletClass and bUsePoolBullet)
				{
					WarmUpBulletPool();
				}
			}

			AdsCurve = WeaponDataRow->AdsCurve;
			AdsFOV = WeaponDataRow->AdsFOV;
			AdsCameraSpeedMod = WeaponDataRow->AdsCameraSpeedMod;
			AdsMovementSpeed = WeaponDataRow->AdsMovementSpeed;

			/* 武器特效、动画 */
			TrailFX_NG = WeaponDataRow->TrailFX_NG;
			ImpactFX_NG = WeaponDataRow->ImpactFX_NG;

			FireMontage = WeaponDataRow->FireMontage;
			ReloadMontage = WeaponDataRow->ReloadMontage;
			InspectMontage = WeaponDataRow->InspectMontage;

			/* 角色动画包 */
			CharFPSAnimPack.OriginTransform = WeaponDataRow->ArmOriginTransform;
			CharFPSAnimPack.AimTransform = WeaponDataRow->ArmAimTransform;

			CharFPSAnimPack.FireMontage = WeaponDataRow->CharFireMontage;
			CharFPSAnimPack.ReloadMontage = WeaponDataRow->CharReloadMontage;
			CharFPSAnimPack.EquipMontage = WeaponDataRow->CharEquipMontage;
			CharFPSAnimPack.InspectMontage = WeaponDataRow->CharInspectMontage;
			CharFPSAnimPack.AdsUpMontage = WeaponDataRow->CharAdsUpMontage;
			CharFPSAnimPack.AdsDownMontage = WeaponDataRow->CharAdsDownMontage;
			CharFPSAnimPack.AdsFireMontage = WeaponDataRow->CharAdsFireMontage;

			CharFPSAnimPack.WalkAnimation = WeaponDataRow->CharWalk;
			CharFPSAnimPack.SprintAnimation = WeaponDataRow->CharSprint;
			CharFPSAnimPack.SlideAnimation = WeaponDataRow->CharSlide;
			CharFPSAnimPack.SlideStartMontage = WeaponDataRow->CharSlideStartMontage;
			CharFPSAnimPack.SlideEndMontage = WeaponDataRow->CharSlideEndMontage;
			CharFPSAnimPack.JumpStartMontage = WeaponDataRow->CharJumpStartMontage;
			CharFPSAnimPack.JumpEndMontage = WeaponDataRow->CharJumpEndMontage;
		}
		else
		{
			UE_LOG(LogWeapon, Error, TEXT("[%s] WeaponDataRow wasn't found."), *GetName());
		}
	}
	else
	{
		UE_LOG(LogWeapon, Error, TEXT("[%s] Weapon didnt setup WeaponDataTable."), *GetName());
	}
}

void APolyWeaponBase::OnRep_WeaponData()
{
	// 内部变量更新
	LastShotTime = 0.0f;
	BurstCount = 0;
	AmmoInClip = WeaponData.AmmoPerClip;
	AmmoInBackup = WeaponData.AmmoPerClip * 2;
}

void APolyWeaponBase::WarmUpBulletPool() const
{
	check(GetWorld());
	check(GetLocalRole() == ROLE_Authority);

	if (BulletClass and BulletClass->ImplementsInterface(UPoolActorInterface::StaticClass()))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = nullptr;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		UPoolManagerBPLibrary::TPoolList& BulletPool = UPoolManagerBPLibrary::Pool.FindOrAdd(BulletClass);
		int count = FMath::CeilToInt(WeaponData.Rate / 60.0f);
		BulletPool.Reserve(count + BulletPool.Num());
		for (int32 i = 0; i < count; i++)
		{
			AActor* Bullet = GetWorld()->SpawnActor<AActor>(BulletClass, FTransform::Identity, SpawnParameters);

			BulletPool.Push(Bullet);

			// Copy from PoolManagerBPLibrary::ReleaseActor()
			Bullet->SetActorTickEnabled(false);
			Bullet->SetActorEnableCollision(false);
			Bullet->SetActorHiddenInGame(true);

			IPoolActorInterface::Execute_OnPoolEnd(Bullet);
		}
	}
	else
	{
		UE_LOG(LogWeapon, Warning,
		       TEXT("[%s] The BulletClass doesn't implement IPoolActorInterface, so no need to warmup."), *GetName());
	}
}

void APolyWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APolyWeaponBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	if (IGKInteractionAgentInterface* InteractionAgent = Cast<IGKInteractionAgentInterface>(OtherActor))
	{
		if (InteractionAgent->GetInteractionComp())
		{
			InteractionAgent->GetInteractionComp()->EnterInteractiveItems(this);
		}
	}
}

void APolyWeaponBase::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	if (IGKInteractionAgentInterface* InteractionAgent = Cast<IGKInteractionAgentInterface>(OtherActor))
	{
		if (InteractionAgent->GetInteractionComp())
		{
			InteractionAgent->GetInteractionComp()->ExitInteractiveItems(this);
		}
	}
}

void APolyWeaponBase::StartFiring()
{
	check(GetLocalRole() == ROLE_Authority);

	if (CanFire())
	{
		HandleFiring();

		// TODO: 开始模拟射击
	}
}

void APolyWeaponBase::HandleFiring()
{
	check(GetLocalRole() == ROLE_Authority);

	++BurstCount;
	--AmmoInClip;
	LastShotTime = UGameplayStatics::GetTimeSeconds(this);

	// TODO: 射击逻辑
	// Spawn Projectile
	check(Holder and WeaponComp and GetWorld());

	FVector CamLoc;
	FRotator CamRot;
	WeaponComp->GetCameraLocationAndRotation(CamLoc, CamRot);
	FVector TraceEnd = CamLoc + CamRot.Vector() * InstantHitRange;

	// 直接击中近距离的目标
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(Holder);
	GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, TRACE_WEAPON, QueryParams);
	if (Hit.IsValidBlockingHit())
	{
		if (Hit.GetActor())
		{
			// 造成伤害
			FPointDamageEvent DamageEvent;
			DamageEvent.Damage = WeaponData.Damage;
			DamageEvent.HitInfo = Hit;
			DamageEvent.DamageTypeClass = UDamageType::StaticClass();
			Hit.GetActor()->TakeDamage(WeaponData.Damage, DamageEvent, WeaponComp->GetOwnerController(), this);
		}
	}
	else
	{
		// 近距离未击中时生成子弹
		SpawnProjectile(Hit);
	}

	// 模拟射击（MuzzleFlash、Trail、Bullet、Impact）
	SimulateFiring(Hit);

	// 射击模式
	if (BurstCount < BurstCountMap[WeaponData.FireMode] and AmmoInClip > 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_Fire, this, &APolyWeaponBase::HandleFiring, WeaponData.FireInterval);
	}
	else
	{
		WeaponComp->NotifyFireFinished();
	}
}

void APolyWeaponBase::StopFiring()
{
	check(GetLocalRole() == ROLE_Authority);

	BurstCount = 0;
	GetWorldTimerManager().ClearTimer(TimerHandle_Fire);
}

void APolyWeaponBase::SimulateFiring_Implementation(const FHitResult& Hit)
{
	// Note: 目前仅兼容单次的枪口火焰和射击音效
	// 产生MuzzleFlash 和 Trail

	if (TrailFX_NG)
	{
		const FTransform MuzzleTransform = GetMeshByRole()->GetSocketTransform(MuzzleSocketName);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, TrailFX_NG, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());
		// TODO: 设置Trail参数
	}

	// 播放Montage（音效、MuzzleFlash特效在动画通知中设置）
	if (WeaponAnimInstance)
	{
		WeaponAnimInstance->Montage_Play(FireMontage);
	}

	// 通知WeaponComp播放角色动画Montage
	if (WeaponComp)
	{
		WeaponComp->SimulateFiring();
	}

	// 短距离击中目标时的命中特效
	if (Hit.IsValidBlockingHit())
	{
		// DrawDebugBox(GetWorld(), Hit.ImpactPoint, FVector(5.0f), FColor::Red, true, -1.0f, 0, 3.0f);
		if (ImpactFX_NG)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this, ImpactFX_NG, Hit.ImpactPoint + Hit.Normal,
				Hit.Normal.Rotation(), FVector::OneVector, true);
		}
		// 生成Recal和音效
	}
}

void APolyWeaponBase::StopSimulateFiring_Implementation()
{
}

void APolyWeaponBase::SpawnProjectile(const FHitResult& Hit)
{
	check(GetLocalRole() == ROLE_Authority);
	check(GetWorld());

	if (BulletClass)
	{
		FTransform SpawnTransform(Hit.TraceStart);
		SpawnTransform.SetRotation((Hit.TraceEnd - Hit.TraceStart).ToOrientationQuat());
		if (bUsePoolBullet)
		{
			UPoolManagerBPLibrary::SpawnActor(this, BulletClass, SpawnTransform);
		}
		else
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner();
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<APolyProjectileBase>(BulletClass, SpawnTransform, SpawnParameters);
		}
	}
}

void APolyWeaponBase::SimulateInspecting()
{
	if (WeaponAnimInstance)
	{
		WeaponAnimInstance->Montage_Play(InspectMontage);
	}
}

void APolyWeaponBase::PickedUp_Implementation(UPolyWeaponComp* NewWeaponComp)
{
	// Note: 附加模型的逻辑在WeaponComp::PickUpWeapon方法中

	if (NewWeaponComp)
	{
		AActor* NewOwner = NewWeaponComp->GetOwner();
		Holder = NewOwner;
		WeaponComp = NewWeaponComp;

		SetOwner(NewOwner);
		FirstPersonMesh->SetHiddenInGame(false);
		SetActorEnableCollision(false); // 不可再拾取

		// [local] 更新 WeaponAnimInstance
		if (IsFirstPerson())
		{
			if (FirstPersonMesh)
			{
				WeaponAnimInstance = FirstPersonMesh->GetAnimInstance();
			}
		}
		else
		{
			if (ThirdPersonMesh)
			{
				WeaponAnimInstance = ThirdPersonMesh->GetAnimInstance();
			}
		}

		// [local] 连接武器到角色模型
		if (GetFirstPersonMesh())
		{
			GetFirstPersonMesh()->AttachToComponent(
				NewWeaponComp->GetFirstPersonMesh(),
				FAttachmentTransformRules::KeepRelativeTransform,
				NewWeaponComp->GetWeaponSocketName());
		}
		if (GetThirdPersonMesh())
		{
			GetThirdPersonMesh()->AttachToComponent(
				NewWeaponComp->GetThirdPersonMesh(),
				FAttachmentTransformRules::KeepRelativeTransform,
				NewWeaponComp->GetWeaponSocketName());
		}
	}
}

void APolyWeaponBase::ThrewDown_Implementation()
{
	Holder = nullptr;

	SetOwner(nullptr);
	FirstPersonMesh->SetHiddenInGame(true);
	SetActorEnableCollision(true); // 可拾取

	WeaponAnimInstance = nullptr;

	// [local] 分离武器模型
	if (GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}
	if (GetThirdPersonMesh())
	{
		GetThirdPersonMesh()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}

	// TODO: 丢出去（物理）
}

void APolyWeaponBase::HandleReloading()
{
	check(GetLocalRole() == ROLE_Authority);
	check(WeaponComp);

	StopFiring();

	float ReloadDuration = 1.0f;

	if (ReloadMontage)
	{
		ReloadDuration = ReloadMontage->CalculateSequenceLength();
		if (ReloadDuration > 0.5f)
		{
			ReloadDuration -= 0.1f;
		}
	}

	GetWorldTimerManager().SetTimer(TimerHandle_Reload, this, &APolyWeaponBase::ReloadCompleted, ReloadDuration);

	SimulateReloading();
}

void APolyWeaponBase::SimulateReloading()
{
	if (WeaponAnimInstance)
	{
		WeaponAnimInstance->Montage_Play(ReloadMontage);
	}
}

void APolyWeaponBase::StopReloading()
{
	check(GetLocalRole() == ROLE_Authority);
}

void APolyWeaponBase::ReloadCompleted()
{
	check(GetLocalRole() == ROLE_Authority);

	const int32 AmmoReloaded = FMath::Min(WeaponData.AmmoPerClip - AmmoInClip, AmmoInBackup);
	if (AmmoReloaded > 0)
	{
		AmmoInClip += AmmoReloaded;
		AmmoInBackup -= AmmoReloaded;
	}
	WeaponComp->NotifyReloadFinished();
}

void APolyWeaponBase::Equipped()
{
	check(GetLocalRole() == ROLE_Authority);
	check(WeaponComp);

	SetActorHiddenInGame(false); // TODO: 联网可能有问题
	GetWorldTimerManager().SetTimer(TimerHandle_Equip, WeaponComp, &UPolyWeaponComp::NotifyEquipFinished, 1.0f);
}

void APolyWeaponBase::UnEquipped()
{
	check(GetLocalRole() == ROLE_Authority);

	SetActorHiddenInGame(true); // TODO: 联网可能有问题
}

bool APolyWeaponBase::Interacted(AActor* InteractionCompOwner)
{
	const bool bCanInteract = IGKInteractiveInterface::Interacted(InteractionCompOwner);
	if (bCanInteract)
	{
		if (const IPolyWeaponHolderInterface* WeaponHolder = Cast<IPolyWeaponHolderInterface>(InteractionCompOwner))
		{
			if (WeaponHolder->GetWeaponComp())
			{
				WeaponHolder->GetWeaponComp()->PickUpWeapon(this);
				return true;
			}
		}
	}
	return false;
}
