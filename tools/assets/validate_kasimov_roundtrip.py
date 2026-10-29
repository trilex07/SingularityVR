"""Blender evaluated-geometry comparison across saved BLEND and exported GLB."""
import bpy,json,sys,hashlib
from pathlib import Path
from mathutils.kdtree import KDTree
root=Path(sys.argv[-1])
def signatures():
 result=set()
 for name in ('kasimov_body','kasimov_magazine','kasimov_lens'):
  obj=bpy.data.objects[name]
  for v in obj.data.vertices:
   influences=tuple(sorted((obj.vertex_groups[g.group].name,round(g.weight,5)) for g in v.groups if g.weight>1e-6))
   assert abs(sum(g.weight for g in v.groups)-1)<1e-5
   result.add((name,tuple(v.co),influences))
 return result
def sample():
 arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
 action=bpy.data.actions.get('Reload_mechanical_source');arm.animation_data_create();arm.animation_data.use_nla=False;arm.animation_data.action=action;arm.animation_data.action_slot=action.slots[0]
 result={}
 for f in (0,30,80,120,180,238):
  bpy.context.scene.frame_set(f);bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get()
  result[f]={}
  for name in ('kasimov_body','kasimov_magazine','kasimov_lens'):
   obj=bpy.data.objects[name];ev=obj.evaluated_get(dg);m=ev.to_mesh();result[f][name]=[tuple(obj.matrix_world@v.co) for v in m.vertices];ev.to_mesh_clear()
 return result,{b.name:(b.parent.name if b.parent else None) for b in arm.data.bones}
bpy.ops.wm.open_mainfile(filepath=str(root/'kasimov_hd_animated.blend'));before,hierarchy=sample();skin_before=signatures()
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for a in list(bpy.data.actions):bpy.data.actions.remove(a)
bpy.ops.import_scene.gltf(filepath=str(root/'kasimov_hd_animated.glb'));after,other=sample()
skin_after=signatures()
trees={}
for name,co,influences in skin_after:trees.setdefault((name,influences),[]).append(co)
for key,coords in list(trees.items()):
 tree=KDTree(len(coords))
 for i,co in enumerate(coords):tree.insert(co,i)
 tree.balance();trees[key]=tree
for name,co,influences in skin_before:
 assert (name,influences) in trees and trees[(name,influences)].find(co)[2]<1e-6,'exported named weights changed'
assert hierarchy==other,'bone hierarchy changed'
error=0
for f,parts in before.items():
 for name,vertices in parts.items():
  tree=KDTree(len(after[f][name]))
  for i,v in enumerate(after[f][name]):tree.insert(v,i)
  tree.balance();error=max(error,max(tree.find(v)[2] for v in vertices))
assert error<.0001,('roundtrip deformation mismatch',error)
report={'status':'BLENDER_ROUNDTRIP_PASS','max_roundtrip_error_m':error,'tolerance_m':.0001,'bones':len(hierarchy),'frames':[0,30,80,120,180,238]}
(root/'roundtrip_validation.json').write_text(json.dumps(report,indent=2));print(report)
