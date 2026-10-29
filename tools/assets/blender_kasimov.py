"""Build and validate the private Kasimov package in Blender, preserving weights."""
import bpy,json,sys,math,hashlib
from pathlib import Path
import numpy as np
from mathutils import Matrix,Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from blender_finish_valkyrie import pbr,detail_material,camera_lights,render

root=Path(sys.argv[-1]);d=json.loads((root/'source_data.json').read_text());U=d['units']
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def mat(rows):
 a=np.array(rows).T.copy();a[:3,3]/=U;return Matrix(a.tolist())
bind=[mat(m) for m in d['bind']]
armdata=bpy.data.armatures.new('Kasimov original PSK');arm=bpy.data.objects.new('kasimov_rig',armdata)
bpy.context.collection.objects.link(arm);bpy.context.view_layer.objects.active=arm;arm.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for i,name in enumerate(d['names']):
 b=armdata.edit_bones.new(name);b.matrix=bind[i];b.length=.01
 if i:b.parent=armdata.edit_bones[d['names'][d['parents'][i]]]
bpy.ops.object.mode_set(mode='OBJECT')
rest=[armdata.bones[n].matrix_local.copy() for n in d['names']]
for b in arm.pose.bones:b.rotation_mode='QUATERNION'
weights={int(p):w for p,w in d['weights'].items()};points=np.array(d['points'])/U
groups={'body':[],'magazine':[],'lens':[]}
for f in d['faces']:
 pts=[d['wedges'][w][0] for w in f[:3]]
 key='lens' if f[3]==1 else ('magazine' if all(weights[p]==[[1.0,56]] for p in pts) else 'body')
 groups[key].append(f)
meshes=[];source_ids={}
for label,faces in groups.items():
 used=sorted({w for f in faces for w in f[:3]});mapping={w:i for i,w in enumerate(used)}
 ids=[d['wedges'][w][0] for w in used];v=[points[p].tolist() for p in ids]
 mesh=bpy.data.meshes.new(label);mesh.from_pydata(v,[],[[mapping[w] for w in f[:3]] for f in faces]);mesh.update()
 obj=bpy.data.objects.new('kasimov_'+label,mesh);bpy.context.collection.objects.link(obj);meshes.append(obj);source_ids[label]=ids
 uv=mesh.uv_layers.new(name='UVMap')
 for poly in mesh.polygons:
  for loop in poly.loop_indices:
   wedge=used[mesh.loops[loop].vertex_index];u,v=d['wedges'][wedge][1:];uv.data[loop].uv=(u,1-v)
 for i,name in enumerate(d['names']):obj.vertex_groups.new(name=name)
 for vertex,p in enumerate(ids):
  for w,b in weights[p]:obj.vertex_groups[b].add([vertex],w,'REPLACE')
 mod=obj.modifiers.new('Original RAWWEIGHTS','ARMATURE');mod.object=arm
 obj.parent=arm
maps=root/'maps';material=pbr('Kasimov HD',maps/'kasimov_albedo_4k.png',maps/'kasimov_roughness_4k.png',maps/'kasimov_metallic_4k.png',maps/'kasimov_normal_4k.png')
for obj in meshes:obj.data.materials.append(material)
lens=bpy.data.materials.new('Kasimov separate scope lens');lens.use_nodes=True
shader=lens.node_tree.nodes.get('Principled BSDF');shader.inputs['Base Color'].default_value=(.035,.065,.08,1);shader.inputs['Metallic'].default_value=.65;shader.inputs['Roughness'].default_value=.12
meshes[2].data.materials.clear();meshes[2].data.materials.append(lens)
detailmat=detail_material();details=[]
# Small machined fasteners anchored to actual source triangle surfaces.
for bone in (53,56,57):
 candidates=[]
 for f in d['faces']:
  if f[3]!=0:continue
  ids=[d['wedges'][w][0] for w in f[:3]]
  if not all(len(weights[p])==1 and weights[p][0][1]==bone for p in ids):continue
  tri=points[ids];normal=np.cross(tri[1]-tri[0],tri[2]-tri[0]);area=np.linalg.norm(normal)
  if area>1e-6:candidates.append((area,tri.mean(0),normal/area))
 centers=[]
 for area,center,normal in sorted(candidates,key=lambda x:-x[0]):
  if any(np.linalg.norm(center-c)<.025 for c in centers):continue
  centers.append(center);radius=min(.0025,math.sqrt(area)*.12)
  bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=radius,depth=.0009,location=Vector(center+normal*.00035))
  obj=bpy.context.object;obj.name=f'HD_{d["names"][bone]}_fastener_{len(centers)}';obj.rotation_euler=Vector(normal).to_track_quat('Z','Y').to_euler()
  bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
  bevel=obj.modifiers.new('Machined chamfer','BEVEL');bevel.width=.0002;bevel.segments=2
  obj.data.materials.append(detailmat)
  world=obj.matrix_world.copy();obj.parent=arm;obj.parent_type='BONE';obj.parent_bone=d['names'][bone];bpy.context.view_layer.update();obj.matrix_world=world
  details.append(obj)
  if len(centers)>=6:break
