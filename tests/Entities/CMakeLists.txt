include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(EffectCombineRulesTest
	EffectCombineRulesTest.cpp
)
target_link_libraries(EffectCombineRulesTest GeneratorTestingSupport)
add_test(
	NAME EffectCombineRulesTest
	COMMAND EffectCombineRulesTest
)

add_executable(EffectImmunityRulesTest
	EffectImmunityRulesTest.cpp
)
target_link_libraries(EffectImmunityRulesTest GeneratorTestingSupport)
add_test(
	NAME EffectImmunityRulesTest
	COMMAND EffectImmunityRulesTest
)

add_executable(EffectNumbersTest
	EffectNumbersTest.cpp
)
target_link_libraries(EffectNumbersTest GeneratorTestingSupport)
add_test(
	NAME EffectNumbersTest
	COMMAND EffectNumbersTest
)

add_executable(EffectSpeedRulesTest
	EffectSpeedRulesTest.cpp
)
target_link_libraries(EffectSpeedRulesTest GeneratorTestingSupport)
add_test(
	NAME EffectSpeedRulesTest
	COMMAND EffectSpeedRulesTest
)

add_executable(FireproofRulesTest
	FireproofRulesTest.cpp
)
target_link_libraries(FireproofRulesTest GeneratorTestingSupport)
add_test(
	NAME FireproofRulesTest
	COMMAND FireproofRulesTest
)

add_executable(HungerRulesTest
	HungerRulesTest.cpp
)
target_link_libraries(HungerRulesTest GeneratorTestingSupport)
add_test(
	NAME HungerRulesTest
	COMMAND HungerRulesTest
)





# Put the projects into solution folders (MSVC):
set_target_properties(
	EffectCombineRulesTest
	EffectImmunityRulesTest
	EffectNumbersTest
	EffectSpeedRulesTest
	FireproofRulesTest
	HungerRulesTest
	PROPERTIES FOLDER Tests/Entities
)
