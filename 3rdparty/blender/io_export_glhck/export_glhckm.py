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

# pylint: disable=C1001, C0111, F0401, R0903, R0912, R0913, R0914, R0915, R0902
# pylint: disable=W0142
# <pep8-80 compliant>

import bpy, os, struct
from bpy_extras.io_utils import path_reference, path_reference_copy
from itertools import zip_longest
from mathutils import Matrix

# File headers
GLHCKM_VERSION = [0, 1]
GLHCKM_HEADER = "glhckm"
GLHCKA_HEADER = "glhcka"

# Map geometryType enum
ENUM_GEOMETRYTYPE = {
    'TRIANGLES':0,
}

# Map materialFlags bitflags
ENUM_MATERIALFLAGS = {
    'LIGHTING': 1 << 0
}

# Map vertexDataFlags bitflags
ENUM_VERTEXDATAFLAGS = {
    'HAS_NORMALS':       1 << 0,
    'HAS_UVS':           1 << 1,
    'HAS_VERTEX_COLORS': 1 << 2,
}

# Binary sizes for datatypes
SZ_INT8  = 1
SZ_INT16 = 2
SZ_INT32 = 4
SZ_COLOR3 = SZ_INT8 * 3
SZ_COLOR4 = SZ_INT8 * 4

# Return string from float without trailing zeroes
def strflt(flt):
    return ("{:.6f}".format(flt)).rstrip("0").rstrip(".")

# Binary size of string
def sz_string(string):
    return SZ_INT8 + len(string.encode('UTF-8'))

# Binary size of long string
def sz_longstring(string):
    return SZ_INT16 + len(string.encode('UTF-8'))

# Binary size of string float
def sz_float(flt):
    return sz_string(strflt(flt))

# Binary size of string vector2d
def sz_vector2(vector):
    return sz_string("{},{}".format(*[strflt(x) for x in vector]))

# Binary size of string vector3d
def sz_vector3(vector):
    return sz_string("{},{},{}".format(*[strflt(x) for x in vector]))

# Binary size of string quaternion
def sz_quaternion(quaternion):
    return sz_string("{},{},{},{}".format(*[strflt(x) for x in quaternion]))

# Binary size of string matrix4x4
def sz_matrix4x4(matrix):
    return sz_longstring("{},{},{},{} " \
                         "{},{},{},{} " \
                         "{},{},{},{} " \
                         "{},{},{},{} ".format(
                             *[strflt(x) for y in matrix for x in y]))

# Binary size of material
def sz_material(mtl):
    return sz_string(mtl.name) + \
           SZ_COLOR3 + SZ_COLOR4 + SZ_COLOR4 + SZ_INT16 + \
           SZ_INT8 + sz_longstring(mtl.textures['diffuse'])

# Binary size of skinbone
def sz_skinbone(bone):
    size = sz_string(bone.aobj.name + "_" + bone.name) + \
           sz_matrix4x4(bone.offset_matrix) + SZ_INT32
    for weight in bone.weights:
        size += SZ_INT32 + sz_float(weight)
    return size

# Binary size of bone
def sz_bone(eobj, bone, matrix):
    return sz_string(eobj.name + "_" + bone.name) + \
           sz_matrix4x4(matrix) + SZ_INT16

# Binary size of node
def sz_node(node):
    size = sz_string(node.name) + SZ_INT32 + SZ_INT32 + SZ_INT32
    for itr in node.rotation_keys:
        size += SZ_INT32 + sz_quaternion(itr[1])
    for itr in node.scaling_keys:
        size += SZ_INT32 + sz_vector3(itr[1])
    for itr in node.translation_keys:
        size += SZ_INT32 + sz_vector3(itr[1])
    return size

# Write block header
def write_block(file, block_name, block_size):
    file.write(bytes(block_name, 'ascii')) # header
    file.write(struct.pack("<I", block_size)) # block size

# Write color3 as binary
def write_color3(file, color):
    file.write(struct.pack("<BBB", *color)) # rgb

# Write color4 as binary
def write_color4(file, color):
    file.write(struct.pack("<BBBB", *color)) # rgba

# Write string with binary length
def write_string(file, string):
    file.write(struct.pack("<B", len(string.encode('UTF-8')))) # length
    file.write(bytes(string, 'UTF-8')) # char array

# Write long string with binary length
def write_longstring(file, string):
    file.write(struct.pack("<H", len(string.encode('UTF-8')))) # length
    file.write(bytes(string, 'UTF-8')) # char array

# Write float to file as string
def write_float(file, flt):
    write_string(file, strflt(flt))

# Write vector2d to file as string
def write_vector2(file, vector):
    write_string(file, "{},{}".format(*[strflt(x) for x in vector]))

# Write vector3d to file as string
def write_vector3(file, vector):
    write_string(file, "{},{},{}".format(*[strflt(x) for x in vector]))

# Write quaternion to file as string
def write_quaternion(file, quaternion):
    write_string(file, "{},{},{},{}".format(strflt(quaternion[1]),
        strflt(quaternion[2]), strflt(quaternion[3]), strflt(quaternion[0])))

# Write matrix4x4 to file as string
def write_matrix4x4(file, matrix):
    write_longstring(file, "{},{},{},{} " \
                           "{},{},{},{} " \
                           "{},{},{},{} " \
                           "{},{},{},{} ".format(
                               *[strflt(x) for y in matrix for x in y]))

# Write material to file as binary
def write_material(file, mtl):
    write_string(file, mtl.name) # name
    write_color3(file, mtl.ambient) # ambient
    write_color4(file, mtl.diffuse) # diffuse
    write_color4(file, mtl.specular) # specular
    file.write(struct.pack("<H", mtl.shininess)) # shininess
    file.write(struct.pack("<B", mtl.flags)) # materialFlags
    write_longstring(file, mtl.textures['diffuse']) # diffuse

