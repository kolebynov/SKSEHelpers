#pragma once

#include "CallPipeline.h"
#include "Hooks/Hooks.h"

#include <SKSE/SKSE.h>

#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeinfo>

namespace Hooks
{
	namespace Detail
	{
		template <HookDefinition THook, class THandler, class TArgs>
		struct HookFunc
		{
			static_assert(false, "Check THandler, it seems it doesn't satisfy hook parameters");
		};

		template <HookDefinition THook, class THandler, class... TArgs>
			requires(
				std::is_invocable_v<std::decay_t<THandler>&, const REL::Relocation<typename THook::FuncDefinition>&, TArgs...>)
		struct HookFunc<THook, THandler, std::tuple<TArgs...>>
		{
		public:
			static typename FunctionTraits<typename THook::FuncDefinition>::ReturnType Invoke(TArgs... args)
			{
				return std::invoke(*CallPipeline, std::forward<TArgs>(args)...);
			}

			static inline std::optional<CallPipeline<std::decay_t<THandler>, REL::Relocation<typename THook::FuncDefinition>>>
				CallPipeline;
		};
	}

	template <HookDefinition THook>
	class HookInstaller
	{
	public:
		template <class THandler>
		static void Install(THandler&& handler)
		{
			using HookFunc =
				Detail::HookFunc<THook, THandler, typename FunctionTraits<typename THook::FuncDefinition>::Parameters>;
			if (HookFunc::CallPipeline.has_value()) {
				const auto message =
					std::format("Hook {} is already installed with handler {}", typeid(THook).name(), typeid(THandler).name());
				SKSE::log::error("{}", message);
				throw std::runtime_error(message);
			}

			auto originalFunc = Hooks::Install<THook>(HookFunc::Invoke);
			HookFunc::CallPipeline.emplace(StartCallPipeline(originalFunc).Wrap(std::forward<THandler>(handler)));
		}
	};
}
