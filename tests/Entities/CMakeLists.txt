include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(EffectSpeedRulesTest
	EffectSpeedRulesTest.cpp
)
target_link_libraries(EffectSpeedRulesTest GeneratorTestingSupport)
add_test(
	NAME EffectSpeedRulesTest
	COMMAND EffectSpeedRulesTest
)
