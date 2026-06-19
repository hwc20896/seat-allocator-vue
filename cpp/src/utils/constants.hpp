#pragma once

namespace Constants {
    inline constexpr auto MAX_ATTEMPTS = 1000;
    inline constexpr auto NUM_THREADS = 4;
    inline constexpr auto ATTEMPTS_PER_THREAD = MAX_ATTEMPTS / NUM_THREADS;
}