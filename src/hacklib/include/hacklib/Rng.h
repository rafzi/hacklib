#ifndef HACKLIB_RNG_H
#define HACKLIB_RNG_H

#include <random>


namespace hl
{
class Rng
{
public:
    using Engine = std::mt19937;

    Rng() : m_rng(std::random_device()()) {}
    explicit Rng(Engine::result_type seed) : m_rng(seed) {}

    Engine& raw() { return m_rng; }

    template <typename T>
    T nextInt(T min, T max)
    {
        std::uniform_int_distribution<T> distr(min, max);
        return distr(m_rng);
    }

    template <typename T>
    T nextReal(T min, T max)
    {
        std::uniform_real_distribution<T> distr(min, max);
        return distr(m_rng);
    }

    bool nextBool(double trueProbability = 0.5)
    {
        std::bernoulli_distribution distr(trueProbability);
        return distr(m_rng);
    }

private:
    Engine m_rng;
};
}

#endif