# UEStress

Unreal Engine Stress Test Project

This is a simple C++ project created to stress test the upper limits for
static meshes, actors, components, etc. It is by no means comprehensive, but
more of a work in progress to hack on as I learn about features that I'd like
to test. For now it just creates a large amount of geometry to test with when
features like Nanite and Lumen are enabled or disabled.

To use this, you will need Unreal Engine and Visual Studio installed. Right
click on UEStress.uproject, click "Generate Visual Studio project files", then
open up the UEStress.sln file that was created. You can then compile and run
the project in the Unreal Engine editor.

It currently does not use blueprints, so changing settings requires updating
the code in the UEStressPawn.cpp constructor and recompiling.

To generate multiple static meshes with high triangle counts quickly, install
Blender and run the following code inside the Blender Python console:

```
total = 100
random_offset = 0
for index in range(total):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=5, radius=50)
    bpy.ops.object.mode_set(mode='EDIT', toggle=False)
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.transform.vertex_random(offset=2, seed=random_offset + index)
    bpy.ops.object.mode_set(mode='OBJECT', toggle=False)
    bpy.ops.object.modifier_add(type='SUBSURF')
    bpy.context.object.modifiers["Subdivision"].levels = 2
    bpy.ops.object.modifier_apply(modifier="Subdivision")
    object = bpy.context.view_layer.objects.active
    object.name = '%02d' % (index % total)
    for polygon in object.data.polygons:
        polygon.use_smooth = True
```

This will create 100 meshes each with about 122k triangles. You can then
export these as a file named '00.fbx' and import them into the 'Meshes'
directory in the editor. The pawn class will then load these on startup.