#pragma once

#include <stdint.h>
#include <chrono>
#include <ctime>
#include <ratio>

namespace apemode {
namespace platform {

class Stopwatch {
public:
    inline void   Start( );
    inline double GetElapsedSeconds( ) const;

private:
    std::chrono::high_resolution_clock::time_point StartTimePoint;
};

void apemode::platform::Stopwatch::Start( ) {
    using namespace std::chrono;
    StartTimePoint = high_resolution_clock::now( );
}

double apemode::platform::Stopwatch::GetElapsedSeconds( ) const {
    using namespace std::chrono;
    const high_resolution_clock::time_point currentTimePoint = high_resolution_clock::now( );
    const duration< double > time_span = duration_cast< duration< double > >( currentTimePoint - StartTimePoint );
    return time_span.count( );
}

} // namespace platform
} // namespace apemode
