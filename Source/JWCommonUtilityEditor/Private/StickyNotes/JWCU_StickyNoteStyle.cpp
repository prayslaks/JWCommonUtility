// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "StickyNotes/JWCU_StickyNoteStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FJWCU_StickyNoteStyle::Style;

void FJWCU_StickyNoteStyle::Initialize()
{
	if (Style.IsValid())
		return;
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("JWCommonUtility"));
	check(Plugin.IsValid());
	Style = MakeShared<FSlateStyleSet>(TEXT("JWCU.StickyNotes"));
	Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources/StickyNotes"));
	const FVector2f Size(16.f, 16.f);
	Style->Set("LockClosed", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("lock_close"), TEXT(".svg")), Size));
	Style->Set("LockOpen", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("lock_open"), TEXT(".svg")), Size));
	Style->Set("Expand", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("note_open"), TEXT(".svg")), Size));
	Style->Set("Collapse", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("note_close"), TEXT(".svg")), Size));
	FSlateStyleRegistry::RegisterSlateStyle(*Style);
}

void FJWCU_StickyNoteStyle::Shutdown()
{
	if (Style.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Style);
		Style.Reset();
	}
}

const ISlateStyle& FJWCU_StickyNoteStyle::Get()
{
	check(Style.IsValid());
	return *Style;
}
