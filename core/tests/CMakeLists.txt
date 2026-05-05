# 외부 의존성 없이 가벼운 자체 테스트 러너 (Phase 1)
# Phase 2에서 Catch2/GoogleTest로 교체 예정

add_executable(tce_tests
    test_main.cpp
    test_series.cpp
    test_indicator.cpp
    test_viewport.cpp
    test_frame.cpp
)

target_link_libraries(tce_tests PRIVATE tce)
target_include_directories(tce_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_SOURCE_DIR}/core/include
)

add_test(NAME tce_tests COMMAND tce_tests)
