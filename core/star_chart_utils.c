#include "star_chart_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

// GLOBAL VARIABLES
position_t player_position = {0.0, 0.0, 0.0}; // Sol
// Position player_position = {0.01, 0.0, 0.01}; // Sol-ish

// Position player_position = {10.29, 5.02, -3.27}; // Tau Ceti
// Position player_position = {10.28, 5.02, -3.29}; //Tau Ceti-ish

// READ DATABASE FILE
star_array_t* parse_file() {
    FILE *file = fopen("data/stars_query.csv", "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR [parse_file()]: FILE FAILED TO OPEN!\n");
        return NULL;
    }

    int max_tokens = 9;
    char line[1024];
    star_array_t *array = create_star_array();

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // remove newline character

        void *tokens[max_tokens];
        int token_count = 0;

        char *token = strtok(line, ",");
        while (token != NULL && token_count < max_tokens) {
            tokens[token_count++] = token;
            token = strtok(NULL, ",");
        }

        if (token_count == max_tokens) {
            char *name = tokens[0];
            int ra_hours = (int)strtod(tokens[1], NULL);
            double ra_minutes = strtod(tokens[2], NULL);
            double ra_seconds = strtod(tokens[3], NULL);
            int dec_degrees = (int)strtod(tokens[4], NULL);
            double dec_minutes = strtod(tokens[5], NULL);
            double dec_seconds = strtod(tokens[6], NULL);
            float light_years = strtod(tokens[7], NULL);
            char *sp_type = tokens[8];

            star_t new_star;
            new_star.name = strdup(name);
            new_star.kd_node = NULL;
            new_star.light_years = light_years;
            new_star.path_cost = FLT_MAX;
            new_star.g_cost = FLT_MAX;
            new_star.came_from = NULL;
            new_star.position = calloc(1, sizeof(position_t));
            if (new_star.position == NULL) {
                fprintf(stderr, "ERROR [parse_file()]: Memory allocation failed for new star allocation\n");
                dealloc_main_star_array(array);
                return NULL;
            }
            new_star.sp_type = strdup(sp_type);
            new_star.jump_range = get_jump_range(sp_type);

            convert_to_3d_coords(to_decimal_ra(ra_hours, ra_minutes, ra_seconds),
                    to_decimal_dec(dec_degrees, dec_minutes, dec_seconds),
                    light_years, &new_star.position->x, &new_star.position->y, &new_star.position->z);

            add_star_to_array(array, &new_star);
        }
    }

    optimize_star_array_size(array);

    fclose(file);
    return array;
}

// CONVERT SPECTRAL TYPE TO "JUMP RANGE" EQUIVALENT
float get_jump_range(const char *sp_type) {
    if (sp_type == NULL || sp_type[0] == '\0') return 3.0f;
    
    // Handle dwarf prefix 'd' - skip to actual spectral class
    const char *sp = sp_type;
    if (sp[0] == 'd' && sp[1] != '\0') sp++;
    
    switch (sp_type[0]) {
        // Consider setting a base jump distance here
        // Players with upgraded drives can add multipliers to the bases below!
        case 'O': case 'B': return 25.0f;
        case 'A': return 20.0f;
        case 'F': return 15.0f;
        case 'G': return 10.0f;
        case 'K': return 10.0f;
        case 'M': return 8.0f;
        default: return 0.0f;
    }
}

// CONVERSION MATH TO DETERMINE X, Y, Z, AND NAVIGATION VECTORS
double sign(double value) { return (value > 0) ? 1.0 : -1.0; }

double to_decimal_ra(double hours, double minutes, double seconds) {
    return (hours * 15) + (minutes * 0.25) + (seconds * 0.004166);
}

double to_decimal_dec(double degrees, double minutes, double seconds) {
    return (fabs(degrees) + (minutes / 60) + (seconds / 3600.0)) * sign(degrees);
}

void convert_to_3d_coords(double a, double b, double c, double *x, double *y, double *z) {
    double a_radians = a * (PI / 180);
    double b_radians = b * (PI / 180);

    *x = (c * cos(b_radians)) * cos(a_radians);
    *y = (c * cos(b_radians)) * sin(a_radians);
    *z = c * sin(b_radians);
}

