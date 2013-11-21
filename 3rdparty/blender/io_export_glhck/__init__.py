# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#  All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

# pylint: disable=C0103, C0111, E1101, F0401, R0903, W0232, W0142
# <pep8-80 compliant>

bl_info = {
    "name": "GLhck format",
    "author": "Jari Vetoniemi",
    "version": (0, 0, 1),
    "blender": (2, 69, 0),
    "location": "File > Export",
    "description": "Export to GLhck native model",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}

import bpy
from bpy.props import BoolProperty, EnumProperty, StringProperty, FloatProperty
from bpy_extras.io_utils import path_reference_mode
from bpy_extras.io_utils import ExportHelper

class ExportGlhck(bpy.types.Operator, ExportHelper):
    """Export to GLhck native model"""

    bl_idname = "export_scene.glhckm"
    bl_label = "Export GLhck"
    bl_options = {'PRESET'}

    filename_ext = ".glhckm"
    filter_glob = StringProperty(
            default="*.glhckm",
            options={'HIDDEN'})

    path_mode = path_reference_mode
    check_extension = True

    use_selection = BoolProperty(
        name="Selection Only",
        description="Export selected objects only",
        default=False)

    use_animations = BoolProperty(
        name="Animations",
        description="Export object and bone animations",
        default=True)

    bake_animations = BoolProperty(
        name="Bake Animations",
        description="Run bake action on each action that needs it (SLOW!" \
                " Use only when exported animation is wrong without)",
        default=False)

    split_animations = BoolProperty(
        name="Split Animations",
        description="Export each animation to own file (.glhcka)",
        default=False)

    use_bones = BoolProperty(
        name="Include Bones",
        description="",
        default=True)

    use_rest_pose = BoolProperty(
        name="Include Rest Pose",
        description="Export bones in their rest pose (for animation)",
        default=True)

    use_mesh_modifiers = BoolProperty(
        name="Apply Modifiers",
        description="Apply modifiers (preview resolution)",
        default=True)

    use_normals = BoolProperty(
        name="Include Normals",
        description="",
        default=True)

    use_uvs = BoolProperty(
        name="Include UVs",
        description="Write out the active UV coordinates",
        default=True)

    use_vertex_colors = BoolProperty(
        name="Include Vertex Colors",
        description="",
        default=False)

    use_materials = BoolProperty(
        name="Include Materials",
        description="Export object materials and textures",
        default=True)

    axis_forward = EnumProperty(
        name="Forward",
        items=(('X', "X Forward", ""),
               ('Y', "Y Forward", ""),
               ('Z', "Z Forward", ""),
               ('-X', "-X Forward", ""),
               ('-Y', "-Y Forward", ""),
               ('-Z', "-Z Forward", ""),
               ),
        default='-Z')

    axis_up = EnumProperty(
        name="Up",
        items=(('X', "X Up", ""),
               ('Y', "Y Up", ""),
               ('Z', "Z Up", ""),
               ('-X', "-X Up", ""),
               ('-Y', "-Y Up", ""),
               ('-Z', "-Z Up", ""),
               ),
        default='Y')

    global_scale = FloatProperty(
        name="Scale",
        min=0.01, max=1000.0,
        default=1.0)

    def execute(self, context):
        from . import export_glhckm
        from mathutils import Matrix
        from bpy_extras.io_utils import axis_conversion

        keywords = self.as_keywords(ignore=("axis_forward",
                                            "axis_up",
                                            "global_scale",
                                            "check_existing",
                                            "filter_glob"))

        global_matrix = (Matrix.Scale(self.global_scale, 4) *
                         axis_conversion(to_forward=self.axis_forward,
                                         to_up=self.axis_up).to_4x4())

        keywords["global_matrix"] = global_matrix

        if 0:
            import cProfile
            cProfile.runctx('export_glhckm.save(context, **keywords)',
                    globals(), locals())
            return {'FINISHED'}
        return export_glhckm.save(context, **keywords)

def menu_func_export(self, context):
    self.layout.operator(ExportGlhck.bl_idname, text="GLhck (.glhckm/.glhcka)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()

# vim: set ts=8 sw=4 tw=0 :
