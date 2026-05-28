#include "star_chart_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

// GLOBAL VARIABLES
Position player_position = {0.0, 0.0, 0.0}; // Sol
// Position player_position = {0.01, 0.0, 0.01}; // Sol-ish

// Position player_position = {10.29, 5.02, -3.27}; // Tau Ceti
// Position player_position = {10.28, 5.02, -3.29}; //Tau Ceti-ish

// READ DATABASE FILE
StarArray *ParseFile() {
    FILE *file = fopen("data/stars_query.csv", "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR [ParseFile()]: FILE FAILED TO OPEN!\n");
        return NULL;
    }

    int max_tokens = 9;
    char line[1024];
    StarArray *array = CreateStarArray();

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
            int raHours = (int)strtod(tokens[1], NULL);
            double raMinutes = strtod(tokens[2], NULL);
            double raSeconds = strtod(tokens[3], NULL);
            int decDegrees = (int)strtod(tokens[4], NULL);
            double decMinutes = strtod(tokens[5], NULL);
            double decSeconds = strtod(tokens[6], NULL);
            float lightyears = strtod(tokens[7], NULL);
            char *sp_type = tokens[8];

            Star new_star;
            new_star.name = strdup(name);
            new_star.kd_node = NULL;
            new_star.lightyears = lightyears;
            new_star.path_cost = FLT_MAX;
            new_star.g_cost = FLT_MAX;
            new_star.came_from = NULL;
            new_star.position = calloc(1, sizeof(Position));
            if (new_star.position == NULL) {
                fprintf(stderr, "ERROR [ParseFile()]: Memory allocation failed for new star allocation\n");
                DeallocMainStarArray(array);
                return NULL;
            }
            new_star.sp_type = strdup(sp_type);
            new_star.jump_range = GetJumpRange(sp_type);

            ConvertTo3DCoords(ToDecimalRA(raHours, raMinutes, raSeconds),
                    ToDecimalDec(decDegrees, decMinutes, decSeconds),
                    lightyears, &new_star.position->x, &new_star.position->y, &new_star.position->z);

            AddStarToArray(array, &new_star);
        }
    }

    OptimizeStarArraySize(array);

    fclose(file);
    return array;
}

