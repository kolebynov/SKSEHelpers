#pragma once

#include "GenericSubscription.h"
#include "Unit.h"

#include <concepts>
#include <cstdint>
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
			if (_notifyDepth == 0) {
				_observers.erase(id);
				return;
			}

			// Observers must not be erased while Notify() iterates them; they are erased when the outermost Notify() ends.
			if (auto it = _observers.find(id); it != _observers.end()) {
				it->second.IsUnsubscribed = true;
			} else {
				std::erase_if(_pendingObservers, [id](const auto& pending) { return pending.first == id; });
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
			AddObserver(id, Callback(std::forward<TObserver>(observer)));
			return Subscription{ this, id };
		}

		/// Подписывает наблюдателя и немедленно вызывает его с указанным значением.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription SubscribeAndNotify(TObserver&& observer, Item arg)
		{
			const SubscriptionId id = _nextId++;
			Callback callback(std::forward<TObserver>(observer));
			callback(arg);
			AddObserver(id, std::move(callback));
			return Subscription{ this, id };
		}

		/// Отправляет значение всем активным наблюдателям.
		/// Наблюдатели, подписанные во время уведомления, получают значения начиная со следующего уведомления.
		void Notify(Item arg)
		{
			++_notifyDepth;
			try {
				for (auto& [id, subscriptionData] : _observers) {
					if (!subscriptionData.IsUnsubscribed) {
						subscriptionData.Observer(arg);
					}
				}
			} catch (...) {
				EndNotify();
				throw;
			}

			EndNotify();
		}

	private:
		struct SubscriptionData
		{
			Callback Observer;
			bool IsUnsubscribed = false;
		};

		// Inserting into _observers while Notify() iterates it may rehash and invalidate the iteration, so observers
		// subscribed during a notification wait in _pendingObservers until the outermost Notify() ends.
		void AddObserver(SubscriptionId id, Callback&& observer)
		{
			if (_notifyDepth == 0) {
				_observers.emplace(id, SubscriptionData{ .Observer = std::move(observer) });
			} else {
				_pendingObservers.emplace_back(id, std::move(observer));
			}
		}

		void EndNotify()
		{
			if (--_notifyDepth != 0) {
				return;
			}

			std::erase_if(_observers, [](const auto& entry) { return entry.second.IsUnsubscribed; });
			for (auto& [id, observer] : _pendingObservers) {
				_observers.emplace(id, SubscriptionData{ .Observer = std::move(observer) });
			}
			_pendingObservers.clear();
		}

		std::unordered_map<SubscriptionId, SubscriptionData> _observers{};
		std::vector<std::pair<SubscriptionId, Callback>> _pendingObservers{};
		SubscriptionId _nextId = 1;
		std::uint32_t _notifyDepth = 0;
	};
}
