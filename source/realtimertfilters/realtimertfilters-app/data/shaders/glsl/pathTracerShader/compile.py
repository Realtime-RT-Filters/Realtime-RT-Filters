import os
shaders = ["raygen.rgen","closesthit.rchit","miss.rmiss","shadow.rmiss"]
for shader in shaders:
    input = shader
    output = shader + ".spv"
    command = f"glslc.exe --target-env=vulkan1.2 -o {output} {input}"
    res = os.system(command)
    if res != 0:
        print("Error")
print ("Done!")