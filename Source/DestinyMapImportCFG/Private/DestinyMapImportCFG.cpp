﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "DestinyMapImportCFG.h"
#include "DestinyMapImportCFGStyle.h"
#include "DestinyMapImportCFGCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialInstanceConstant.h"
#include "EditorAssetLibrary.h"
#include "Factories/FbxImportUI.h"
#include "Factories/TextureFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/ImportSettings.h"
#include "AutomatedAssetImportData.h"
#include "EditorFramework/AssetImportData.h"
#include "EditorReimportHandler.h"
#include "Factories/FbxFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/StaticMeshActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/RectLight.h"
#include "Components/LightComponent.h"
#include "AssetToolsModule.h"
#include "Factories/Factory.h"
#include "Editor.h"
#include "IAssetTools.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureLightProfile.h"
#include "JsonObjectConverter.h"
#include "Editor/EditorEngine.h"
#include "Engine/Light.h"
#include "ToolMenus.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialExpressionTextureSample.h"

static const FName DestinyMapImportCFGTabName("DestinyMapImportCFG");

#define LOCTEXT_NAMESPACE "FDestinyMapImportCFGModule"




void FDestinyMapImportCFGModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FDestinyMapImportCFGStyle::Initialize();
	FDestinyMapImportCFGStyle::ReloadTextures();

	FDestinyMapImportCFGCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FDestinyMapImportCFGCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FDestinyMapImportCFGModule::PluginButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FDestinyMapImportCFGCommands::Get().ImportCharmCFGCommand,
		FExecuteAction::CreateRaw(this, &FDestinyMapImportCFGModule::ImportCharmCFGButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDestinyMapImportCFGModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DestinyMapImportCFGTabName, FOnSpawnTab::CreateRaw(this, &FDestinyMapImportCFGModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FDestinyMapImportCFGTabTitle", "DestinyMapImportCFG"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FDestinyMapImportCFGModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FDestinyMapImportCFGStyle::Shutdown();

	FDestinyMapImportCFGCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DestinyMapImportCFGTabName);
}


TSharedRef<SDockTab> FDestinyMapImportCFGModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString("Destiny Map Import CFG"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString("Model Import Options"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				.Padding(10.0f)
				[
					SNew(SButton)
						.Text(FText::FromString("Import Map Models from Charm (*.CFG)"))
						.OnClicked_Lambda([this]() -> FReply {
						this->ImportCharmCFGButtonClicked();
						return FReply::Handled();
					})
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bImportTextures ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bImportTextures = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock).Text(FText::FromString("Import Textures"))
						]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString("Import Texture Format:"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCFGTextureFormatCombo)
						.IsEnabled_Lambda([this]() { return bImportTextures; })
						.OnFormatChanged(FOnFormatChanged::CreateLambda([this](ETextureFormat Format)
					{
						this->SelectedFormat = Format;
					}))
				]


				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bImportMaterials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bImportMaterials = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock)
								.Text(FText::FromString("Import Materials"))
								.ToolTipText(FText::FromString("Imports Materials based on Below Settings"))
						]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoHeight()
						[
							SNew(STextBlock)
								.Text(FText::FromString("Destiny Material Import Settings:"))
						]
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoHeight()
						[
							SNew(SCheckBox)
								.IsEnabled_Lambda([this]() { return bImportMaterials && bImportTextures; })
								.IsChecked_Lambda([this]() { return bMaterialGen ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bMaterialGen = (NewState == ECheckBoxState::Checked); })
								.Content()
								[
									SNew(STextBlock)
										.Text(FText::FromString("Add Texture Samples"))
										.ToolTipText(FText::FromString("Adds all required Texture Samples to Materials"))
								]
						]
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoHeight()
						[
							SNew(SCheckBox)
								.IsEnabled_Lambda([this]() { return bMaterialGen && bImportMaterials && bImportTextures; })
								.IsChecked_Lambda([this]() { return bDiffuseApply ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bDiffuseApply = (NewState == ECheckBoxState::Checked); })
								.Content()
								[
									SNew(STextBlock)
										.Text(FText::FromString("Apply Diffuse Map"))
										.ToolTipText(FText::FromString("Attempt to Apply Diffuse Map to Materials"))
								]
						]
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				.Padding(15)
				[
					SNew(STextBlock)
						.Text(FText::FromString("Map Building Options"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoHeight()
				.Padding(10.0f)
				[
					SNew(SButton)
						.Text(FText::FromString("Build Map in Level"))
						.OnClicked_Lambda([this]() -> FReply {
						this->BuildMapButtonClicked();
						return FReply::Handled();
					})
				]
				/*
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bUseCurrentMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bUseCurrentMap = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock).Text(FText::FromString("Import To Current Map"))
						]
				]
				*/
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SNumericEntryBox<float>)
						.MinValue(0.0f)
						.MaxValue(1000.0f)
						.Value_Lambda([this]() -> TOptional<float> { return fMapScale; })
						.OnValueChanged_Lambda([this](float NewValue) { fMapScale = NewValue; })
						.LabelVAlign(VAlign_Center)
						.Label()
						[
							SNew(STextBlock).Text(FText::FromString("Map Import Scale"))
						]
				]
				/*
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bImportAtmosphere ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bImportAtmosphere = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock).Text(FText::FromString("Import Atmosphere"))
						]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bImportCubeMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bImportCubeMap = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock).Text(FText::FromString("Import CubeMap"))
						]
				]
				*/
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bImportLights ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bImportLights = (NewState == ECheckBoxState::Checked); })
						.Content()
						[
							SNew(STextBlock).Text(FText::FromString("Import Lights"))
						]
				]
				/*
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SNumericEntryBox<float>)
						.MinValue(0.0f)
						.MaxValue(100000.0f)
						.Value_Lambda([this]() -> TOptional<float> { return fLightIntensity; })
						.OnValueChanged_Lambda([this](float NewValue) { fLightIntensity = NewValue; })
						.LabelVAlign(VAlign_Center)
						.Label()
						[
							SNew(STextBlock).Text(FText::FromString("Light Intensity"))
						]
				]
				*/
			];
}

void FDestinyMapImportCFGModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DestinyMapImportCFGTabName);
}

void FDestinyMapImportCFGModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FDestinyMapImportCFGCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FDestinyMapImportCFGCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FDestinyMapImportCFGModule::BuildMapButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;

	void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetActiveTopLevelWindow().IsValid())
	{
		ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	}

	TArray<FString> OutFiles;
	if (!DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("Choose Charm CFG File/s"), FPaths::ProjectContentDir(), TEXT(""), TEXT("CFG files (*.cfg)|*.cfg|All files (*.*)|*.*"), EFileDialogFlags::Multiple, OutFiles)) return;
	if (OutFiles.Num() == 0) return;
	FDestinyMapImportCFGModule::FImportToMap(OutFiles);
	
	FString ConfigPath = OutFiles[0];
	if (bImportLights == true)
	{
		FDestinyMapImportCFGModule::FImportLightingToMap(ConfigPath);
	}
}

void FDestinyMapImportCFGModule::FImportLightingToMap(FString ConfigPath)
{
	CFGFolderName = FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)).Replace(TEXT(" "), TEXT("_"));
	FString LightsPath = FPaths::Combine(FPaths::GetPath(ConfigPath), TEXT("/Rendering/Lights.json"));
	FString JsonRaw;
	if (!FFileHelper::LoadFileToString(JsonRaw, *LightsPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load lights JSON from %s"), *LightsPath);
		return;
	}

	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonRaw);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse lights JSON"));
		return;
	}

	for (const auto& LightPair : RootObj->Values)
	{
		FString LightName = LightPair.Key;
		TSharedPtr<FJsonObject> LightObj = LightPair.Value->AsObject();
		if (!LightObj.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Instances;
		if (!LightObj->TryGetArrayField("Instances", Instances)) continue;

		FString Type = LightObj->GetStringField("Type");

		FLinearColor Color = FLinearColor::White;
		if (LightObj->HasTypedField<EJson::Array>("Color"))
		{
			const TArray<TSharedPtr<FJsonValue>>& ColorArray = LightObj->GetArrayField("Color");
			if (ColorArray.Num() >= 3)
			{
				Color.R = ColorArray[0]->AsNumber();
				Color.G = ColorArray[1]->AsNumber();
				Color.B = ColorArray[2]->AsNumber();
				if (ColorArray.Num() >= 4)
					Color.A = ColorArray[3]->AsNumber();
			}
		}

		float Attenuation = 1000.f;
		if (LightObj->HasTypedField<EJson::Number>("Attenuation"))
		{
			Attenuation = LightObj->GetNumberField("Attenuation") * 1000.f;
		}

		FString CookieHash;
		if (LightObj->HasTypedField<EJson::String>("Cookie"))
		{
			CookieHash = LightObj->GetStringField("Cookie");
		}

		for (const TSharedPtr<FJsonValue>& InstanceVal : *Instances)
		{
			TSharedPtr<FJsonObject> InstanceObj = InstanceVal->AsObject();
			if (!InstanceObj.IsValid()) continue;

			FVector Location(
				InstanceObj->GetArrayField("Translation")[0]->AsNumber(),
				InstanceObj->GetArrayField("Translation")[1]->AsNumber(),
				InstanceObj->GetArrayField("Translation")[2]->AsNumber()
			);
			Location *= fMapScale;
			FQuat Rotation(
				InstanceObj->GetArrayField("Rotation")[0]->AsNumber(),
				InstanceObj->GetArrayField("Rotation")[1]->AsNumber(),
				InstanceObj->GetArrayField("Rotation")[2]->AsNumber(),
				InstanceObj->GetArrayField("Rotation")[3]->AsNumber()
			);
			FVector Scale(
				InstanceObj->GetArrayField("Scale")[0]->AsNumber(),
				InstanceObj->GetArrayField("Scale")[1]->AsNumber(),
				InstanceObj->GetArrayField("Scale")[2]->AsNumber()
			);
			Location.Y *= -1.f;
			Rotation.Y *= -1.f;
			Rotation.W *= -1.f;
			FTransform Transform(Rotation, Location, Scale);

			if (Type == "Line")
			{
				ARectLight* Light = GEditor->GetEditorWorldContext().World()->SpawnActor<ARectLight>(ARectLight::StaticClass(), Transform);
				Light->SetActorLabel(*LightName);
				Light->SetCastShadows(false);
				Light->GetLightComponent()->SetLightColor(Color);
				//Light->GetLightComponent()->Intensity(fLightIntensity);
				//Light->GetLightComponent()->Bounds(Attenuation);
			}
			else if (Type == "Spot")
			{
				ASpotLight* Light = GEditor->GetEditorWorldContext().World()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Transform);
				Light->SetActorLabel(*LightName);
				Light->SetCastShadows(false);
				Light->GetLightComponent()->SetLightColor(Color);
				//Light->GetLightComponent()->Intensity(fLightIntensity);
				//Light->GetLightComponent()->Bounds(Attenuation);

				if (!CookieHash.IsEmpty())
				{
					FString CookieAssetPath = "/Game/" + CFGFolderName + "/Textures/Lights/" + CookieHash + TEXT(".") + CookieHash;
					UTexture* CookieTexture = Cast<UTexture>(StaticLoadObject(UTexture::StaticClass(), nullptr, *CookieAssetPath));
					if (CookieTexture)
					{
						Light->GetLightComponent()->SetLightFunctionMaterial(nullptr); // Placeholder if you want to assign a light function later
						// Note: Direct cookie projection from texture isn't supported natively without light function materials.
					}
				}
			}
			else if (Type == "Shadowing")
			{
				ASpotLight* Light = GEditor->GetEditorWorldContext().World()->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Transform);
				Light->SetActorLabel(*LightName);
				Light->SetCastShadows(true);
				Light->GetLightComponent()->SetLightColor(Color);
				//Light->GetLightComponent()->Intensity(fLightIntensity);
				//Light->GetLightComponent()->Bounds(Attenuation);
			}
		}
	}
}

