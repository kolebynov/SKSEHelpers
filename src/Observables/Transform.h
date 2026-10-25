#pragma once

#include "Observable.h"

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace Observables::_Detail
{
	/// Observable-обёртка, преобразующая каждое значение исходного потока через mapper.
	template <Observable TObservable, typename TMapper>
	class TransformObservable
	{
	public:
		/// Исходный observable после снятия ссылок и cv-квалификаторов.
		using Source = std::decay_t<TObservable>;
		/// Тип значения исходного observable.
		using SourceItem = Source::Item;
		static_assert(std::invocable<TMapper&, SourceItem>, "TMapper must be invocable");
		/// Тип значения после преобразования mapper.
		using Item = std::invoke_result_t<TMapper&, SourceItem>;
		/// Токен подписки исходного observable.
		using Subscription = Source::Subscription;

		/// Создаёт преобразующий observable поверх источника и функции преобразования.
		TransformObservable(TObservable&& observable, TMapper&& mapper) :
			_observable(std::forward<TObservable>(observable)),
			_mapper(std::forward<TMapper>(mapper)) {}

		/// Подписывает наблюдателя на преобразованные значения.
		template <std::invocable<Item> TObserver>
		[[nodiscard]]
		Subscription Subscribe(TObserver&& observer)
		{
			auto mapper = _mapper;
			return _observable.Subscribe([mapper, observer = std::forward<TObserver>(observer)](SourceItem item) {
				std::invoke(observer, std::invoke(mapper, item));
			});
		}

	private:
		TObservable _observable;
		TMapper _mapper;
	};

	/// Pipe-adaptor для создания TransformObservable из произвольного observable.
	template <typename TMapper>
	class TransformAdaptor
	{
	public:
		/// Создаёт адаптер с указанной функцией преобразования.
		explicit TransformAdaptor(TMapper&& mapper) :
			_mapper(std::forward<TMapper>(mapper)) {}

		/// Применяет перемещаемый адаптер к observable.
		template <Observable TObservable>
		auto operator()(TObservable&& observable) &&
		{
			return TransformObservable<TObservable, TMapper>(
				std::forward<TObservable>(observable),
				std::forward<TMapper>(_mapper));
		}

		/// Применяет копируемый адаптер к observable.
		template <Observable TObservable>
		auto operator()(TObservable&& observable) const &
		{
			return TransformObservable<TObservable, TMapper>(
				std::forward<TObservable>(observable),
				_mapper);
		}

	private:
		TMapper _mapper;
	};
}

namespace Observables
{
	/// Создаёт adaptor, преобразующий значения observable через mapper.
	template <typename TMapper>
	static auto Transform(TMapper&& mapper)
	{
		return _Detail::TransformAdaptor<TMapper>(std::forward<TMapper>(mapper));
	}
}
