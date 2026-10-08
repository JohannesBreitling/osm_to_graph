# osm_to_graph
This application converts a OpenStreetMap xml file (e.g. downloaded from (here)[https://extract.bbbike.org/]) to a road graph, that can be used as an input for [jbmaps](https://github.com/JohannesBreitling/jbmaps).

## Usage
It requires CMake and C++20. To simplify, we use `just` for the most important commands. Compile the runner using:

```sh
just config release
just build release convert_osm_to_graph
```

Then run it with:
```sh
just run release convert_osm_to_graph <path_to_osm_file> <path_to_output_dir>
```

It will convert the input osm xml file to a road network in adjacency array representation and save the different
attributes to multiple files. It will also create an address dictionary used for routing queries later.
Use the full folder as an input for jbmaps.
