# _StarChart_

A spatial navigation and pathfinding engine written in C.

StarChart ingests astronomical catalog data, transforms celestial coordinates into 3D Cartesian space, builds efficient spatial indexing structures, and performs navigation operations such as nearest-neighbor searches, radius searches, and jump-route pathfinding between stars.

The long-term goal is to function as a navigation subsystem for a larger space exploration game engine.

## Capabilities

   1. CSV star catalog parsing
   2. ICRS to Cartesian coordinate conversion
   3. Dynamic star storage
   4. KD-tree spatial indexing
   5. Hash map star lookup by name
   6. Radius-based searches
   7. Nearest-neighbor searches
   8. A* pathfinding for interstellar jump-route generation
   9. Memory-safe dynamic allocation and cleanup
   10. Makefile-based build system
   11. Spectral-type-based jump range calculation
   12. Unreachable destination fallback routing

## How It Works

Star data is loaded from CSV catalogs containing celestial coordinates
(Right Ascension, Declination, and distance).

These coordinates are converted from the ICRS celestial reference frame
into three-dimensional Cartesian coordinates (x, y, z).

The resulting stars are stored in a dynamic array and indexed using
specialized data structures:

- KD-tree for spatial searches
- Hash map for direct name lookups

CSV Data  
   ↓  
ICRS → Cartesian  
   ↓  
Dynamic Star Array  
   ↓  
KD-Tree + Hash Map  
   ↓  
Search + Pathfinding

These structures support navigation operations such as nearest-neighbor
searches, radius searches, and jump-route pathfinding.

#### Jump Range Mechanics
Each star is assigned a jump range based on its spectral classification,
derived from the `sp_type` field in the catalog data. More luminous stars
produce deeper gravity wells and greater jump gate energy, allowing longer
interstellar jumps.

| Class | Example      | Jump Range |
|-------|--------------|------------|
| O, B  | Rigel        | 25.0 ly    |
| A     | Sirius       | 20.0 ly    |
| F     | Procyon      | 15.0 ly    |
| G     | Sol          | 10.0 ly    |
| K     | Epsilon Indi | 7.0 ly     |
| M     | Proxima Cen  | 8.0 ly     |
| Other | White dwarfs | 0.0 ly     |

Stars with zero jump range appear on the map but cannot serve as
navigation waypoints.

## Project Architecture:

   StarChart  
   ├── core/  
   ├── data/  
   ├── docs/  
   ├── experimental/  
   ├── tools/  
   ├── Makefile  
   └── README.md  

### _core/_

Contains the primary application source code.

   - star_chart.c
   - star_chart_utils.c
   - star_chart_utils.h

### _data/_

Contains star catalog data.

   - stars.csv
   - stars_query.csv

### _tools/_

Contains utility scripts used to generate data.

   - simbad_query.py

### _experimental/_

Visualization and testing utilities.

   - star_view.py
   - star_view_enhanced.py
   - etc.

### _docs/_

Project documentation and design notes.

   - star_path_pseudocode.txt
   - future_dev.txt

## Building

### Quick Start

    git clone https://github.com/CodyChase88/StarChart
    cd StarChart
    make
    ./sc

#### Requirements:
   - GCC
   - Make
   - Standard C Library
   - libm  

#### Compiler Flags:
   - -Wall
   - -Wextra
   - -Werror
   - -g

#### Make Targets
   - 'make' → build project
   - 'make clean' → remove build artifacts
   - 'make re' → clean + rebuild

#### Running  
_(After building)_
- ./sc

## Example Output

    [voider@TheVoid StarChart]$ make
    mkdir -p obj
    gcc -Wall -Wextra -Werror -g -c core/star_chart.c -o obj/star_chart.o
    mkdir -p obj
    gcc -Wall -Wextra -Werror -g -c core/star_chart_utils.c -o obj/star_chart_utils.o
    gcc -Wall -Wextra -Werror -g -o sc obj/star_chart.o obj/star_chart_utils.o -lm
    [voider@TheVoid StarChart]$ ./sc

    StarChart initialized
    Loaded 1152 stars from SIMBAD catalog
    Building star map...
    Searching route from 'Sol' to '* del Eri'

    Path:
    Sol -> G 272-61 -> * eps Eri -> * omi02 Eri -> BD-17 588BC -> BD-17 588 -> * del Eri
    
    Total distance: 43.83 ly
    -----------------------

## Data Sources

Star data is derived from astronomical catalogs and may be supplemented using queries generated through the SIMBAD tooling script.

The __tools/simbad_query.py__ script can be used to generate updated star datasets.

Python tooling is _optional_ and is _not required_ to build or run the core application.

## Data Integrity

StarChart operates on real astronomical catalog data and does not procedurally generate star positions.

## Development Notes

#### Design goals:

   1. Maintain a pure C navigation engine.
   2. Keep spatial operations efficient through indexing structures.
   3. Minimize unnecessary allocations.
   4. Separate data generation from runtime execution.

#### Future Development

See _'docs/future_dev.txt'_ for planned features, ideas, experiments, and roadmap items.

## Technical Highlights

- Dynamic memory management
- KD-tree spatial indexing
- Hash map lookups
- A* pathfinding
- Spectral classification mapped to jump ranges for realistic routing constraints
- Fallback routing when destination is unreachable — nav computer
  finds nearest accessible star and reroutes to that location, providing the distance from there to intended destination
- Modular C architecture
- Warning-free compilation (-Wall -Wextra -Werror)
- Valgrind-clean memory management

## Acknowledgements

Astronomical data sourced from the SIMBAD Astronomical Database operated by CDS, Strasbourg, France.