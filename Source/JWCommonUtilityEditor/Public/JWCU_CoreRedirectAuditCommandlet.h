// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "JWCU_CoreRedirectAuditCommandlet.generated.h"

/**
 * Loads project content packages without saving them so -DebugCoreRedirects can attribute
 * runtime redirect applications to the package that triggered each load.
 */
UCLASS()
class JWCOMMONUTILITYEDITOR_API UJWCU_CoreRedirectAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UJWCU_CoreRedirectAuditCommandlet();

	virtual int32 Main(const FString& Params) override;
};