void FDestinyMapImportCFGModule::ImportCharmCFGButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;

	void* ParentWindowHandle = nullptr;
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetActiveTopLevelWindow().IsValid())
	{
		ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	}
	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot();
	TextureFactory->SuppressImportOverwriteDialog();

	UFbxFactory* FbxFactory = NewObject<UFbxFactory>();
	FbxFactory->AddToRoot();
	FbxFactory->ConfigureProperties(); // initializes ImportUI

	TArray<FString> OutFiles;
	if (!DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("Choose Charm CFG File/s"), FPaths::ProjectContentDir(), TEXT(""), TEXT("CFG files (*.cfg)|*.cfg|All files (*.*)|*.*"), EFileDialogFlags::Multiple, OutFiles)) return;
	if (OutFiles.Num() == 0) return;
	for (auto& file : OutFiles)
	{
		FString ConfigPath = file;
		CFGFolderName = FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)).Replace(TEXT(" "), TEXT("_"));

		FString TextureImportPath = TEXT("/Game/") + CFGFolderName + TEXT("/Textures");

		if (!UEditorAssetLibrary::DoesDirectoryExist(TextureImportPath)) UEditorAssetLibrary::MakeDirectory(TextureImportPath);

		FString FileContents;
		if (!FFileHelper::LoadFileToString(FileContents, *ConfigPath)) return;

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid()) return;
		if (RootObject->GetStringField("ExportType") != TEXT("Map")) return;

		FString Type = RootObject->GetStringField("Type");
		FString AssetsPath = RootObject->GetStringField("AssetsPath");
		TSharedPtr<FJsonObject> Parts = RootObject->GetObjectField("Parts");

		FString DestinationPath = TEXT("/Game/") + CFGFolderName + TEXT("/Models/") + Type;

		if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationPath))
			UEditorAssetLibrary::MakeDirectory(DestinationPath);

		for (auto& Pair : Parts->Values)
		{
			FString ModelName = Pair.Key;
			TSharedPtr<FJsonObject> SubMap = Pair.Value->AsObject();
			if (!SubMap.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("SubMap is invalid for model: %s"), *ModelName);
				continue;
			}

			if (SubMap->Values.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("SubMap->Values is empty for: %s"), *ModelName);
				continue;
			}


			TArray<FString> ImportedMaterialRefs;


			if (Type == "Terrain")
			{
				int ChunkIndex = 0;
				while (true)
				{
					FString ChunkName = ModelName + TEXT("_") + FString::FromInt(ChunkIndex);
					if (UEditorAssetLibrary::DoesAssetExist(DestinationPath + "/" + ChunkName)) continue;
					FString SourcePath = FPaths::Combine(AssetsPath, TEXT("Models"), Type, ChunkName + TEXT(".fbx"));
					if (!FPaths::FileExists(SourcePath)) break;
					/*
					if (bSkipExistingAssets)
					{
						FString AssetPath = DestinationPath + TEXT("/") + ChunkName + TEXT(".") + ChunkName;
						if (UEditorAssetLibrary::DoesAssetExist(AssetPath)) {
							++ChunkIndex;
							continue;
						}
					}
					*/

					FbxFactory->ImportUI->bImportAsSkeletal = false;
					FbxFactory->ImportUI->bImportMaterials = false;
					FbxFactory->ImportUI->bImportTextures = false;
					FbxFactory->ImportUI->SkeletalMeshImportData->ImportUniformScale = fMapScale;
					FbxFactory->ImportUI->StaticMeshImportData->ImportUniformScale = fMapScale;
					FbxFactory->ImportUI->StaticMeshImportData->bConvertScene = false;
					FbxFactory->ImportUI->StaticMeshImportData->bConvertScene = false;
					FbxFactory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
					UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
					ImportData->FactoryName = TEXT("FbxFactory");
					ImportData->Factory = FbxFactory;
					ImportData->DestinationPath = DestinationPath;
					ImportData->Filenames.Add(SourcePath);



					FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
					IAssetTools& AssetTools = AssetToolsModule.Get();

					TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);

					if (ImportedAssets.Num() == 0)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to read JSON value ImportedAssets"));
					}
					if (ImportedAssets.Num() > 0 && bImportMaterials == true)
					{
						for (UObject* Imported : ImportedAssets)
						{

							FSkeletalMaterial EmptySkeletalMaterialSlot;
							if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Imported))
							{
								TArray<FStaticMaterial> UpdatedMaterials = StaticMesh->GetStaticMaterials();
								for (FStaticMaterial& StaticMaterialSlot : UpdatedMaterials)
								{
									FinalStaticMaterialSlot = StaticMaterialSlot;
									FString MaterialRef = StaticMaterialSlot.ImportedMaterialSlotName.ToString();
									FString TrimmedMaterialRef = MaterialRef;
									
									// Strip _ncl1_ suffixes if present
									int32 NclIndex = MaterialRef.Find(TEXT("_ncl1_"));
									if (NclIndex != INDEX_NONE)
									{
										TrimmedMaterialRef = MaterialRef.Left(NclIndex);
										FinalStaticMaterialSlot.ImportedMaterialSlotName = FName(*TrimmedMaterialRef);
										UE_LOG(LogTemp, Warning, TEXT("Detected renamed material: %s → %s"), *MaterialRef, *TrimmedMaterialRef);
									}
									


									//if (ImportedMaterialRefs.Contains(TrimmedMaterialRef))
									//	continue;

									ImportedMaterialRefs.Add(TrimmedMaterialRef);
									FString MaterialJsonPath = FPaths::Combine(AssetsPath, TEXT("Materials"), TrimmedMaterialRef + TEXT(".json"));
									if (!FPaths::FileExists(MaterialJsonPath)) continue;

									FString JsonContent;
									if (!FFileHelper::LoadFileToString(JsonContent, *MaterialJsonPath)) continue;

									TSharedPtr<FJsonObject> MaterialJson;
									TSharedRef<TJsonReader<>> MaterialReader = TJsonReaderFactory<>::Create(JsonContent);
									if (!FJsonSerializer::Deserialize(MaterialReader, MaterialJson) || !MaterialJson.IsValid()) {
										UE_LOG(LogTemp, Warning, TEXT("Failed to parse material JSON in %s"), *MaterialJsonPath);
										continue;
									}

									if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material"))) {
										UE_LOG(LogTemp, Warning, TEXT("Missing 'Material' object in %s"), *MaterialJsonPath);
										continue;
									}

									if (!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel"))) {
										UE_LOG(LogTemp, Warning, TEXT("Missing 'Pixel' object under 'Material' in %s"), *MaterialJsonPath);
										continue;
									}

									if (!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures"))) {
										UE_LOG(LogTemp, Warning, TEXT("Missing 'Textures' object under 'Material.Pixel' in %s"), *MaterialJsonPath);
										continue;
									}
									if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material")) ||
										!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel")) ||
										!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures")))
									{
										UE_LOG(LogTemp, Warning, TEXT("Malformed material JSON in %s"), *MaterialJsonPath);
										continue;
									}


									// Import Textures
									TMap<FString, TSharedPtr<FJsonValue>> TextureMap = MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->GetObjectField("Textures")->Values;



									if (bImportTextures == true && bImportMaterials == false)
									{
										FDestinyMapImportCFGModule::FImportTextures(MaterialJson, file, TextureFactory);
									}


									if (bImportMaterials == true)
									{
										FDestinyMapImportCFGModule::FImportMaterials(file, StaticMaterialSlot, EmptySkeletalMaterialSlot, MaterialJson, true, TextureFactory);
										StaticMaterialSlot = FinalStaticMaterialSlot;
									}
								}
								StaticMesh->SetStaticMaterials(UpdatedMaterials);
								StaticMesh->MarkPackageDirty();
								StaticMesh->PostEditChange();
							}
						}
					}

					++ChunkIndex;

				}
			}
			
			else
			{

				if (UEditorAssetLibrary::DoesAssetExist(DestinationPath + "/" + ModelName)) continue;
				FString SourcePath = FPaths::Combine(AssetsPath, TEXT("Models"), Type, ModelName + TEXT(".fbx"));
				if (!FPaths::FileExists(SourcePath)) break;

				if (!FbxFactory->ImportUI)
				{
					UE_LOG(LogTemp, Error, TEXT("FbxFactory->ImportUI is null."));
					return;
				}

				FbxFactory->ImportUI->bImportAsSkeletal = false;
				FbxFactory->ImportUI->bImportMaterials = false;
				FbxFactory->ImportUI->bImportTextures = false;
				FbxFactory->ImportUI->SkeletalMeshImportData->ImportUniformScale = fMapScale;
				FbxFactory->ImportUI->StaticMeshImportData->ImportUniformScale = fMapScale;
				FbxFactory->ImportUI->StaticMeshImportData->bConvertScene = false;
				FbxFactory->ImportUI->StaticMeshImportData->bConvertScene = false;
				FbxFactory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
				UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
				ImportData->FactoryName = TEXT("FbxFactory");
				ImportData->Factory = FbxFactory;
				ImportData->DestinationPath = DestinationPath;
				ImportData->Filenames.Add(SourcePath);

				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
				IAssetTools& AssetTools = AssetToolsModule.Get();

				TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);

				if (ImportedAssets.Num() == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to read JSON value ImportedAssets"));
				}
				if (ImportedAssets.Num() > 0 && bImportMaterials == true)
				{
					for (UObject* Imported : ImportedAssets)
					{
						FStaticMaterial EmptyStaticMaterialSlot;
						FSkeletalMaterial EmptySkeletalMaterialSlot;
						if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Imported))
						{
							TArray<FStaticMaterial> UpdatedMaterials = StaticMesh->GetStaticMaterials();
							for (FStaticMaterial& StaticMaterialSlot : UpdatedMaterials)
							{
								FinalStaticMaterialSlot = StaticMaterialSlot;
								FString MaterialRef = StaticMaterialSlot.ImportedMaterialSlotName.ToString();
								FString TrimmedMaterialRef = MaterialRef;
								
								// Strip _ncl1_ suffixes if present
								int32 NclIndex = MaterialRef.Find(TEXT("_ncl1_"));
								if (NclIndex != INDEX_NONE)
								{
									TrimmedMaterialRef = MaterialRef.Left(NclIndex);
									FinalStaticMaterialSlot.ImportedMaterialSlotName = FName(*TrimmedMaterialRef);
									UE_LOG(LogTemp, Warning, TEXT("Detected renamed material: %s → %s"), *MaterialRef, *TrimmedMaterialRef);
								}
								


								//if (ImportedMaterialRefs.Contains(TrimmedMaterialRef))
								//	continue;

								ImportedMaterialRefs.Add(TrimmedMaterialRef);
								FString MaterialJsonPath = FPaths::Combine(AssetsPath, TEXT("Materials"), TrimmedMaterialRef + TEXT(".json"));

								FString JsonContent;
								if (!FFileHelper::LoadFileToString(JsonContent, *MaterialJsonPath)) continue;

								TSharedPtr<FJsonObject> MaterialJson;
								TSharedRef<TJsonReader<>> MaterialReader = TJsonReaderFactory<>::Create(JsonContent);
								if (!FJsonSerializer::Deserialize(MaterialReader, MaterialJson) || !MaterialJson.IsValid()) {
									UE_LOG(LogTemp, Warning, TEXT("Failed to parse material JSON in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Material' object in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Pixel' object under 'Material' in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Textures' object under 'Material.Pixel' in %s"), *MaterialJsonPath);
									continue;
								}
								if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material")) ||
									!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel")) ||
									!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures")))
								{
									UE_LOG(LogTemp, Warning, TEXT("Malformed material JSON in %s"), *MaterialJsonPath);
									continue;
								}


								// Import Textures
								TMap<FString, TSharedPtr<FJsonValue>> TextureMap = MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->GetObjectField("Textures")->Values;
								


								if (bImportTextures == true && bImportMaterials == false)
								{
									FDestinyMapImportCFGModule::FImportTextures(MaterialJson, file, TextureFactory);
								}


								if (bImportMaterials == true)
								{
									FDestinyMapImportCFGModule::FImportMaterials(file, StaticMaterialSlot, EmptySkeletalMaterialSlot, MaterialJson, true, TextureFactory);
									StaticMaterialSlot = FinalStaticMaterialSlot;
								}
							}

							StaticMesh->SetStaticMaterials(UpdatedMaterials);
							StaticMesh->MarkPackageDirty();
							StaticMesh->PostEditChange();
						}

						else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Imported))
						{
							TArray<FSkeletalMaterial> UpdatedMaterials = SkeletalMesh->GetMaterials();
							for (FSkeletalMaterial& SkeletalMaterialSlot : UpdatedMaterials)
							{
								FinalSkeletalMaterialSlot = SkeletalMaterialSlot;
								FString MaterialRef = SkeletalMaterialSlot.ImportedMaterialSlotName.ToString();
								
								FString TrimmedMaterialRef = MaterialRef;
								
								// Strip _ncl1_ suffixes if present
								int32 NclIndex = MaterialRef.Find(TEXT("_ncl1_"));
								if (NclIndex != INDEX_NONE)
								{
									TrimmedMaterialRef = MaterialRef.Left(NclIndex);
									FinalSkeletalMaterialSlot.ImportedMaterialSlotName = FName(*TrimmedMaterialRef);
									UE_LOG(LogTemp, Warning, TEXT("Detected renamed material: %s → %s"), *MaterialRef, *TrimmedMaterialRef);
								}
								
								
								//if (ImportedMaterialRefs.Contains(TrimmedMaterialRef))
									//continue;

								ImportedMaterialRefs.Add(TrimmedMaterialRef);
								FString MaterialJsonPath = FPaths::Combine(AssetsPath, TEXT("Materials"), TrimmedMaterialRef + TEXT(".json"));

								FString JsonContent;
								if (!FFileHelper::LoadFileToString(JsonContent, *MaterialJsonPath)) continue;

								TSharedPtr<FJsonObject> MaterialJson;
								TSharedRef<TJsonReader<>> MaterialReader = TJsonReaderFactory<>::Create(JsonContent);
								if (!FJsonSerializer::Deserialize(MaterialReader, MaterialJson) || !MaterialJson.IsValid()) {
									UE_LOG(LogTemp, Warning, TEXT("Failed to parse material JSON in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Material' object in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Pixel' object under 'Material' in %s"), *MaterialJsonPath);
									continue;
								}

								if (!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures"))) {
									UE_LOG(LogTemp, Warning, TEXT("Missing 'Textures' object under 'Material.Pixel' in %s"), *MaterialJsonPath);
									continue;
								}
								if (!MaterialJson->HasTypedField<EJson::Object>(TEXT("Material")) ||
									!MaterialJson->GetObjectField("Material")->HasTypedField<EJson::Object>(TEXT("Pixel")) ||
									!MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->HasTypedField<EJson::Object>(TEXT("Textures")))
								{
									UE_LOG(LogTemp, Warning, TEXT("Malformed material JSON in %s"), *MaterialJsonPath);
									continue;
								}


								// Import Textures
								TMap<FString, TSharedPtr<FJsonValue>> TextureMap = MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->GetObjectField("Textures")->Values;
								

								if (bImportTextures == true && bImportMaterials == false)
								{
									FDestinyMapImportCFGModule::FImportTextures(MaterialJson, file, TextureFactory);
								}


								if (bImportMaterials == true)
								{
									FDestinyMapImportCFGModule::FImportMaterials(file, EmptyStaticMaterialSlot, SkeletalMaterialSlot, MaterialJson, false, TextureFactory);
									SkeletalMaterialSlot = FinalSkeletalMaterialSlot;
								}
							}
							SkeletalMesh->SetMaterials(UpdatedMaterials);
							SkeletalMesh->MarkPackageDirty();
							SkeletalMesh->PostEditChange();
						}

					}





				}



			}
		}
	}


}

