if(MSVC)
    add_definitions(-DFCAppPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(JtReaderGui_LIBS
    FreeCADGui
    JtReader
)

SET(JtReaderGui_SRCS
    AppJtReaderGui.cpp
    AppJtReaderGuiPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(JtReaderGui SHARED ${JtReaderGui_SRCS})
target_link_libraries(JtReaderGui ${JtReaderGui_LIBS})


fc_target_copy_resource(JtReaderGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/JtReader
    ${CMAKE_BINARY_DIR}/Mod/JtReader
    InitGui.py)

SET_BIN_DIR(JtReaderGui JtReaderGui /Mod/JtReader)
SET_PYTHON_PREFIX_SUFFIX(JtReaderGui)

INSTALL(TARGETS JtReaderGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
