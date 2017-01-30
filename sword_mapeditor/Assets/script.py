import bpy
import os

path = 'C:/Users/James/Desktop/render_projects/sword_mapeditor/Assets'  # set this path

for root, dirs, files in os.walk(path):
    for f in files:
        if f.lower().endswith('.objc') :
            #mesh_file = os.path.join(path, f)
            mesh_file = os.path.join(root, f).replace("\\", "/")
           
            os.rename(mesh_file, mesh_file[:-1])