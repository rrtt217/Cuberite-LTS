include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(CommandBlockRulesTest
	CommandBlockRulesTest.cpp
)
target_link_libraries(CommandBlockRulesTest GeneratorTestingSupport)
add_test(
	NAME CommandBlockRulesTest
	COMMAND CommandBlockRulesTest
)

add_executable(CommandBlockNBTTest
	CommandBlockNBTTest.cpp
	${PROJECT_SOURCE_DIR}/src/WorldStorage/CommandBlockNBT.cpp
	${PROJECT_SOURCE_DIR}/src/WorldStorage/FastNBT.cpp
)
target_link_libraries(CommandBlockNBTTest GeneratorTestingSupport)
add_test(
	NAME CommandBlockNBTTest
	COMMAND CommandBlockNBTTest
)





# Put the projects into solution folders (MSVC):
set_target_properties(
	CommandBlockNBTTest
	CommandBlockRulesTest
	PROPERTIES FOLDER Tests/BlockEntities
)
