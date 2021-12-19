import bpy

scene = bpy.context.scene
with open('/home/lorenzo/myfilename.json','w') as f:
    f.write("\"scenegraph\":\n[\n{\n\"meshes\":[\n")
    for obj in scene.objects:
        loc, rot, scale = obj.matrix_world.decompose()
        f.write("{\n")
        f.write(f'"path":"{obj.name}",\n')
        f.write(f'"material":"ObjMaterial",\n')
        f.write('"transform": {\n')
        f.write(f'"scale":[{scale.x},{scale.z},{scale.x}],\n')
        f.write(f'"rotation":[{rot.x},{rot.z},{rot.x}],\n')
        f.write(f'"translation":[{loc.x},{loc.z},{loc.x}]\n')
        f.write("}\n")
        f.write("},\n")
    f.write("]\n}\n]")
    