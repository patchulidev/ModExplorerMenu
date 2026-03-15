#pragma once

// Papyrus native function bindings for the "Modex" script class.
// Decoupled from the SKSE C++ messaging API (ModexInterface.h).
// Registration entry point: PapyrusAPI::Register(RE::BSScript::IVirtualMachine*)

namespace Modex::PapyrusAPI
{
	constexpr auto SCRIPT_NAME = "Modex";

	bool Register(RE::BSScript::IVirtualMachine* a_vm);
}