# Write skinbone to file as binary
def write_skinbone(file, bone):
    write_string(file, bone.aobj.name + "_" + bone.name) # name
    write_matrix4x4(file, bone.offset_matrix) # offsetMatrix
    file.write(struct.pack("<I", len(bone.weights))) # weightCount
    for index, weight in zip_longest(bone.indices, bone.weights): # weights
        file.write(struct.pack("<I", index)) # vertexIndex
        write_float(file, weight) # weight

# Write node to file as binary
def write_node(file, node):
    write_string(file, node.name) # name
    file.write(struct.pack("<I", len(node.rotation_keys))) # rotationCount
    file.write(struct.pack("<I", len(node.scaling_keys))) # scalingCount
    file.write(struct.pack("<I", len(node.translation_keys))) # translationCount

    # quaternionKeys
    for frame, key in node.rotation_keys:
        file.write(struct.pack("<I", frame)) # frame
        write_quaternion(file, key) # quaternion

    # scalingKeys
    for frame, key in node.scaling_keys:
        file.write(struct.pack("<I", frame)) # frame
        write_vector3(file, key) # vector

    # translationKeys
    for frame, key in node.translation_keys:
        file.write(struct.pack("<I", frame)) # frame
        write_vector3(file, key) # vector

# Almost equality check of floating point
def almost_equal(aflt, bflt, error=0.0001):
    return (aflt + error > bflt and aflt - error < bflt)

# Almost equality check of 3d vector
def ae3d(vec1, vec2, error=0.0001):
    return (almost_equal(vec1[0], vec2[0], error) and
            almost_equal(vec1[1], vec2[1], error) and
            almost_equal(vec1[2], vec2[2], error))

# Almost equality check of quaternion
def aeqt(qat1, qat2, error=0.0001):
    return (ae3d(qat1, qat2, error) and almost_equal(qat1[3], qat2[3], error))

# Round 2d vector
def r2d(vec, num=6):
    return round(vec[0], num), round(vec[1], num)

# Round 3d vector
def r3d(vec, num=6):
    return r2d(vec) + (round(vec[2], num),)

# Round quaternion
def rqt(qat, num=6):
    return r3d(qat) + (round(qat[3], num),)

# Clamp to range
def clamp(val, minimum, maximum):
    return max(minimum, min(val, maximum))

# Blender color to RGB unsigned byte color
def bc3b(bcol):
    return clamp(round(bcol[0] * 255), 0, 255), \
           clamp(round(bcol[1] * 255), 0, 255), \
           clamp(round(bcol[2] * 255), 0, 255)

# Blender color to RGBA unsigned byte color
def bc4b(bcol):
    return bc3b(bcol) + (clamp(round(bcol[3] * 255), 0, 255),)

# Sort list with each element name field
def sort_by_name_field(lst):
    def sort_key(obj):
        return obj.name
    return sorted(lst, key=sort_key)

class Material:
    def __init__(self, bmtl, mesh, options):
        self.bmtl = bmtl
        self.name = bmtl.name

        images = Material._get_texture_images(bmtl)
        if images['diffuse'] is None and len(mesh.uv_textures):
            images['diffuse'] = mesh.uv_textures.active.data[:][0].image

        self.textures = {}
        src_dir = os.path.dirname(bpy.data.filepath)
        dst_dir = os.path.dirname(options['filepath'])
        for key in images.keys():
            if images[key] is not None:
                self.textures[key] = path_reference(images[key].filepath,
                        src_dir, dst_dir, options['path_mode'], "",
                        options['copy_set'], images[key].library)
            else:
                self.textures[key] = ''

        self.ambient = bc3b(bmtl.ambient * bmtl.diffuse_color)

        self.diffuse = list(bmtl.diffuse_intensity * bmtl.diffuse_color)
        self.diffuse.append(bmtl.alpha)
        self.diffuse = bc4b(self.diffuse)

        self.specular = list(bmtl.specular_intensity * bmtl.specular_color)
        self.specular.append(bmtl.specular_alpha)
        self.specular = bc4b(self.specular)

        self.shininess = bmtl.specular_hardness

        self.flags = 0
        if bmtl.use_shadeless:
            self.flags |= ENUM_MATERIALFLAGS['LIGHTING']

    @staticmethod
    def _get_texture_images(mtl):
        """Get relevant images from material"""

        images = {'diffuse':None,
                  'normal':None,
                  'displacement':None,
                  'reflection':None,
                  'ambient':None,
                  'alpha':None,
                  'translucency':None,
                  'emit':None,
                  'specular_intensity':None,
                  'specular_color':None,
                  'specular_hardness':None}

        # Create a list of textures that have type 'IMAGE'
        textures = [mtl.texture_slots[tslot]
                for tslot in mtl.texture_slots.keys()
                if mtl.texture_slots[tslot].texture is not None
                and mtl.texture_slots[tslot].texture.type == 'IMAGE']
        textures = [tex for tex in textures if tex.texture.image is not None]

        for tex in textures:
            image = tex.texture.image
            if tex.use_map_color_diffuse and not tex.use_map_warp and \
               tex.texture_coords != 'REFLECTION':
                images['diffuse'] = image
            if tex.use_map_normal:
                images['normal'] = image
            if tex.use_map_displacement:
                images['displacement'] = image
            if tex.use_map_color_diffuse and tex.texture_coords == 'REFLECTION':
                images['reflection'] = image
            if tex.use_map_ambient:
                images['ambient'] = image
            if tex.use_map_alpha:
                images['alpha'] = image
            if tex.use_map_translucency:
                images['translucency'] = image
            if tex.use_map_emit:
                images['emit'] = image
            if tex.use_map_specular:
                images['specular'] = image
            if tex.use_map_color_spec:
                images['specular_color'] = image
            if tex.use_map_hardness:
                images['specular_hardness'] = image

        return images

def materials_from_blender_mesh(mesh, options):
    """Get materials from blender object"""

    materials = []
    for mtl in mesh.materials:
        if mtl is not None:
            materials.append(Material(mtl, mesh, options))
    return materials

