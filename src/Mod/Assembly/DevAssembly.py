import FreeCAD,Assembly,time

print "Script to test the assembly development"

# sequence to test recompute behaviour
#       L1---\
#      /  \   \
#    L2   L3   \
#   /  \ /  \  /
#  L4   L5   L6

Doc = FreeCAD.newDocument("RecomputeTests")
L1 = Doc.addObject("App::FeatureTest","Label_1")
L2 = Doc.addObject("App::FeatureTest","Label_2")
L3 = Doc.addObject("App::FeatureTest","Label_3")
L4 = Doc.addObject("App::FeatureTest","Label_4")
L5 = Doc.addObject("App::FeatureTest","Label_5")
L6 = Doc.addObject("App::FeatureTest","Label_6")
L1.LinkList = [L2,L3,L6]
L2.Link = L4
L2.LinkList = [L5]
L3.LinkList = [L5,L6]

(0, 0, 0, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount)
Doc.recompute()==3
(1, 1, 1, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount)
L5.touch()
(1, 1, 1, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount)
Doc.recompute()==4
(2, 2, 2, 0, 1, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L4.touch()
Doc.recompute()==3
(3, 3, 2, 1, 1, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L5.touch()
Doc.recompute()==4
(4, 4, 3, 1, 2, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L6.touch()
Doc.recompute()==3
(5, 4, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L2.touch()
Doc.recompute()==2
(6, 5, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L1.touch()
Doc.recompute()==1
(7, 5, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount) 
L6.Link = L1 # create a circular dependency
Doc.recompute()==-1
L6.Link = None  # resolve the circular dependency
Doc.recompute()==3

