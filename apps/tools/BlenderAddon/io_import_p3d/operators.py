import os
from ctypes import c_float, c_uint8

import bpy
from bpy.props import FloatVectorProperty, IntProperty, StringProperty

from . import p3d_lib
from .helpers import _get_active_cwr_mesh
from .import_p3d import do_import

_BLENDER_5 = bpy.app.version >= (5, 0, 0)


def _iter_fcurves(action, obj=None):
    """Iterate fcurves from action — works on both Blender 4.x and 5.0+."""
    if _BLENDER_5:
        for layer in action.layers:
            for strip in layer.strips:
                for cb in strip.channelbags:
                    yield from cb.fcurves
    else:
        yield from action.fcurves


class POSEIDON_OT_scan_vfs(bpy.types.Operator):
    """Mount game directory and scan for P3D models"""

    bl_idname = "poseidon.scan_vfs"
    bl_label = "Scan P3D Models"

    def execute(self, context):
        props = context.scene.poseidon
        game_dir = bpy.path.abspath(props.game_data_dir)
        if not game_dir or not os.path.isdir(game_dir):
            self.report({"ERROR"}, "Invalid game data directory")
            return {"CANCELLED"}

        p3d_lib.load()
        p3d_lib.vfs_unmount()
        count = p3d_lib.vfs_mount(game_dir)
        if count == 0:
            self.report({"WARNING"}, "No PBO archives found")
            return {"CANCELLED"}

        paths = p3d_lib.vfs_find(".p3d")
        props.vfs_p3d_list.clear()
        for p in paths:
            item = props.vfs_p3d_list.add()
            item.vfs_path = p
            item.name = os.path.basename(p)

        props.vfs_p3d_index = 0
        self.report({"INFO"}, f"Found {len(paths)} P3D models in {count} PBOs")
        return {"FINISHED"}


class POSEIDON_OT_import_vfs_p3d(bpy.types.Operator):
    """Import selected P3D model from VFS"""

    bl_idname = "poseidon.import_vfs_p3d"
    bl_label = "Import VFS P3D"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        import tempfile
        from ctypes import c_uint8

        props = context.scene.poseidon
        if props.vfs_p3d_index < 0 or props.vfs_p3d_index >= len(props.vfs_p3d_list):
            self.report({"ERROR"}, "No model selected")
            return {"CANCELLED"}

        item = props.vfs_p3d_list[props.vfs_p3d_index]
        game_dir = bpy.path.abspath(props.game_data_dir)

        p3d_lib.load()
        dll = p3d_lib.get()
        vfs_path = item.vfs_path
        print(f"[P3D] VFS import: {vfs_path}")

        path_bytes = vfs_path.replace("/", "\\").encode("utf-8")
        size = dll.pf_vfs_read(path_bytes, None, 0)
        if size <= 0:
            err = dll.pf_last_error()
            msg = err.decode() if err else "unknown"
            self.report({"ERROR"}, f"Failed to read from VFS: {msg}")
            return {"CANCELLED"}

        buf = (c_uint8 * size)()
        dll.pf_vfs_read(path_bytes, buf, size)

        tmp_dir = tempfile.mkdtemp(prefix="p3d_vfs_")
        tmp_file = os.path.join(tmp_dir, os.path.basename(vfs_path))
        with open(tmp_file, "wb") as f:
            f.write(bytes(buf))

        options = {
            "lod_filter": "ALL",
            "max_lod_resolution": 1e30,
            "import_materials": True,
            "import_normals": True,
            "import_selections": True,
            "import_proxies": True,
            "import_properties": True,
            "game_data_dir": game_dir,
        }
        result = do_import(context, tmp_file, options)

        try:
            os.remove(tmp_file)
            os.rmdir(tmp_dir)
        except OSError:
            pass

        return result


