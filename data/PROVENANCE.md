# Input provenance and schema

All three fixtures were created for this project in the current AI-assisted
session. No proprietary CAD/ANSYS files or third-party meshes are included.
The examples are dedicated to the public domain under CC0 1.0:
https://creativecommons.org/publicdomain/zero/1.0/

- rod.stl: rectangular prism, x=0..1 m, y=0..0.01 m, z=0..0.01 m, twelve triangles.
  It already exists as an ASCII STL file imported by the application. Zero normals
  are permitted because the renderer draws edges; winding/topology is not analysed.
- synthetic_temperature.csv: 84 nodes, 21 axial positions and four corner nodes
  per cross-section. Temperature = 100-80*x+500*x*(1-x) C, x measured in meters.
  This is **synthetic ANSYS-schema data**, NOT an actual ANSYS export or simulation.
- invalid.csv: deliberately invalid non-finite temperature for error demonstration.

For an actual ANSYS export, use nodal temperatures with Cartesian coordinates,
convert coordinates to meters and temperature to Celsius, and export five
unquoted columns with exactly this header:
node_id,x_m,y_m,z_m,temperature_C
An export from a different coordinate system must be transformed upstream. A real
ANSYS export was not available or tested; the supported contract is the stated CSV.