class SkinBone:
    def __init__(self, bobj, aobj, name, options):
        self.aobj = aobj
        self.name = name
        self.bone = aobj.data.bones[name]

        self.indices = []
        self.weights = []

        # BoneMatrix transforms mesh vertices into the space of the bone.
        # Here are the final transformations in order:
        #  - Object Space to World Space
        #  - World Space to Armature Space
        #  - Armature Space to Bone Space
        # This way, when matrix is transformed by the bone's Frame matrix,
        # the vertices will be in their final world position.

        amat = options['global_matrix'] * aobj.matrix_world
        self.offset_matrix = self.bone.matrix_local.inverted()
        self.offset_matrix *= (bobj.matrix_local * amat).inverted()
        self.offset_matrix *= bobj.matrix_local

    def add_vertex(self, index, weight):
        self.indices.append(index)
        self.weights.append(weight)

def skin_bones_from_blender_object(bobj, options):
    """Get bone vertex groups from blender object"""

    armature_modifier_list = [modifier for modifier in bobj.modifiers
        if modifier.type == 'ARMATURE' and modifier.show_viewport]

    armature_objects = [modifier.object for modifier in armature_modifier_list
            if modifier.object is not None]

    skin_bones = []
    for aobj in armature_objects:
        # Determine the names of the bone vertex groups
        pose_bone_names = [bone.name for bone in aobj.pose.bones]
        vertex_group_names = [group.name for group in bobj.vertex_groups]
        used_bone_names = set(pose_bone_names).intersection(vertex_group_names)

        # Create a SkinBone for each group name
        skin_bones.extend([SkinBone(bobj, aobj, bone_name, options)
            for bone_name in used_bone_names])

    return skin_bones

def blender_object_to_mesh(context, bobj, options, has_bones=False):
    """Convert blender object to mesh for export"""

    if options['use_mesh_modifiers']:
        # Certain modifiers shouldn't be applied in some cases
        # Deactivate them until after mesh generation is complete

        deactivated_modifier_list = []

        # If we're exporting armature data, we shouldn't apply
        # armature modifiers to the mesh
        if has_bones:
            deactivated_modifier_list = [modifier for modifier in bobj.modifiers
                if modifier.type == 'ARMATURE' and modifier.show_viewport]

        for modifier in deactivated_modifier_list:
            modifier.show_viewport = False

        mesh = bobj.to_mesh(context.scene, True, 'PREVIEW')

        # Restore the deactivated modifiers
        for modifier in deactivated_modifier_list:
            modifier.show_viewport = True
    else:
        mesh = bobj.to_mesh(context.scene, False, 'PREVIEW')

    # finally transform
    mesh.transform(options['global_matrix'] * bobj.matrix_world)
    return mesh

def blender_object_to_data(context, bobj, options):
    """Turn blender object to bunch of data usable for our export"""

    vertices = []
    normals = []
    uvs = []
    vertex_colors = []
    indices = []
    materials = []
    skin_bones = []
    used_bones = []

    if bobj.type == 'EMPTY' or bobj.data is None:
        return {'vertices':vertices,
                'normals':normals,
                'uvs':uvs,
                'vertex_colors':vertex_colors,
                'indices':indices,
                'materials':materials,
                'skin_bones':skin_bones}

    if options['use_bones']:
        skin_bones = skin_bones_from_blender_object(bobj, options)
        vertex_group_map = {group.index : group for group in bobj.vertex_groups}

    # helper function for getting all the skin bones for vertex group
    def get_skin_bones_for_vgroup(vgroup):
        bones = []
        vertex_group = vertex_group_map[vgroup.group]
        for bone in skin_bones:
            if vertex_group.name == bone.name:
                bones.append(bone)
        return bones

    mesh = blender_object_to_mesh(context, bobj, options, len(skin_bones))

    if options['use_materials']:
        materials = materials_from_blender_mesh(mesh, options)

    active_uvs = None
    if options['use_uvs'] and len(mesh.tessface_uv_textures) > 0:
        active_uvs = mesh.tessface_uv_textures.active.data

    active_vertex_colors = None
    if options['use_vertex_colors'] and len(mesh.vertex_colors) > 0:
        active_vertex_colors = mesh.vertex_colors.active.data

    vertex_count = 0
    stored_data = {}
    for fidx, face in enumerate(mesh.tessfaces):
        tmp_faces = []
        for fvidx, vidx in enumerate(face.vertices):
            vertex = r3d(mesh.vertices[vidx].co)

            if options['use_normals']:
                if face.use_smooth:
                    normal = mesh.vertices[vidx].normal
                else:
                    normal = face.normal
                normal = r3d(normal)
            else:
                normal = (0.0, 0.0, 0.0)

            if active_uvs is not None:
                uvc = r2d(active_uvs[fidx].uv[fvidx])
            else:
                uvc = (0.0, 0.0)

            if active_vertex_colors is not None:
                color = bc4b(active_vertex_colors[vidx].color + (1.0,))
            else:
                color = (0, 0, 0, 0)

            # Get total weight for vertex and number of influences
            influences = 0
            weight_total = 0.0
            if len(skin_bones):
                for vgroup in mesh.vertices[vidx].groups:
                    bones = get_skin_bones_for_vgroup(vgroup)
                    for bone in bones:
                        weight_total += vgroup.weight
                        influences += 1

            # Check for duplicate vertex
            key = vertex, normal, uvc, color, round(weight_total, 6), influences
            duplicate_index = stored_data.get(key)

            if (duplicate_index is None):
                # Store new vertex
                stored_data[key] = vertex_count
                vertices.append(vertex)
                if options['use_normals']:
                    normals.append(normal)
                if active_uvs is not None:
                    uvs.append(uvc)
                if active_vertex_colors is not None:
                    vertex_colors.append(color)

                # Add vertex to BoneGroup if it's in any
                if influences:
                    for vgroup in mesh.vertices[vidx].groups:
                        bones = get_skin_bones_for_vgroup(vgroup)
                        for bone in bones:
                            if bone not in used_bones:
                                used_bones.append(bone)
                            weight = vgroup.weight / weight_total
                            bone.add_vertex(vertex_count, weight)

                tmp_faces.append(vertex_count)
                vertex_count += 1
            else:
                # Reuse the vertex
                tmp_faces.append(duplicate_index)

        # Is the format already triangles?
        if len(tmp_faces) == 3:
            indices.append(tmp_faces)
        else:
            indices.append([tmp_faces[0], tmp_faces[1], tmp_faces[2]])
            indices.append([tmp_faces[0], tmp_faces[2], tmp_faces[3]])

    bpy.data.meshes.remove(mesh)
    return {'vertices':vertices,
            'normals':normals,
            'uvs':uvs,
            'vertex_colors':vertex_colors,
            'indices':indices,
            'materials':materials,
            'skin_bones':used_bones}