class POSEIDON_OT_import_vfs_p3d_by_index(bpy.types.Operator):
    """Import P3D model from VFS by list index"""

    bl_idname = "poseidon.import_vfs_p3d_by_index"
    bl_label = "Import VFS P3D"
    bl_options = {"REGISTER", "UNDO"}

    index: IntProperty(default=-1)

    def execute(self, context):
        props = context.scene.poseidon
        idx = self.index
        if idx < 0 or idx >= len(props.vfs_p3d_list):
            self.report({"ERROR"}, "Invalid index")
            return {"CANCELLED"}
        props.vfs_p3d_index = idx
        return bpy.ops.poseidon.import_vfs_p3d()


class POSEIDON_OT_highlight_selection(bpy.types.Operator):
    """Highlight a named selection (vertex group) with color"""

    bl_idname = "poseidon.highlight_selection"
    bl_label = "Highlight Selection"
    bl_options = {"REGISTER", "UNDO"}

    group_name: StringProperty()
    color: FloatVectorProperty(size=3, default=(1.0, 0.0, 0.0))

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != "MESH":
            return {"CANCELLED"}

        if obj.mode != "OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")

        mesh = obj.data
        vg = obj.vertex_groups.get(self.group_name)
        if not vg:
            self.report({"WARNING"}, f"Vertex group '{self.group_name}' not found")
            return {"CANCELLED"}

        vc_name = "CWR_Highlight"
        if vc_name in mesh.color_attributes:
            mesh.color_attributes.remove(mesh.color_attributes[vc_name])
        vc = mesh.color_attributes.new(name=vc_name, type="FLOAT_COLOR", domain="POINT")

        for i in range(len(mesh.vertices)):
            vc.data[i].color = (1.0, 1.0, 1.0, 1.0)

        vg_idx = vg.index
        r, g, b = self.color
        count = 0
        for v in mesh.vertices:
            for grp in v.groups:
                if grp.group == vg_idx and grp.weight > 0.01:
                    vc.data[v.index].color = (r, g, b, 1.0)
                    count += 1
                    break

        mesh.color_attributes.active_color = vc
        self.report({"INFO"}, f"Highlighted {count} vertices in '{self.group_name}'")
        return {"FINISHED"}


class POSEIDON_OT_clear_highlight(bpy.types.Operator):
    """Remove selection highlight colors"""

    bl_idname = "poseidon.clear_highlight"
    bl_label = "Clear Highlight"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != "MESH":
            return {"CANCELLED"}
        mesh = obj.data
        vc_name = "CWR_Highlight"
        if vc_name in mesh.color_attributes:
            mesh.color_attributes.remove(mesh.color_attributes[vc_name])
        return {"FINISHED"}


class POSEIDON_OT_select_material_faces(bpy.types.Operator):
    """Select faces using this material and enter edit mode"""

    bl_idname = "poseidon.select_material_faces"
    bl_label = "Select Material Faces"
    bl_options = {"REGISTER", "UNDO"}

    mat_index: IntProperty(default=0)

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != "MESH":
            return {"CANCELLED"}
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="DESELECT")
        obj.active_material_index = self.mat_index
        bpy.ops.object.material_slot_select()
        return {"FINISHED"}


class POSEIDON_OT_activate_material(bpy.types.Operator):
    """Set active material slot on the object"""

    bl_idname = "poseidon.activate_material"
    bl_label = "Activate Material"
    bl_options = {"REGISTER", "UNDO"}

    mat_index: IntProperty(default=0)

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != "MESH":
            return {"CANCELLED"}
        obj.active_material_index = self.mat_index
        self.report({"INFO"}, f"Active material: {obj.material_slots[self.mat_index].material.name}")
        return {"FINISHED"}


class POSEIDON_OT_select_group_vertices(bpy.types.Operator):
    """Select vertices in a named selection and enter edit mode"""

    bl_idname = "poseidon.select_group_vertices"
    bl_label = "Select Group Vertices"
    bl_options = {"REGISTER", "UNDO"}

    group_name: StringProperty()

    def execute(self, context):
        obj = context.active_object
        if not obj or obj.type != "MESH":
            return {"CANCELLED"}
        vg = obj.vertex_groups.get(self.group_name)
        if not vg:
            self.report({"WARNING"}, f"Vertex group '{self.group_name}' not found")
            return {"CANCELLED"}
        obj.vertex_groups.active = vg
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="DESELECT")
        bpy.ops.object.vertex_group_select()
        return {"FINISHED"}