// CONVERT SPECTRAL TYPE TO "JUMP RANGE" EQUIVALENT
float GetJumpRange(const char *sp_type) {
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
double Sign(double value) { return (value > 0) ? 1.0 : -1.0; }

double ToDecimalRA(double hours, double minutes, double seconds) {
    return (hours * 15) + (minutes * 0.25) + (seconds * 0.004166);
}

double ToDecimalDec(double degrees, double minutes, double seconds) {
    return (fabs(degrees) + (minutes / 60) + (seconds / 3600.0)) * Sign(degrees);
}

void ConvertTo3DCoords(double A, double B, double C, double *x, double *y, double *z) {
    double A_radians = A * (PI / 180);
    double B_radians = B * (PI / 180);

    *x = (C * cos(B_radians)) * cos(A_radians);
    *y = (C * cos(B_radians)) * sin(A_radians);
    *z = C * sin(B_radians);
}

// DATA STRUCTURE CREATION
StarArray *CreateStarArray() {
    StarArray *array = calloc(1, sizeof(StarArray));
    if (!array) {
        fprintf(stderr, "ERROR [CreateStarArray()]: MEMORY ALLOCATION FAILED FOR STAR ARRAY!\n");
        return NULL;
    }

    array->size = 0;
    array->capacity = 1024; // Initial capacity
    array->stars = calloc(array->capacity, sizeof(Star));
    if (!array->stars) {
        fprintf(stderr, "ERROR [CreateStarArray()]: MEMORY ALLOCATION FAILED FOR STARS ARRAY!\n");
        return NULL;
    }

    return array;
}

KDNode *CreateBalancedKDTree(Star **stars, int start, int end, int depth) {
    if (start > end)
        return NULL;

    int axis = depth % 3;

    switch (axis) {
        case 0: qsort(stars + start, (end - start) + 1, sizeof(Star*), CompareNodeX); break;
        case 1: qsort(stars + start, (end - start) + 1, sizeof(Star*), CompareNodeY); break;
        case 2: qsort(stars + start, (end - start) + 1, sizeof(Star*), CompareNodeZ); break;
        default: return NULL; break;
    }

    int mid = (start + end) / 2;

    KDNode *node = calloc(1, sizeof(KDNode));
    if (node == NULL) {
        fprintf(stderr, "ERROR [CreateBalancedKDTree()]: MEMORY ALLOCATION FAILED FOR KD NODE!\n");
        return NULL;
    }

    node->star = stars[mid];
    node->left = NULL;
    node->right = NULL;
    node->star->kd_node = node;

    if (start <= mid - 1) {
        node->left = CreateBalancedKDTree(stars, start, mid - 1, depth + 1);
    }

    if (mid + 1 <= end) {
        node->right = CreateBalancedKDTree(stars, mid + 1, end, depth + 1);
    }

    return node;
}

HashMap *CreateHashMap(StarArray *star_array, int size) {
    HashMap *map = calloc(1, sizeof(HashMap));
    if (map == NULL) {
        fprintf(stderr, "ERROR [CreateHashMap()]: MEMORY ALLOCATION FAILED FOR HASHMAP!\n");
        return NULL;
    }

    map->size = size;
    map->buckets = calloc(map->size, sizeof(HashEntry*));
    if (map->buckets == NULL) {
        fprintf(stderr, "ERROR [CreateHashMap()]: MEMORY ALLOCATION FAILED FOR HASH ENTRY!\n");
        free(map);
        return NULL;
    }

    map->count = 0;

    for (int i = 0; i < star_array->size; i++) {
        AddToHashMap(map, star_array->stars[i].name, &star_array->stars[i]);
    }

    return map;
}

// ARRAY UTILITY FUNCTIONS
void AddStarToArray(StarArray *array, Star *star) {
    if (array->size == array->capacity) {
        array->capacity = (int)array->capacity * 1.5; // Increase capacity if full
        array->stars = realloc(array->stars, array->capacity * sizeof(Star));

        if (!array->stars) {
            fprintf(stderr, "ERROR [AddStarToArray()]: MEMORY ALLOCATION FAILED DURING REALLOC!\n");
            return;
        }
    }

    array->stars[array->size++] = *star;
}

void OptimizeStarArraySize(StarArray *array) {
    if (array->size < array->capacity) {
        array->stars = realloc(array->stars, array->size * sizeof(Star));
        if (!array->stars && array->size > 0) {
            fprintf(stderr, "ERROR [OptimizeStarArraySize()]: MEMORY REALLOCATION FAILED DURING OPTIMIZATION!\n");
            return;
        }
        array->capacity = array->size;
    }
}

void PrintStarValues(StarArray *star_array) {
    for (size_t i = 0; i < star_array->size; i++) {
        printf("%s: (%.2f, %.2f, %.2f)\n", star_array->stars[i].name,
                star_array->stars[i].position->x, star_array->stars[i].position->y,
                star_array->stars[i].position->z);
        printf("Light years from Sol: %.3f\n", star_array->stars[i].lightyears);
        printf("path score: %.2f\n\n", star_array->stars[i].path_cost);
    }
}

void DeallocSubStarArray(StarArray *array) {
    if (array) {
        free(array->stars);
        free(array);
    }
}

void DeallocMainStarArray(StarArray *array) {
    if (array) {
        for (int i = 0; i < array->size; i++) {
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
StarArray* GetNeighbors(KDNode *root, Position *center, float radius) {
    StarArray *result = CreateStarArray();
    RadiusSearch(root, center, radius, 0, result);
    return result;
}

void RadiusSearch(KDNode *node, Position *center, float radius, int depth, StarArray *result) {
    if (node == NULL) {
        // Base case
        return;
    }

    float distance = CalculateDistanceToPosition(node->star, center);
    if (distance <= radius) {
        AddStarToArray(result, node->star);
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
        RadiusSearch(node->left, center, radius, depth + 1, result);

        if (fabs(diff) <= radius) {
            RadiusSearch(node->right, center, radius, depth + 1, result);
        }
    } else {
        RadiusSearch(node->right, center, radius, depth + 1, result);

        if (fabs(diff) <= radius) {

            RadiusSearch(node->left, center, radius, depth + 1, result);
        }
    }
}

Star* NearestNeighbor(KDNode *root, const Position reference) {
    double current_best_distance = DBL_MAX;
    Star *neighbor = NearestNeighborSearch(root, reference, 0, NULL, &current_best_distance);

    return neighbor;
}

Star* NearestNeighborSearch(KDNode *node, const Position reference, int depth, Star *current_closest_star, double *current_best_distance){
    if (node == NULL) {
        return current_closest_star;
    }

    double distance = CalculateDistance(node->star, reference);
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

    KDNode *near_subtree = NULL;
    KDNode *far_subtree = NULL;

    if (diff <= 0) {
        near_subtree = node->left;
        far_subtree = node->right;
    } else {
        near_subtree = node->right;
        far_subtree = node->left;
    }

    current_closest_star = NearestNeighborSearch(near_subtree, reference, depth + 1, current_closest_star, current_best_distance);

    if (fabs(diff) < *current_best_distance) {
        current_closest_star = NearestNeighborSearch(far_subtree, reference, depth + 1, current_closest_star, current_best_distance);
    }

    return current_closest_star;
}

Star* FindNearestReachableStar(Star* destination, KDNode* root, HashMap* map) {
    StarArray* candidates = NULL;
    float search_radius = 5.0f;
    Star* nearest_reachable = NULL;

    while (search_radius < 50.0f && nearest_reachable == NULL) {
        if (candidates) DeallocSubStarArray(candidates);

        candidates = GetNeighbors(root, destination->position, search_radius);
        for (int i = 0; i < candidates->size; i++) {
            Star* candidate = GetFromHashMap(map, candidates->stars[i].name);
            if (candidate != NULL && candidate->jump_range > 0.0f && candidate != destination) {
                nearest_reachable = candidate;
                break;
            }
        }
        search_radius += 2.0f;
    }

    if (candidates) DeallocSubStarArray(candidates);
    return nearest_reachable;
}

void PrintKDTree(KDNode *node) {
    if (node == NULL) {
        return;
    }
    PrintKDTree(node->left);

    printf("%s: (%.2f, %.2f, %.2f), %.2f\n", node->star->name, node->star->position->x,
            node->star->position->y, node->star->position->z, node->star->lightyears);

    PrintKDTree(node->right);
}

void DeallocKDTree(KDNode *node) {
    if (node) {
        DeallocKDTree(node->left);
        DeallocKDTree(node->right);
        free(node);
    }
}

// HASHMAP UTILITY FUNCTIONS
unsigned long hash(const char *key) {
    // djb2 algorithm
    unsigned long hash = 5381;
    int c = 0;

    while (c = *key++) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

void ResizeHashMap(HashMap *map) {
    int new_size = map->size * 2;
    HashEntry **new_buckets = calloc(new_size, sizeof(HashEntry*));
    if (new_buckets == NULL) {
        fprintf(stderr, "ERROR [ResizeHashMap()]: MEMORY ALLOCATION FAILED DURING HASH MAP RESIZE!\n");
        return;
    }

    for (int i = 0; i < map->size; i++) {
        HashEntry *entry = map->buckets[i];
        while (entry) {
            HashEntry *next_entry = entry->next;
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

void AddToHashMap(HashMap *map, const char *key, Star *value) {
    // Check and resize if load factor exceeds 0.75
    if ((float)(map->count + 1) / map->size > 0.75) {
        ResizeHashMap(map);
    }

    unsigned int index = hash(key) % map->size; // Compute bucket index
                                                // Allocate new hash entry
    HashEntry *new_entry = calloc(1, sizeof(HashEntry));
    if (!new_entry) {
        fprintf(stderr, "ERROR [AddToHashMap()]: MEMORY ALLOCATION FAILED DURING ADD TO HASH MAP!\n");
        return;
    }
    new_entry->key = strdup(key);
    if (!new_entry->key) {
        fprintf(stderr, "ERROR [AddToHashMap()]: MEMORY ALLOCATION FAILED FOR KEY STRING!\n");
        free(new_entry);
        return;
    }
    new_entry->value = value;

    // Handle collisions with chaining
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;

    map->count++;
}

Star* GetFromHashMap(HashMap *map, const char *key) {
    unsigned int index = hash(key) % map->size;
    HashEntry *entry = map->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        } 

        entry = entry->next;

    }
    return NULL;
}

void DeallocHashMap(HashMap *map) {
    if (map == NULL) {
        return;
    }

    for (int i = 0; i < map->size; i++) {
        HashEntry *entry = map->buckets[i];
        while (entry) {
            HashEntry *temp = entry;
            entry = entry->next;
            free(temp->key);
            free(temp);
        }
    }
    free(map->buckets);
    free(map);
}

// OTHER UTILITY FUNCTIONS
void SetPlayerPosition(float x, float y, float z) {
    player_position.x = x;
    player_position.y = y;
    player_position.z = z;
}

double CalculateDistance(const Star *star, const Position reference) {
    double dx = star->position->x - reference.x;
    double dy = star->position->y - reference.y;
    double dz = star->position->z - reference.z;

    return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

int CompareNodeX(const void *a, const void *b) {
    const Star *StarA = *(Star **)a;
    const Star *StarB = *(Star **)b;

    if (StarA->position->x < StarB->position->x)
        return -1;
    if (StarA->position->x > StarB->position->x)
        return 1;

    // Tie-breaking on y-coordinate
    if (StarA->position->y < StarB->position->y)
        return -1;
    if (StarA->position->y > StarB->position->y)
        return 1;

    // Tie-breaking on z-coordinate
    return (StarA->position->z < StarB->position->z) ? -1 : (StarA->position->z > StarB->position->z) ? 1 : 0;
}

int CompareNodeY(const void *a, const void *b) {
    const Star *StarA = *(Star **)a;
    const Star *StarB = *(Star **)b;

    if (StarA->position->y < StarB->position->y)
        return -1;
    if (StarA->position->y > StarB->position->y)
        return 1;

    // Tie-breaking on x-coordinate
    if (StarA->position->x < StarB->position->x)
        return -1;
    if (StarA->position->x > StarB->position->x)
        return 1;

    // Tie-breaking on z-coordinate
    return (StarA->position->z < StarB->position->z) ? -1 : (StarA->position->z > StarB->position->z) ? 1 : 0;
}

int CompareNodeZ(const void *a, const void *b) {
    const Star *StarA = *(Star **)a;
    const Star *StarB = *(Star **)b;

    if (StarA->position->z < StarB->position->z)
        return -1;
    if (StarA->position->z > StarB->position->z)
        return 1;

    // Tie-breaking on x-coordinate
    if (StarA->position->x < StarB->position->x)
        return -1;
    if (StarA->position->x > StarB->position->x)
        return 1;

    // Tie-breaking on y-coordinate
    return (StarA->position->y < StarB->position->y) ? -1 : (StarA->position->y > StarB->position->y) ? 1 : 0;
}

double CalculateDistanceToPosition(Star* star, Position* pos) {
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

void StarPath(const char *destination_key, KDNode *root, HashMap *map, StarArray *star_array) {
    Star *destination = GetFromHashMap(map, destination_key);
    if (destination == NULL) {
        fprintf(stderr, "ERROR [StarPath()]: Destination '%s' not found in star map.\n", destination_key);
        return;
    }
    
    StarArray *open_set = CreateStarArray();
    HashMap *visited = CreateHashMap(open_set, 1024);

    Star *origin = NearestNeighbor(root, player_position);
    origin->g_cost = 0;
    origin->path_cost = origin->g_cost + CalculateEuclideanDistance(origin, destination);

    // 6. Add origin to open set before passing to StarPathBuild()
    AddStarToArray(open_set, GetFromHashMap(map, origin->name));

    StarArray* star_path = StarPathBuild(destination, open_set, root, map, visited);
    
    // Fallback: if destination unreachable, route to nearest reachable star
    if (star_path->size == 0) {
        printf("Destination unreachable. Finding nearest accessible star...\n");

        // Find nearest star to destination
        Star *nearest_reachable = FindNearestReachableStar(destination, root, map);

        if (nearest_reachable != NULL) {
            // Reset search state
            ResetStarPathState(star_array);
            DeallocHashMap(visited);
            visited = CreateHashMap(open_set, 1024);

            // Reset origin and open_set
            SecureZero(open_set->stars, open_set->capacity * sizeof(Star));
            open_set->size = 0;
            origin->g_cost = 0;
            origin->path_cost = origin->g_cost + CalculateEuclideanDistance(origin, nearest_reachable);
            AddStarToArray(open_set, GetFromHashMap(map, origin->name));

            printf("Routing to nearest accessible star: %s\n", nearest_reachable->name);
            printf("Note: %s is %.2f ly from your destination %s\n\n", nearest_reachable->name,
                    CalculateEuclideanDistance(nearest_reachable, destination),
                    destination_key);

            DeallocSubStarArray(star_path);
            star_path = StarPathBuild(nearest_reachable, open_set, root, map, visited);
        } else {
            printf("No accessible stars found near destination '%s'.\n", destination_key);
        }
    }
    
    PrintStarPath(star_path);  

    DeallocSubStarArray(star_path);
    DeallocHashMap(visited);
    DeallocSubStarArray(open_set);
    // star_path is freed in main
}

StarArray* StarPathBuild(Star *destination, StarArray *open_set, KDNode *root, HashMap *map, HashMap *visited) {
    StarArray *star_path = CreateStarArray();
    do {
        // 1. Set current star to the smallest node on the heap
        Star* current_star = PopMin(open_set);
        
        if (strncmp(current_star->name, destination->name, strlen(destination->name) + 1) == 0) {
            Star *curr_path_star = destination;
        	while (curr_path_star != NULL) {
            	AddStarToArray(star_path, curr_path_star);
            	curr_path_star = curr_path_star->came_from;
        	}
            // Reverse star_path (MAKE THIS A FUNCTION LATER)
            int left = 0;
            int right = star_path->size - 1;
            while (left < right) {
                Star temp = star_path->stars[left];
                star_path->stars[left] = star_path->stars[right];
                star_path->stars[right] = temp;
                left++;
                right--;
            }
            free(current_star);
            return star_path;
        }

        // Is current star already in visited list?
        if (GetFromHashMap(visited, current_star->name)) {
			free(current_star);
            continue;
        } else {
            // 2. If curr star is not in visited list, add to visited list
            AddToHashMap(visited, current_star->name, current_star);
            StarArray *neighbors = GetNeighbors(root, current_star->position, current_star->jump_range);
            for (int i = 0; i < neighbors->size; i++) {
				Star *real_neighbor = GetFromHashMap(map, neighbors->stars[i].name);
                
                // Skip stars with no jump capability
                if (real_neighbor->jump_range == 0.0f) continue;

                float tentative_g_cost = current_star->g_cost + CalculateEuclideanDistance(current_star, real_neighbor);
                if (tentative_g_cost < real_neighbor->g_cost) {
					real_neighbor->g_cost = tentative_g_cost;
                    real_neighbor->path_cost = tentative_g_cost + CalculateEuclideanDistance(real_neighbor, destination);
                    Star *real_current = GetFromHashMap(map, current_star->name);
                    real_neighbor->came_from = real_current;
                	AddToHeap(open_set, real_neighbor);                
				}
            }
            DeallocSubStarArray(neighbors);
        }
        free(current_star);
    } while (open_set->size > 0);
    
    if (star_path->size == 0) {
        printf("Destination unreachable from current location.\n");
    }
    
    return star_path;
}               

void PrintStarPath(StarArray *array) {
    for (int i = 0; i < array->size; i++) {
        printf("%s", array->stars[i].name);
        if (i < array->size - 1) {
            printf(" -> ");
        }
    } printf("\n");

    float total = 0;
    for (int i = 0; i < array->size - 1; i++) {
        float hop = CalculateEuclideanDistance(&array->stars[i], &array->stars[i + 1]);
        printf(" %s -> %s: %.2f ly\n", array->stars[i].name, array->stars[i + 1].name, hop);
        total += hop;
    }
    printf("Total distance: %.2f ly\n", total);
}

// Differs from 'CalculateDistance(...)' in that this compares the distance between stars, not a player position
float CalculateEuclideanDistance(Star *current, Star *goal) {
    float dx = goal->position->x - current->position->x;
    float dy = goal->position->y - current->position->y;
    float dz = goal->position->z - current->position->z;

    return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

// TODO: Replace with search ID pattern for efficiency (Phase 2)
void ResetStarPathState(StarArray *array) {
    for (int i = 0; i < array->size; i++) {
        array->stars[i].g_cost = FLT_MAX;
        array->stars[i].path_cost = FLT_MAX;
        array->stars[i].came_from = NULL;
    }
}

// HEAP FUNCTIONS
void Heapify(StarArray *heap) {
    for (int i = (heap->size / 2) - 1; i >= 0; i--) {
        SiftDown(heap, i);
    }
}

void AddToHeap(StarArray *heap, Star *node) {
    if (heap == NULL) {
        fprintf(stderr, "No heap structure found.\n");
        return;
    }
    AddStarToArray(heap, node);
    SiftUp(heap, heap->size - 1);    
}

void SiftUp(StarArray* heap, int index) {
    if (index <= 0) return;
    int parent = (index - 1) / 2;

    if (heap->stars[index].path_cost >= heap->stars[parent].path_cost) return;
    else {
        Star temp = heap->stars[parent];
        heap->stars[parent] = heap->stars[index];
        heap->stars[index] = temp;
    }
    SiftUp(heap, parent);
}

void SiftDown(StarArray* heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && (heap->stars[left].path_cost < heap->stars[smallest].path_cost)) {
        smallest = left;
    }

    if (right < heap->size && (heap->stars[right].path_cost < heap->stars[smallest].path_cost)) {
        smallest = right;
    }

    if (index != smallest) {
        Star temp = heap->stars[index];
        heap->stars[index] = heap->stars[smallest];
        heap->stars[smallest] = temp;

        SiftDown(heap, smallest);
    }
}

Star* PopMin(StarArray* heap) {
    if (heap->size <= 0) {
        return NULL;
    }

    Star* min = calloc(1, sizeof(Star));
    if (min == NULL) {
        fprintf(stderr, "ERROR [PopMin()]: Memory allocation failed.\n");
    }
    memcpy(min, heap->stars, sizeof(Star));

    heap->stars[0] = heap->stars[heap->size - 1];
    heap->size--;

    SiftDown(heap, 0);

    return min;
}

void Peek(StarArray* heap) {
    printf("Min node: %s\n", heap->stars[0].name);
}

// SECURITY UTIL FUNCTIONS
void SecureZero(void *ptr, size_t size) {
    volatile unsigned char *p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

