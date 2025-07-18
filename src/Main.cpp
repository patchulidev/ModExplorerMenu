#define DLLEXPORT __declspec(dllexport)

#include "include/C/Console.h"
#include "include/D/Data.h"
#include "include/F/Frame.h"
#include "include/H/Hooks.h"
#include "include/I/InputManager.h"
#include "include/L/Language.h"
#include "include/P/Persistent.h"
#include "include/S/Settings.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:  // Skypatcher loads here
			Modex::PersistentData::GetSingleton()->LoadAllKits();
			Modex::PersistentData::GetSingleton()->LoadBlacklist();
			Modex::PersistentData::GetSingleton()->LoadUserdata();
			PrettyLog::Info("Finished Loading Persistent Userdata!");

			Modex::Language::GetSingleton()->BuildLanguageList();
			Modex::FontManager::GetSingleton()->BuildFontLibrary();
			PrettyLog::Info("Finished Loading Language & Font Libraries!");

			Modex::Settings::GetSingleton()->LoadSettings(Modex::Settings::ini_main_path);
			Modex::Settings::GetSingleton()->LoadUserFontSetting();
			Modex::Menu::GetSingleton()->RefreshFont();
			PrettyLog::Info("Finished Loading User Font Settings!");

			Modex::GraphicManager::Init();
			PrettyLog::Info("Finished GraphicManager Initialization!");

			Modex::Data::GetSingleton()->Run();
			PrettyLog::Info("Finished Data Initialization!");

			Modex::InputManager::GetSingleton()->Init();
			PrettyLog::Info("Finished Setting Up InputManager!");
			
			Modex::Frame::GetSingleton()->Install();
			PrettyLog::Info("Done!");

			PrettyLog::ReportSummary();
			break;
		case SKSE::MessagingInterface::kPostLoad:
			break;
		case SKSE::MessagingInterface::kPostPostLoad:
			Hooks::Install();
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
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SetupLog();

	SKSE::Init(a_skse);

	// Install SKSE hooks.
	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}

	return true;
}