class POSEIDON_OT_toggle_proxies(bpy.types.Operator):
    """Toggle proxy visibility for this model"""

    bl_idname = "poseidon.toggle_proxies"
    bl_label = "Toggle Proxies"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        obj = _get_active_cwr_mesh(context)
        if not obj:
            return {"CANCELLED"}
        for col in bpy.data.collections:
            if obj.name in col.objects:
                for child_col in col.children:
                    if child_col.name.endswith("_Proxies"):
                        lc = self._find_layer_collection(context.view_layer.layer_collection, child_col)
                        if lc:
                            lc.exclude = not lc.exclude
                break
        return {"FINISHED"}

    def _find_layer_collection(self, parent_lc, target_col):
        if parent_lc.collection == target_col:
            return parent_lc
        for child in parent_lc.children:
            found = self._find_layer_collection(child, target_col)
            if found:
                return found
        return None


class POSEIDON_OT_select_proxy(bpy.types.Operator):
    """Select a proxy object"""

    bl_idname = "poseidon.select_proxy"
    bl_label = "Select Proxy"
    bl_options = {"REGISTER", "UNDO"}

    proxy_name: StringProperty()

    def execute(self, context):
        proxy = bpy.data.objects.get(self.proxy_name)
        if not proxy:
            self.report({"WARNING"}, f"Proxy '{self.proxy_name}' not found")
            return {"CANCELLED"}
        bpy.ops.object.select_all(action="DESELECT")
        proxy.select_set(True)
        context.view_layer.objects.active = proxy
        return {"FINISHED"}


class POSEIDON_OT_scan_animations(bpy.types.Operator):
    """Scan VFS for RTM animation files and match bones to active model"""

    bl_idname = "poseidon.scan_animations"
    bl_label = "Scan Animations"

    def execute(self, context):
        props = context.scene.poseidon
        if not p3d_lib.vfs_mounted():
            self.report({"ERROR"}, "VFS not mounted — scan models first")
            return {"CANCELLED"}

        dll = p3d_lib.get()
        paths = p3d_lib.vfs_find(".rtm")

        obj = _get_active_cwr_mesh(context)
        vgroups = set()
        if obj:
            vgroups = {vg.name.lower() for vg in obj.vertex_groups}

        props.vfs_rtm_list.clear()
        loaded = 0
        failed = 0

        for path in paths:
            path_bytes = path.replace("/", "\\").encode("utf-8")
            size = dll.pf_vfs_read(path_bytes, None, 0)
            if size <= 0:
                failed += 1
                continue

            buf = (c_uint8 * size)()
            dll.pf_vfs_read(path_bytes, buf, size)

            handle = dll.pf_rtm_load_from_memory(buf, size)
            if not handle:
                failed += 1
                continue

            bone_count = dll.pf_rtm_bone_count(handle)
            phase_count = dll.pf_rtm_phase_count(handle)

            bone_names = []
            match_count = 0
            for i in range(bone_count):
                name = dll.pf_rtm_bone_name(handle, i).decode()
                bone_names.append(name)
                if name.lower() in vgroups:
                    match_count += 1

            dll.pf_rtm_free(handle)

            item = props.vfs_rtm_list.add()
            item.vfs_path = path
            item.name = os.path.basename(path)
            item.bone_count = bone_count
            item.phase_count = phase_count
            item.match_count = match_count
            item.bone_names = ",".join(bone_names)
            loaded += 1

        props.vfs_rtm_index = 0
        msg = f"Loaded {loaded} RTM animations"
        if failed:
            msg += f" ({failed} failed)"
        if obj:
            msg += f" — matched against {obj.name}"
        self.report({"INFO"}, msg)
        return {"FINISHED"}