class Node:
    def __init__(self, name):
        self.name = name
        self.rotation_keys = []
        self.scaling_keys = []
        self.translation_keys = []

class ExportAnimation:
    def __init__(self, action):
        self.name = action.name
        self.action = action
        self.first_frame = round(action.frame_range[0])
        self.last_frame = round(action.frame_range[1])
        self.objects = []

    def __repr__(self):
        return "[ExportAnimation: {}]".format(self.name)

    def add_object(self, eobj):
        """Add object relevant to this animation, this also
           means child objects! ExportAnimation should never
           enumerate each object's children"""

        if eobj not in self.objects:
            self.objects.append(eobj)

    def _generate_nodes(self, context, options):
        """Generate nodes for animation"""

        # We bake only the selected objects!
        # Thus select the objects relevant to this animation
        if options['bake_animations']:
            old_layers = context.scene.layers[:]
            old_active = bpy.context.scene.objects.active
            old_selection = context.selected_objects

            # Select all relevant objects
            context.scene.layers = [True for l in old_layers]
            for bobj in old_selection:
                bobj.select = False
            for eobj in self.objects:
                if eobj.bobj.type != 'ARMATURE':
                    # Only armatures need baking(?)
                    eobj.baked_action = eobj.action
                    continue

                eobj.bobj.select = True
                bpy.context.scene.objects.active = eobj.bobj
                eobj.bobj.animation_data.action = self.action

                from bpy_extras.anim_utils import bake_action as bake
                eobj.baked_action = bake(self.first_frame, self.last_frame, 1,
                        only_selected=False,
                        do_pose=True,
                        do_object=False,
                        do_visual_keying=True,
                        do_constraint_clear=False,
                        do_parents_clear=False,
                        do_clean=True,
                        action=None)

                eobj.bobj.select = False

            # Restore selection and layers
            for bobj in old_selection:
                bobj.select = True
            bpy.context.scene.objects.active = old_active
            context.scene.layers = old_layers

        # No baking
        if not options['bake_animations']:
            for eobj in self.objects:
                eobj.baked_action = self.action

        # Something failed, fallback
        for eobj in self.objects:
            if eobj.baked_action is None:
                for eobj2 in self.objects:
                    eobj2.bobj.animation_data.action = eobj2.action
                return []

        def generate_nodes_from_armature(eobj):
            """Generate nodes for animation from Armature ExportObject"""
            nodes = [Node(eobj.name + "_" + bone.name)
                    for bone in eobj.bobj.pose.bones]

            old_rotation = {}
            old_scaling = {}
            old_translation = {}
            old_rotation_mode = {}
            for bone in eobj.bobj.pose.bones:
                old_rotation[bone.name] = [1, 0, 0, 0]
                old_scaling[bone.name] = [1, 1, 1]
                old_translation[bone.name] = [0, 0, 0]
                old_rotation_mode[bone.name] = bone.rotation_mode
                bone.rotation_mode = 'QUATERNION'

            for frame in range(self.first_frame, self.last_frame + 1):
                context.scene.frame_set(frame)
                for bone, node in zip_longest(eobj.bobj.pose.bones, nodes):
                    matrix = bone.matrix
                    if bone.parent:
                        matrix = bone.parent.matrix.inverted() * matrix

                    rotation = rqt(bone.bone.matrix.to_quaternion() *
                            bone.rotation_quaternion)
                    scaling = r3d(matrix.to_scale())
                    translation = r3d(matrix.to_translation())

                    if not aeqt(old_rotation[bone.name], rotation):
                        node.rotation_keys.append([frame, rotation])
                        old_rotation[bone.name] = rotation

                    if not ae3d(old_scaling[bone.name], scaling):
                        node.scaling_keys.append([frame, scaling])
                        old_scaling[bone.name] = scaling

                    if not ae3d(old_translation[bone.name], translation):
                        node.translation_keys.append([frame, translation])
                        old_translation[bone.name] = translation

            for bone in eobj.bobj.pose.bones:
                bone.rotation_mode = old_rotation_mode[bone.name]

            return nodes

        def generate_nodes_from_object(eobj):
            """Generate nodes for animation from Object ExportObject"""

            nodes = []
            old_rotation = [1, 0, 0, 0]
            old_scaling = [1, 1, 1]
            old_translation = [0, 0, 0]

            node = Node(eobj.name)
            nodes.append(node)
            for frame in range(self.first_frame, self.last_frame + 1):
                context.scene.frame_set(frame)

                rotation = rqt(eobj.bobj.rotation_quaternion)
                scaling = r3d(eobj.bobj.matrix_local.to_scale())
                translation = r3d(eobj.bobj.matrix_local.to_translation())

                if not aeqt(old_rotation, rotation):
                    node.rotation_keys.append([frame, rotation])
                    old_rotation = rotation

                if not ae3d(old_scaling, scaling):
                    node.scaling_keys.append([frame, scaling])
                    old_scaling = scaling

                if not ae3d(old_translation, translation):
                    node.translation_keys.append([frame, translation])
                    old_translation = translation

            return nodes

        nodes = []
        for eobj in self.objects:
            old_rotation_mode = eobj.bobj.rotation_mode
            eobj.bobj.rotation_mode = 'QUATERNION'
            eobj.bobj.animation_data.action = eobj.baked_action

            if eobj.bobj.type == 'ARMATURE':
                nodes.extend(generate_nodes_from_armature(eobj))
            else:
                nodes.extend(generate_nodes_from_object(eobj))

            eobj.bobj.animation_data.action = eobj.action
            eobj.bobj.rotation_mode = old_rotation_mode

            if eobj.baked_action is not self.action:
                bpy.data.actions.remove(eobj.baked_action)
            eobj.baked_action = None

        return nodes

    def write(self, context, file, options):
        """Write animation data to file"""

        old_frame = context.scene.frame_current
        nodes = self._generate_nodes(context, options)
        context.scene.frame_set(old_frame)

        node_data_size = 0
        for node in nodes:
            node_data_size += sz_node(node)

        block_size  = sz_string(self.name) # name
        block_size += SZ_INT32 # nodeCount
        block_size += node_data_size # nodes

        print("struct AND (size: {}) {{".format(block_size))
        print("   name          = {} (size: {})".format(self.name, sz_string(self.name)))
        print("   nodeCount     = {} (size: {})".format(len(nodes), SZ_INT32))
        print("   nodes         = stripped (size: {})".format(node_data_size))
        print("};")

        write_block(file, "AND", block_size) # header
        write_string(file, self.name) # name
        file.write(struct.pack("<I", len(nodes))) # nodeCount
        for node in nodes:
            write_node(file, node) # nodes