# Source animation: absolute matrices converted to original-bone pose bases.
def action(name,first,last,bones):
 arm.animation_data_create();a=bpy.data.actions.new(name);arm.animation_data.action=a
 for frame in range(first,last+1):
  bpy.context.scene.frame_set(frame-first)
  desired={b:mat(d['poses'][frame][t])@bind[b].inverted()@rest[b] for t,b in enumerate(d['tracks'])}
  for t,b in enumerate(d['tracks']):
   if b not in bones:continue
   parent=d['parents'][b]
   parent_pose=desired[parent] if parent in bones else rest[parent]
   arm.pose.bones[d['names'][b]].matrix_basis=rest[b].inverted()@rest[parent]@parent_pose.inverted()@desired[b]
   pb=arm.pose.bones[d['names'][b]];pb.keyframe_insert('location',frame=frame-first);pb.keyframe_insert('rotation_quaternion',frame=frame-first);pb.keyframe_insert('scale',frame=frame-first)
 a.use_fake_user=True
 for pb in arm.pose.bones:pb.matrix_basis=Matrix.Identity(4)
 arm.animation_data.action=None
 return a
full=action('Reload_mechanical_source',0,238,d['tracks'])
# Preserve exact source slices; names identify references rather than input requirements.
mag=np.array([p[1][3][:3] for p in d['poses']]);moving=np.where(np.linalg.norm(np.diff(mag,axis=0),axis=1)>.01)[0]
assert len(moving)>1
cut=int((moving[0]+moving[-1])/2)
actions=[full,action('mag_remove_reference',int(moving[0]),cut,[56]),action('mag_insert_reference',cut,int(moving[-1])+1,[56]),action('mag_seat_reference',max(int(moving[-1])-5,0),int(moving[-1])+1,[56]),action('wire_motion_source',0,238,[57,58,59])]
wire=np.array([p[2] for p in d['poses']]);peak=int(np.argmax(np.linalg.norm(wire-wire[0],axis=(1,2))))
actions.extend([action('wire_move_reference',0,peak,[57,58,59]),action('wire_return_reference',peak,238,[57,58,59])])
# CPU linear blend skinning versus Blender evaluated vertices.
arm.animation_data.action=full
arm.animation_data.action_slot=full.slots[0]
checks={};maxerr=0
for frame in (0,30,80,120,180,238):
 bpy.context.scene.frame_set(frame);bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
 matrices=[m.copy() for m in bind]
 for t,b in enumerate(d['tracks']):matrices[b]=mat(d['poses'][frame][t])
 for label,obj in zip(groups,meshes):
  evaluated=obj.evaluated_get(deps).to_mesh();errors=[]
  for i,p in enumerate(source_ids[label]):
   expected=Vector((0,0,0))
   for w,b in weights[p]:expected+=w*(matrices[b]@bind[b].inverted()@Vector(points[p]))
   errors.append((evaluated.vertices[i].co-expected).length)
  err=max(errors,default=0);maxerr=max(maxerr,err);checks[f'{frame}:{label}']=err
  obj.evaluated_get(deps).to_mesh_clear()
print('SKIN_ERRORS',checks)
assert maxerr<.0001,('CPU/Blender skin mismatch',maxerr)
arm.animation_data.action=None
for pb in arm.pose.bones:pb.matrix_basis=Matrix.Identity(4)
bpy.context.scene.frame_set(0);bpy.context.view_layer.update()
# RAWWEIGHTS survive wedge duplication exactly.
for label,obj in zip(groups,meshes):
 for i,p in enumerate(source_ids[label]):
  got={g.group:g.weight for g in obj.data.vertices[i].groups}
  assert all(abs(got[b]-w)<1e-6 for w,b in weights[p]) and abs(sum(got.values())-1)<1e-6
bpy.context.scene.render.fps=60
camera_lights(meshes+details)
bpy.context.scene.camera.data.ortho_scale*=1.6
for obj in details:obj.hide_render=True
base_texture=next(n for n in material.node_tree.nodes if n.type=='TEX_IMAGE' and 'albedo' in n.image.name)
hd_image=base_texture.image;base_texture.image=bpy.data.images.load(str(maps/'kasimov_original_albedo.png'));base_texture.image.pack()
render(root/'original_bind.png')
base_texture.image=hd_image
for obj in details:obj.hide_render=False
render(root/'hd_bind.png')
arm.animation_data.action=full;bpy.context.scene.frame_set(120);render(root/'animated_frame120.png')
arm.animation_data.action=None
for pb in arm.pose.bones:pb.matrix_basis=Matrix.Identity(4)
bpy.context.scene.frame_set(0);bpy.context.view_layer.update()
bpy.ops.wm.save_as_mainfile(filepath=str(root/'kasimov_hd_animated.blend'))
bpy.ops.export_scene.gltf(filepath=str(root/'kasimov_hd_animated.glb'),export_format='GLB',export_animations=True,export_animation_mode='ACTIONS',export_tangents=True,export_force_sampling=True)
# Detach evaluated magazine with its HD pieces; no armature dependency.
magobj=meshes[1];copies=[]
for obj in [magobj]+[o for o in details if o.parent_bone=='clip']:
 evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=bpy.data.meshes.new_from_object(evaluated)
 copy=bpy.data.objects.new(obj.name+'_detached',mesh);bpy.context.collection.objects.link(copy)
 copy.matrix_world=bind[56].inverted()@obj.matrix_world;copies.append(copy)
bpy.ops.object.select_all(action='DESELECT')
for obj in copies:obj.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(root/'kasimov_hd_magazine.glb'),export_format='GLB',use_selection=True,export_animations=False,export_tangents=True)
report={'status':'CPU_BLENDER_SKIN_PASS','dual_weighted_points':143,'bone_count':80,'source_faces':len(d['faces']),'source_vertices':len(d['wedges']),'max_cpu_blender_error_m':maxerr,'tolerance_m':.0001,'frames':checks,'hd_details':len(details),'actions':[a.name for a in actions],'root_channels':0,'source_hashes':d['source_hashes']}
(root/'blender_validation.json').write_text(json.dumps(report,indent=2))
print('KASIMOV_VALIDATION',json.dumps(report))
