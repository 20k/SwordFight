import bpy
import os

path = 'C:/Users/James/Desktop/render_projects/sword_mapeditor/Assets'  # set this path

for root, dirs, files in os.walk(path):
    for f in files:
        if f.lower().endswith('.obj') :
            #mesh_file = os.path.join(path, f)
            mesh_file = os.path.join(root, f).replace("\\", "/")
            print (mesh_file)
            #obj_file = os.path.splitext(mesh_file)[0] + ".obj"

            obj_file = mesh_file

            bpy.ops.object.select_all(action='SELECT')
            bpy.ops.object.delete()

            bpy.ops.import_scene.obj(filepath=mesh_file) # change this line

            bpy.ops.object.select_all(action='SELECT')
            
            for obj in bpy.data.objects:
                bpy.context.scene.objects.active = obj
                bpy.ops.object.mode_set(mode='EDIT')
                
                if not bpy.context.object.data.uv_layers:
                    bpy.ops.uv.cube_project()
                    
                bpy.ops.object.mode_set(mode='OBJECT')

            bpy.ops.export_scene.obj(filepath=(obj_file), use_normals=True, use_uvs=True, use_materials=True, use_triangles=True, use_blen_objects=True, axis_forward='Z')