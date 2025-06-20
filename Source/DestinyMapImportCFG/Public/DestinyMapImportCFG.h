// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/TextureFactory.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Modules/ModuleManager.h"


class FToolBarBuilder;
class FMenuBuilder;
/*
UENUM(BlueprintType)
enum EImportMapTarget : uint8
{
	IMT_None UMETA(DisplayName = "No Map Import"),
	IMT_Current UMETA(DisplayName = "Import to Open Map"),
	IMT_New UMETA(DisplayName = "Import to New Map"),
	IMT_MAX,
};
*/
UENUM(BlueprintType)
enum class ETextureFormat : uint8
{
	TF_Auto     UMETA(DisplayName = "Automatic"),
	TF_PNG      UMETA(DisplayName = "*.PNG"),
	TF_TIF      UMETA(DisplayName = "*.TIF/*.TIFF"),
	TF_TGA      UMETA(DisplayName = "*.TGA")
};


DECLARE_DELEGATE_OneParam(FOnFormatChanged, ETextureFormat)

class SCFGTextureFormatCombo : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCFGTextureFormatCombo) {}
        SLATE_EVENT(FOnFormatChanged, OnFormatChanged)
    SLATE_END_ARGS()
	using FComboItemType = TSharedPtr<FString>;

		void Construct(const FArguments& InArgs)
		{
			OnFormatChanged = InArgs._OnFormatChanged;

			Options.Add(MakeShared<FString>("Automatic"));
			Options.Add(MakeShared<FString>("*.PNG"));
			Options.Add(MakeShared<FString>("*.TIF/*.TIFF"));
			Options.Add(MakeShared<FString>("*.TGA"));

			CurrentItem = Options[0];

			ChildSlot
				[
					SNew(SComboBox<FComboItemType>)
						.OptionsSource(&Options)
						.OnSelectionChanged(this, &SCFGTextureFormatCombo::OnSelectionChanged)
						.OnGenerateWidget(this, &SCFGTextureFormatCombo::MakeWidgetForOption)
						.InitiallySelectedItem(CurrentItem)
						[
							SNew(STextBlock)
								.Text(this, &SCFGTextureFormatCombo::GetCurrentItemLabel)
						]
				];
		}


	ETextureFormat SelectedFormat = ETextureFormat::TF_Auto;
	ETextureFormat GetSelectedFormat() const { return SelectedFormat; }

private:

	FOnFormatChanged OnFormatChanged;
	TSharedRef<SWidget> MakeWidgetForOption(FComboItemType InOption)
	{
		return SNew(STextBlock).Text(FText::FromString(*InOption));
	}

	void OnSelectionChanged(FComboItemType NewValue, ESelectInfo::Type)
	{
		CurrentItem = NewValue;

		if (!NewValue.IsValid()) return;

		const FString& SelectedString = *NewValue;

		if (SelectedString == "Automatic")
		{
			SelectedFormat = ETextureFormat::TF_Auto;
		}
		else if (SelectedString == "*.PNG")
		{
			SelectedFormat = ETextureFormat::TF_PNG;
		}
		else if (SelectedString == "*.TIF/*.TIFF")
		{
			SelectedFormat = ETextureFormat::TF_TIF;
		}
		else if (SelectedString == "*.TGA")
		{
			SelectedFormat = ETextureFormat::TF_TGA;
		}

		if (OnFormatChanged.IsBound())
		{
			OnFormatChanged.Execute(SelectedFormat);
		}
	}


	FText GetCurrentItemLabel() const
	{
		if (CurrentItem.IsValid())
		{
			return FText::FromString(*CurrentItem);
		}
		return FText::FromString(TEXT("<<Invalid option>>"));
	}

private:
	FComboItemType CurrentItem;
	TArray<FComboItemType> Options;

};

#undef LOCTEXT_NAMESPACE

class FDestinyMapImportCFGModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	void ImportCharmCFGButtonClicked();
	void BuildMapButtonClicked();
	void FImportMaterials(FString ConfigPath, FStaticMaterial& StaticMaterialSlot, FSkeletalMaterial& SkeletalMaterialSlot, TSharedPtr<FJsonObject> MaterialJson, bool isStaticMesh, UTextureFactory* TextureFactory);
	void FImportTextures(TSharedPtr<FJsonObject> MaterialJson, FString ConfigPath, UTextureFactory* TextureFactory);
	void FImportToMap(TArray<FString> OutFiles);
	void FImportLightingToMap(FString ConfigPath);


	// Import vars
	bool bImportTextures = true;
	bool bImportMaterials = true;
	bool bUseCurrentMap = true;
	bool bMaterialGen = true UMETA(EditCondition = "bImportMaterials");
	bool bDiffuseApply = true UMETA(EditCondition = "bSkipMaterialGen");
	float fMapScale = 100.f;
	bool bImportAtmosphere = false;
	bool bImportCubeMap = false;
	float fLightIntensity = 10.0f;
	bool bImportLights = true;
	ETextureFormat SelectedFormat = ETextureFormat::TF_Auto;
	FString CFGFolderName;
	FStaticMaterial FinalStaticMaterialSlot;
	FSkeletalMaterial FinalSkeletalMaterialSlot;
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
