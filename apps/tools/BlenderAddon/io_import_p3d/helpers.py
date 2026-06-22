import bpy


def _find_cwr_root_collection(obj):
    """Walk up collection tree to find the CWR model root collection."""
    if not obj:
        return None
    for col in bpy.data.collections:
        if obj.name in col.objects:
            for parent_col in bpy.data.collections:
                if col.name in [c.name for c in parent_col.children]:
                    if any(c.name in ("Visual", "Cockpit", "Special") for c in parent_col.children):
                        return parent_col
            if any(c.name in ("Visual", "Cockpit", "Special") for c in col.children):
                return col
    return None


def _get_cwr_objects(root_col):
    """Get all mesh objects from a CWR collection tree."""
    objs = []
    if not root_col:
        return objs
    for obj in root_col.all_objects:
        if obj.type == "MESH":
            objs.append(obj)
    return objs


def _get_active_cwr_mesh(context):
    """Return the active object if it's a CWR mesh (has cwr_lod_index)."""
    obj = context.active_object
    if obj and obj.type == "MESH" and "cwr_lod_index" in obj:
        return obj
    return None