// DATA STRUCTURE CREATION
star_array_t* create_star_array() {
    star_array_t *array = calloc(1, sizeof(star_array_t));
    if (!array) {
        fprintf(stderr, "ERROR [create_star_array()]: MEMORY ALLOCATION FAILED FOR STAR ARRAY!\n");
        return NULL;
    }

    array->size = 0;
    array->capacity = 1024; // Initial capacity
    array->stars = calloc(array->capacity, sizeof(star_t));
    if (!array->stars) {
        fprintf(stderr, "ERROR [create_star_array()]: MEMORY ALLOCATION FAILED FOR STARS ARRAY!\n");
        return NULL;
    }

    return array;
}

kd_node_t* create_balanced_kd_tree(star_t **stars, int start, int end, int depth) {
    if (start > end)
        return NULL;

    int axis = depth % 3;

    switch (axis) {
        case 0: qsort(stars + start, (end - start) + 1, sizeof(star_t*), compare_node_x); break;
        case 1: qsort(stars + start, (end - start) + 1, sizeof(star_t*), compare_node_y); break;
        case 2: qsort(stars + start, (end - start) + 1, sizeof(star_t*), compare_node_z); break;
        default: return NULL; break;
    }

    int mid = (start + end) / 2;

    kd_node_t *node = calloc(1, sizeof(kd_node_t));
    if (node == NULL) {
        fprintf(stderr, "ERROR [create_balanced_kd_tree()]: MEMORY ALLOCATION FAILED FOR KD NODE!\n");
        return NULL;
    }

    node->star = stars[mid];
    node->left = NULL;
    node->right = NULL;
    node->star->kd_node = node;

    if (start <= mid - 1) {
        node->left = create_balanced_kd_tree(stars, start, mid - 1, depth + 1);
    }

    if (mid + 1 <= end) {
        node->right = create_balanced_kd_tree(stars, mid + 1, end, depth + 1);
    }

    return node;
}

hash_map_t* create_hash_map(star_array_t *star_array, int size) {
    hash_map_t *map = calloc(1, sizeof(hash_map_t));
    if (map == NULL) {
        fprintf(stderr, "ERROR [create_hash_map()]: MEMORY ALLOCATION FAILED FOR HASHMAP!\n");
        return NULL;
    }

    map->size = size;
    map->buckets = calloc(map->size, sizeof(hash_entry_t*));
    if (map->buckets == NULL) {
        fprintf(stderr, "ERROR [create_hash_map()]: MEMORY ALLOCATION FAILED FOR HASH ENTRY!\n");
        free(map);
        return NULL;
    }

    map->count = 0;

    for (size_t i = 0; i < star_array->size; i++) {
        add_to_hash_map(map, star_array->stars[i].name, &star_array->stars[i]);
    }

    return map;
}

// ARRAY UTILITY FUNCTIONS
void add_star_to_array(star_array_t *array, star_t *star) {
    if (array->size == array->capacity) {
        array->capacity = (int)array->capacity * 1.5; // Increase capacity if full
        array->stars = realloc(array->stars, array->capacity * sizeof(star_t));

        if (!array->stars) {
            fprintf(stderr, "ERROR [add_star_to_array()]: MEMORY ALLOCATION FAILED DURING REALLOC!\n");
            return;
        }
    }

    array->stars[array->size++] = *star;
}

void optimize_star_array_size(star_array_t *array) {
    if (array->size < array->capacity) {
        array->stars = realloc(array->stars, array->size * sizeof(star_t));
        if (!array->stars && array->size > 0) {
            fprintf(stderr, "ERROR [optimize_star_array_size()]: MEMORY REALLOCATION FAILED DURING OPTIMIZATION!\n");
            return;
        }
        array->capacity = array->size;
    }
}

void print_star_values(star_array_t *star_array) {
    for (size_t i = 0; i < star_array->size; i++) {
        printf("%s: (%.2f, %.2f, %.2f)\n", star_array->stars[i].name,
                star_array->stars[i].position->x, star_array->stars[i].position->y,
                star_array->stars[i].position->z);
        printf("Light years from Sol: %.3f\n", star_array->stars[i].light_years);
        printf("path score: %.2f\n\n", star_array->stars[i].path_cost);
    }
}

void dealloc_sub_star_array(star_array_t *array) {
    if (array) {
        free(array->stars);
        free(array);
    }
}

void dealloc_main_star_array(star_array_t *array) {
    if (array) {
        for (size_t i = 0; i < array->size; i++) {
            if (array->stars[i].name) {
                free(array->stars[i].name);
            }
            if (array->stars[i].position) {
                free(array->stars[i].position);
            }
            if (array->stars[i].sp_type) {
                free(array->stars[i].sp_type);
            }
        }
        free(array->stars); // Free stars array
        free(array);        // Free array struct
    }
}

