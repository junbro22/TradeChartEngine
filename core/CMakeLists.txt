set(TCE_SOURCES
    src/api.cpp
    src/chart.cpp
    src/data/series.cpp
    src/indicator/ma.cpp
    src/indicator/ema.cpp
    src/indicator/rsi.cpp
    src/indicator/macd.cpp
    src/indicator/bollinger.cpp
    src/indicator/stochastic.cpp
    src/indicator/atr.cpp
    src/indicator/ichimoku.cpp
    src/indicator/psar.cpp
    src/indicator/supertrend.cpp
    src/indicator/vwap.cpp
    src/indicator/dmi.cpp
    src/indicator/cci.cpp
    src/indicator/williams_r.cpp
    src/indicator/obv.cpp
    src/indicator/mfi.cpp
    src/indicator/pivot.cpp
    src/drawing/drawing.cpp
    src/drawing/drawing_renderer.cpp
    src/viewport/viewport.cpp
    src/renderer/frame_builder.cpp
    src/renderer/label_builder.cpp
)

if(TCE_BUILD_SHARED)
    add_library(tce SHARED ${TCE_SOURCES})
else()
    add_library(tce STATIC ${TCE_SOURCES})
endif()

target_include_directories(tce
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(tce PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(tce PRIVATE /W4)
else()
    target_compile_options(tce PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(tce PROPERTIES
    OUTPUT_NAME tce
    PUBLIC_HEADER "include/tce/tce.h;include/tce/types.h"
)
