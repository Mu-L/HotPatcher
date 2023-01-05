#include "SVersionUpdaterWidget.h"
// engine header
#include "FVersionUpdaterManager.h"
#include "HotPatcherCore.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "HttpModule.h"
#include "Misc/FileHelper.h"
#include "Json.h"
#include "VersionUpdaterStyle.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#define LOCTEXT_NAMESPACE "VersionUpdaterWidget"


void SChildModWidget::Construct(const FArguments& InArgs)
{
	CurrentVersion = InArgs._CurrentVersion.Get();
	ModName = InArgs._ModName.Get();
	Description = InArgs._Description.Get();
	RemoteVersion = InArgs._RemoteVersion.Get();
	URL = InArgs._URL.Get();
	UpdateURL = InArgs._UpdateURL.Get();
	bIsBuiltInMod = InArgs._bIsBuiltInMod.Get();
	
	ChildSlot
	[
		SAssignNew(HorizontalBox,SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()->FText
			{
				FString DisplayStr;
				if(bIsBuiltInMod)
				{
					DisplayStr = FString::Printf(TEXT("%s (Built-in Mod)"),*GetModName());
				}
				else
				{
					DisplayStr = FString::Printf(TEXT("%s v%.1f"),*GetModName(),GetCurrentVersion());
				}
				return UKismetTextLibrary::Conv_StringToText(DisplayStr);
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SHyperlink)
			.Text(UKismetTextLibrary::Conv_StringToText(Description))
			.OnNavigate_Lambda([this]()
			{
				if(!URL.IsEmpty())
				{
					FPlatformProcess::LaunchURL(*URL, NULL, NULL);
				}
			})
		]
	];

	// Update Version
	if(!bIsBuiltInMod && RemoteVersion > CurrentVersion)
	{
		FString LaunchURL = UpdateURL;
		if(LaunchURL.IsEmpty()){ LaunchURL = URL;}

		if(!LaunchURL.IsEmpty())
		{
			HorizontalBox->AddSlot()
			.Padding(8.0f, 0.0f, 4.0f, 0.0f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(12)
				.HeightOverride(12)
				[
					SNew(SImage)
					.Image(FVersionUpdaterStyle::GetBrush("Updater.SpawnableIconOverlay"))
				]
			];

			HorizontalBox->AddSlot()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(UKismetTextLibrary::Conv_StringToText(FString::Printf(TEXT("New Version: "))))
			];
			
			HorizontalBox->AddSlot()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHyperlink)
				.Text(UKismetTextLibrary::Conv_StringToText(FString::Printf(TEXT("%.1f"),RemoteVersion)))
				.OnNavigate_Lambda([LaunchURL]()
				{
					FPlatformProcess::LaunchURL(*LaunchURL, NULL, NULL);
				})
			];
		}
	}
}

void SChildModManageWidget::Construct(const FArguments& InArgs)
{
	ToolName = InArgs._ToolName.Get();
	bShowPayInfo = InArgs._bShowPayInfo.Get();
	
	ChildSlot
	[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SAssignNew(ExpanderButton,SButton)
				.ButtonStyle(FEditorStyle::Get(), "NoBorder")
				.HAlign(HAlign_Center)
				.ContentPadding(2)
				.OnClicked_Lambda([this]()->FReply
				{
					EVisibility ChildModVisibility = ChildModBorder->GetVisibility();
					if (ChildModVisibility == EVisibility::Visible)
					{
						ChildModBorder->SetVisibility(EVisibility::Collapsed);
					}
					if (ChildModVisibility == EVisibility::Collapsed)
					{
						ChildModBorder->SetVisibility(EVisibility::Visible);
					}
							
					return FReply::Handled();
				})
				.ToolTipText_Lambda([this]()->FText { return UKismetTextLibrary::Conv_StringToText(FString::Printf(TEXT("%s Mods"),*ToolName)); })
				[
					SNew(SImage)
					.Image_Lambda([this]()->const FSlateBrush*
					{
						if( ExpanderButton->IsHovered() )
						{
							return FEditorStyle::GetBrush("DetailsView.PulldownArrow.Down.Hovered");
						}
						else
						{
							return FEditorStyle::GetBrush("DetailsView.PulldownArrow.Down");
						}
					})
				]
			]
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SAssignNew(ChildModBorder,SBorder)
				.BorderImage(FVersionUpdaterStyle::GetBrush("Updater.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.Padding(20,0,20,0)
					[
						SAssignNew(ChildModBox,SVerticalBox)
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(20,0,20,0)
					.AutoWidth()
					[
						SAssignNew(PayBox,SVerticalBox)
					]
				]
			]
	];
	if(bShowPayInfo)
	{
		PayBox->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(0.f,2.f,0.f,2.f)
		[
			SNew(STextBlock)
			.Text_Lambda([]()->FText{ return UKismetTextLibrary::Conv_StringToText(TEXT("Help me make HotPatcher better.")); })
		];
		PayBox->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(0.f,2.f,0.f,0.f)
		[
			SNew(SBox)
			.HeightOverride(100.f)
			.WidthOverride(100.f)
			[
				SAssignNew(PayImage,SImage)
				.Image(FVersionUpdaterStyle::GetBrush("Updater.WechatPay"))
			]
		];
		
		PayBox->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(0.f,2.f,0.f,0.f)
		[
			SAssignNew(PaymentButtonWrapper,SHorizontalBox)
		];
		AddPayment(TEXT("WechatPay"),"Updater.WechatPay");
		AddPayment(TEXT("AliPay"),"Updater.AliPay");
		SetPaymentFocus(TEXT("WechatPay"));
	}
}