void FDestinyMapImportCFGModule::FImportToMap(TArray<FString> OutFiles)
{
	
	for (auto& CFGFile : OutFiles)
	{
		FString ConfigPath = CFGFile;
		CFGFolderName = FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)).Replace(TEXT(" "), TEXT("_"));
		FString FileContents;
		if (!FFileHelper::LoadFileToString(FileContents, *ConfigPath)) continue;

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid()) continue;

		// Skip if the export type is not "Map"
		if (RootObject->GetStringField("ExportType") != TEXT("Map")) continue;

		// Extract necessary information
		FString Type = RootObject->GetStringField("Type");
		FString FolderName = RootObject->GetStringField("MeshName");
		FString AssetsPath = RootObject->GetStringField("AssetsPath");

		// Handle instances (meshes) — only once!
		TSharedPtr<FJsonObject> InstancesObject = RootObject->GetObjectField("Instances");
		if (InstancesObject.IsValid())
		{
			for (const auto& InstancePair : InstancesObject->Values)
			{

				FString MeshName = InstancePair.Key;
				const TArray<TSharedPtr<FJsonValue>>& InstanceArray = InstancePair.Value->AsArray();
				if (Type == TEXT("Terrain"))
				{
					int32 TerrainChunkIndex = 0;
					while (true)
					{
						FString SplitMeshName = MeshName + FString::Printf(TEXT("_%d"), TerrainChunkIndex);
						FString SplitAssetPath = "/Game/" + CFGFolderName + "/Models/" + Type + "/" + SplitMeshName + "." + SplitMeshName;
						UStaticMesh* TerrainMeshAsset = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *SplitAssetPath));
						if (!TerrainMeshAsset) break;
						FTransform Transform;
						AStaticMeshActor* NewActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
						if (NewActor)
						{
							NewActor->GetStaticMeshComponent()->SetStaticMesh(TerrainMeshAsset);
							NewActor->SetActorLabel(SplitMeshName);
							NewActor->SetFolderPath(FName(*FolderName));
						}
						++TerrainChunkIndex;
					}
				}
				else if (Type == TEXT("Decorators"))
				{
					UWorld* World = GEditor->GetEditorWorldContext().World();
					if (!World) return;

					TMap<FString, UHierarchicalInstancedStaticMeshComponent*> MeshToHISM;

					AActor* DecoratorContainer = World->SpawnActor<AActor>(AActor::StaticClass());
					DecoratorContainer->SetActorLabel(TEXT("Decorator_Batch"));
					DecoratorContainer->SetFolderPath(FName(*FolderName));

					for (const TSharedPtr<FJsonValue>& InstanceVal : InstanceArray)
					{
						TSharedPtr<FJsonObject> InstanceObj = InstanceVal->AsObject();
						if (!InstanceObj.IsValid()) continue;

						FVector Location(
							InstanceObj->GetArrayField("Translation")[0]->AsNumber(),
							InstanceObj->GetArrayField("Translation")[1]->AsNumber(),
							InstanceObj->GetArrayField("Translation")[2]->AsNumber()
						);
						Location *= fMapScale;
						FQuat Rotation(
							InstanceObj->GetArrayField("Rotation")[0]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[1]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[2]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[3]->AsNumber()
						);
						FVector Scale(
							InstanceObj->GetArrayField("Scale")[0]->AsNumber(),
							InstanceObj->GetArrayField("Scale")[1]->AsNumber(),
							InstanceObj->GetArrayField("Scale")[2]->AsNumber()
						);
						Location.Y *= -1.f;
						Rotation.Y *= -1.f;
						Rotation.W *= -1.f;
						FTransform Transform(Rotation, Location, Scale);

						FString AssetPath = "/Game/" + CFGFolderName + "/Models/" + Type + "/" + MeshName + "." + MeshName;
						if (UStaticMesh* MeshAsset = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *AssetPath)))
						{
							if (!MeshToHISM.Contains(MeshName))
							{
								UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(DecoratorContainer);
								HISM->SetStaticMesh(MeshAsset);
								HISM->RegisterComponent();
								HISM->AttachToComponent(DecoratorContainer->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
								MeshToHISM.Add(MeshName, HISM);
							}

							MeshToHISM[MeshName]->AddInstance(Transform);
						}
					}
				}
				else
				{
					for (const TSharedPtr<FJsonValue>& InstanceVal : InstanceArray)
					{
						TSharedPtr<FJsonObject> InstanceObj = InstanceVal->AsObject();
						if (!InstanceObj.IsValid()) continue;

						FVector Location(
							InstanceObj->GetArrayField("Translation")[0]->AsNumber(),
							InstanceObj->GetArrayField("Translation")[1]->AsNumber(),
							InstanceObj->GetArrayField("Translation")[2]->AsNumber()
						);
						Location *= fMapScale;
						FQuat Rotation(
							InstanceObj->GetArrayField("Rotation")[0]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[1]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[2]->AsNumber(),
							InstanceObj->GetArrayField("Rotation")[3]->AsNumber()
						);
						FVector Scale(
							InstanceObj->GetArrayField("Scale")[0]->AsNumber(),
							InstanceObj->GetArrayField("Scale")[1]->AsNumber(),
							InstanceObj->GetArrayField("Scale")[2]->AsNumber()
						);
						Location.Y *= -1.f;
						Rotation.Y *= -1.f;
						Rotation.W *= -1.f;
						FTransform Transform(Rotation, Location, Scale);

						FString AssetPath = "/Game/" + CFGFolderName + "/Models/" + Type + "/" + MeshName + "." + MeshName;
						if (UStaticMesh* MeshAsset = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *AssetPath)))
						{
							if (MeshAsset)
							{
								AStaticMeshActor* NewActor = GEditor->GetEditorWorldContext().World()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
								if (NewActor)
								{
									NewActor->GetStaticMeshComponent()->SetStaticMesh(MeshAsset);
									NewActor->SetActorLabel(MeshName);
									NewActor->SetFolderPath(FName(*FolderName));
								}
							}
						}

						if (USkeletalMesh* MeshAsset = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *AssetPath)))
						{
							if (MeshAsset)
							{
								ASkeletalMeshActor* NewActor = GEditor->GetEditorWorldContext().World()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), Transform);
								if (NewActor)
								{
									NewActor->GetSkeletalMeshComponent()->SetSkeletalMesh(MeshAsset);
									NewActor->SetActorLabel(MeshName);
									NewActor->SetFolderPath(FName(*FolderName));
								}
							}
						}
					}
				}
			}
		}


	}
}


