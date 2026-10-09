include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/)

set (SHARED_SRCS
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PacketFraming.cpp
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PacketLayout.cpp
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PayloadReader.cpp
	${PROJECT_SOURCE_DIR}/src/StringUtils.cpp
	${PROJECT_SOURCE_DIR}/src/StringCompression.cpp
)

set (SHARED_HDRS
	../TestHelpers.h
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PacketFraming.h
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PacketLayout.h
	${PROJECT_SOURCE_DIR}/Tools/ProtoProxy/PayloadReader.h
)

set (SRCS
	ProtoProxyProtocolTest.cpp
)

source_group("Shared" FILES ${SHARED_SRCS} ${SHARED_HDRS})
source_group("Sources" FILES ${SRCS})
add_executable(ProtoProxyProtocolTest ${SRCS} ${SHARED_SRCS} ${SHARED_HDRS})
target_link_libraries(ProtoProxyProtocolTest fmt::fmt libdeflate)
add_test(NAME ProtoProxyProtocolTest COMMAND ProtoProxyProtocolTest)



# Put the projects into solution folders (MSVC):
set_target_properties(
	ProtoProxyProtocolTest
	PROPERTIES FOLDER Tests
)
