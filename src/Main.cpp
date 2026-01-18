#include "pch.h"
#include "core/Hooks.h"
#include "core/Graphic.h"
#include "core/InputManager.h"
#include "data/Data.h"
#include "ui/core/UIManager.h"

#include "localization/FontManager.h"
#include "localization/Locale.h"

#include "config/UserData.h"
#include "config/UserConfig.h"
#include "config/ThemeConfig.h"
#include "config/EquipmentConfig.h"
#include "config/BlacklistConfig.h"


namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:  // Skypatcher loads here
			Modex::UserConfig::GetSingleton()->LoadSettings();
			Modex::PrettyLog::Debug("UserConfig Initialized.");

			Modex::Locale::GetSingleton()->Load(false);
			Modex::PrettyLog::Debug("Localization Initialized.");

			Modex::UserData::GetSingleton()->LoadAll();
			Modex::PrettyLog::Debug("UserData Initialized.");

			Modex::ThemeConfig::GetSingleton()->Load(true);
			Modex::PrettyLog::Debug("ThemeConfig Initialized.");

			Modex::BlacklistConfig::GetSingleton()->Load(true);
			Modex::PrettyLog::Debug("BlacklistConfig Initialized.");

			Modex::EquipmentConfig::GetSingleton()->Load();
			Modex::PrettyLog::Debug("EquipmentConfig Initialized.");

			Modex::FontManager::GetSingleton()->Load();
			Modex::PrettyLog::Info("Language & FontManager Initialized.");
			
			Modex::GraphicManager::Init(); // move to open
			Modex::PrettyLog::Info("GraphicManager Initialized.");

			// TODO: Load elsewhere
			Modex::Data::GetSingleton()->Run();
			Modex::PrettyLog::Info("Data Manager Initialized.");
			
			
			Modex::PrettyLog::Info("Done!");
			Modex::PrettyLog::ReportSummary();
			
			// Modex::UIManager::GetSingleton()->ShowBanner();
			// Modex::UIBanner::GetSingleton()->Display();

			break;
		case SKSE::MessagingInterface::kPostLoad:
			break;
		case SKSE::MessagingInterface::kPostPostLoad:
			Hooks::Install();
			RE::UI::GetSingleton()->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(Hooks::IMenuOpenCloseEvent::GetSingleton());
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			break;
		}
	}
	void SetupLog()
	{
		auto logsFolder = SKSE::log::log_directory();

		if (!logsFolder) {
			SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
		}

		auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
		auto logFilePath = *logsFolder / std::format("{}.log", pluginName);

		auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
		auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
		spdlog::set_default_logger(std::move(loggerPtr));
		spdlog::set_level(spdlog::level::debug); // TODO: Move back to settings
		spdlog::flush_on(spdlog::level::debug);
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	// while (!IsDebuggerPresent()) Sleep(1000);
	
	SKSE::Init(a_skse);

	SetupLog();

	Modex::PrettyLog::Info("Hello World!");

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}

#ifdef ENABLE_SKYRIM_VR
	// code
#endif

#ifdef ENABLE_SKYRIM_AE
	// code
#endif
