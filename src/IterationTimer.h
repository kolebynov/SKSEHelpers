#pragma once

#include <chrono>
#include <concepts>

template <std::floating_point T>
class IterationTimer
{
	using Duration = std::chrono::duration<T>;

public:
	explicit IterationTimer(Duration duration, Duration elapsed = Duration::zero(), bool enabled = true) :
		_elapsedTime(elapsed), _iterationTime(duration), _enabled(enabled)
	{}

	[[nodiscard]] Duration ElapsedTime() const { return _elapsedTime; }

	[[nodiscard]] Duration IterationTime() const { return _iterationTime; }

	void SetIterationTime(Duration duration) { _iterationTime = duration; }

	[[nodiscard]] bool Enabled() const { return _enabled; }

	void SetEnabled(bool enabled) { _enabled = enabled; }

	[[nodiscard]] T Progress() const
	{
		if (_iterationTime <= Duration::zero()) {
			return T{ 0 };
		}

		return _elapsedTime / _iterationTime;
	}

	bool Tick(Duration delta)
	{
		int iterationsPassed = 0;
		return Tick(delta, iterationsPassed);
	}

	bool Tick(T delta)
	{
		int iterationsPassed = 0;
		return Tick(delta, iterationsPassed);
	}

	bool Tick(T delta, int& iterationsPassed) { return Tick(Duration{ delta }, iterationsPassed); }

	bool Tick(Duration delta, int& iterationsPassed)
	{
		iterationsPassed = 0;

		if (!_enabled || _iterationTime <= Duration::zero()) {
			return false;
		}

		_elapsedTime += delta;

		if (_elapsedTime >= _iterationTime) [[unlikely]] {
			iterationsPassed = static_cast<int>(_elapsedTime / _iterationTime);
			_elapsedTime -= _iterationTime * static_cast<T>(iterationsPassed);
			return true;
		}

		return false;
	}

	void Reset() { _elapsedTime = Duration::zero(); }

private:
	Duration _elapsedTime{ Duration::zero() };
	Duration _iterationTime;
	bool _enabled{ true };
};