class POSEIDON_OT_refresh_animation_match(bpy.types.Operator):
    """Re-match animation bones against active model's vertex groups"""

    bl_idname = "poseidon.refresh_animation_match"
    bl_label = "Refresh Matches"

    def execute(self, context):
        props = context.scene.poseidon
        obj = _get_active_cwr_mesh(context)
        if not obj:
            self.report({"WARNING"}, "No CWR mesh selected")
            return {"CANCELLED"}

        vgroups = {vg.name.lower() for vg in obj.vertex_groups}
        updated = 0

        for item in props.vfs_rtm_list:
            if not item.bone_names:
                continue
            bones = item.bone_names.split(",")
            match_count = sum(1 for b in bones if b.lower() in vgroups)
            if item.match_count != match_count:
                item.match_count = match_count
                updated += 1

        self.report({"INFO"}, f"Matched against {obj.name}: {updated} items updated")
        return {"FINISHED"}


class POSEIDON_OT_load_animation(bpy.types.Operator):
    """Load selected RTM animation — create armature and keyframes"""

    bl_idname = "poseidon.load_animation"
    bl_label = "Load Animation"
    bl_options = {"REGISTER", "UNDO"}

    @staticmethod
    def _rtm_matrix(buf):
        """Column-major 16-float buffer → Blender Matrix with Y↔Z axis swap."""
        import mathutils

        mat = mathutils.Matrix(
            (
                (buf[0], buf[4], buf[8], buf[12]),
                (buf[1], buf[5], buf[9], buf[13]),
                (buf[2], buf[6], buf[10], buf[14]),
                (buf[3], buf[7], buf[11], buf[15]),
            )
        )
        # CWR Y↔Z swap (same as _convert_matrix in import_p3d.py)
        swap = mathutils.Matrix(
            (
                (1, 0, 0, 0),
                (0, 0, 1, 0),
                (0, 1, 0, 0),
                (0, 0, 0, 1),
            )
        )
        return swap @ mat @ swap.inverted()

    @staticmethod
    def _vertex_group_centroid(obj, group_name):
        """Compute centroid of vertices in a vertex group.
        Falls back to stored centroid if vertices were removed (proxy stripping)."""
        import mathutils

        vg = obj.vertex_groups.get(group_name)
        if vg:
            vg_idx = vg.index
            total = mathutils.Vector((0, 0, 0))
            count = 0
            for v in obj.data.vertices:
                for g in v.groups:
                    if g.group == vg_idx and g.weight > 0.01:
                        total += v.co
                        count += 1
                        break
            if count > 0:
                return total / count
        # Fallback: centroid stored during import (before proxy face stripping)
        stored = obj.get(f"cwr_centroid_{group_name}")
        if stored:
            return mathutils.Vector(stored)
        return mathutils.Vector((0, 0, 0))

    def execute(self, context):
        import mathutils

        props = context.scene.poseidon
        if props.vfs_rtm_index < 0 or props.vfs_rtm_index >= len(props.vfs_rtm_list):
            self.report({"ERROR"}, "No animation selected")
            return {"CANCELLED"}

        item = props.vfs_rtm_list[props.vfs_rtm_index]
        obj = _get_active_cwr_mesh(context)

        dll = p3d_lib.get()
        path_bytes = item.vfs_path.replace("/", "\\").encode("utf-8")
        size = dll.pf_vfs_read(path_bytes, None, 0)
        if size <= 0:
            self.report({"ERROR"}, f"Cannot read {item.vfs_path}")
            return {"CANCELLED"}

        buf = (c_uint8 * size)()
        dll.pf_vfs_read(path_bytes, buf, size)
        handle = dll.pf_rtm_load_from_memory(buf, size)
        if not handle:
            self.report({"ERROR"}, "Failed to parse RTM")
            return {"CANCELLED"}

        bone_count = dll.pf_rtm_bone_count(handle)
        phase_count = dll.pf_rtm_phase_count(handle)
        bone_names = [dll.pf_rtm_bone_name(handle, i).decode() for i in range(bone_count)]
        bone_centroids = {}
        if obj:
            for bname in bone_names:
                bone_centroids[bname] = self._vertex_group_centroid(obj, bname)
        if context.active_object and context.active_object.mode != "OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")
        anim_name = os.path.splitext(os.path.basename(item.vfs_path))[0]
        arm_data = bpy.data.armatures.new(f"{anim_name}_Armature")
        arm_obj = bpy.data.objects.new(f"{anim_name}_Armature", arm_data)
        context.collection.objects.link(arm_obj)
        context.view_layer.objects.active = arm_obj
        arm_obj.select_set(True)

        if obj:
            arm_obj.location = obj.location
        bpy.ops.object.mode_set(mode="EDIT")
        BONE_LENGTH = 0.15

        for bname in bone_names:
            bone = arm_data.edit_bones.new(bname)
            head = bone_centroids.get(bname, mathutils.Vector((0, 0, 0)))
            bone.head = head
            bone.tail = head + mathutils.Vector((0, BONE_LENGTH, 0))

        bpy.ops.object.mode_set(mode="OBJECT")
        fps = context.scene.render.fps
        action = bpy.data.actions.new(name=anim_name)
        arm_obj.animation_data_create()
        arm_obj.animation_data.action = action

        bpy.ops.object.mode_set(mode="POSE")
        for bname in bone_names:
            pbone = arm_obj.pose.bones.get(bname)
            if pbone:
                pbone.rotation_mode = "QUATERNION"

        # CWR engine: v' = Σ w_i * M_i * v_orig (absolute bone transforms).
        # Use absolute transforms — the model displaces from import pose
        # to the animation's bone-transformed pose, matching the engine.
        prev_quats = {}
        mat_buf = (c_float * 16)()
        for phase_idx in range(phase_count):
            frame = 1 + phase_idx

            for bi, bname in enumerate(bone_names):
                dll.pf_rtm_bone_matrix(handle, phase_idx, bi, mat_buf)
                rtm_mat = self._rtm_matrix(mat_buf)

                pbone = arm_obj.pose.bones.get(bname)
                if not pbone:
                    continue

                rest = pbone.bone.matrix_local
                basis = rest.inverted() @ rtm_mat @ rest
                pbone.matrix_basis = basis

                # Ensure consecutive quaternions stay in same hemisphere
                q = pbone.rotation_quaternion.copy()
                prev = prev_quats.get(bname)
                if prev is not None and prev.dot(q) < 0:
                    q.negate()
                    pbone.rotation_quaternion = q
                prev_quats[bname] = q

                pbone.keyframe_insert(data_path="location", frame=frame)
                pbone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
                pbone.keyframe_insert(data_path="scale", frame=frame)
        for fcurve in _iter_fcurves(action, arm_obj):
            for kp in fcurve.keyframe_points:
                kp.interpolation = "LINEAR"

        bpy.ops.object.mode_set(mode="OBJECT")

        # Normalize bone weights per vertex.
        # The C API returns selection weights as byte/255 but the engine uses
        # byte/128 (WeightScale=128) then normalizes to sum=1.0.  Without this
        # step, bone weights sum to ~0.5-0.8 and Blender fills the gap with
        # rest position, causing visible pose errors (~25 cm on shoulder verts).
        if obj:
            bone_name_set = set(bone_names)
            bone_vg_indices = {vg.index for vg in obj.vertex_groups if vg.name in bone_name_set}
            for vert in obj.data.vertices:
                bone_total = 0.0
                bone_groups = []
                for g in vert.groups:
                    if g.group in bone_vg_indices and g.weight > 0.0:
                        bone_total += g.weight
                        bone_groups.append(g)
                if bone_total > 0.0 and abs(bone_total - 1.0) > 1e-6:
                    scale = 1.0 / bone_total
                    for g in bone_groups:
                        g.weight *= scale
        if obj:
            mod = obj.modifiers.new(name="Armature", type="ARMATURE")
            mod.object = arm_obj

        dll.pf_rtm_free(handle)

        context.scene.frame_start = 1
        context.scene.frame_end = phase_count
        context.scene.frame_set(1)

        matched = (
            sum(1 for b in bone_names if obj and b.lower() in {vg.name.lower() for vg in obj.vertex_groups})
            if obj
            else 0
        )
        self.report({"INFO"}, f"Loaded {anim_name}: {bone_count} bones, {phase_count} phases, {matched} matched")
        return {"FINISHED"}