class ExportObject:
    def __init__(self, bobj):
        self.bobj = bobj
        self.name = bobj.name
        self.parent = None
        self.children = []

        # workaround to remember action after baking one for export
        # when you run bake_action it seems to set the baked action active
        if bobj.animation_data is not None:
            self.action = bobj.animation_data.action
            self.baked_action = None

    def __repr__(self):
        return "[ExportObject: {} '{}']".format(self.name, self.bobj.type)

    def _write_object(self, context, file, options):
        """Write object to file"""

        data = blender_object_to_data(context, self.bobj, options)
        vertices = data['vertices']
        normals = data['normals']
        uvs = data['uvs']
        vertex_colors = data['vertex_colors']
        indices = data['indices']
        materials = data['materials']
        skin_bones = data['skin_bones']

        geometry_type = ENUM_GEOMETRYTYPE['TRIANGLES']

        vertex_data_flags = 0
        if len(normals):
            vertex_data_flags |= ENUM_VERTEXDATAFLAGS['HAS_NORMALS']
        if len(uvs):
            vertex_data_flags |= ENUM_VERTEXDATAFLAGS['HAS_UVS']
        if len(vertex_colors):
            vertex_data_flags |= ENUM_VERTEXDATAFLAGS['HAS_VERTEX_COLORS']

        vertex_data_size = 0
        for vertex in vertices:
            vertex_data_size += sz_vector3(vertex)
        for normal in normals:
            vertex_data_size += sz_vector3(normal)
        for uvc in uvs:
            vertex_data_size += sz_vector2(uvc)
        vertex_data_size += len(vertex_colors) * SZ_COLOR3
        index_data_size = len(indices) * 3 * SZ_INT32

        material_data_size = 0
        for mtl in materials:
            material_data_size += sz_material(mtl)

        bone_data_size = 0
        for bone in skin_bones:
            bone_data_size += sz_skinbone(bone)

        block_size  = sz_string(self.name) # name
        block_size += SZ_INT8 # geometryType
        block_size += SZ_INT8 # vertexDataFlags
        block_size += SZ_INT32 # indexCount
        block_size += SZ_INT32 # vertexCount
        block_size += SZ_INT16 # materialCount
        block_size += SZ_INT16 # skinBoneCount
        block_size += SZ_INT16 # childCount
        block_size += index_data_size # indices
        block_size += vertex_data_size # vertices
        block_size += material_data_size # materials
        block_size += bone_data_size # skinBones

        print("struct OBD (size: {}) {{".format(block_size))
        print("   name          = {} (size: {})".format(self.name, sz_string(self.name)))
        print("   geometryType  = {} (size: {})".format(geometry_type, SZ_INT8))
        print("   vrtxDataFlags = {} (size: {})".format(vertex_data_flags, SZ_INT8))
        print("   indexCount    = {} (size: {})".format(len(indices)*3, SZ_INT32))
        print("   vertexCount   = {} (size: {})".format(len(vertices), SZ_INT32))
        print("   materialCount = {} (size: {})".format(len(materials), SZ_INT16))
        print("   skinBoneCount = {} (size: {})".format(len(skin_bones), SZ_INT16))
        print("   childCount    = {} (size: {})".format(len(self.children), SZ_INT16))
        print("   indices       = stripped (size: {})".format(index_data_size))
        print("   vertices      = stripped (size: {})".format(vertex_data_size))
        print("   materials     = stripped (size: {})".format(material_data_size))
        print("   skinBones     = stripped (size: {})".format(bone_data_size))
        print("};")

        write_block(file, "OBD", block_size) # header
        write_string(file, self.name) # name
        file.write(struct.pack("<B", geometry_type)) # geometryType
        file.write(struct.pack("<B", vertex_data_flags)) # vertexDataFlags
        file.write(struct.pack("<i", len(indices)*3)) # indexCount
        file.write(struct.pack("<i", len(vertices))) # vertexCount
        file.write(struct.pack("<H", len(materials))) # materialCount
        file.write(struct.pack("<H", len(skin_bones))) # skinBoneCount
        file.write(struct.pack("<H", len(self.children))) # childCount

        # indices
        for index in indices:
            file.write(struct.pack("<III", *index)) # index

        # vertices
        for idx in range(len(vertices)):
            write_vector3(file, vertices[idx])
            if len(normals):
                write_vector3(file, normals[idx])
            if len(uvs):
                write_vector2(file, uvs[idx])
            if len(vertex_colors):
                write_color4(file, vertex_colors[idx])

        for mtl in materials:
            write_material(file, mtl) # materials

        for bone in skin_bones:
            write_skinbone(file, bone) # skinBones

    def _write_armature(self, file, options):
        """Write armature to file"""

        bone_matrices = []
        bone_data_size = 0
        for bone in self.bobj.data.bones:
            if options['use_rest_pose']:
                matrix = bone.matrix_local
                if bone.parent:
                    matrix = bone.parent.matrix_local.inverted() * matrix
            else:
                pose_bone = self.bobj.pose.bones[bone.name]
                matrix = pose_bone.matrix
                if pose_bone.parent:
                    matrix = pose_bone.parent.matrix.inverted() * matrix

            bone_data_size += sz_bone(self, bone, matrix)
            bone_matrices.append(matrix)

        matrix = options['global_matrix'] * self.bobj.matrix_local # root bone matrix

        block_size  = SZ_INT16 # boneCount
        block_size += sz_string(self.name) # root bone name
        block_size += sz_matrix4x4(matrix) # root bone matrix
        block_size += SZ_INT16 # root bone parent
        block_size += bone_data_size # bones

        print("struct BND (size: {}) {{".format(block_size))
        print("   boneCount     = {} (size: {})".format(len(self.bobj.data.bones)+1, SZ_INT16))
        print("   bones         = stripped (size: {})".format(bone_data_size))
        print("};")

        write_block(file, "BND", block_size) # header
        file.write(struct.pack("<H", len(self.bobj.data.bones)+1)) # boneCount

        # root bone (actually part of bones)
        write_string(file, self.name) # name
        write_matrix4x4(file, matrix) # transformationMatrix
        file.write(struct.pack("<H", 0)) # parent

        # bones
        for bone, matrix in zip_longest(self.bobj.data.bones, bone_matrices):
            def get_parent_idx():
                if bone.parent:
                    idx = 1
                    for ibn in self.bobj.data.bones:
                        if ibn == bone.parent:
                            return idx
                        idx += 1
                return 0

            parent = get_parent_idx()
            write_string(file, self.name + "_" + bone.name) # name
            write_matrix4x4(file, matrix) # transformationMatrix
            file.write(struct.pack("<H", parent)) # parent

    def write(self, context, file, options):
        """Write object/armature data to file"""

        if self.bobj.type == 'ARMATURE':
            self._write_armature(file, options)
        else:
            self._write_object(context, file, options)

        for eobj in self.children:
            eobj.write(context, file, options)