// KD-TREE UTILITY FUNCTIONS

// Returns all stars within radius light years of center point
// Used by Nav Computer for local space awareness and pathfinding
star_array_t* get_neighbors(kd_node_t *root, position_t *center, float radius) {
    star_array_t *result = create_star_array();
    radius_search(root, center, radius, 0, result);
    return result;
}

void radius_search(kd_node_t *node, position_t *center, float radius, int depth, star_array_t *result) {
    if (node == NULL) {
        // Base case
        return;
    }

    float distance = calculate_distance_to_position(node->star, center);
    if (distance <= radius) {
        add_star_to_array(result, node->star);
    }

    int axis = depth % 3;
    float diff = 0;

    switch (axis) {
        case 0:
            diff = center->x - node->star->position->x; 
            break;
        case 1:
            diff = center->y - node->star->position->y; 
            break;
        case 2:
            diff = center->z - node->star->position->z;
            break;
        default:
            break;
    }

    if (diff <= 0) {
        radius_search(node->left, center, radius, depth + 1, result);

        if (fabs(diff) <= radius) {
            radius_search(node->right, center, radius, depth + 1, result);
        }
    } else {
        radius_search(node->right, center, radius, depth + 1, result);

        if (fabs(diff) <= radius) {

            radius_search(node->left, center, radius, depth + 1, result);
        }
    }
}

star_t* nearest_neighbor(kd_node_t *root, const position_t reference) {
    double current_best_distance = DBL_MAX;
    star_t *neighbor = nearest_neighbor_search(root, reference, 0, NULL, &current_best_distance);

    return neighbor;
}

star_t* nearest_neighbor_search(kd_node_t *node, const position_t reference, int depth, star_t *current_closest_star, double *current_best_distance){
    if (node == NULL) {
        return current_closest_star;
    }

    double distance = calculate_distance(node->star, reference);
    if (distance < *current_best_distance) {
        *current_best_distance = distance;
        current_closest_star = node->star;
    }

    int axis = depth % 3;
    double diff = 0.0;

    if (axis == 0) {
        diff = reference.x - node->star->position->x;
    }
    else if (axis == 1) {
        diff = reference.y - node->star->position->y;
    } else {
        diff = reference.z - node->star->position->z;
    }

    kd_node_t *near_subtree = NULL;
    kd_node_t *far_subtree = NULL;

    if (diff <= 0) {
        near_subtree = node->left;
        far_subtree = node->right;
    } else {
        near_subtree = node->right;
        far_subtree = node->left;
    }

    current_closest_star = nearest_neighbor_search(near_subtree, reference, depth + 1, current_closest_star, current_best_distance);

    if (fabs(diff) < *current_best_distance) {
        current_closest_star = nearest_neighbor_search(far_subtree, reference, depth + 1, current_closest_star, current_best_distance);
    }

    return current_closest_star;
}

star_t* find_nearest_reachable_star(star_t *destination, kd_node_t *root, hash_map_t *map) {
    star_array_t *candidates = NULL;
    float search_radius = 5.0f;
    star_t *nearest_reachable = NULL;

    while (search_radius < 50.0f && nearest_reachable == NULL) {
        if (candidates) dealloc_sub_star_array(candidates);

        candidates = get_neighbors(root, destination->position, search_radius);
        for (size_t i = 0; i < candidates->size; i++) {
            star_t *candidate = get_from_hash_map(map, candidates->stars[i].name);
            if (candidate != NULL && candidate->jump_range > 0.0f && candidate != destination) {
                nearest_reachable = candidate;
                break;
            }
        }
        search_radius += 2.0f;
    }

    if (candidates) dealloc_sub_star_array(candidates);
    return nearest_reachable;
}

void print_kd_tree(kd_node_t *node) {
    if (node == NULL) {
        return;
    }
    print_kd_tree(node->left);

    printf("%s: (%.2f, %.2f, %.2f), %.2f\n", node->star->name, node->star->position->x,
            node->star->position->y, node->star->position->z, node->star->light_years);

    print_kd_tree(node->right);
}

void dealloc_kd_tree(kd_node_t *node) {
    if (node) {
        dealloc_kd_tree(node->left);
        dealloc_kd_tree(node->right);
        free(node);
    }
}

