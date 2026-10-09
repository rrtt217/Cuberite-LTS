include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(EndermanTeleportRulesTest
	EndermanTeleportRulesTest.cpp
)
target_link_libraries(EndermanTeleportRulesTest GeneratorTestingSupport)
add_test(
	NAME EndermanTeleportRulesTest
	COMMAND EndermanTeleportRulesTest
)

add_executable(EndermanBlockRulesTest
	EndermanBlockRulesTest.cpp
)
target_link_libraries(EndermanBlockRulesTest GeneratorTestingSupport)
add_test(
	NAME EndermanBlockRulesTest
	COMMAND EndermanBlockRulesTest
)
