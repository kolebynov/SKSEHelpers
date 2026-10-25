#pragma once

#include "Hooks/HookInstaller.h"
#include "Hooks/HooksLib.h"
#include "TaskScheduler.h"

#include <RE/Skyrim.h>

#include <chrono>

namespace SkyrimSchedulers
{
	// Wall clock (steady_clock); ticks every frame, including menus and loading screens.
	using MainLoopScheduler = TaskScheduler<>;
	// Sum of PlayerCharacter::Update deltas: world time with slow time applied, stops while the world is paused.
	using PlayerUpdateScheduler = TaskScheduler<std::chrono::duration<double>>;

	struct Schedulers
	{
		MainLoopScheduler& MainLoop;
		PlayerUpdateScheduler& PlayerUpdate;
	};

	// Installs MainLoopUpdateHook (WriteCall: needs SKSE::AllocTrampoline first) and PlayerUpdateHook on the first call;
	// later calls return the same schedulers. Both tick after the original function, on the main thread.
	inline Schedulers Install()
	{
		static MainLoopScheduler mainLoop{ std::chrono::steady_clock::now().time_since_epoch() };
		static PlayerUpdateScheduler playerUpdate{ PlayerUpdateScheduler::Duration{} };
		[[maybe_unused]] static const bool installed = [] {
			Hooks::HookInstaller<Hooks::MainLoopUpdateHook>::Install([](const auto& original, RE::Main* main, float delta) {
				original(main, delta);
				mainLoop.Tick(std::chrono::steady_clock::now().time_since_epoch());
			});
			Hooks::HookInstaller<Hooks::PlayerUpdateHook>::Install(
				[time = PlayerUpdateScheduler::Duration{}](const auto& original, RE::PlayerCharacter* player,
					float delta) mutable {
					original(player, delta);
					time += PlayerUpdateScheduler::Duration{ delta };
					playerUpdate.Tick(time);
				});
			return true;
		}();
		return { mainLoop, playerUpdate };
	}
}