void FDestinyMapImportCFGModule::FImportTextures(TSharedPtr<FJsonObject> MaterialJson, FString ConfigPath, UTextureFactory* TextureFactory)
{
	CFGFolderName = FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)).Replace(TEXT(" "), TEXT("_"));

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *ConfigPath)) return;

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid()) return;
	if (RootObject->GetStringField("ExportType") != TEXT("Map")) return;

	FString Type = RootObject->GetStringField("Type");
	FString AssetsPath = RootObject->GetStringField("AssetsPath");
	TSharedPtr<FJsonObject> Parts = RootObject->GetObjectField("Parts");

	FString DestinationPath = TEXT("/Game/") + CFGFolderName + TEXT("/Models/") + Type;

	for (auto& Pair : Parts->Values)
	{
		FString ModelName = Pair.Key;
		TSharedPtr<FJsonObject> SubMap = Pair.Value->AsObject();

		for (auto& MaterialPair : SubMap->Values)
		{
			FString MaterialRef = MaterialPair.Value->AsString(); // e.g., "8EE5FF80"
			FString MaterialJsonPath = FPaths::Combine(AssetsPath, TEXT("Materials"), MaterialRef + TEXT(".json"));

			if (!FPaths::FileExists(MaterialJsonPath))
			{
				UE_LOG(LogTemp, Warning, TEXT("Missing material JSON file: %s"), *MaterialJsonPath);
				continue;
			}

			FString JsonContent;
			if (FFileHelper::LoadFileToString(JsonContent, *MaterialJsonPath))
			{
			}

			FString TextureImportPath = TEXT("/Game/") + CFGFolderName + TEXT("/Textures");
			TMap<FString, TSharedPtr<FJsonValue>> TextureMap = MaterialJson->GetObjectField("Material")
				->GetObjectField("Pixel")
				->GetObjectField("Textures")->Values;

			for (const auto& TextureEntry : TextureMap)
			{
				TSharedPtr<FJsonObject> TextureObj = TextureEntry.Value->AsObject();
				FString Hash = TextureObj->GetStringField("Hash");
				FString Format = TextureObj->GetStringField("Format");
				FString Colorspace = TextureObj->GetStringField("Colorspace");

				FString TextureFormat;
				FString TextureSourcePath;

				FString PNGPath = FPaths::Combine(AssetsPath, TEXT("Textures"), Hash + TEXT(".png"));
				FString TGAPath = FPaths::Combine(AssetsPath, TEXT("Textures"), Hash + TEXT(".tga"));
				FString TifPath = FPaths::Combine(AssetsPath, TEXT("Textures"), Hash + TEXT(".tif"));
				FString TiffPath = FPaths::Combine(AssetsPath, TEXT("Textures"), Hash + TEXT(".tiff"));

				switch (SelectedFormat)
				{
				case ETextureFormat::TF_PNG:
					if (FPaths::FileExists(PNGPath))
					{
						TextureFormat = ".png";
						TextureSourcePath = PNGPath;
					}
					else
					{
						continue;
					}
					break;

				case ETextureFormat::TF_TGA:
					if (FPaths::FileExists(TGAPath))
					{
						TextureFormat = ".tga";
						TextureSourcePath = TGAPath;
					}
					else
					{
						continue;
					}
					break;

				case ETextureFormat::TF_TIF:
					if (FPaths::FileExists(TiffPath))
					{
						TextureFormat = ".tiff";
						TextureSourcePath = TiffPath;
					}
					else if (FPaths::FileExists(TifPath))
					{
						TextureFormat = ".tif";
						TextureSourcePath = TifPath;
					}
					else
					{
						continue;
					}
					break;

				case ETextureFormat::TF_Auto:
				default:
					if (FPaths::FileExists(TGAPath))
					{
						TextureFormat = ".tga";
						TextureSourcePath = TGAPath;
					}
					else if (FPaths::FileExists(PNGPath))
					{
						TextureFormat = ".png";
						TextureSourcePath = PNGPath;
					}
					else if (FPaths::FileExists(TiffPath))
					{
						TextureFormat = ".tiff";
						TextureSourcePath = TiffPath;
					}
					else if (FPaths::FileExists(TifPath))
					{
						TextureFormat = ".tif";
						TextureSourcePath = TifPath;
					}
					else
					{
						continue;
					}
					break;
				}


				// For PNG or TGA we still need to generate path
				if (TextureSourcePath.IsEmpty() && !TextureFormat.IsEmpty())
				{
					TextureSourcePath = FPaths::Combine(AssetsPath, TEXT("Textures"), Hash + TextureFormat);
					if (!FPaths::FileExists(TextureSourcePath)) continue;
				}

				// Proceed to import
				UAutomatedAssetImportData* TextureImportData = NewObject<UAutomatedAssetImportData>();
				TextureImportData->FactoryName = TEXT("TextureFactory");
				TextureImportData->Factory = TextureFactory;
				TextureImportData->DestinationPath = TextureImportPath;
				TextureImportData->Filenames.Add(TextureSourcePath);

				FAssetToolsModule& TextureAssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
				TArray<UObject*> ImportedTextures = TextureAssetToolsModule.Get().ImportAssetsAutomated(TextureImportData);

				for (UObject* ImportedObj : ImportedTextures)
				{
					if (UTexture2D* ImportedTex = Cast<UTexture2D>(ImportedObj))
					{
						ImportedTex->SRGB = (Colorspace == TEXT("sRGB"));
						if (Format == "BC1_UNORM_SRGB")	ImportedTex->CompressionSettings = TC_Default;
						else if (Format == "BC7_UNORM_SRGB" || Format == "BC7_UNORM")	ImportedTex->CompressionSettings = TC_BC7;
						else if (Format == "BC5_UNORM") ImportedTex->CompressionSettings = TC_Normalmap;
						else if (Format == "BC4_UNORM")	ImportedTex->CompressionSettings = TC_Alpha;
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Unknown Texture Format for Texture %s: %s"), *Hash, *Format);
							ImportedTex->CompressionSettings = TC_Default;
						}
						ImportedTex->PostEditChange();
						ImportedTex->MarkPackageDirty();
					}
				}
			}



		}
	}
}

