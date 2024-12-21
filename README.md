# StarChart
A program used for mapping and navigating the stars!


The program parses a .csv file of stars and associated data, turns the ICRS coords (RA/Dec) and turns them into Cartesian coords (x, y, z); it stores these as star structs in a dynamic array. From there sub structures are built (KD-tree, and hash map) for spatial operations (nearest neighbor, radius search) and fast direct look-ups respectively. Ultimately, these structures and functions are used to create a star path originating from a "player position" to a destination star!


---------- <<< OLD README FILE BELOW, WORKING ON UPDATING THIS THING >>> ------------

# Star Chart Data

## Introduction
This CSV file contains data on various stars, including their names, Right Ascension (RA), Declination (Dec), and distance from Sol in lightyears. This data is used in the Star Chart program to convert the star's celestial coordinates to 3D cartesian coordinates for mapping a star chart.

## Key
| Column Name      | Description                                     |
|------------------|-------------------------------------------------|
| `Star Name`      | The name of the star                            |
| `RA Hours`       | The first value of three in the RA coordinate   |
| `RA Minutes`     | The second value of three in the RA coordinate  |
| `RA Seconds`     | The third value of three in the RA coordinate   |
| `Dec Degrees`    | The first value of three in the DEC coordinate  |
| `Dec Minutes`    | The second value of three in the DEC coordinate |
| `Dec Seconds`    | The third value of three in the DEC coordinate  |
| `Distance (ly)`  | The distance from Earth in light-years          |

## Data Format
- **Star Name**: String (text)
- **RA Hours/Minutes/Seconds**: Double (first part of the celestial coordinates)
- **Dec Degrees/Minutes/Seconds**: Double (second part of the celestial coordinates)
- **Distance**: Double (in light-years)

## Usage Instructions
This CSV file is read during the 'ParseFile(Star *star_list)' call in the main 'star_chart.c' file. The implementation can be found in 'star_chart_utils.c'.

## Contact Information
For questions, please contact Cody Chase at cody.m.chase@gmail.com.

## Version History
- **v1.0**: Initial release

## Acknowledgments
This CSV file was derived from information provided by the SIMBAD Astronomical Database operated at CDS, Strasbourg, France. 
