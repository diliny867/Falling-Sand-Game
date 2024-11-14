import os

file_name = "ALL_MATERIALS.h"
file = open(file_name, "w")

file.write("#pragma once\n\n")

directory = "materials"
for filename in os.listdir(directory):
    filepath = os.path.join(directory, filename)
    if os.path.isfile(filepath):
        file.write(f'#include "{directory}/{filename}"\n')
        print(f'included "{directory}/{filename}"')

print(f'"{file_name}" is ready')