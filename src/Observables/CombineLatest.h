#pragma once

#include "Observable.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Observables::_Detail
{
	/// Observable, отправляющий кортеж последних значений после инициализации всех источников.
	template <Observable... TObservables>
		requires (sizeof...(TObservables) > 0)
	class CombineLatestObservable
	{
	public:
		/// Кортеж последних значений всех исходных observable.
		using LastValues = std::tuple<std::decay_t<typename std::decay_t<TObservables>::Item>...>;
		/// Тип значения, передаваемого подписчикам.
		using Item = const LastValues&;
		/// Кортеж RAII-токенов подписок на исходные observable.
		using Subscriptions = std::tuple<typename std::decay_t<TObservables>::Subscription...>;

	private:
		struct SubscriptionData
		{
			LastValues LastValues{};
			std::array<bool, sizeof...(TObservables)> ValueInitializationStatuses{};
			bool AllInitialized = false;
		};

	public:
		/// RAII-токен, владеющий всеми подписками CombineLatest.
		class Subscription
		{
		public:
			/// Создаёт токен из подписок источников и общего состояния последних значений.
			Subscription(Subscriptions&& subscriptions, std::unique_ptr<SubscriptionData>&& subscriptionData) :
				_subscriptionData(std::move(subscriptionData)),
				_subscriptions(std::move(subscriptions))
			{
			}

			Subscription(const Subscription&) = delete;
			Subscription& operator=(const Subscription&) = delete;
			Subscription(Subscription&&) = default;
			Subscription& operator=(Subscription&&) = default;

			/// Отсоединяет все дочерние подписки, оставляя их активными у источников.
			void Detach() && noexcept
			{
				std::apply([](auto&&... sub) { (std::move(sub).Detach(), ...); }, _subscriptions);
				_subscriptionData.release();
			}

		private:
			std::unique_ptr<SubscriptionData> _subscriptionData;
			Subscriptions _subscriptions;
		};

		/// Создаёт CombineLatest поверх набора исходных observable.
		CombineLatestObservable(TObservables&&... observables) :
			_observables(std::forward<TObservables>(observables)...)
		{
		}

		/// Подписывает наблюдателя на кортеж последних значений всех источников.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription Subscribe(TObserver&& observer)
		{
			std::unique_ptr<SubscriptionData> subscriptionData = std::make_unique<SubscriptionData>();
			auto observerPtr = std::make_shared<std::decay_t<TObserver>>(std::forward<TObserver>(observer));

			Subscriptions subscriptions = [&, observerPtr]<std::size_t... Is>(std::index_sequence<Is...>) {
				return Subscriptions{ std::get<Is>(_observables).Subscribe([observerPtr, subscriptionData = subscriptionData.get()](auto item) {
					std::get<Is>(subscriptionData->LastValues) = item;
					if (subscriptionData->AllInitialized) {
						std::invoke(*observerPtr, subscriptionData->LastValues);
						return;
					}

					std::get<Is>(subscriptionData->ValueInitializationStatuses) = true;
					subscriptionData->AllInitialized = std::apply(
						[](auto&&... status) { return (status && ...); },
						subscriptionData->ValueInitializationStatuses);

					if (subscriptionData->AllInitialized) {
						std::invoke(*observerPtr, subscriptionData->LastValues);
					}
				})... };
			}(std::make_index_sequence<std::tuple_size_v<LastValues>>{});

			return Subscription(std::move(subscriptions), std::move(subscriptionData));
		}

	private:
		std::tuple<TObservables...> _observables;
	};
}

namespace Observables
{
	/// Объединяет несколько observable и отправляет последние значения после обновления любого источника.
	template <Observable... TObservables>
		requires (sizeof...(TObservables) > 0)
	static auto CombineLatest(TObservables&&... observables)
	{
		return _Detail::CombineLatestObservable<TObservables...>(std::forward<TObservables>(observables)...);
	}
}
