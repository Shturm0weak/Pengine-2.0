#pragma once

#include "Core.h"

#include <random>
#include <mutex>
#include <type_traits>

namespace Pengine
{

    class PENGINE_API RandomGenerator
    {
    public:
        RandomGenerator(const RandomGenerator&) = delete;
        RandomGenerator& operator=(const RandomGenerator&) = delete;
        RandomGenerator(RandomGenerator&&) = delete;
        RandomGenerator& operator=(RandomGenerator&&) = delete;

        static RandomGenerator& GetInstance()
        {
            static RandomGenerator instance;
            return instance;
        }

        template<typename T>
        T Get(T min, T max)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
            {
                return std::uniform_int_distribution<T>{min, max}(m_Engine);
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                return std::uniform_real_distribution<T>{min, max}(m_Engine);
            }
            else
            {
                static_assert(sizeof(T) == 0, "Unsupported type for random generation");
            }
        }

    private:
        std::mt19937 m_Engine;
        std::mutex m_Mutex;

        RandomGenerator()
        {
            std::random_device randomDevice;
            m_Engine.seed(randomDevice());
        }
    };

}
