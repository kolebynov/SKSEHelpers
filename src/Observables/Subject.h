#pragma once

#include "GenericSubscription.h"
#include "Unit.h"

#include <concepts>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Observables
{
	/// Многоадресный источник событий, уведомляющий всех активных подписчиков.
	template <typename TItem = Unit>
	class Subject
	{
	public:
		/// Тип значения, отправляемого наблюдателям.
		using Item = TItem;
		/// Тип функции-наблюдателя, вызываемой при уведомлении.
		using Callback = std::move_only_function<void(Item)>;

		/// Создаёт пустой источник событий.
		Subject() = default;

		Subject(const Subject&) = delete;
		Subject& operator=(const Subject&) = delete;

		// Subscriptions keep an owner pointer, so moving the event system would leave existing tokens
		// pointing at the old object address.
		Subject(Subject&&) = delete;
		Subject& operator=(Subject&&) = delete;

		/// Удаляет подписку по идентификатору или помечает её для удаления после текущего уведомления.
		void Unsubscribe(SubscriptionId id) noexcept
		{
			if (_isNotifying) {
				_observers[id].IsUnsubscribed = true;
			}
			else {
				_observers.erase(id);
			}
		}

		/// RAII-токен подписки на этот источник событий.
		using Subscription = GenericSubscription<Subject>;

		/// Подписывает наблюдателя на будущие уведомления.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription Subscribe(TObserver&& observer)
		{
			const SubscriptionId id = _nextId++;
			_observers.emplace(id, SubscriptionData { .Observer = std::forward<TObserver>(observer) });
			return Subscription{ this, id };
		}

		/// Подписывает наблюдателя и немедленно вызывает его с указанным значением.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription SubscribeAndNotify(TObserver&& observer, Item arg)
		{
			const SubscriptionId id = _nextId++;
			_observers.emplace(id, SubscriptionData { .Observer = std::forward<TObserver>(observer) });
			_observers[id].Observer(arg);
			return Subscription{ this, id };
		}

		/// Отправляет значение всем активным наблюдателям.
		void Notify(Item arg)
		{
			_isNotifying = true;
			std::vector<SubscriptionId> unsubscribeList;

			for (auto& [id, subscriptionData] : _observers) {
				if (subscriptionData.IsUnsubscribed) {
					unsubscribeList.push_back(id);
				} else {
					subscriptionData.Observer(arg);
				}
			}

			for (auto id : unsubscribeList) {
				_observers.erase(id);
			}

			_isNotifying = false;
		}

	private:
		struct SubscriptionData
		{
			Callback Observer;
			bool IsUnsubscribed = false;
		};

		std::unordered_map<SubscriptionId, SubscriptionData> _observers{};
		SubscriptionId _nextId = 1;
		bool _isNotifying = false;
	};
}
