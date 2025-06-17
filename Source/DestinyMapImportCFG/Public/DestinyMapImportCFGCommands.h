// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DestinyMapImportCFGStyle.h"

class FDestinyMapImportCFGCommands : public TCommands<FDestinyMapImportCFGCommands>
{
public:

	FDestinyMapImportCFGCommands()
		: TCommands<FDestinyMapImportCFGCommands>(TEXT("DestinyMapImportCFG"), NSLOCTEXT("Contexts", "DestinyMapImportCFG", "DestinyMapImportCFG Plugin"), NAME_None, FDestinyMapImportCFGStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
	TSharedPtr< FUICommandInfo > ImportCharmCFGCommand;
};