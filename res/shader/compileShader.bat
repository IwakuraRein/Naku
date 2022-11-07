for %%x in (*.vert) do glslc %%x -o %%x.spv
for %%x in (*.frag) do glslc %%x -o %%x.spv
for %%x in (*.geom) do glslc %%x -o %%x.spv