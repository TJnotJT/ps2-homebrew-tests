#!/bin/bash

# Define the list of elements
uv_choices=(0 1)
clip_choices=(0 1)
int_choices=(0 1)
tri_choices=(0 1)

uv_str=("st" "uv")
clip_str=("noclip" "clip")
int_str=("noint" "int")
tri_str=("sprite" "tri")

# Iterate over each element in the list
for uv in "${uv_choices[@]}"; do
  for clip in "${clip_choices[@]}"; do
    for int in "${int_choices[@]}"; do
      for tri in "${tri_choices[@]}"; do
        file_name="uv-bounds-${uv_str[$uv]}-${clip_str[$clip]}-${int_str[$int]}-${tri_str[$tri]}"
        defines="-DUV_TYPE=${uv} -DCLIP=${clip} -DINT=${int} -DTRIANGLE=${tri}"
        make "$@" FILE="${file_name}" DEFINES="${defines}"
      done
    done
  done
done