class GlhckExporter:
    def __init__(self):
        self.lists = {}

    def list_count(self, key):
        """Get count of every object and their children in list"""

        def animation_count(lst):
            return len(lst)

        def object_count(lst):
            count = 0
            for eobj in lst:
                count += object_count(eobj.children) + 1
            return count

        countmap = {'OBJECT'   :object_count,
                    'ARMATURE' :object_count,
                    'ANIMATION':animation_count}
        return countmap[key](self.lists[key])

    def _gather_data(self, context, options):
        """ Collect list of exportable objects, bones and animations """

        self.lists = {'OBJECT':[], 'ARMATURE':[], 'ANIMATION':[]}
        listmap = {'EMPTY':'OBJECT', 'MESH':'OBJECT', 'ARMATURE':'ARMATURE'}
        actionmap = {}

        if options['use_bones']:
            whitelist = ['EMPTY', 'MESH', 'ARMATURE']
        else:
            whitelist = ['EMPTY', 'MESH']

        if options['use_selection']:
            # Selected objects only
            export_list = list(context.selected_objects)
        else:
            # What you see, is what you get (check your layers)
            export_list = list(context.selectable_objects)

        # Add children and missing armatures to export_list
        def add_children(bobj):
            armatures = [modifier.object for modifier in bobj.modifiers
                    if modifier.type == 'ARMATURE' and modifier.show_viewport
                    and modifier.object is not None]
            for arm in armatures:
                if arm not in export_list:
                    export_list.append(arm)
            for bchd in bobj.children:
                if bchd not in export_list:
                    export_list.append(bchd)
                add_children(bchd)

        # We mutate this list
        for bobj in export_list[:]:
            add_children(bobj)

        # Should now contain filtered list of all objects or selected objects
        # and their children
        export_list = [ob for ob in export_list if ob.type in whitelist]

        # 1. Check if object or its children/bones contain animation
        # 2. Use actionmap to figure out already created animations
        # 3. Add object to animation
        def animations_from_object(eobj):
            def action_contains_object(action, eobj):
                for group in action.groups:
                    if group.name == eobj.name:
                        return True
                    if eobj.bobj.type == 'ARMATURE':
                        for bone in eobj.bobj.data.bones:
                            if group.name == bone.name:
                                return True
                return False

            if not options['use_animations']:
                return

            if eobj.bobj.animation_data is None:
                return

            for action in bpy.data.actions:
                if action_contains_object(action, eobj):
                    if action.name in actionmap:
                        eanm = actionmap[action.name]
                    else:
                        eanm = ExportAnimation(action)
                        actionmap[action.name] = eanm
                    eanm.add_object(eobj)

        # 1. Build reparented children (Cant be parent of armature, vice versa)
        # 2. Check if child has animation what we haven't yet added,
        #    and reference that possible animation
        def reparented_children(parent, src_list):
            dst_list = []
            for bobj in src_list:
                if bobj.type not in whitelist:
                    continue

                eobj = ExportObject(bobj)
                if parent.bobj.type == 'ARMATURE' and bobj.type != 'ARMATURE':
                    self.lists['OBJECT'].append(eobj)
                elif bobj.type == 'ARMATURE':
                    self.lists['ARMATURE'].append(eobj)
                else:
                    eobj.parent = parent
                    dst_list.append(eobj)

                eobj.children = reparented_children(eobj, bobj.children)
                animations_from_object(eobj)
            return dst_list

        # 1. Filter export_list to root ExportObjects
        # 2. Add them to correct self.lists[] using listmap
        # 3. Build reparented children for ExportObjects
        # 4. Build list of animations for our selection,
        #    and reference each object to their own animation
        for bobj in export_list:
            if bobj.parent not in export_list:
                eobj = ExportObject(bobj)
                self.lists[listmap[bobj.type]].append(eobj)
                eobj.children = reparented_children(eobj, bobj.children)
                animations_from_object(eobj)

        # List of animations can be now get from actionmap
        self.lists['ANIMATION'] = actionmap.values()

        # Finally sort our gathered lists
        for key in self.lists:
            self.lists[key] = sort_by_name_field(self.lists[key])

    def write(self, context, filepath, options):
        """Gather and write data from blender scene to file"""

        # Store filepath to options
        options['filepath'] = filepath
        options['copy_set'] = set()

        # Exit edit mode before exporting, so current object states are
        # exported properly.
        if bpy.ops.object.mode_set.poll():
            bpy.ops.object.mode_set(mode='OBJECT')

        self._gather_data(context, options)
        bnh = self.list_count('ARMATURE')
        obh = self.list_count('OBJECT')
        anh = self.list_count('ANIMATION')

        if bnh == 0 and obh == 0 and anh == 0:
            print("Nothing to export")
            return

        print("---- Armature List ----")
        print(self.lists['ARMATURE'])
        print("")

        print("---- Animation List ----")
        print(self.lists['ANIMATION'])
        print("")

        print("---- Object List ----")
        print(self.lists['OBJECT'])
        print("")

        print("---- Readable Header ----")
        print("BNH {}".format(bnh))
        print("OBH {}".format(obh))
        print("ANH {}".format(anh))
        print("")

        file = None
        if bnh > 0 or obh > 0 or (anh > 0 and not options['split_animations']):
            file = open(filepath, 'wb')
            file.write(bytes(GLHCKM_HEADER, 'ascii'))
            file.write(struct.pack("<BB", *GLHCKM_VERSION))

        if bnh > 0:
            write_block(file, "BNH", bnh) # header block
        if obh > 0:
            write_block(file, "OBH", obh) # header block
        if not options['split_animations'] and anh > 0:
            write_block(file, "ANH", anh) # header block

        # Export bones
        for eobj in self.lists['ARMATURE']:
            eobj.write(context, file, options)

        # Export objects
        for eobj in self.lists['OBJECT']:
            eobj.write(context, file, options)

        # Export animations
        for eanm in self.lists['ANIMATION']:
            if options['split_animations']:
                if file is not None:
                    file.close()
                path = os.path.dirname(filepath) + "/" + eanm.name + ".glhcka"
                print(path)
                file = open(path, 'wb')
                file.write(bytes(GLHCKA_HEADER, 'ascii'))
                file.write(struct.pack("<BB", *GLHCKM_VERSION))
                write_block(file, "ANH", 1) # header block
            eanm.write(context, file, options)

        file.close()

        # Copy all collected files from export
        path_reference_copy(options['copy_set'])


