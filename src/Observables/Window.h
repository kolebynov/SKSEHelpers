#pragma once

#include "Observable.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace Observables::_Detail
{
	/// Observable, отправляющий скользящее окно из TWindow последних значений исходного потока.
	/// Окно сдвигается на один элемент, поэтому уведомление приходит на каждое значение после заполнения окна.
	/// Окно меньше двух элементов не имеет смысла: оно эквивалентно исходному потоку.
	template <Observable TObservable, std::uint32_t TWindow>
		requires(TWindow > 1)
	class WindowObservable
	{
	public:
		/// Исходный observable после снятия ссылок и cv-квалификаторов.
		using Source = std::decay_t<TObservable>;
		/// Тип значения исходного observable.
		using SourceItem = Source::Item;
		/// Тип значения исходного observable, пригодный для хранения в окне.
		using SourceItemValue = std::decay_t<SourceItem>;
		static_assert(std::default_initializable<SourceItemValue>,
			"Source item must be default initializable to preallocate the window");
		static_assert(std::assignable_from<SourceItemValue&, SourceItem>, "Source item must be assignable into the window");
		/// Тип значения, передаваемого наблюдателям: ссылка на окно от самого старого значения к самому новому.
		using Item = const std::array<SourceItemValue, TWindow>&;
		/// Токен подписки исходного observable.
		using Subscription = Source::Subscription;

		/// Создаёт скользящее окно поверх источника.
		WindowObservable(TObservable&& observable) : _observable(std::forward<TObservable>(observable)) {}

		/// Подписывает наблюдателя на окна значений; первые TWindow - 1 значений только заполняют окно.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription Subscribe(TObserver&& observer)
		{
			return _observable.Subscribe(
				[observer = std::forward<TObserver>(observer), state = SubscriptionState{}](SourceItem item) mutable {
					if (state.FilledCount < TWindow) [[unlikely]] {
						state.Window[state.FilledCount] = item;
						++state.FilledCount;
						if (state.FilledCount == TWindow) {
							std::invoke(observer, state.Window);
						}

						return;
					}

					std::ranges::shift_left(state.Window, 1);
					state.Window[TWindow - 1] = item;
					std::invoke(observer, state.Window);
				});
		}

	private:
		struct SubscriptionState
		{
			std::array<SourceItemValue, TWindow> Window;
			std::uint32_t FilledCount = 0;
		};

		TObservable _observable;
	};

	/// Pipe-adaptor для создания WindowObservable из произвольного observable.
	template <std::uint32_t TWindow>
	class WindowAdaptor
	{
	public:
		/// Применяет адаптер к observable; адаптер не хранит состояния, поэтому копия и перемещение эквивалентны.
		template <Observable TObservable>
		auto operator()(TObservable&& observable) const
		{
			return WindowObservable<TObservable, TWindow>(std::forward<TObservable>(observable));
		}
	};
}

namespace Observables
{
	/// Создаёт adaptor, собирающий значения observable в скользящее окно из TWindow последних значений.
	template <std::uint32_t TWindow>
	static auto Window()
	{
		return _Detail::WindowAdaptor<TWindow>{};
	}
}
