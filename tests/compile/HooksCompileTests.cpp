#include "Hooks/HookInstaller.h"
#include "Hooks/Hooks.h"

#include <cstddef>

namespace
{
	struct WriteCallHook
	{
		using FuncDefinition = int(int);
		static constexpr auto Id = REL::RelocationID(1, 2);
		static constexpr auto Offset = REL::VariantOffset(0x10, 0x20, 0x30);
		static constexpr auto Type = Hooks::HookType::WriteCall;
	};

	struct DetourHook
	{
		using FuncDefinition = void();
		static constexpr auto Id = REL::RelocationID(1, 2);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = Hooks::HookType::Detour;
	};

	struct VFuncHook
	{
		using FuncDefinition = bool(void*, float);
		static constexpr auto VTable = REL::VariantID(1, 2, 0x3);
		static constexpr std::size_t Index = 5;
		static constexpr auto Type = Hooks::HookType::VFunc;
	};

	// Address fields with the vfunc type.
	struct VFuncTypeWithAddressHook
	{
		using FuncDefinition = void();
		static constexpr auto Id = REL::RelocationID(1, 2);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = Hooks::HookType::VFunc;
	};

	// Vtable fields with an address type.
	struct AddressTypeWithVTableHook
	{
		using FuncDefinition = void();
		static constexpr auto VTable = REL::VariantID(1, 2, 0x3);
		static constexpr std::size_t Index = 5;
		static constexpr auto Type = Hooks::HookType::WriteCall;
	};

	// FuncDefinition must be a function type, not a function pointer.
	struct FunctionPointerHook
	{
		using FuncDefinition = void (*)();
		static constexpr auto Id = REL::RelocationID(1, 2);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = Hooks::HookType::WriteCall;
	};

	struct WrongIndexTypeHook
	{
		using FuncDefinition = void();
		static constexpr auto VTable = REL::VariantID(1, 2, 0x3);
		static constexpr int Index = 5;
		static constexpr auto Type = Hooks::HookType::VFunc;
	};

	struct MissingOffsetHook
	{
		using FuncDefinition = void();
		static constexpr auto Id = REL::RelocationID(1, 2);
		static constexpr auto Type = Hooks::HookType::WriteCall;
	};
}

static_assert(Hooks::AddressHookDefinition<WriteCallHook>);
static_assert(Hooks::AddressHookDefinition<DetourHook>);
static_assert(!Hooks::VFuncHookDefinition<WriteCallHook>);
static_assert(Hooks::VFuncHookDefinition<VFuncHook>);
static_assert(!Hooks::AddressHookDefinition<VFuncHook>);

static_assert(Hooks::HookDefinition<WriteCallHook>);
static_assert(Hooks::HookDefinition<DetourHook>);
static_assert(Hooks::HookDefinition<VFuncHook>);

static_assert(!Hooks::HookDefinition<VFuncTypeWithAddressHook>);
static_assert(!Hooks::HookDefinition<AddressTypeWithVTableHook>);
static_assert(!Hooks::HookDefinition<FunctionPointerHook>);
static_assert(!Hooks::HookDefinition<WrongIndexTypeHook>);
static_assert(!Hooks::HookDefinition<MissingOffsetHook>);
static_assert(!Hooks::HookDefinition<int>);

// Instantiates Install with a stateful (mutable) handler; never called.
void InstallMutableHandler()
{
	Hooks::HookInstaller<WriteCallHook>::Install(
		[calls = 0](const REL::Relocation<int(int)>& original, int value) mutable { return original(value) + ++calls; });
}