/*
	TC_Default					UMETA(DisplayName = "Default (DXT1/5, BC1/3 on DX11)"),
	TC_Normalmap				UMETA(DisplayName = "Normalmap (DXT5, BC5 on DX11)"),
	TC_Masks					UMETA(DisplayName = "Masks (no sRGB)"),
	TC_Grayscale				UMETA(DisplayName = "Grayscale (G8/16, RGB8 sRGB)"),
	TC_Displacementmap			UMETA(DisplayName = "Displacementmap (G8/16)"),
	TC_VectorDisplacementmap	UMETA(DisplayName = "VectorDisplacementmap (RGBA8)"),
	TC_HDR						UMETA(DisplayName = "HDR (RGBA16F, no sRGB)"),
	TC_EditorIcon				UMETA(DisplayName = "UserInterface2D (RGBA)"),
	TC_Alpha					UMETA(DisplayName = "Alpha (no sRGB, BC4 on DX11)"),
	TC_DistanceFieldFont		UMETA(DisplayName = "DistanceFieldFont (G8)"),
	TC_HDR_Compressed			UMETA(DisplayName = "HDR Compressed (RGB, BC6H, DX11)"),
	TC_BC7						UMETA(DisplayName = "BC7 (DX11, optional A)"),
	TC_HalfFloat				UMETA(DisplayName = "Half Float (R16F)"),
	TC_LQ				        UMETA(Hidden, DisplayName = "Low Quality (BGR565/BGR555A1)", ToolTip = "BGR565/BGR555A1, fallback to DXT1/DXT5 on Mac platform"),
	TC_EncodedReflectionCapture	UMETA(Hidden),
	TC_SingleFloat				UMETA(DisplayName = "Single Float (R32F)"),
	TC_HDR_F32					UMETA(DisplayName = "HDR High Precision (RGBA32F)"),
	TC_MAX,
*/

