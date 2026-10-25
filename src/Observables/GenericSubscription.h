#pragma once

#include <concepts>
#include <cstdint>
#include <utility>

namespace Observables
{
	/// Уникальный идентификатор активной подписки внутри владельца подписок.
	using SubscriptionId = std::uint64_t;

	/// Требование к владельцу подписок, который умеет отписывать наблюдателя по идентификатору.
	template <typename TOwner>
	concept SubscriptionOwner = requires(TOwner* owner)
	{
		{ owner->Unsubscribe(SubscriptionId{}) } -> std::same_as<void>;
	};

	/// RAII-токен подписки, автоматически отписывающий наблюдателя при уничтожении.
	template <typename TOwner>
	class GenericSubscription
	{
	public:
		/// Создаёт пустой токен без привязки к владельцу.
		GenericSubscription() = default;

		/// Создаёт токен для подписки с указанным владельцем и идентификатором.
		GenericSubscription(TOwner* owner, SubscriptionId id) noexcept
			requires SubscriptionOwner<TOwner> :
			_owner(owner), _id(id) {}

		GenericSubscription(const GenericSubscription&) = delete;
		GenericSubscription& operator=(const GenericSubscription&) = delete;

		/// Передаёт владение подпиской из другого токена.
		GenericSubscription(GenericSubscription&& other) noexcept :
			_owner(std::exchange(other._owner, nullptr)),
			_id(std::exchange(other._id, 0)) {}

		/// Отписывает текущую подписку и принимает владение другой.
		GenericSubscription& operator=(GenericSubscription&& other) noexcept
		{
			if (this != &other) {
				Reset();
				_owner = std::exchange(other._owner, nullptr);
				_id = std::exchange(other._id, 0);
			}

			return *this;
		}

		/// Автоматически отписывает наблюдателя, если токен всё ещё владеет подпиской.
		~GenericSubscription()
		{
			Reset();
		}

		/// Отсоединяет токен от подписки, оставляя наблюдателя подписанным у владельца.
		void Detach() && noexcept
		{
			_owner = nullptr;
			_id = 0;
		}

	private:
		void Reset() noexcept
		{
			static_assert(SubscriptionOwner<TOwner>);

			if (_owner != nullptr) {
				_owner->Unsubscribe(_id);
				_owner = nullptr;
				_id = 0;
			}
		}

		TOwner* _owner = nullptr;
		SubscriptionId _id = 0;
	};
}
