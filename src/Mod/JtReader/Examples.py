# example call of the JtReader stuff

import FreeCAD,JtReader

Tiny = App.getHomePath() + "../data/tests/Jt/Engine/2_Cylinder_Engine/Rocker_Arm_Shaft_123_601_0_Parts.jt"
Mid = App.getHomePath() + "../data/tests/Jt/Engine/2_Cylinder_Engine/Rocker_Mount_Cap_123_608_0_Parts.jt"
Eng = "C:/temp/EngClientJt/fmsr_319a6047a01b11e58e25940bee48e670----.jt"
EngRef = App.getHomePath() + "../data/tests/Jt/Engine/2_Cylinder_Engine.jt"
Vis = "C:/temp/V-Jt/fmsr_bb3c3a9f612c11e59d252c44fd840378----.jt"

App.newDocument()
PartObj = App.ActiveDocument.addObject("App::Part","TestPart")

JtReader.readJtPart(Eng,PartObj)


