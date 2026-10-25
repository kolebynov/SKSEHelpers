#pragma once

#include "Subject.h"
#include "TrackedValue.h"

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace Observables
{
	/// Реактивное значение, уведомляющее подписчиков только при изменении по comparer.
	template <class T, class TComparer = std::equal_to<T>>
		requires std::invocable<TComparer, const T&, const T&> &&
	             std::same_as<std::invoke_result_t<TComparer, const T&, const T&>, bool> && std::same_as<T, std::decay_t<T>>
	class ReactiveObject
	{
	public:
		/// Тип значения, передаваемого наблюдателям.
		using Item = const T&;
		/// RAII-токен подписки на изменения значения.
		using Subscription = Subject<Item>::Subscription;

		/// Создаёт реактивное значение с пользовательским или стандартным сравнением.
		ReactiveObject(T value, TComparer comparer = {}) : _value(value, comparer) {}

		ReactiveObject(const ReactiveObject&) = delete;
		ReactiveObject& operator=(const ReactiveObject&) = delete;

		ReactiveObject(ReactiveObject&&) = delete;
		ReactiveObject& operator=(ReactiveObject&&) = delete;

		/// Возвращает текущее значение.
		const T& Value() const { return _value.Get(); }

		/// Обновляет значение копированием и уведомляет подписчиков, если оно изменилось.
		void Set(const T& newValue)
		{
			if (_value.Set(newValue)) {
				_subject.Notify(_value.Get());
			}
		}

		/// Обновляет значение перемещением и уведомляет подписчиков, если оно изменилось.
		void Set(T&& newValue)
		{
			if (_value.Set(std::move(newValue))) {
				_subject.Notify(_value.Get());
			}
		}

		/// Подписывает наблюдателя на изменения с опциональным немедленным уведомлением текущим значением.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		auto Subscribe(TObserver&& observer, bool notifyCurrentValue = false)
		{
			return !notifyCurrentValue ? _subject.Subscribe(std::forward<TObserver>(observer)) :
			                             _subject.SubscribeAndNotify(std::forward<TObserver>(observer), _value.Get());
		}

		/// Адаптер, превращающий ReactiveObject в observable с уведомлением текущим значением при подписке.
		struct NotifiedOnSubscribe
		{
		public:
			/// Тип значения, передаваемого наблюдателям.
			using Item = Item;
			/// RAII-токен подписки на изменения значения.
			using Subscription = Subscription;

			/// Создаёт адаптер для указанного реактивного значения.
			explicit NotifiedOnSubscribe(ReactiveObject& reactiveObject) noexcept : _reactiveObject(reactiveObject) {}

			/// Подписывает наблюдателя и сразу отправляет ему текущее значение.
			template <std::invocable<Item> TObserver>
			[[nodiscard]]
			auto Subscribe(TObserver&& observer)
			{
				return _reactiveObject.Subscribe(std::forward<TObserver>(observer), true);
			}

		private:
			ReactiveObject& _reactiveObject;
		};

		/// Возвращает адаптер подписки с немедленным уведомлением текущим значением.
		NotifiedOnSubscribe ToNotifyOnSubscribe() & { return NotifiedOnSubscribe(*this); }

	private:
		Subject<Item> _subject{};
		TrackedValue<T, TComparer> _value;
	};
}
