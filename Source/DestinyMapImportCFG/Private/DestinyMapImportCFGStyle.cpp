// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestinyMapImportCFGStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FDestinyMapImportCFGStyle::StyleInstance = nullptr;

void FDestinyMapImportCFGStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FDestinyMapImportCFGStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FDestinyMapImportCFGStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("DestinyMapImportCFGStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FDestinyMapImportCFGStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("DestinyMapImportCFGStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("DestinyMapImportCFG")->GetBaseDir() / TEXT("Resources"));

	Style->Set("DestinyMapImportCFG.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	Style->Set("DestinyMapImportCFG.ImportCharmCFG", new IMAGE_BRUSH_SVG(TEXT("CharmCFGIcon"), Icon20x20));
	return Style;
}

void FDestinyMapImportCFGStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FDestinyMapImportCFGStyle::Get()
{
	return *StyleInstance;
}
