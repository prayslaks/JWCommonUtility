// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWCU_CoreRedirectAuditCommandlet.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogJWCUCoreRedirectAudit, Log, All);

UJWCU_CoreRedirectAuditCommandlet::UJWCU_CoreRedirectAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UJWCU_CoreRedirectAuditCommandlet::Main(const FString& Params)
{
	FString PackageRootsArgument = TEXT("/Game");
	FParse::Value(*Params, TEXT("PackageRoots="), PackageRootsArgument);

	int32 MaxPackages = 0;
	FParse::Value(*Params, TEXT("MaxPackages="), MaxPackages);

	int32 GarbageCollectionFrequency = 100;
	FParse::Value(*Params, TEXT("GarbageCollectionFrequency="), GarbageCollectionFrequency);
	GarbageCollectionFrequency = FMath::Max(GarbageCollectionFrequency, 1);

	TArray<FString> PackageRootStrings;
	PackageRootsArgument.ParseIntoArray(PackageRootStrings, TEXT("+"), true);
	if (PackageRootStrings.IsEmpty())
	{
		UE_LOG(LogJWCUCoreRedirectAudit, Error, TEXT("No package roots were supplied."));
		return 1;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (FString& PackageRoot : PackageRootStrings)
	{
		PackageRoot.TrimStartAndEndInline();
		if (!PackageRoot.StartsWith(TEXT("/")))
		{
			PackageRoot.InsertAt(0, TEXT('/'));
		}
		Filter.PackagePaths.Add(FName(*PackageRoot));
	}

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TSet<FName> UniquePackageNames;
	for (const FAssetData& Asset : Assets)
	{
		if (!Asset.PackageName.IsNone())
		{
			UniquePackageNames.Add(Asset.PackageName);
		}
	}

	TArray<FName> PackageNames = UniquePackageNames.Array();
	PackageNames.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});

	if (MaxPackages > 0 && PackageNames.Num() > MaxPackages)
	{
		PackageNames.SetNum(MaxPackages);
	}

	UE_LOG(
		LogJWCUCoreRedirectAudit,
		Display,
		TEXT("JWCU_CORE_REDIRECT_AUDIT_START|Packages=%d|Roots=%s"),
		PackageNames.Num(),
		*PackageRootsArgument);

	int32 LoadedPackageCount = 0;
	int32 FailedPackageCount = 0;
	for (int32 PackageIndex = 0; PackageIndex < PackageNames.Num(); ++PackageIndex)
	{
		const FName PackageName = PackageNames[PackageIndex];
		UE_LOG(LogJWCUCoreRedirectAudit, Display, TEXT("JWCU_CORE_REDIRECT_PACKAGE_BEGIN|%s"), *PackageName.ToString());

		UPackage* LoadedPackage = LoadPackage(nullptr, *PackageName.ToString(), LOAD_None);
		if (LoadedPackage)
		{
			++LoadedPackageCount;
			UE_LOG(LogJWCUCoreRedirectAudit, Display, TEXT("JWCU_CORE_REDIRECT_PACKAGE_END|%s|Loaded"), *PackageName.ToString());
		}
		else
		{
			++FailedPackageCount;
			UE_LOG(LogJWCUCoreRedirectAudit, Error, TEXT("JWCU_CORE_REDIRECT_PACKAGE_END|%s|Failed"), *PackageName.ToString());
		}

		if ((PackageIndex + 1) % GarbageCollectionFrequency == 0)
		{
			CollectGarbage(RF_NoFlags);
		}
	}

	CollectGarbage(RF_NoFlags);
	UE_LOG(
		LogJWCUCoreRedirectAudit,
		Display,
		TEXT("JWCU_CORE_REDIRECT_AUDIT_END|Loaded=%d|Failed=%d"),
		LoadedPackageCount,
		FailedPackageCount);

	return FailedPackageCount == 0 ? 0 : 1;
}
