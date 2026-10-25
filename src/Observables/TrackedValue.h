#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace Observables
{
	/// Хранит текущее и предыдущее значение, чтобы определить изменение через comparer.
	template <class T, class TComparer = std::equal_to<T>>
		requires std::invocable<TComparer, const T&, const T&> &&
				 std::same_as<std::invoke_result_t<TComparer, const T&, const T&>, bool>
	struct TrackedValue
	{
	public:
		/// Создаёт отслеживаемое значение с пользовательским или стандартным сравнением.
		TrackedValue(T value, TComparer comparer = {}) noexcept : _prevValue(value), _value(value), _comparer(comparer) {}

		/// Возвращает true, если текущее значение отличается от предыдущего.
		bool IsChanged() const { return !_comparer(_prevValue, _value); }

		/// Возвращает текущее значение.
		const T& Get() const { return _value; }

		/// Обновляет значение копированием и возвращает, изменилось ли оно.
		bool Set(const T& value)
		{
			_prevValue = _value;
			_value = value;
			return IsChanged();
		}

		/// Обновляет значение перемещением и возвращает, изменилось ли оно.
		bool Set(T&& value)
		{
			_prevValue = _value;
			_value = std::move(value);
			return IsChanged();
		}

	private:
		T _prevValue;
		T _value;
		TComparer _comparer;
	};
}
