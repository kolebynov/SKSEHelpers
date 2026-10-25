#pragma once

#include "Hooks.h"

#include <RE/Skyrim.h>

#include <cstddef>
#include <cstdint>

namespace Hooks
{
	struct MainLoopUpdateHook
	{
		using FuncDefinition = void(RE::Main*, float);
		static constexpr auto Id = REL::RelocationID(35551, 36544);
		static constexpr auto Offset = REL::VariantOffset(0x11F, 0x160, 0x11F);
		static constexpr auto Type = HookType::WriteCall;
	};

	struct D3DInitHook
	{
		using FuncDefinition = void();
		static constexpr auto Id = REL::RelocationID(75595, 77226);
		static constexpr auto Offset = REL::VariantOffset(0x50, 0x2BC, 0x00);  // VR unknown
		static constexpr auto Type = HookType::WriteCall;
	};

	struct D3DPresentHook
	{
		using FuncDefinition = void(std::uint32_t);
		static constexpr auto Id = REL::RelocationID(75461, 77246);
		static constexpr auto Offset = REL::VariantOffset(0x9, 0x9, 0x15);
		static constexpr auto Type = HookType::WriteCall;
	};

	struct RendererBegin
	{
		using FuncDefinition = void(RE::BSGraphics::Renderer*, std::uint32_t);
		static constexpr auto Id = REL::RelocationID(75460, 77245);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Hooks below come from SkyrimDungeons and are verified on SE 1.5.97 only:
	// AE ids and AE/VR offsets are unknown and left 0.

	// --- Call sites (HookType::WriteCall) ---

