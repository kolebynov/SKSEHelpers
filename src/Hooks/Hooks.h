#pragma once

#include "TraitUtils.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// MinHook.h pulls in <Windows.h>; CommonLib headers must be parsed before it.
#include <MinHook.h>

#include <concepts>
#include <cstdint>
#include <exception>

namespace Hooks
{
	enum class HookType
	{
		WriteCall = 1,
		Detour,
	};

	template <typename T>
	concept HookDefinition = FunctionTraits<typename T::FuncDefinition>::IsFunction && requires {
		{ T::Id } -> std::same_as<const REL::RelocationID&>;
		{ T::Offset } -> std::same_as<const REL::VariantOffset&>;
		{ T::Type } -> std::same_as<const HookType&>;
	};

	template <HookDefinition THook>
	REL::Relocation<typename THook::FuncDefinition> Install(typename THook::FuncDefinition* func)
	{
		auto targetAddress = REL::Relocation<>(THook::Id, THook::Offset).address();
		if constexpr (THook::Type == HookType::WriteCall) {
			return REL::Relocation<typename THook::FuncDefinition>(SKSE::GetTrampoline().write_call<5>(targetAddress, func));
		} else if constexpr (THook::Type == HookType::Detour) {
			auto status = MH_Initialize();
			if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
				SKSE::log::critical("MinHook init failed");
				std::terminate();
			}

			// MinHook only records the detour address and never calls it, so pass it as a plain address.
			const auto detour = reinterpret_cast<LPVOID>(reinterpret_cast<std::uintptr_t>(func));
			LPVOID origAddress;
			if (MH_CreateHook(reinterpret_cast<LPVOID>(targetAddress), detour, &origAddress) != MH_OK) {
				SKSE::log::critical("Failed to create hook");
				std::terminate();
			}

			if (MH_EnableHook(reinterpret_cast<LPVOID>(targetAddress)) != MH_OK) {
				SKSE::log::critical("Failed to enable hook");
				std::terminate();
			}

			return REL::Relocation<typename THook::FuncDefinition>(reinterpret_cast<uintptr_t>(origAddress));
		} else {
			static_assert(!sizeof(THook::Type), "Unsupported hook type");
			return 0;
		}
	}
}
