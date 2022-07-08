#pragma once

#include <cstdint>

template <typename T, typename Total, size_t N>
class RollingAverage
{
public:
	RollingAverage& operator()(T sample)
	{
		total_ += sample;
		if (num_samples_ < N)
			samples_[num_samples_++] = sample;
		else
		{
			T& oldest = samples_[num_samples_++ % N];
			total_ -= oldest;
			oldest = sample;
		}
		return *this;
	}

	Total get() const { return total_ / std::min(num_samples_, N); }

private:
	T samples_[N];
	size_t num_samples_{0};
	Total total_{0};
};
