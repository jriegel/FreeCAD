if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

if(MSVC)
    set(Path_LIBS
        Robot
        ${QT_QTCORE_LIBRARY}
        FreeCADApp
    )
else(MSVC)
    set(Path_LIBS
        Robot
        Part
        ${QT_QTCORE_LIBRARY}
        FreeCADApp
    )
endif(MSVC)

generate_from_xml(CommandPy)
generate_from_xml(PathPy)
generate_from_xml(ToolPy)
generate_from_xml(TooltablePy)

SET(Python_SRCS
    CommandPy.xml
    CommandPyImp.cpp
    PathPy.xml
    PathPyImp.cpp
    ToolPy.xml
    TooltablePy.xml
    TooltablePyImp.cpp
)

SET(Mod_SRCS
    AppPath.cpp
    AppPathPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(Path_SRCS
    Command.cpp
    Command.h
    Path.cpp
    Path.h
    Tooltable.cpp
    Tooltable.h
    PropertyPath.cpp
    PropertyPath.h
    FeaturePath.cpp
    FeaturePath.h
    PropertyTooltable.cpp
    PropertyTooltable.h
    FeaturePathCompound.cpp
    FeaturePathCompound.h
    FeaturePathShape.cpp
    FeaturePathShape.h
    ${Mod_SRCS}
    ${Python_SRCS}
)

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

if (WIN32)
    add_definitions(-DEIGEN2_SUPPORT)
    FILE( GLOB KDL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.cpp )
    FILE( GLOB KDL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.hpp
                        ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.inl)

    FILE( GLOB UTIL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cpp
                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cxx)
    FILE( GLOB UTIL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.h
                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.hpp)

    SET(Path_SRCS
        ${Path_SRCS}
        ${KDL_SRCS}
        ${KDL_HPPS}
        ${UTIL_SRCS}
        ${UTIL_HPPS}
    )

    SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )
endif(WIN32)

SET(Path_Scripts
    Init.py
)

add_library(Path SHARED ${Path_SRCS})
target_link_libraries(Path ${Path_LIBS})

fc_target_copy_resource(Path 
    ${CMAKE_SOURCE_DIR}/src/Mod/Path
    ${CMAKE_BINARY_DIR}/Mod/Path
    ${Path_Scripts})

fc_target_copy_resource(Path 
    ${CMAKE_SOURCE_DIR}/src/Mod/Path
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Path
    ${Path_Resources})

SET_BIN_DIR(Path Path /Mod/Path)
SET_PYTHON_PREFIX_SUFFIX(Path)

INSTALL(TARGETS Path DESTINATION ${CMAKE_INSTALL_LIBDIR})
