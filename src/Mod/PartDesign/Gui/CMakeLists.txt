if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(PartDesignGui_LIBS
    PartDesign
    SketcherGui
    PartGui
    SketcherGui
    FreeCADGui
)

set(PartDesignGui_MOC_HDRS
    TaskFeaturePick.h
    ReferenceSelection.h
    TaskSketchBasedParameters.h
    TaskPadParameters.h
    TaskPocketParameters.h
    TaskChamferParameters.h
    TaskFilletParameters.h
    TaskDraftParameters.h
    TaskThicknessParameters.h
    TaskDressUpParameters.h
    #TaskHoleParameters.h
    TaskRevolutionParameters.h
    TaskGrooveParameters.h
    TaskTransformedMessages.h
    TaskTransformedParameters.h
    TaskMirroredParameters.h
    TaskLinearPatternParameters.h
    TaskPolarPatternParameters.h
    TaskScaledParameters.h
    TaskMultiTransformParameters.h
    TaskDatumParameters.h
    TaskDatumShapeBinder.h
    TaskBooleanParameters.h
    TaskPrimitiveParameters.h
    TaskPipeParameters.h
    TaskLoftParameters.h
)
fc_wrap_cpp(PartDesignGui_MOC_SRCS ${PartDesignGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${PartDesignGui_MOC_SRCS})

qt4_add_resources(PartDesignGui_SRCS Resources/PartDesign.qrc)

set(PartDesignGui_UIC_SRCS
    TaskFeaturePick.ui
    TaskPadParameters.ui
    TaskPocketParameters.ui
    TaskChamferParameters.ui
    TaskFilletParameters.ui
    TaskDraftParameters.ui
    TaskThicknessParameters.ui
    TaskBooleanParameters.ui
    #TaskHoleParameters.ui
    TaskRevolutionParameters.ui
    TaskGrooveParameters.ui
    TaskTransformedMessages.ui
    TaskMirroredParameters.ui
    TaskLinearPatternParameters.ui
    TaskPolarPatternParameters.ui
    TaskScaledParameters.ui
    TaskMultiTransformParameters.ui
    TaskDatumParameters.ui
    TaskDatumShapeBinder.ui
    TaskPrimitiveParameters.ui
    TaskPipeParameters.ui
    TaskPipeOrientation.ui
    TaskPipeScaling.ui
    TaskLoftParameters.ui
    DlgReference.ui
)
qt4_wrap_ui(PartDesignGui_UIC_HDRS ${PartDesignGui_UIC_SRCS})

SET(PartDesignGuiViewProvider_SRCS CommandPrimitive.cpp
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderBody.cpp
    ViewProviderBody.h
    ViewProviderPad.cpp
    ViewProviderPad.h
    #ViewProviderHole.cpp
    #ViewProviderHole.h
    ViewProviderPocket.cpp
    ViewProviderPocket.h
    ViewProviderChamfer.cpp
    ViewProviderChamfer.h
    ViewProviderFillet.cpp
    ViewProviderFillet.h
    ViewProviderDraft.cpp
    ViewProviderDraft.h
    ViewProviderThickness.cpp
    ViewProviderThickness.h
    ViewProviderDressUp.cpp
    ViewProviderDressUp.h
    ViewProviderRevolution.cpp
    ViewProviderRevolution.h
    ViewProviderGroove.cpp
    ViewProviderGroove.h
    ViewProviderTransformed.cpp
    ViewProviderTransformed.h
    ViewProviderMirrored.cpp
    ViewProviderMirrored.h
    ViewProviderLinearPattern.cpp
    ViewProviderLinearPattern.h
    ViewProviderPolarPattern.cpp
    ViewProviderPolarPattern.h
    ViewProviderScaled.cpp
    ViewProviderScaled.h
    ViewProviderMultiTransform.cpp
    ViewProviderMultiTransform.h
    ViewProviderDatum.cpp
    ViewProviderDatum.h
    ViewProviderDatumPoint.cpp
    ViewProviderDatumPoint.h
    ViewProviderDatumLine.cpp
    ViewProviderDatumLine.h
    ViewProviderDatumPlane.cpp
    ViewProviderDatumPlane.h
    ViewProviderDatumCS.cpp
    ViewProviderDatumCS.h
    ViewProviderDatumShapeBinder.h
    ViewProviderDatumShapeBinder.cpp
    ViewProviderBoolean.cpp
    ViewProviderBoolean.h
    ViewProviderAddSub.cpp
    ViewProviderAddSub.h
    ViewProviderPrimitive.h
    ViewProviderPrimitive.cpp
    ViewProviderPipe.h
    ViewProviderPipe.cpp
    ViewProviderLoft.h
    ViewProviderLoft.cpp
)
SOURCE_GROUP("ViewProvider" FILES ${PartDesignGuiViewProvider_SRCS})

SET(PartDesignGuiTaskDlgs_SRCS
    TaskFeaturePick.ui
    TaskFeaturePick.cpp
    TaskFeaturePick.h
    ReferenceSelection.cpp
    ReferenceSelection.h
    TaskSketchBasedParameters.cpp
    TaskSketchBasedParameters.h
    TaskPadParameters.ui
    TaskPadParameters.cpp
    TaskPadParameters.h
    TaskPocketParameters.ui
    TaskPocketParameters.cpp
    TaskPocketParameters.h
    TaskChamferParameters.ui
    TaskChamferParameters.cpp
    TaskChamferParameters.h
    TaskFilletParameters.ui
    TaskFilletParameters.cpp
    TaskFilletParameters.h
    TaskDraftParameters.ui
    TaskDraftParameters.cpp
    TaskDraftParameters.h
    TaskThicknessParameters.ui
    TaskThicknessParameters.cpp
    TaskThicknessParameters.h
    TaskDressUpParameters.cpp
    TaskDressUpParameters.h
    TaskRevolutionParameters.ui
    TaskRevolutionParameters.cpp
    TaskRevolutionParameters.h
    TaskGrooveParameters.ui
    TaskGrooveParameters.cpp
    TaskGrooveParameters.h
    TaskTransformedMessages.ui
    TaskTransformedMessages.cpp
    TaskTransformedMessages.h
    TaskTransformedParameters.cpp
    TaskTransformedParameters.h
    TaskMirroredParameters.ui
    TaskMirroredParameters.cpp
    TaskMirroredParameters.h
    TaskLinearPatternParameters.ui
    TaskLinearPatternParameters.cpp
    TaskLinearPatternParameters.h
    TaskPolarPatternParameters.ui
    TaskPolarPatternParameters.cpp
    TaskPolarPatternParameters.h
    TaskScaledParameters.ui
    TaskScaledParameters.cpp
    TaskScaledParameters.h
    TaskMultiTransformParameters.ui
    TaskMultiTransformParameters.cpp
    TaskMultiTransformParameters.h
    #TaskHoleParameters.ui
    #TaskHoleParameters.cpp
    #TaskHoleParameters.h
    TaskDatumParameters.ui
    TaskDatumParameters.cpp
    TaskDatumParameters.h
    TaskDatumShapeBinder.ui
    TaskDatumShapeBinder.cpp
    TaskDatumShapeBinder.h
    TaskBooleanParameters.ui
    TaskBooleanParameters.cpp
    TaskBooleanParameters.h
    TaskPrimitiveParameters.h
    TaskPrimitiveParameters.cpp
    TaskPipeParameters.ui
    TaskPipeOrientation.ui
    TaskPipeScaling.ui
    TaskPipeParameters.h
    TaskPipeParameters.cpp
    TaskLoftParameters.ui
    TaskLoftParameters.h
    TaskLoftParameters.cpp
)
SOURCE_GROUP("TaskDialogs" FILES ${PartDesignGuiTaskDlgs_SRCS})

SET(PartDesignGuiModule_SRCS
    AppPartDesignGui.cpp
    AppPartDesignGuiPy.cpp
    Command.cpp
    CommandPrimitive.cpp
    Resources/PartDesign.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${PartDesignGuiModule_SRCS})

SET(PartDesignGui_Scripts
    InitGui.py
    TestPartDesignGui.py
)

SET(PartDesignGui_SRCS
    ${PartDesignGui_SRCS}
    ${PartDesignGui_UIC_HDRS}
    ${PartDesignGuiModule_SRCS}
    ${PartDesignGuiTaskDlgs_SRCS}
    ${PartDesignGuiViewProvider_SRCS}
)

add_library(PartDesignGui SHARED ${PartDesignGui_SRCS})
target_link_libraries(PartDesignGui ${PartDesignGui_LIBS})


fc_target_copy_resource(PartDesignGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/PartDesign
    ${CMAKE_BINARY_DIR}/Mod/PartDesign
    ${PartDesignGui_Scripts})

SET_BIN_DIR(PartDesignGui PartDesignGui /Mod/PartDesign)
SET_PYTHON_PREFIX_SUFFIX(PartDesignGui)

INSTALL(TARGETS PartDesignGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