##
## GLhck Model Export v0.1
##
## NOTE: This format is not yet considered "final"
##       Drastic changes can happen, and maintaining easy backwards
##       compatibility thus is not yet a goal.
##
## Planned changes:
##     - MAD block for materials
##     - Reference to MAD blocks from OBD blocks
##     - DBD blocks for duplicate objects
##     - EBD blocks for empty objects
##     - Optional zlib (.gz) compression
##
## Version history:
##     -- 0.1 (Thu Nov 14 00:42:23 UTC 2013)
##         First release
##
## Glhck model format is combination of binary and text.
## Text is used to serialize floats and block headers.
##
## Some major points of the format:
##     - Everything is little-endian
##     - Matrices are in column-major
##     - Quaternions are in order x,y,z,w
##     - Floats, Vectors, Quaternions, Matrices are packed as strings
##     - Strings for names and textures filepaths are UTF-8
##
## Glhck files start with either glhckm or glhcka header text followed by
## version (major, minor) serialized as two uint8_t.
##
##     glhckm header is used for the master format and can contain any data
##     (BND, OBD, AND blocks).
##
##     glhcka header is used for the animation format and should only contain
##     animation data (AND blocks).
##
##     Version can be used to provide backwards compatibility.
##
## Blocks should be ordered in following order:
##     1. BND blocks
##     2. OBD blocks
##     3. AND blocks
##
##     This makes it easier to reference skeleton bones to objects through
##     skinbones for importers
##
## The format contains following data blocks (BND, OBD, AND) with structs:
##
##     // Arrays which size cant be known without reading data block for each
##     // element are marked with <no-known-size> comment.
##
##     struct STRING {
##        uint8_t len;
##        char str[len];
##     }
##
##     struct LONGSTRING {
##        uint16_t len;
##        char str[len];
##     }
##
##     struct MATRIX4x4 {
##        LONGSTRING m4x4;
##        // Matrix is serialized as (column-major):
##        // "%f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f"
##     };
##
##     // Actual bone information
##     struct BONE {
##        STRING name; // never empty, and must be unique!
##        MATRIX4x4 transformationMatrix;
##        uint16_t parent; // 0 == root bone
##     };
##
##     // Always starts with the root bone (armature)
##     struct BND {
##        uint16_t boneCount;
##        BONE bones[boneCount]; // <no-known-size>
##     };
##
##     struct COLOR4 {
##        uint8_t r, g, b, a;
##     };
##
##     struct COLOR3 {
##        uint8_t r, g, b;
##     };
##
##     struct FLOAT {
##        STRING flt;
##        // Float is serialised as:
##        // "%f"
##     };
##
##     // Bitflags for materialFlags member of MATERIAL struct
##     enum {
##        LIGHTING = 1<<0,
##     };
##
##     struct MATERIAL {
##        STRING name;
##        COLOR3 ambient;
##        COLOR4 diffuse;
##        COLOR4 specularity;
##        uint16_t shininess; // range: 1 - 511
##        uint8_t materialFlags;
##        LONGSTRING diffuseTexture;
##     };
##
##     struct WEIGHT {
##        uint32_t vertexIndex;
##        FLOAT weight;
##     };
##
##     struct SKINBONE {
##        STRING name; // must reference to BONE
##        MATRIX4x4 offsetMatrix;
##        uint32_t weightCount;
##        WEIGHT weights[weightCount]; // <no-known-size>
##     };
##
##     struct VECTOR3 {
##        STRING vec3;
##        // Vector is serialized as (x,y,z):
##        // "%f,%f,%f"
##     };
##
##     struct VECTOR2 {
##        STRING vec2;
##        // Vector is serialized as (x,y):
##        // "%f,%f"
##     };
##
##     struct VERTEXDATA {
##        VECTOR3 vertex; // always exists
##        VECTOR3 normal; // only if (vertexDataFlags & HAS_NORMALS)
##        VECTOR2 uv;     // only if (vertexDataFlags & HAS_UV)
##        COLOR4 color;   // only if (vertexDataFlags & HAS_VERTEX_COLORS)
##     };
##
##     enum geometryTypeEnum {
##        TRIANGLES,
##     };
##
##     // Bitflags for vertexDataFlags member of OB struct
##     enum {
##        HAS_NORMALS       = 1<<0,
##        HAS_UV            = 1<<1,
##        HAS_VERTEX_COLORS = 1<<2,
##     };
##
##     // Repeated until there is no children and their children
##     struct OBD {
##        STRING name; // should not be empty, if used for animation (unique)
##        geometryTypeEnum geometryType; // uint8_t
##        uint8_t vertexDataFlags;
##        int32_t indexCount;
##        int32_t vertexCount;
##        uint16_t materialCount;
##        uint16_t skinBoneCount;
##        uint16_t childCount;
##        uint32_t indices[indexCount];
##        VERTEXDATA vertices[vertexCount]; // <no-known-size>
##        MATERIAL materials[materialCount]; // <no-known-size>
##        SKINBONE skinBones[skinBoneCount]; // <no-known-size>
##        OB children[childCount]; // <no-known-size>
##     };
##
##     struct QUATERNION {
##        STRING quat;
##        // Quaternion is serialized as (x,y,z,w):
##        // "%f,%f,%f,%f"
##     };
##
##     struct QUATERNIONKEY {
##        uint32_t frame;
##        QUATERNION quaternion;
##     };
##
##     struct VECTORKEY {
##        uint32_t frame;
##        VECTOR3 vector;
##     };
##
##     struct NODE {
##        STRING name; // must reference to a OBD/BONE
##        uint32_t rotationCount;
##        uint32_t scalingCount;
##        uint32_t translationCount;
##        QUATERNIONKEY quaternionKeys[rotationCount]; // <no-known-size>
##        VECTORKEY scalingKeys[scalingCount]; // <no-known-size>
##        VECTORKEY translationKeys[translationCount]; // <no-known-size>
##     };
##
##     struct AND {
##        STRING name;
##        uint32_t nodeCount;
##        NODE nodes[nodeCount]; // <no-known-size>
##     };
##
## Every data block has header block at top of file:
##     BNH<uint32_t> (BND blocks count in file)
##     OBH<uint32_t> (OBD blocks count in file)
##     ANH<uint32_t> (AND blocks count in file)
##
## The header block doesn't need to be written, if the count of data blocks is 0
##
## Example of glhckm file:
##     glhckm<uint8_t:0><uint8_t:1>
##     BNH<uint32_t:1>
##     OBH<uint32_t:4>
##     ANH<uint32_t:0>
##     BND<uint32_t:size><...data...>
##     OBD<uint32_t:size><...data...>
##     OBD<uint32_t:size><...data...>
##     OBD<uint32_t:size><...data...>
##     OBD<uint32_t:size><...data...>
##
## When importing data, the block sizes make it easier to skip data you don't
## care about.
##

