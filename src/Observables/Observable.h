#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace Observables
{
	/// Пустой наблюдатель для проверки сигнатуры Subscribe в концепте Observable.
	template <typename TArg>
	struct TestObserver
	{
		/// Принимает значение наблюдаемого потока без побочных эффектов.
		void operator()(TArg) const
		{
		}
	};

	/// Требование к токену подписки: он должен поддерживать одноразовый Detach для rvalue.
	template <typename TSubscription>
	concept ObservableSubscription =
		requires
		{
			{ std::declval<TSubscription>().Detach() } -> std::same_as<void>;
		} &&
		!requires(TSubscription& subscription)
		{
			subscription.Detach();
		};

	/// Требование к наблюдаемому источнику значений с типом Item, Subscription и методом Subscribe.
	template <typename TObservable, typename TDecayedObservable = std::decay_t<TObservable>>
	concept Observable = requires(TObservable& observable)
	{
		typename TDecayedObservable::Subscription;
		typename TDecayedObservable::Item;

		requires ObservableSubscription<typename TDecayedObservable::Subscription>;

		{ observable.Subscribe(TestObserver<typename TDecayedObservable::Item>{}) } -> std::same_as<typename TDecayedObservable::Subscription>;
	};

	/// Применяет adaptor к observable в стиле pipe-оператора.
	template <Observable TObservable, typename TAdaptor>
	requires requires(TObservable&& observable, TAdaptor&& adaptor) {
		std::forward<TAdaptor>(adaptor)(std::forward<TObservable>(observable));
	}
	auto operator|(TObservable&& observable, TAdaptor&& adaptor)
	{
		return std::forward<TAdaptor>(adaptor)(std::forward<TObservable>(observable));
	}
}

