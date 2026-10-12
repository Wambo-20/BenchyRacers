"""Create only the lightweight cosmetic item material; preserve world assets."""
import sys
from pathlib import Path
import unreal as ue
sys.path.insert(0, str(Path(ue.Paths.project_dir()) / "Tools"))
from bootstrap_unreal import material, expression, custom, result, finish
m = material("M_ItemGlow")
m.set_editor_property("blend_mode", ue.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
v = expression(m, ue.MaterialExpressionVertexColor)
uv = expression(m, ue.MaterialExpressionTextureCoordinate)
exposure = expression(m, ue.MaterialExpressionEyeAdaptationInverse)
# Maintain legible FX from bright tropical daylight to the sunset course.
result(custom(m, "return C*E*2.2;", {"C": v, "E": exposure}), m, ue.MaterialProperty.MP_EMISSIVE_COLOR)
result(custom(m, "return A*pow(saturate(1-abs(UV.x*2-1)),1.4);",
              {"A": (v,"A"), "UV": uv}, ue.CustomMaterialOutputType.CMOT_FLOAT1), m, ue.MaterialProperty.MP_OPACITY)
finish(m)
ue.log("BENCHY_ITEM_MATERIAL_READY")