def save(context, filepath,
        use_selection=False,
        use_animations=True,
        bake_animations=False,
        split_animations=False,
        use_bones=True,
        use_rest_pose=True,
        use_mesh_modifiers=True,
        use_normals=True,
        use_uvs=True,
        use_vertex_colors=False,
        use_materials=True,
        global_matrix=None,
        path_mode='AUTO'):

    print("")
    print(":::::::::::::::::::::::::::::")
    print(":: GLhck Model Export v0.1 ::")
    print(":::::::::::::::::::::::::::::")
    print(":: filepath           = {}".format(filepath))
    print(":: use_selection      = {}".format(use_selection))
    print(":: use_animations     = {}".format(use_animations))
    print(":: bake_animations    = {}".format(bake_animations))
    print(":: split_animations   = {}".format(split_animations))
    print(":: use_bones          = {}".format(use_bones))
    print(":: use_rest_pose      = {}".format(use_rest_pose))
    print(":: use_mesh_modifiers = {}".format(use_mesh_modifiers))
    print(":: use_uvs            = {}".format(use_uvs))
    print(":: use_normals        = {}".format(use_normals))
    print(":: use_vertex_colors  = {}".format(use_vertex_colors))
    print(":: use_materials      = {}".format(use_materials))
    print(":: path_mode          = {}".format(path_mode))
    print("")
    print("{}".format(global_matrix))
    print("")

    import time
    time_start = time.time()

    if not global_matrix:
        global_matrix = Matrix()

    options = {}
    for i in ['use_selection',      'use_animations', 'bake_animations',
              'split_animations',   'use_bones',      'use_rest_pose',
              'use_mesh_modifiers', 'use_normals',    'use_uvs',
              'use_vertex_colors',  'use_materials',
              'path_mode',          'global_matrix']:
        options[i] = locals()[i]

    exporter = GlhckExporter()
    exporter.write(context, filepath, options)

    print("")
    print("::::::::::::::::::::::::::::")
    print(":: {}".format(filepath))
    print(":: Finished in {:.4f}".format(time.time() - time_start))
    print("::::::::::::::::::::::::::::")
    print("")
    return {'FINISHED'}

# vim: set ts=8 sw=4 tw=0 :