void FDestinyMapImportCFGModule::FImportMaterials(FString ConfigPath, FStaticMaterial& StaticMaterialSlot, FSkeletalMaterial& SkeletalMaterialSlot, TSharedPtr<FJsonObject> MaterialJson, bool isStaticMesh, UTextureFactory* TextureFactory)
{
	CFGFolderName = FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)).Replace(TEXT(" "), TEXT("_"));
	FString MaterialName;
	if (isStaticMesh == true)
	{
		MaterialName = StaticMaterialSlot.ImportedMaterialSlotName.ToString();
		
		UE_LOG(LogTemp, Warning, TEXT("FImportMaterials called with slot name: %s"), *StaticMaterialSlot.ImportedMaterialSlotName.ToString());
	}
	else
	{
		MaterialName = SkeletalMaterialSlot.ImportedMaterialSlotName.ToString();
	}

	FString TrimmedMaterialRef = MaterialName;
	int32 NclIndex = MaterialName.Find(TEXT("_ncl1_"));
	if (NclIndex != INDEX_NONE)
	{
		TrimmedMaterialRef = MaterialName.Left(NclIndex);
		FinalStaticMaterialSlot.ImportedMaterialSlotName = FName(*TrimmedMaterialRef);
		UE_LOG(LogTemp, Warning, TEXT("Detected renamed material: %s → %s"), *MaterialName, *TrimmedMaterialRef);
	}

	FString MatPath = "/Game/" + CFGFolderName + "/Materials/" + TrimmedMaterialRef + "." + TrimmedMaterialRef;
	UMaterial* NewMaterial;
	if (UObject* LoadedObj = UEditorAssetLibrary::LoadAsset(MatPath))
	{

		if (!LoadedObj)
		{
			UE_LOG(LogTemp, Warning, TEXT("No Object Loaded: %s"), *LoadedObj->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Object Successfully Loaded: %s"), *LoadedObj->GetName());
		}
		NewMaterial = Cast<UMaterial>(LoadedObj);
		if (!NewMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("No Material Loaded: %s"), *NewMaterial->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Material Successfully Loaded: %s"), *NewMaterial->GetName());
		}
		if (NewMaterial)
		{
		}
	}
	else
	{

		MatPath = "/Game/" + CFGFolderName + "/Materials/" + TrimmedMaterialRef;
		UPackage* Package = CreatePackage(*MatPath);
		NewMaterial = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
		NewMaterial->AddToRoot();

		UMaterialExpressionTextureSample* FirstSRGBSample = nullptr;
		if (bImportTextures == true)
		{
			if (bMaterialGen)
			{
				FDestinyMapImportCFGModule::FImportTextures(MaterialJson, ConfigPath, TextureFactory);

				TMap<FString, TSharedPtr<FJsonValue>> TextureMap = MaterialJson->GetObjectField("Material")->GetObjectField("Pixel")->GetObjectField("Textures")->Values;


				int32 Index = 0;
				for (const auto& TextureEntry : TextureMap)
				{
					TSharedPtr<FJsonObject> TextureObj = TextureEntry.Value->AsObject();
					FString Hash = TextureObj->GetStringField("Hash");
					FString Colorspace = TextureObj->GetStringField("Colorspace");
					FString TexturePath = "/Game/" + CFGFolderName + "/Textures/" + Hash + "." + Hash;
					UTexture2D* TextureAsset = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
					if (!TextureAsset) continue;

					UMaterialExpressionTextureSample* TextureSample = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
					TextureSample->Texture = TextureAsset;
					TextureSample->Material = NewMaterial;
					TextureSample->SamplerType = (Colorspace == TEXT("sRGB")) ? SAMPLERTYPE_Color : SAMPLERTYPE_LinearColor;
					TextureSample->Desc = Hash;

					// Place expressions starting at (-320, 0), stepping vertically by 300
					TextureSample->MaterialExpressionEditorX = -320;
					TextureSample->MaterialExpressionEditorY = Index * 300;

					NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSample);

					if (bDiffuseApply && !FirstSRGBSample && Colorspace == TEXT("sRGB"))
					{
						FirstSRGBSample = TextureSample;
					}

					++Index;
				}



				if (bDiffuseApply && FirstSRGBSample)
				{
					NewMaterial->GetEditorOnlyData()->BaseColor.Expression = FirstSRGBSample;
				}
			}
		}

		NewMaterial->PostEditChange();
		NewMaterial->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(NewMaterial);

	}
	if (isStaticMesh == true)
	{
		FinalStaticMaterialSlot.MaterialInterface = NewMaterial;
	}
	else
	{
		FinalSkeletalMaterialSlot.MaterialInterface = NewMaterial;
	}
}
	


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDestinyMapImportCFGModule, DestinyMapImportCFG)