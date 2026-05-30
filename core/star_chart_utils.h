#ifndef STAR_CHART_UTILS_H
#define STAR_CHART_UTILS_H

#include <stddef.h> // for size_t

#define PI 3.14159265358979323846

// Forward declaration of KDNode; driven by StarPath (GetNeighbors)
typedef struct kd_node kd_node_t;
typedef struct position position_t;

typedef struct star {
	char *name;
	kd_node_t *kd_node;
	position_t *position;
    struct star *came_from; // For A* path finding
    char *sp_type;
    float jump_range;
    float g_cost;
	float light_years;
	float path_cost; // For star path
} star_t;

typedef struct star_array {
	star_t *stars;
	size_t capacity;
	size_t size;
} star_array_t;

typedef struct kd_node {
	star_t *star;
	kd_node_t *left;
	kd_node_t *right;
} kd_node_t;

typedef struct position {
	double x;
	double y;
	double z;
} position_t;

typedef struct hash_entry {
	char *key;
	star_t *value;
	struct hash_entry *next; // For chaining
} hash_entry_t;

typedef struct hash_map {
	hash_entry_t **buckets;
	int count;
	int size;
} hash_map_t;

typedef struct min_heap {
	star_t *elements;
	int capacity;
	int size;
} min_heap_t;

// GLOBAL VARIABLE
extern position_t player_position;

// READ DATABASE FILE
star_array_t* parse_file();

// CONVERT SPECTRAL TYPE TO "JUMP RANGE" EQUIVALENT
float get_jump_range(const char *sp_type);

// CONVERSION MATH TO DETERMINE X, Y, Z, AND NAVIGATION VECTORS
double sign(double value);
double to_decimal_ra(double hours, double minutes, double seconds);
double to_decimal_dec(double degrees, double minutes, double seconds);
void convert_to_3d_coords(double a, double b, double c, double *x, double *y, double *z);

// DATA STRUCTURE CREATION
star_array_t* create_star_array();
kd_node_t* create_balanced_kd_tree(star_t **stars, int start, int end, int depth);
hash_map_t* create_hash_map(star_array_t *star_array, int size);

// ARRAY UTILITY FUNCTIONS
void add_star_to_array(star_array_t *array, star_t *star);
void optimize_star_array_size(star_array_t *array);
void print_star_values(star_array_t *array);
void dealloc_sub_star_array(star_array_t *array);
void dealloc_main_star_array(star_array_t *array);

// KD-TREE UTILITY FUNCTIONS
//TODO: Notice that the GetNeighbors wrapper returns StarArray* but it's worker function returns void, because we're calling the worker function
//      as a variable in the wrapper. Mimic that structure for NearestNeighbor

star_array_t* get_neighbors(kd_node_t *root, position_t *center, float radius);
void radius_search(kd_node_t *node, position_t *center, float radius, int depth, star_array_t *result);
star_t* nearest_neighbor(kd_node_t *root, const position_t reference);
star_t* nearest_neighbor_search(kd_node_t *root, const position_t reference, int depth, star_t *current_closest_star, double *current_best_distance);
star_t* find_nearest_reachable_star(star_t *destination, kd_node_t *root, hash_map_t *map);
void print_kd_tree(kd_node_t *node);
void dealloc_kd_tree(kd_node_t *node);

// HASHMAP UTILITY FUNCTIONS
unsigned long hash(const char *key);
void resize_hash_map(hash_map_t *map);
void add_to_hash_map(hash_map_t *map, const char *key, star_t *value);
star_t* get_from_hash_map(hash_map_t *map, const char *key);
void dealloc_hash_map(hash_map_t *map);

// OTHER UTILITY FUNCTIONS
void set_player_position(float x, float y, float z);
double calculate_distance(const star_t *star, const position_t reference);
int compare_node_x(const void *a, const void *b);
int compare_node_y(const void *a, const void *b);
int compare_node_z(const void *a, const void *b);
double calculate_distance_to_position(star_t *star, position_t *pos);


// STAR PATH FUNCTIONS
void star_path(const char *destination_key, kd_node_t *root, hash_map_t *map, star_array_t *star_array);
star_array_t* star_path_build(star_t *destination, star_array_t *array, kd_node_t *root, hash_map_t *map,  hash_map_t *visited);
void print_star_path(star_array_t *array);
float calculate_euclidean_distance(star_t *current, star_t *goal);
void reset_star_path_state(star_array_t *array);

// HEAP FUNCTIONS
void heapify(star_array_t *heap);
void add_to_heap(star_array_t *heap, star_t *node);
void sift_up(star_array_t *heap, size_t index);
void sift_down(star_array_t *heap, size_t index);
star_t* pop_min(star_array_t *heap);
void peek(star_array_t *heap);

// SECURITY UTIL FUNCTIONS
void secure_zero(void *ptr, size_t size);

#endif