bool SChildModManageWidget::AddChildMod(const FChildModDesc& ModDesc)
{
	bool bStatus = false;
	if(ChildModBox.IsValid())
	{
		ChildModBox.Get()->AddSlot()
		.AutoHeight()
		.VAlign(EVerticalAlignment::VAlign_Center)
		.Padding(0.f,2.f,0.f,2.f)
		[
			SNew(SChildModWidget)
			.ModName(ModDesc.ModName)
			.CurrentVersion(ModDesc.CurrentVersion)
			.RemoteVersion(ModDesc.RemoteVersion)
			.Description(ModDesc.Description)
			.URL(ModDesc.URL)
			.UpdateURL(ModDesc.UpdateURL)
			.bIsBuiltInMod(ModDesc.bIsBuiltInMod)
		];
		bStatus = true;
	}
	return bStatus;
}

bool SChildModManageWidget::AddPayment(const FString& Name, const FString& ImageBrushName)
{
	bool bStatus = false;
	if(PaymentButtonWrapper.IsValid())
	{
		TSharedPtr<STextBlock> TmpPayTextBlock = nullptr;
		SChildModManageWidget::FPaymentInfo PaymentInfo;
		PaymentInfo.Name = Name;
		PaymentInfo.BrushName = ImageBrushName;
		
		PaymentButtonWrapper->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SAssignNew(PaymentInfo.Button,SButton)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.OnClicked_Lambda([this,Name]()->FReply
			{
				SetPaymentFocus(Name);
				return FReply::Handled();
			})
			[
				SAssignNew(PaymentInfo.TextBlock,STextBlock)
				.ColorAndOpacity(FLinearColor::White)
				.Text_Lambda([Name]()->FText{ return UKismetTextLibrary::Conv_StringToText(Name); })
			]
		];
		
		PaymentInfoMap.Add(Name,PaymentInfo);
		bStatus = true;
	}
	return bStatus;
}

void SChildModManageWidget::SetPaymentFocus(const FString& Name)
{
	for(const auto& PaymentInfo:PaymentInfoMap)
	{
		PaymentInfo.Value.TextBlock->SetColorAndOpacity(FLinearColor::White);
	}

	if(PaymentInfoMap.Contains(Name))
	{
		SChildModManageWidget::FPaymentInfo& FocusPaymentInfo = *PaymentInfoMap.Find(Name);
		FocusPaymentInfo.TextBlock->SetColorAndOpacity(FColor::Orange);
		PayImage->SetImage(FVersionUpdaterStyle::GetBrush(*FocusPaymentInfo.BrushName));
	}
}

