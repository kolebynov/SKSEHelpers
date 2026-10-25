#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <concepts>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>

#ifndef NDEBUG
#	include <unordered_set>
#endif

namespace TESLookup
{
	template <class... TArgs>
	[[noreturn]] void LogCriticalAndThrowError(fmt::format_string<TArgs...> format, TArgs&&... args)
	{
		auto message = fmt::format(format, std::forward<TArgs>(args)...);
		SKSE::log::critical("{}", message);
		throw std::runtime_error(message);
	}

#ifndef NDEBUG
	inline void CheckForAlreadyLookup(const std::string& id)
	{
		static std::unordered_set<std::string> ids{};
		auto [_, inserted] = ids.insert(id);
		if (!inserted) {
			SKSE::log::warn("Already looked up for id: {}", id);
		}
	}
#endif

	template <class T = RE::TESForm>
		requires std::same_as<T, std::decay_t<T>>
	T* LookupByEditorId(const std::string_view& editorId, bool throwOnError)
	{
#ifndef NDEBUG
		CheckForAlreadyLookup(editorId.data());
#endif

		const auto form = RE::TESForm::LookupByEditorID(editorId);
		if (!form) {
			if (throwOnError) {
				LogCriticalAndThrowError("Form not found by editor id {}", editorId);
			}

			return nullptr;
		}

		auto resultForm = form ? form->As<T>() : nullptr;
		if (!resultForm && throwOnError) {
			LogCriticalAndThrowError("Form {} is not of type {}", editorId, typeid(T).name());
		}

		return resultForm;
	}

	template <class T = RE::TESForm>
		requires std::same_as<T, std::decay_t<T>>
	T* LookupByFormId(RE::FormID formId, std::string_view modName, bool throwOnError)
	{
#ifndef NDEBUG
		CheckForAlreadyLookup(std::format("{}:{:X}", modName, formId));
#endif

		const auto form = RE::TESDataHandler::GetSingleton()->LookupForm(formId, modName);
		if (!form) {
			if (throwOnError) {
				LogCriticalAndThrowError("Form not found by form id {}:{:X}", modName, formId);
			}

			return nullptr;
		}

		auto resultForm = form ? form->As<T>() : nullptr;
		if (!resultForm && throwOnError) {
			LogCriticalAndThrowError("Form {}:{:X} is not of type {}", modName, formId, typeid(T).name());
		}

		return resultForm;
	}

	template <class T = RE::TESForm>
		requires std::same_as<T, std::decay_t<T>>
	T* LookupByAbsoluteFormId(RE::FormID formId, bool throwOnError)
	{
#ifndef NDEBUG
		CheckForAlreadyLookup(std::format("{:X}", formId));
#endif

		const auto form = RE::TESForm::LookupByID(formId);
		if (!form) {
			if (throwOnError) {
				LogCriticalAndThrowError("Form not found by form id {:X}", formId);
			}

			return nullptr;
		}

		auto resultForm = form ? form->As<T>() : nullptr;
		if (!resultForm && throwOnError) {
			LogCriticalAndThrowError("Form {:X} is not of type {}", formId, typeid(T).name());
		}

		return resultForm;
	}
}