	// Game update call inside 35565 (SkyrimDungeons' per-frame game tick).
	struct GameUpdateHook
	{
		using FuncDefinition = void();
		static constexpr auto Id = REL::RelocationID(35565, 0);
		static constexpr auto Offset = REL::VariantOffset(0x748, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// PlayerCharacter::ActivatePickRef -> TESObjectREFR::ActivateRef (player's Activate key).
	struct ActivateRefHook
	{
		using FuncDefinition = bool(RE::TESObjectREFR*, RE::TESObjectREFR*, std::uint8_t, RE::TESBoundObject*, std::int32_t,
			bool);
		static constexpr auto Id = REL::RelocationID(39471, 0);
		static constexpr auto Offset = REL::VariantOffset(0x135, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// ActivateHandler::ProcessButton hold branch -> PlayerCharacter::StartGrabObject.
	struct StartGrabObjectHook
	{
		using FuncDefinition = void(RE::PlayerCharacter*);
		static constexpr auto Id = REL::RelocationID(41346, 0);
		static constexpr auto Offset = REL::VariantOffset(0xFD, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// TESObjectCONT::Activate -> container open/close animation driver.
	struct ContainerOpenCloseAnimationHook
	{
		using FuncDefinition = void(RE::TESObjectREFR*, RE::TESObjectREFR*);
		static constexpr auto Id = REL::RelocationID(17485, 0);
		static constexpr auto Offset = REL::VariantOffset(0x21C, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Animation graph update of an IAnimationGraphManagerHolder.
	struct UpdateAnimationGraphHook
	{
		using FuncDefinition = bool(RE::IAnimationGraphManagerHolder*, RE::BSAnimationUpdateData*);
		static constexpr auto Id = REL::RelocationID(19729, 0);
		static constexpr auto Offset = REL::VariantOffset(0x96, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Player per-frame camera update (39377) -> PlayerCamera::Update.
	struct PlayerCameraUpdateHook
	{
		using FuncDefinition = void(RE::PlayerCamera*);
		static constexpr auto Id = REL::RelocationID(39377, 0);
		static constexpr auto Offset = REL::VariantOffset(0x99, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Melee get-hit reaction (37633) -> block recoil (aggressor, largeRecoil; always false here).
	struct BlockRecoilHook
	{
		using FuncDefinition = void(RE::Actor*, bool);
		static constexpr auto Id = REL::RelocationID(37633, 0);
		static constexpr auto Offset = REL::VariantOffset(0x8B0, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Melee get-hit reaction (37633) -> block stamina cost of the hit.
	struct BlockStaminaCostHook
	{
		using FuncDefinition = float(RE::HitData*);
		static constexpr auto Id = REL::RelocationID(37633, 0);
		static constexpr auto Offset = REL::VariantOffset(0x8D4, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// HitData damage mitigation (42842): blocked fraction; damage *= (1 - result).
	struct BlockPercentHook
	{
		using FuncDefinition = float(float, float, float, char, char, char);
		static constexpr auto Id = REL::RelocationID(42842, 0);
		static constexpr auto Offset = REL::VariantOffset(0x437, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// HitData damage mitigation (42842): is the target blocking.
	struct BlockingCheckHook
	{
		using FuncDefinition = bool(RE::Actor*);
		static constexpr auto Id = REL::RelocationID(42842, 0);
		static constexpr auto Offset = REL::VariantOffset(0x231, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// HitData damage mitigation (42842) -> Character::GetAttackAngle (block cone test).
	struct BlockAttackAngleHook
	{
		using FuncDefinition = bool(void*, void*, void*, void*, void*, float, void*, char);
		static constexpr auto Id = REL::RelocationID(42842, 0);
		static constexpr auto Offset = REL::VariantOffset(0x2EA, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// HitData::Populate (42832) -> damage mitigation step.
	struct HitDataMitigationHook
	{
		using FuncDefinition = void(RE::HitData*, char);
		static constexpr auto Id = REL::RelocationID(42832, 0);
		static constexpr auto Offset = REL::VariantOffset(0x37C, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// HitData::PopulateFromProjectile (42833) -> damage mitigation step.
	struct ProjectileHitDataMitigationHook
	{
		using FuncDefinition = void(RE::HitData*, char);
		static constexpr auto Id = REL::RelocationID(42833, 0);
		static constexpr auto Offset = REL::VariantOffset(0x13E, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Melee swing (37650) -> Character::HitData for the picked target (attacker, target, projectile, leftHand).
	struct PrimaryMeleeHitHook
	{
		using FuncDefinition = void(RE::Actor*, RE::Actor*, std::uintptr_t, char);
		static constexpr auto Id = REL::RelocationID(37650, 0);
		static constexpr auto Offset = REL::VariantOffset(0x38B, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Sweep loop (37689) -> Character::HitData for each extra actor.
	struct SweepMeleeHitHook
	{
		using FuncDefinition = void(RE::Actor*, RE::Actor*, std::uintptr_t, char);
		static constexpr auto Id = REL::RelocationID(37689, 0);
		static constexpr auto Offset = REL::VariantOffset(0xDD, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Melee swing (37650) -> BGSEntryPoint::HandleEntryPoint(kSetSweepAttack, attacker, weapon, &result).
	struct SweepAttackEntryPointHook
	{
		using FuncDefinition = void(std::uint32_t, RE::Actor*, RE::TESBoundObject*, float*);
		static constexpr auto Id = REL::RelocationID(37650, 0);
		static constexpr auto Offset = REL::VariantOffset(0x343, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// NPC attack check (38047) -> ActorValueOwner::get_attack_cost.
	struct AttackCostCheckHook
	{
		using FuncDefinition = float(RE::ActorValueOwner*, RE::BGSAttackData*);
		static constexpr auto Id = REL::RelocationID(38047, 0);
		static constexpr auto Offset = REL::VariantOffset(0xBB, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Melee swing (37650) -> ActorValueOwner::get_attack_cost.
	struct AttackCostSwingHook
	{
		using FuncDefinition = float(RE::ActorValueOwner*, RE::BGSAttackData*);
		static constexpr auto Id = REL::RelocationID(37650, 0);
		static constexpr auto Offset = REL::VariantOffset(0x16E, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// ActorEquipManager equip implementation calls; last argument is the equip params (first field ExtraDataList*).
	struct EquipObjectHook
	{
		using FuncDefinition = void(RE::ActorEquipManager*, RE::Actor*, RE::TESBoundObject*, void*);
		static constexpr auto Id = REL::RelocationID(37938, 0);
		static constexpr auto Offset = REL::VariantOffset(0xE5, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	struct EquipEntryHook
	{
		using FuncDefinition = void(RE::ActorEquipManager*, RE::Actor*, RE::TESBoundObject*, void*);
		static constexpr auto Id = REL::RelocationID(37937, 0);
		static constexpr auto Offset = REL::VariantOffset(0xBC, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// Call sites of ActorEquipManager::ReEquipRememberedHandItem (37957); one type per site.
	template <std::uint64_t SeId, std::ptrdiff_t SeOffset>
	struct ReEquipHandItemHook
	{
		using FuncDefinition = void(RE::ActorEquipManager*, RE::Actor*, RE::TESBoundObject*, RE::BGSEquipSlot*);
		static constexpr auto Id = REL::RelocationID(SeId, 0);
		static constexpr auto Offset = REL::VariantOffset(SeOffset, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	using ReEquipHandItemHook1 = ReEquipHandItemHook<37973, 0x294>;
	using ReEquipHandItemHook2 = ReEquipHandItemHook<37974, 0x297>;
	using ReEquipHandItemHook3 = ReEquipHandItemHook<37975, 0x2B8>;
	using ReEquipHandItemHook4 = ReEquipHandItemHook<37978, 0x201>;
	using ReEquipHandItemHook5 = ReEquipHandItemHook<37978, 0x217>;
	using ReEquipHandItemHook6 = ReEquipHandItemHook<37979, 0x204>;
	using ReEquipHandItemHook7 = ReEquipHandItemHook<37979, 0x21A>;
	using ReEquipHandItemHook8 = ReEquipHandItemHook<37980, 0x233>;
	using ReEquipHandItemHook9 = ReEquipHandItemHook<37980, 0x249>;

	// Actor regen (37831) -> per-frame actor value ticks.
	struct HealthRegenTickHook
	{
		using FuncDefinition = void(RE::Actor*, float);
		static constexpr auto Id = REL::RelocationID(37831, 0);
		static constexpr auto Offset = REL::VariantOffset(0x183, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	struct StaminaRegenTickHook
	{
		using FuncDefinition = void(RE::Actor*, float);
		static constexpr auto Id = REL::RelocationID(37831, 0);
		static constexpr auto Offset = REL::VariantOffset(0x18E, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	struct MagickaRegenTickHook
	{
		using FuncDefinition = void(RE::Actor*, float);
		static constexpr auto Id = REL::RelocationID(37831, 0);
		static constexpr auto Offset = REL::VariantOffset(0x1A5, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// BGSSaveLoadGame "Player Location" (34745) -> LookupFormByID for the saved cell/worldspace.
	struct SavedCellLookupHook
	{
		using FuncDefinition = RE::TESForm*(RE::FormID);
		static constexpr auto Id = REL::RelocationID(34745, 0);
		static constexpr auto Offset = REL::VariantOffset(0xA9, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// BGSSaveLoadGame "Player Location" (34745) -> LookupFormByID for the safe marker.
	struct SafeMarkerLookupHook
	{
		using FuncDefinition = RE::TESForm*(RE::FormID);
		static constexpr auto Id = REL::RelocationID(34745, 0);
		static constexpr auto Offset = REL::VariantOffset(0x48D, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// PlayerCharacter::LoadGame (39463) -> ControlMap::SetControlsState with masks read from the save.
	struct LoadedControlsStateHook
	{
		using FuncDefinition = void(RE::ControlMap*, std::uint32_t, std::uint32_t);
		static constexpr auto Id = REL::RelocationID(39463, 0);
		static constexpr auto Offset = REL::VariantOffset(0xB3C, 0, 0);
		static constexpr auto Type = HookType::WriteCall;
	};

	// --- Function entries (HookType::Detour) ---

	// Actor::StartCombat.
	struct StartCombatHook
	{
		using FuncDefinition = bool(RE::Actor*, RE::Actor*, RE::CombatGroup*);
		static constexpr auto Id = REL::RelocationID(37608, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Writes animation pose transforms (const float* pose, &BShkbAnimationGraph::boneNodes, count) into bone nodes.
	struct PoseWriteHook
	{
		using FuncDefinition = void(const float*, RE::BSTArray<RE::BShkbAnimationGraph::BoneNodeEntry>*, std::int32_t);
		static constexpr auto Id = REL::RelocationID(62933, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Player attack angle check.
	struct PlayerAttackAngleHook
	{
		using FuncDefinition = bool(RE::Actor*, RE::TESObjectREFR*, const RE::NiPoint3*, const RE::NiPoint3*, RE::BGSAttackData*,
			float, float*, bool);
		static constexpr auto Id = REL::RelocationID(46033, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// ActorValueOwner weapon speed.
	struct GetWeaponSpeedHook
	{
		using FuncDefinition = float(RE::ActorValueOwner*, RE::TESObjectWEAP*, char);
		static constexpr auto Id = REL::RelocationID(25851, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Attack action gate.
	struct AttackGateHook
	{
		using FuncDefinition = std::uint64_t(void*, RE::TESActionData*, void*, void*);
		static constexpr auto Id = REL::RelocationID(38047, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// PlayerControls action dispatch.
	struct PlayerDoActionHook
	{
		using FuncDefinition = std::uint64_t(RE::PlayerControls*, std::uint32_t, std::uint32_t);
		static constexpr auto Id = REL::RelocationID(41271, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Melee get-hit reaction.
	struct HitReactionHook
	{
		using FuncDefinition = void(RE::Actor*, RE::HitData*);
		static constexpr auto Id = REL::RelocationID(37633, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Actor::ModActorValue (this, ACTOR_VALUE_MODIFIER, av, value, attacker).
	struct ModActorValueHook
	{
		using FuncDefinition = void(RE::Actor*, std::uint32_t, RE::ActorValue, float, RE::Actor*);
		static constexpr auto Id = REL::RelocationID(37523, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// PlayerSkills::AdvanceSkill.
	struct AdvanceSkillHook
	{
		using FuncDefinition = void(RE::PlayerCharacter::PlayerSkills*, RE::ActorValue, float, void*, std::uint32_t, bool, bool);
		static constexpr auto Id = REL::RelocationID(40554, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// UIMessageQueue::AddMessage.
	struct AddUIMessageHook
	{
		using FuncDefinition = void(RE::UIMessageQueue*, const RE::BSFixedString&, RE::UI_MESSAGE_TYPE, RE::IUIMessageData*);
		static constexpr auto Id = REL::RelocationID(13530, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// Favorite toggles run from item menus' ProcessMessage; argument is the menu.
	struct InventoryFavoriteHook
	{
		using FuncDefinition = void(void*);
		static constexpr auto Id = REL::RelocationID(50990, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	struct ContainerFavoriteHook
	{
		using FuncDefinition = void(void*);
		static constexpr auto Id = REL::RelocationID(50221, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	struct MagicFavoriteHook
	{
		using FuncDefinition = void(void*);
		static constexpr auto Id = REL::RelocationID(51165, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// TESForm::GetWeight.
	struct GetWeightHook
	{
		using FuncDefinition = float(RE::TESForm*);
		static constexpr auto Id = REL::RelocationID(14809, 0);
		static constexpr auto Offset = REL::VariantOffset(0, 0, 0);
		static constexpr auto Type = HookType::Detour;
	};

	// --- Virtual functions (HookType::VFunc) ---
	// Vtables come from CommonLib and cover SE/AE/VR; slot indices are verified on SE 1.5.97 only.

	// ActivateHandler::ProcessButton.
	struct ActivateHandlerProcessButtonHook
	{
		using FuncDefinition = void(RE::ActivateHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
		static constexpr auto VTable = RE::VTABLE_ActivateHandler[0];
		static constexpr std::size_t Index = 0x04;
		static constexpr auto Type = HookType::VFunc;
	};

	// Actor::SetObjectReference on NPCs (base object assigned).
	struct CharacterSetObjectReferenceHook
	{
		using FuncDefinition = void(RE::Actor*, RE::TESBoundObject*);
		static constexpr auto VTable = RE::VTABLE_Character[0];
		static constexpr std::size_t Index = 0x84;
		static constexpr auto Type = HookType::VFunc;
	};

	// Actor::ShouldRespondToActorCollision on NPCs.
	struct CharacterShouldRespondToActorCollisionHook
	{
		using FuncDefinition = bool(RE::Actor*, const RE::MovementMessageActorCollision&, const RE::ActorHandlePtr&);
		static constexpr auto VTable = RE::VTABLE_Character[0];
		static constexpr std::size_t Index = 0x126;
		static constexpr auto Type = HookType::VFunc;
	};

	// Actor::HandleHealthDamage (victim, attacker, damage) on NPCs.
	struct CharacterHandleHealthDamageHook
	{
		using FuncDefinition = void(RE::Actor*, RE::Actor*, float);
		static constexpr auto VTable = RE::VTABLE_Character[0];
		static constexpr std::size_t Index = 0x104;
		static constexpr auto Type = HookType::VFunc;
	};

	// Actor::HandleHealthDamage (victim, attacker, damage) on the player.
	struct PlayerHandleHealthDamageHook
	{
		using FuncDefinition = void(RE::Actor*, RE::Actor*, float);
		static constexpr auto VTable = RE::VTABLE_PlayerCharacter[0];
		static constexpr std::size_t Index = 0x104;
		static constexpr auto Type = HookType::VFunc;
	};

	// PlayerCharacter::UseAmmo; returns the ammo left after the shot.
	struct PlayerUseAmmoHook
	{
		using FuncDefinition = std::uint32_t(RE::PlayerCharacter*, std::uint32_t);
		static constexpr auto VTable = RE::VTABLE_PlayerCharacter[0];
		static constexpr std::size_t Index = 0xD2;
		static constexpr auto Type = HookType::VFunc;
	};

	// IAnimationGraphManagerHolder::NotifyAnimationGraph of the player.
	struct PlayerNotifyAnimationGraphHook
	{
		using FuncDefinition = bool(RE::IAnimationGraphManagerHolder*, const RE::BSFixedString&);
		static constexpr auto VTable = RE::VTABLE_PlayerCharacter[3];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// IAnimationGraphManagerHolder::NotifyAnimationGraph of NPCs.
	struct CharacterNotifyAnimationGraphHook
	{
		using FuncDefinition = bool(RE::IAnimationGraphManagerHolder*, const RE::BSFixedString&);
		static constexpr auto VTable = RE::VTABLE_Character[3];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// MagicTarget::CheckResistance on NPCs; returns the multiplier applied to the effect (1.0 = nothing resisted).
	struct CharacterCheckResistanceHook
	{
		using FuncDefinition = float(RE::MagicTarget*, RE::MagicItem*, RE::Effect*, RE::TESBoundObject*);
		static constexpr auto VTable = RE::VTABLE_Character[4];
		static constexpr std::size_t Index = 0x0A;
		static constexpr auto Type = HookType::VFunc;
	};

	// MagicTarget::CheckResistance on the player.
	struct PlayerCheckResistanceHook
	{
		using FuncDefinition = float(RE::MagicTarget*, RE::MagicItem*, RE::Effect*, RE::TESBoundObject*);
		static constexpr auto VTable = RE::VTABLE_PlayerCharacter[4];
		static constexpr std::size_t Index = 0x0A;
		static constexpr auto Type = HookType::VFunc;
	};

	// ThirdPersonState::Update.
	struct ThirdPersonStateUpdateHook
	{
		using FuncDefinition = void(RE::ThirdPersonState*, RE::BSTSmartPointer<RE::TESCameraState>&);
		static constexpr auto VTable = RE::VTABLE_ThirdPersonState[0];
		static constexpr std::size_t Index = 0x03;
		static constexpr auto Type = HookType::VFunc;
	};

	// ThirdPersonState::GetRotation.
	struct ThirdPersonStateGetRotationHook
	{
		using FuncDefinition = void(RE::ThirdPersonState*, RE::NiQuaternion&);
		static constexpr auto VTable = RE::VTABLE_ThirdPersonState[0];
		static constexpr std::size_t Index = 0x04;
		static constexpr auto Type = HookType::VFunc;
	};

	// ThirdPersonState::GetTranslation.
	struct ThirdPersonStateGetTranslationHook
	{
		using FuncDefinition = void(RE::ThirdPersonState*, RE::NiPoint3&);
		static constexpr auto VTable = RE::VTABLE_ThirdPersonState[0];
		static constexpr std::size_t Index = 0x05;
		static constexpr auto Type = HookType::VFunc;
	};

	// ThirdPersonState::HandleLookInput.
	struct ThirdPersonStateHandleLookInputHook
	{
		using FuncDefinition = void(RE::ThirdPersonState*, const RE::NiPoint2&);
		static constexpr auto VTable = RE::VTABLE_ThirdPersonState[0];
		static constexpr std::size_t Index = 0x0F;
		static constexpr auto Type = HookType::VFunc;
	};

	// ThirdPersonState's PlayerInputHandler::ProcessButton (zoom, Toggle POV).
	struct ThirdPersonStateProcessButtonHook
	{
		using FuncDefinition = void(RE::PlayerInputHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
		static constexpr auto VTable = RE::VTABLE_ThirdPersonState[1];
		static constexpr std::size_t Index = 0x04;
		static constexpr auto Type = HookType::VFunc;
	};

	// TogglePOVHandler::ProcessButton.
	struct TogglePOVHandlerProcessButtonHook
	{
		using FuncDefinition = void(RE::TogglePOVHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
		static constexpr auto VTable = RE::VTABLE_TogglePOVHandler[0];
		static constexpr std::size_t Index = 0x04;
		static constexpr auto Type = HookType::VFunc;
	};

	// AttackStopHandler::ExecuteHandler; clears meleeAttackState for every actor.
	struct AttackStopHandlerHook
	{
		using FuncDefinition = bool(RE::IHandlerFunctor<RE::Actor, RE::BSFixedString>*, RE::Actor&, const RE::BSFixedString&);
		static constexpr auto VTable = RE::VTABLE_AttackStopHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// SneakHandler::CanProcess.
	struct SneakHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::SneakHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_SneakHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// JumpHandler::CanProcess.
	struct JumpHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::JumpHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_JumpHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// SprintHandler::CanProcess.
	struct SprintHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::SprintHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_SprintHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// AttackBlockHandler::CanProcess.
	struct AttackBlockHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::AttackBlockHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_AttackBlockHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// MenuOpenHandler::CanProcess.
	struct MenuOpenHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::MenuOpenHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_MenuOpenHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// FavoritesHandler::CanProcess.
	struct FavoritesHandlerCanProcessHook
	{
		using FuncDefinition = bool(RE::FavoritesHandler*, RE::InputEvent*);
		static constexpr auto VTable = RE::VTABLE_FavoritesHandler[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// ValueModifierEffect::ModifyActorValue (target, value, av); each effect class has its own vtable.
	struct ValueModifierEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_ValueModifierEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	struct PeakValueModifierEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_PeakValueModifierEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	struct DualValueModifierEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_DualValueModifierEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	struct AccumulatingValueModifierEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_AccumulatingValueModifierEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	struct TargetValueModifierEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_TargetValueModifierEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	struct AbsorbEffectModifyActorValueHook
	{
		using FuncDefinition = void(RE::ValueModifierEffect*, RE::Actor*, float, RE::ActorValue);
		static constexpr auto VTable = RE::VTABLE_AbsorbEffect[0];
		static constexpr std::size_t Index = 0x20;
		static constexpr auto Type = HookType::VFunc;
	};

	// ModelReferenceEffect::UpdatePosition.
	struct ModelReferenceEffectUpdatePositionHook
	{
		using FuncDefinition = void(RE::ModelReferenceEffect*);
		static constexpr auto VTable = RE::VTABLE_ModelReferenceEffect[0];
		static constexpr std::size_t Index = 0x3B;
		static constexpr auto Type = HookType::VFunc;
	};

	// HUDMenu::AdvanceMovie.
	struct HUDMenuAdvanceMovieHook
	{
		using FuncDefinition = void(RE::HUDMenu*, float, std::uint32_t);
		static constexpr auto VTable = RE::VTABLE_HUDMenu[0];
		static constexpr std::size_t Index = 0x05;
		static constexpr auto Type = HookType::VFunc;
	};

	// MagicMenu::AdvanceMovie.
	struct MagicMenuAdvanceMovieHook
	{
		using FuncDefinition = void(RE::MagicMenu*, float, std::uint32_t);
		static constexpr auto VTable = RE::VTABLE_MagicMenu[0];
		static constexpr std::size_t Index = 0x05;
		static constexpr auto Type = HookType::VFunc;
	};

	// BGSImpactManager's BSTEventSink<BGSCombatImpactEvent>::ProcessEvent (impact sounds of a hit).
	struct CombatImpactProcessEventHook
	{
		using FuncDefinition = RE::BSEventNotifyControl(RE::BSTEventSink<RE::BGSCombatImpactEvent>*,
			const RE::BGSCombatImpactEvent*, RE::BSTEventSource<RE::BGSCombatImpactEvent>*);
		static constexpr auto VTable = RE::VTABLE_BGSImpactManager[1];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// NiParticleSystem::Do_UpdateSystem (runs the modifiers and rebuilds modelBound).
	struct ParticleSystemUpdateHook
	{
		using FuncDefinition = void(RE::NiParticleSystem*, float);
		static constexpr auto VTable = RE::VTABLE_NiParticleSystem[0];
		static constexpr std::size_t Index = 0x39;
		static constexpr auto Type = HookType::VFunc;
	};

	// Journal_SystemTab::Accept; registers the QuitToMainMenu / QuitToDesktop callbacks.
	struct JournalSystemTabAcceptHook
	{
		using FuncDefinition = void(RE::Journal_SystemTab*, RE::FxDelegateHandler::CallbackProcessor*);
		static constexpr auto VTable = RE::VTABLE_Journal_SystemTab[0];
		static constexpr std::size_t Index = 0x01;
		static constexpr auto Type = HookType::VFunc;
	};

	// IMenu::PostDisplay of LoadingMenu.
	struct LoadingMenuPostDisplayHook
	{
		using FuncDefinition = void(RE::LoadingMenu*);
		static constexpr auto VTable = RE::VTABLE_LoadingMenu[0];
		static constexpr std::size_t Index = 0x06;
		static constexpr auto Type = HookType::VFunc;
	};

	// IMenu::PostDisplay of MistMenu.
	struct MistMenuPostDisplayHook
	{
		using FuncDefinition = void(RE::MistMenu*);
		static constexpr auto VTable = RE::VTABLE_MistMenu[0];
		static constexpr std::size_t Index = 0x06;
		static constexpr auto Type = HookType::VFunc;
	};

	// IMenu::PostDisplay of LoadWaitSpinner.
	struct LoadWaitSpinnerPostDisplayHook
	{
		using FuncDefinition = void(RE::LoadWaitSpinner*);
		static constexpr auto VTable = RE::VTABLE_LoadWaitSpinner[0];
		static constexpr std::size_t Index = 0x06;
		static constexpr auto Type = HookType::VFunc;
	};

	// IMenu::PostDisplay of MainMenu.
	struct MainMenuPostDisplayHook
	{
		using FuncDefinition = void(RE::MainMenu*);
		static constexpr auto VTable = RE::VTABLE_MainMenu[0];
		static constexpr std::size_t Index = 0x06;
		static constexpr auto Type = HookType::VFunc;
	};

	// IMenu::ProcessMessage of MainMenu.
	struct MainMenuProcessMessageHook
	{
		using FuncDefinition = RE::UI_MESSAGE_RESULTS(RE::MainMenu*, RE::UIMessage&);
		static constexpr auto VTable = RE::VTABLE_MainMenu[0];
		static constexpr std::size_t Index = 0x04;
		static constexpr auto Type = HookType::VFunc;
	};
}
