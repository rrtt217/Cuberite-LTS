include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ChorusRulesTest
	ChorusRulesTest.cpp
)
target_link_libraries(ChorusRulesTest GeneratorTestingSupport)
add_test(
	NAME ChorusRulesTest
	COMMAND ChorusRulesTest
)





# Put the projects into solution folders (MSVC):
set_target_properties(
	ChorusRulesTest
	PROPERTIES FOLDER Tests/Blocks
)