void SVersionUpdaterWidget::Construct(const FArguments& InArgs)
{
	static bool GBrushInited = false;
	if(!GBrushInited)
	{
		FVersionUpdaterStyle::Initialize(FString::Printf(TEXT("%s_UpdaterStyle"),*GToolName));
		FVersionUpdaterStyle::ReloadTextures();
		GBrushInited = true;
	}
	
	SetToolUpdateInfo(
		InArgs._ToolName.Get().ToString(),
		InArgs._DeveloperName.Get().ToString(),
		InArgs._DeveloperWebsite.Get().ToString(),
		InArgs._UpdateWebsite.Get().ToString()
		);
	CurrentVersion = InArgs._CurrentVersion.Get();
	PatchVersion = InArgs._PatchVersion.Get();
	bool bhPayment = FPaths::FileExists(IPluginManager::Get().FindPlugin(ToolName)->GetBaseDir() / TEXT("Resources/hPayment.txt"));
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage(FVersionUpdaterStyle::GetBrush("Updater.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(40)
						.HeightOverride(40)
						[
							SNew(SImage)
							.Image(FVersionUpdaterStyle::GetBrush("Updater.QuickLaunch"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10,0,10,0)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 4, 2, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SHyperlink)
								.Text_Raw(this,&SVersionUpdaterWidget::GetToolName)
								.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0)
							[
								SNew(SOverlay)
							]
						]
						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(2, 4, 2, 4)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0,0,4,0)
								[
									SNew(STextBlock)
									.Text_Raw(this,&SVersionUpdaterWidget::GetCurrentVersionText)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SAssignNew(UpdateInfoWidget,SHorizontalBox)
									.Visibility(EVisibility::Collapsed)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SBox)
										.WidthOverride(18)
										.HeightOverride(18)
										[
											SNew(SImage)
											.Image(FVersionUpdaterStyle::GetBrush("Updater.SpawnableIconOverlay"))
										]
									]
									+ SHorizontalBox::Slot()
									.Padding(2,0,0,0)
									.AutoWidth()
									[
										SNew(SHyperlink)
										.Text_Raw(this,&SVersionUpdaterWidget::GetLatstVersionText)
										.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite)
									]
								]
							]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SOverlay)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(10,0,10,0)
				.VAlign(VAlign_Center)
				[
					SNew(SHyperlink)
					.Text_Raw(this,&SVersionUpdaterWidget::GetDeveloperDescrible)
					.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite)
				]
			]
		]
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(0,4,0,4)
		.AutoHeight()
		[
			SAssignNew(ChildModManageWidget,SChildModManageWidget)
			.ToolName(ToolName)
			.bShowPayInfo(!bhPayment)
		]
	];
	if(!FVersionUpdaterManager::Get().IsRequestFinished())
	{
		FVersionUpdaterManager::Get().AddOnFinishedCallback([&]()
		{
			OnRemoveVersionFinished();
		});
		
		FVersionUpdaterManager::Get().RequestRemoveVersion(GRemoteVersionFile);
	}
	else
	{
		OnRemoveVersionFinished();
	}
	
	FVersionUpdaterManager::Get().Update();
}

void SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite()
{
	FPlatformProcess::LaunchURL(*UpdateWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite()
{
	FPlatformProcess::LaunchURL(*DeveloperWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::SetToolUpdateInfo(const FString& InToolName, const FString& InDeveloperName,
	const FString& InDeveloperWebsite, const FString& InUpdateWebsite)
{
	ToolName = InToolName;
	DeveloperName = InDeveloperName;
	DeveloperWebsite = InDeveloperWebsite;
	UpdateWebsite = InUpdateWebsite;
}


void SVersionUpdaterWidget::OnRemoveVersionFinished()
{
	FRemteVersionDescrible* ToolRemoteVersion = FVersionUpdaterManager::Get().GetRemoteVersionByName(*GetToolName().ToString());
	if(ToolRemoteVersion)
	{
		int32 RemoteMainVersion = ToolRemoteVersion->Version;
		int32 RemotePatchVersion = ToolRemoteVersion->PatchVersion;
		
		SetToolUpdateInfo(GetToolName().ToString(),ToolRemoteVersion->Author,ToolRemoteVersion->Website,ToolRemoteVersion->URL);
		if(CurrentVersion < RemoteMainVersion || (CurrentVersion == RemoteMainVersion && RemotePatchVersion > PatchVersion))
		{
			UpdateInfoWidget->SetVisibility(EVisibility::Visible);
		}
		RemoteVersion = *ToolRemoteVersion;
		
		auto CreateChildMod = [this](const TMap<FName,FChildModDesc>& ModsDesc,TFunction<bool(const FChildModDesc&)> Condition = [](const FChildModDesc&)->bool{return true;})
		{
			for(const auto& ModDesc:ModsDesc)
			{
				if(FVersionUpdaterManager::Get().ModIsActivteCallback &&
					FVersionUpdaterManager::Get().ModIsActivteCallback(ModDesc.Value.ModName))
				{
					if(Condition && Condition(ModDesc.Value))
					{
						ChildModManageWidget->AddChildMod(ModDesc.Value);
					}
				}
			}
		};
		// Built-in Mods
		CreateChildMod(RemoteVersion.ModsDesc,[](const FChildModDesc& Desc)->bool { return Desc.bIsBuiltInMod;});
		// Not Built-in Mods
		CreateChildMod(RemoteVersion.ModsDesc,[](const FChildModDesc& Desc)->bool { return !Desc.bIsBuiltInMod;});
		// local 3rd mods
		auto GetLocalModsLabme = [this]()->TMap<FName,FChildModDesc>
		{
			TMap<FName,FChildModDesc> LocalModsMap;
			if(FVersionUpdaterManager::Get().RequestLocalRegistedMods)
			{
				TArray<FChildModDesc> LocalMods = FVersionUpdaterManager::Get().RequestLocalRegistedMods();
				for(const auto& LocalMod:LocalMods)
				{
					if(!RemoteVersion.ModsDesc.Contains(*LocalMod.ModName))
					{
						LocalModsMap.Add(*LocalMod.ModName,LocalMod);
					}
				}
			}
			return LocalModsMap;
		};
		CreateChildMod(GetLocalModsLabme());
	}
}

DEFINE_LOG_CATEGORY_STATIC(LogVersionUpdater, All, All);

#undef LOCTEXT_NAMESPACE
