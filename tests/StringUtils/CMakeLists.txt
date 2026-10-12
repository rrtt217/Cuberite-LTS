include_directories(${PROJECT_SOURCE_DIR}/src/)

set (SHARED_SRCS
	${PROJECT_SOURCE_DIR}/src/StringUtils.cpp
)

set (SHARED_HDRS
	../TestHelpers.h
	${PROJECT_SOURCE_DIR}/src/StringUtils.h
)

set (SRCS
	CommandLineSplitTest.cpp
)


source_group("Shared" FILES ${SHARED_SRCS} ${SHARED_HDRS})
source_group("Sources" FILES ${SRCS})
add_executable(CommandLineSplitTest ${SRCS} ${SHARED_SRCS} ${SHARED_HDRS})
target_link_libraries(CommandLineSplitTest fmt::fmt)
if (WIN32)
	target_link_libraries(CommandLineSplitTest ws2_32)
endif()
add_test(NAME CommandLineSplitTest COMMAND CommandLineSplitTest)





# Put the projects into solution folders (MSVC):
set_target_properties(
	CommandLineSplitTest
	PROPERTIES FOLDER Tests/StringUtils
)
