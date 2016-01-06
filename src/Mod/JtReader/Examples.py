# example call of the JtReader stuff

import FreeCAD,JtReader

App.newDocument()
Part = App.ActiveDocument.addObject("App::Part","TestPart")
JtPart = App.ActiveDocument.addObject("JtReader::JtPart","TestJtPart")
Part.Items = [JtPart]

#JtPart.JtFileName = App.getHomePath() + "../data/tests/Jt/Engine/2_Cylinder_Engine/Rocker_Mount_Cap_123_608_0_Parts.jt"
JtPart.JtFileName = "C:/temp/EngClientJt/fmsr_319a6047a01b11e58e25940bee48e670----.jt"