// HASHMAP UTILITY FUNCTIONS
unsigned long hash(const char *key) {
    // djb2 algorithm
    unsigned long hash = 5381;
    int c = 0;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

void resize_hash_map(hash_map_t *map) {
    int new_size = map->size * 2;
    hash_entry_t **new_buckets = calloc(new_size, sizeof(hash_entry_t*));
    if (new_buckets == NULL) {
        fprintf(stderr, "ERROR [resize_hash_map()]: MEMORY ALLOCATION FAILED DURING HASH MAP RESIZE!\n");
        return;
    }

    for (int i = 0; i < map->size; i++) {
        hash_entry_t *entry = map->buckets[i];
        while (entry) {
            hash_entry_t *next_entry = entry->next;
            unsigned int index = hash(entry->key) % new_size;

            entry->next = new_buckets[index];
            new_buckets[index] = entry;

            entry = next_entry;
        }
    }
    free(map->buckets);
    map->buckets = new_buckets;
    map->size = new_size;
}

void add_to_hash_map(hash_map_t *map, const char *key, star_t *value) {
    // Check and resize if load factor exceeds 0.75
    if ((float)(map->count + 1) / map->size > 0.75) {
        resize_hash_map(map);
    }

    unsigned int index = hash(key) % map->size; // Compute bucket index
                                                // Allocate new hash entry
    hash_entry_t *new_entry = calloc(1, sizeof(hash_entry_t));
    if (!new_entry) {
        fprintf(stderr, "ERROR [add_to_hash_map()]: MEMORY ALLOCATION FAILED DURING ADD TO HASH MAP!\n");
        return;
    }
    new_entry->key = strdup(key);
    if (!new_entry->key) {
        fprintf(stderr, "ERROR [add_to_hash_map()]: MEMORY ALLOCATION FAILED FOR KEY STRING!\n");
        free(new_entry);
        return;
    }
    new_entry->value = value;

    // Handle collisions with chaining
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;

    map->count++;
}

star_t* get_from_hash_map(hash_map_t *map, const char *key) {
    unsigned int index = hash(key) % map->size;
    hash_entry_t *entry = map->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        } 

        entry = entry->next;

    }
    return NULL;
}

void dealloc_hash_map(hash_map_t *map) {
    if (map == NULL) {
        return;
    }

    for (int i = 0; i < map->size; i++) {
        hash_entry_t *entry = map->buckets[i];
        while (entry) {
            hash_entry_t *temp = entry;
            entry = entry->next;
            free(temp->key);
            free(temp);
        }
    }
    free(map->buckets);
    free(map);
}

// OTHER UTILITY FUNCTIONS
void set_player_position(float x, float y, float z) {
    player_position.x = x;
    player_position.y = y;
    player_position.z = z;
}

