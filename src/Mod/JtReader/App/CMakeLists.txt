if(MSVC)
    add_definitions(-DFCAppImport -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH -DNO_JT_MULTITHREADING)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DNO_JT_MULTITHREADING)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtAttribute
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtData
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtDecode
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtElement
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtNode
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtProperty
    ${CMAKE_CURRENT_SOURCE_DIR}/TkJt/JtToTopoDS
)

link_directories(${OCC_LIBRARY_DIR})

set(JtReader_LIBS
    Mesh
    #${OCC_OCAF_LIBRARIES}
    #${OCC_OCAF_DEBUG_LIBRARIES}
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}

)

SET(JtReader_SRCS
    AppJtReader.cpp
    AppJtReaderPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    TestJtReader.cpp
    TestJtReader.h
	FcLodHandler.h
	FcLodHandler.cpp
    JcadLibReader.h
    JcadLibReader.cpp
    TkJtLibReader.h
    TkJtLibReader.cpp
	JtPart.h
	JtPart.cpp
	JtPartPyImp.cpp
	JtPartHandle.h
	JtPartHandle.cpp
)

generate_from_xml(JtPartPy)

SET(JtReader_XML_SRCS
    JtPartPy.xml
)
SOURCE_GROUP("XML" FILES ${JtReader_XML_SRCS})

# JrJt sources 
FILE( GLOB JRJT_SRCS JrJt/*.cpp JrJt/*.h )
SOURCE_GROUP("JrJt" FILES ${JRJT_SRCS}  )

# TkJt sources 
FILE( GLOB TKJT_JtAttribute_SRCS TkJt/JtAttribute/*.cxx TkJt/JtAttribute/*.hxx )
SOURCE_GROUP("TkJt/JtAttribute" FILES ${TKJT_JtAttribute_SRCS}  )
FILE( GLOB TKJT_JtData_SRCS TkJt/JtData/*.cxx TkJt/JtData/*.hxx )
SOURCE_GROUP("TkJt/JtData" FILES ${TKJT_JtData_SRCS}  )
FILE( GLOB TKJT_JtDecode_SRCS TkJt/JtDecode/*.cxx TkJt/JtDecode/*.hxx )
SOURCE_GROUP("TkJt/JtDecode" FILES ${TKJT_JtDecode_SRCS}  )
FILE( GLOB TKJT_JtElement_SRCS TkJt/JtElement/*.cxx TkJt/JtElement/*.hxx )
SOURCE_GROUP("TkJt/JtElement" FILES ${TKJT_JtElement_SRCS}  )
FILE( GLOB TKJT_JtNode_SRCS TkJt/JtNode/*.cxx TkJt/JtNode/*.hxx )
SOURCE_GROUP("TkJt/JtNode" FILES ${TKJT_JtNode_SRCS}  )
FILE( GLOB TKJT_JtProperty_SRCS TkJt/JtProperty/*.cxx TkJt/JtProperty/*.hxx )
SOURCE_GROUP("TkJt/JtProperty" FILES ${TKJT_JtProperty_SRCS}  )
FILE( GLOB TKJT_JtToTopoDS_SRCS TkJt/JtToTopoDS/*.cxx TkJt/JtToTopoDS/*.hxx )
SOURCE_GROUP("TkJt/JtToTopoDS" FILES ${TKJT_JtToTopoDS_SRCS}  )


SOURCE_GROUP("Module" FILES ${JtReader_SRCS})

add_library(JtReader SHARED ${JtReader_SRCS} ${JRJT_SRCS} ${TKJT_JtAttribute_SRCS} ${TKJT_JtData_SRCS} ${TKJT_JtDecode_SRCS} ${TKJT_JtElement_SRCS} ${TKJT_JtNode_SRCS} ${TKJT_JtProperty_SRCS} ${TKJT_JtToTopoDS_SRCS} ${JtReader_XML_SRCS})
target_link_libraries(JtReader ${JtReader_LIBS})

fc_target_copy_resource(JtReader 
    ${CMAKE_SOURCE_DIR}/src/Mod/JtReader
    ${CMAKE_BINARY_DIR}/Mod/JtReader
    Init.py )

fc_target_copy_resource(JtReader
   ${CMAKE_SOURCE_DIR}/src/Mod/JtReader/App/jcadlib
    ${CMAKE_BINARY_DIR}/Mod/JtReader
    FcJtPlugin.jar)

SET_BIN_DIR(JtReader JtReader /Mod/JtReader)
SET_PYTHON_PREFIX_SUFFIX(JtReader)

INSTALL(TARGETS JtReader DESTINATION ${CMAKE_INSTALL_LIBDIR})
