// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestinyMapImportCFGCommands.h"

#define LOCTEXT_NAMESPACE "FDestinyMapImportCFGModule"

void FDestinyMapImportCFGCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "DestinyMapImportCFG", "Bring up DestinyMapImportCFG window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ImportCharmCFGCommand, "Import Charm CFG", "Import a Charm CFG file", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