double calculate_distance(const star_t *star, const position_t reference) {
    double dx = star->position->x - reference.x;
    double dy = star->position->y - reference.y;
    double dz = star->position->z - reference.z;

    return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

int compare_node_x(const void *a, const void *b) {
    const star_t *star_a = *(star_t**)a;
    const star_t *star_b = *(star_t**)b;

    if (star_a->position->x < star_b->position->x)
        return -1;
    if (star_a->position->x > star_b->position->x)
        return 1;

    // Tie-breaking on y-coordinate
    if (star_a->position->y < star_b->position->y)
        return -1;
    if (star_a->position->y > star_b->position->y)
        return 1;

    // Tie-breaking on z-coordinate
    return (star_a->position->z < star_b->position->z) ? -1 : (star_a->position->z > star_b->position->z) ? 1 : 0;
}

int compare_node_y(const void *a, const void *b) {
    const star_t *star_a = *(star_t**)a;
    const star_t *star_b = *(star_t**)b;

    if (star_a->position->y < star_b->position->y)
        return -1;
    if (star_a->position->y > star_b->position->y)
        return 1;

    // Tie-breaking on x-coordinate
    if (star_a->position->x < star_b->position->x)
        return -1;
    if (star_a->position->x > star_b->position->x)
        return 1;

    // Tie-breaking on z-coordinate
    return (star_a->position->z < star_b->position->z) ? -1 : (star_a->position->z > star_b->position->z) ? 1 : 0;
}

int compare_node_z(const void *a, const void *b) {
    const star_t *star_a = *(star_t**)a;
    const star_t *star_b = *(star_t**)b;

    if (star_a->position->z < star_b->position->z)
        return -1;
    if (star_a->position->z > star_b->position->z)
        return 1;

    // Tie-breaking on x-coordinate
    if (star_a->position->x < star_b->position->x)
        return -1;
    if (star_a->position->x > star_b->position->x)
        return 1;

    // Tie-breaking on y-coordinate
    return (star_a->position->y < star_b->position->y) ? -1 : (star_a->position->y > star_b->position->y) ? 1 : 0;
}

double calculate_distance_to_position(star_t *star, position_t *pos) {
    double dx = star->position->x - pos->x;
    double dy = star->position->y - pos->y;
    double dz = star->position->z - pos->z;

    return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

// STAR PATH FUNCTIONS
// In the future, replace 'radius' with player's jump distance
// Remove the expanding radius search (*1.5) because player's can't jump outside of jump distance
// Better idea: allow the expansion and with player jump distance, determine (based on count of expansions)
// how many jumps will need to happen to reach next star in path
// GAME NOTE: Players can opt to jump directly to destination but solar worm holes will be quicker which would
// incentivize using star system navigation instead of interstellar jumps

void star_path(const char *destination_key, kd_node_t *root, hash_map_t *map, star_array_t *star_array) {
    star_t *destination = get_from_hash_map(map, destination_key);
    if (destination == NULL) {
        fprintf(stderr, "ERROR [star_path()]: Destination '%s' not found in star map.\n", destination_key);
        return;
    }
    
    star_array_t *open_set = create_star_array();
    hash_map_t *visited = create_hash_map(open_set, 1024);

    star_t *origin = nearest_neighbor(root, player_position);
    origin->g_cost = 0;
    origin->path_cost = origin->g_cost + calculate_euclidean_distance(origin, destination);

    // 6. Add origin to open set before passing to StarPathBuild()
    add_star_to_array(open_set, get_from_hash_map(map, origin->name));

    star_array_t *star_path = star_path_build(destination, open_set, root, map, visited);
    
    // Fallback: if destination unreachable, route to nearest reachable star
    if (star_path->size == 0) {
        printf("Destination unreachable. Finding nearest accessible star...\n");

        // Find nearest star to destination
        star_t *nearest_reachable = find_nearest_reachable_star(destination, root, map);

        if (nearest_reachable != NULL) {
            // Reset search state
            reset_star_path_state(star_array);
            dealloc_hash_map(visited);
            visited = create_hash_map(open_set, 1024);

            // Reset origin and open_set
            secure_zero(open_set->stars, open_set->capacity * sizeof(star_t));
            open_set->size = 0;
            origin->g_cost = 0;
            origin->path_cost = origin->g_cost + calculate_euclidean_distance(origin, nearest_reachable);
            add_star_to_array(open_set, get_from_hash_map(map, origin->name));

            printf("Routing to nearest accessible star: %s\n", nearest_reachable->name);
            printf("Note: %s is %.2f ly from your destination %s\n\n", nearest_reachable->name,
                    calculate_euclidean_distance(nearest_reachable, destination),
                    destination_key);

            dealloc_sub_star_array(star_path);
            star_path = star_path_build(nearest_reachable, open_set, root, map, visited);
        } else {
            printf("No accessible stars found near destination '%s'.\n", destination_key);
        }
    }
    
    print_star_path(star_path);  

    dealloc_sub_star_array(star_path);
    dealloc_hash_map(visited);
    dealloc_sub_star_array(open_set);
    // star_path is freed in main
}

star_array_t* star_path_build(star_t *destination, star_array_t *open_set, kd_node_t *root, hash_map_t *map, hash_map_t *visited) {
    star_array_t *star_path = create_star_array();
    do {
        // 1. Set current star to the smallest node on the heap
        star_t *current_star = pop_min(open_set);
        
        if (strncmp(current_star->name, destination->name, strlen(destination->name) + 1) == 0) {
            star_t *curr_path_star = destination;
        	while (curr_path_star != NULL) {
            	add_star_to_array(star_path, curr_path_star);
            	curr_path_star = curr_path_star->came_from;
        	}
            // Reverse star_path (MAKE THIS A FUNCTION LATER)
            int left = 0;
            int right = star_path->size - 1;
            while (left < right) {
                star_t temp = star_path->stars[left];
                star_path->stars[left] = star_path->stars[right];
                star_path->stars[right] = temp;
                left++;
                right--;
            }
            free(current_star);
            return star_path;
        }

        // Is current star already in visited list?
        if (get_from_hash_map(visited, current_star->name)) {
			free(current_star);
            continue;
        } else {
            // 2. If curr star is not in visited list, add to visited list
            add_to_hash_map(visited, current_star->name, current_star);
            star_array_t *neighbors = get_neighbors(root, current_star->position, current_star->jump_range);
            for (size_t i = 0; i < neighbors->size; i++) {
				star_t *real_neighbor = get_from_hash_map(map, neighbors->stars[i].name);
                
                // Skip stars with no jump capability
                if (real_neighbor->jump_range == 0.0f) continue;

                float tentative_g_cost = current_star->g_cost + calculate_euclidean_distance(current_star, real_neighbor);
                if (tentative_g_cost < real_neighbor->g_cost) {
					real_neighbor->g_cost = tentative_g_cost;
                    real_neighbor->path_cost = tentative_g_cost + calculate_euclidean_distance(real_neighbor, destination);
                    star_t *real_current = get_from_hash_map(map, current_star->name);
                    real_neighbor->came_from = real_current;
                	add_to_heap(open_set, real_neighbor);                
				}
            }
            dealloc_sub_star_array(neighbors);
        }
        free(current_star);
    } while (open_set->size > 0);
    
    if (star_path->size == 0) {
        printf("Destination unreachable from current location.\n");
    }
    
    return star_path;
}               

void print_star_path(star_array_t *array) {
    for (size_t i = 0; i < array->size; i++) {
        printf("%s", array->stars[i].name);
        if (array->size > 0 && i < array->size - 1) {
            printf(" -> ");
        }
    } printf("\n");

    float total = 0;
    for (size_t i = 0; i < array->size - 1; i++) {
        float hop = calculate_euclidean_distance(&array->stars[i], &array->stars[i + 1]);
        printf(" %s -> %s: %.2f ly\n", array->stars[i].name, array->stars[i + 1].name, hop);
        total += hop;
    }
    printf("Total distance: %.2f ly\n", total);
}

// Differs from 'CalculateDistance(...)' in that this compares the distance between stars, not a player position
float calculate_euclidean_distance(star_t *current, star_t *goal) {
    float dx = goal->position->x - current->position->x;
    float dy = goal->position->y - current->position->y;
    float dz = goal->position->z - current->position->z;

    return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

// TODO: Replace with search ID pattern for efficiency (Phase 2)
void reset_star_path_state(star_array_t *array) {
    for (size_t i = 0; i < array->size; i++) {
        array->stars[i].g_cost = FLT_MAX;
        array->stars[i].path_cost = FLT_MAX;
        array->stars[i].came_from = NULL;
    }
}

// HEAP FUNCTIONS
void heapify(star_array_t *heap) {
    for (int i = (heap->size / 2) - 1; i >= 0; i--) {
        sift_down(heap, i);
    }
}

void add_to_heap(star_array_t *heap, star_t *node) {
    if (heap == NULL) {
        fprintf(stderr, "No heap structure found.\n");
        return;
    }
    add_star_to_array(heap, node);
    sift_up(heap, heap->size - 1);    
}

void sift_up(star_array_t *heap, size_t index) {
    if (index <= 0) return;
    size_t parent = (index - 1) / 2;

    if (heap->stars[index].path_cost >= heap->stars[parent].path_cost) return;
    else {
        star_t temp = heap->stars[parent];
        heap->stars[parent] = heap->stars[index];
        heap->stars[index] = temp;
    }
    sift_up(heap, parent);
}

void sift_down(star_array_t *heap, size_t index) {
    size_t smallest = index;
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;

    if (left < heap->size && (heap->stars[left].path_cost < heap->stars[smallest].path_cost)) {
        smallest = left;
    }

    if (right < heap->size && (heap->stars[right].path_cost < heap->stars[smallest].path_cost)) {
        smallest = right;
    }

    if (index != smallest) {
        star_t temp = heap->stars[index];
        heap->stars[index] = heap->stars[smallest];
        heap->stars[smallest] = temp;

        sift_down(heap, smallest);
    }
}

star_t* pop_min(star_array_t *heap) {
    if (heap->size <= 0) {
        return NULL;
    }

    star_t *min = calloc(1, sizeof(star_t));
    if (min == NULL) {
        fprintf(stderr, "ERROR [PopMin()]: Memory allocation failed.\n");
    }
    memcpy(min, heap->stars, sizeof(star_t));

    heap->stars[0] = heap->stars[heap->size - 1];
    heap->size--;

    sift_down(heap, 0);

    return min;
}

void peek(star_array_t *heap) {
    printf("Min node: %s\n", heap->stars[0].name);
}

// SECURITY UTIL FUNCTIONS
void secure_zero(void *ptr, size_t size) {
    volatile unsigned char *p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

