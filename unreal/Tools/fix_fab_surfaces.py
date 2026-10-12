"""Repair open scan backs and box UV projection without touching other materials."""
import unreal as ue
for name in ['Nordic','Ice']:
    mat=ue.load_asset('/Game/Materials/M_Fab_'+name)
    assert mat
    mat.set_editor_property('two_sided',True)
    ue.MaterialEditingLibrary.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat)
for name in ['Wood','Rust']:
    mat=ue.load_asset('/Game/Materials/M_Fab_'+name)
    normal=next(n for n in ue.MaterialEditingLibrary.get_material_expressions(mat) if isinstance(n,ue.MaterialExpressionVertexNormalWS))
    for node in ue.MaterialEditingLibrary.get_material_expressions(mat):
        if isinstance(node,ue.MaterialExpressionCustom) and node.get_editor_property('code')=='return P.xy*.004;':
            inputs=list(node.get_editor_property('inputs'));entry=ue.CustomInput();entry.set_editor_property('input_name','N');inputs.append(entry);node.set_editor_property('inputs',inputs)
            ue.MaterialEditingLibrary.connect_material_expressions(normal,'',node,'N')
            node.set_editor_property('code','float3 A=abs(N); return (A.z>.6 ? P.xy : A.x>A.y ? P.zy : P.xz)*.004;')
    ue.MaterialEditingLibrary.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat)
ue.log('BENCHY_FAB_SURFACES_FIXED')
