# StarChart
A program used for mapping and navigating the stars!


The program parses a .csv file of stars and associated data, turns the ICRS coords (RA/Dec) and turns them into Cartesian coords (x, y, z); it stores these as star structs in a dynamic array. From there sub structures are built (KD-tree, and hash map) for spatial operations (nearest neighbor, radius search) and fast direct look-ups respectively. Ultimately, these structures and functions are used to create a star path originating from a "player position" to a destination star!
