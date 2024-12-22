#include "star_chart_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

// GLOBAL VARIABLES
// Position player_position = {0.0, 0.0, 0.0}; // Sol
Position player_position = {0.01, 0.0, 0.01}; // Sol-ish

// Position player_position = {10.29, 5.02, -3.27}; // Tau Ceti
// Position player_position = {10.28, 5.02, -3.29}; //Tau Ceti-ish

// READ DATABASE FILE
StarArray *ParseFile() {
  FILE *file = fopen("stars.csv", "r");
  if (file == NULL) {
    fprintf(stderr, "ERROR [ParseFile()]: FILE FAILED TO OPEN!\n");
    return NULL;
  }

  int max_tokens = 8;
  char line[1024];
  StarArray *array = CreateStarArray();

  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\n")] = 0; // remove newline character

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

      Star new_star;
      new_star.name = strdup(name);
      new_star.lightyears = lightyears;
      new_star.path_cost = FLT_MAX;
      new_star.position = calloc(1, sizeof(Position));
      if (new_star.position == NULL) {
        fprintf(stderr, "ERROR [ParseFile()]: Memory allocation failed for new star allocation\n");
        DeallocMainStarArray(array);
        return NULL;
      }

      ConvertTo3DCoords(ToDecimalRA(raHours, raMinutes, raSeconds),
                        ToDecimalDec(decDegrees, decMinutes, decSeconds),
                        lightyears, &new_star.position->x, &new_star.position->y, &new_star.position->z);

      AddStarToArray(array, &new_star);
    }
  }

  OptimizeStarArraySize(array);

  // printf("Number of stars in the array: %d\n", array->size);
  // printf("Allocated capacity of the array: %d\n", array->capacity);

  fclose(file);
  return array;
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

KDNode *CreateBalancedKDTree(Star *stars, int start, int end, int depth) {
  if (start > end)
    return NULL;

  int axis = depth % 3;
  
  switch (axis) {
  case 0: qsort(stars + start, (end - start) + 1, sizeof(Star), CompareNodeX); break;
  case 1: qsort(stars + start, (end - start) + 1, sizeof(Star), CompareNodeY); break;
  case 2: qsort(stars + start, (end - start) + 1, sizeof(Star), CompareNodeZ); break;
  default: return NULL; break;
  }

  int mid = (start + end) / 2;

  KDNode *node = calloc(1, sizeof(KDNode));
  if (node == NULL) {
    fprintf(stderr, "ERROR [CreateBalancedKDTree()]: MEMORY ALLOCATION FAILED FOR KD NODE!\n");
    return NULL;
  }

  node->star = &stars[mid];
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
    array->capacity = (int)array->capacity * 1.5; // Double the capacity if full
    array->stars = realloc(array->stars, array->capacity * sizeof(Star));

    if (!array->stars) {
      fprintf(stderr, "ERROR [AddStarToArray()]: MEMORY ALLOCATION FAILED DURING REALLOC!\n");
      return;
    }
  }

  array->stars[array->size++] = *star;

  // printf("Number of stars in the array: %d\n", array->size);
  // printf("Allocated capacity of the array: %d\n", array->capacity);
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
    }
    free(array->stars); // Free stars array
    free(array);        // Free array struct
  }
}

// KD-TREE UTILITY FUNCTIONS
StarArray* StarSearchRange(KDNode *root, Star *center, float radius) {
  StarArray *result = CreateStarArray();
  // Position ghost_position = player_position;

  // RadiusSearch(root, ghost_position, radius, 0, result);

  RadiusSearch(root, center, radius, 0, result);

  // float distance_from_player = 0;

  // printf("\n\nStars within %.2f light years:\n", radius);

  // for (int i = 0; i < result->size; i++) {
  //   Star *star = &result->stars[i];
  //   distance_from_player = CalculateDistance(star, &player_position);
  //   printf("  - %s at %.2f light years\n", star->name, distance_from_player);
  // }

  return result;
}

void RadiusSearch(KDNode *node, Star *center, float radius, int depth, StarArray *result) {
  if (node == NULL) {
    // Base case
    return;
  }

  // printf("Visiting node: %s at depth %d\n", node->star->name, depth);

  float distance = CalculateEuclideanDistance(node->star, center);
  if (distance <= radius) {
    AddStarToArray(result, node->star);
  }

  int axis = depth % 3;
  float diff = 0;

  
  switch (axis) {
  case 0:
    diff = center->position->x - node->star->position->x; 
    break;
  case 1:
    diff = center->position->y - node->star->position->y; 
    break;
  case 2:
    diff = center->position->z - node->star->position->z;
    break;
  default:
    break;
  }

  // printf("Axis: %d, Diff: %.2f, Radius: %.2f\n", axis, diff, radius);

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
  
  // printf("Closest star is %s at a distance of %.2f light years from current location.\n\n", neighbor->name, current_best_distance);

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

      // printf("  Rehashed key '%s' to new index %u\n", entry->key, index);

      entry = next_entry;
    }
  }
  free(map->buckets);
  map->buckets = new_buckets;
  map->size = new_size;

  // printf("Resizing complete. New size: %d\n", map->size);
}

void AddToHashMap(HashMap *map, const char *key, Star *value) {
  // Check and resize if load factor exceeds 0.75
  if ((float)(map->count + 1) / map->size > 0.75) {
    // printf("Resizing hash map. Current size: %u, count: %u\n", map->size, map->count);
    ResizeHashMap(map);
  }

  unsigned int index = hash(key) % map->size; // Compute bucket index
  // printf("Inserting key '%s' into index %u\n", key, index);

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
  // printf("Key '%s' successfully added to hash map\n", key);
}

Star* GetFromHashMap(HashMap *map, const char *key) {
  unsigned int index = hash(key) % map->size;
  // printf("Hash index for key '%s': %u\n", key, index);

  // printf("Traversing chain at index %u:\n", index);
  HashEntry *entry = map->buckets[index];

  while (entry) {
    // printf("Checking key: '%s'\n", entry->key);

    if (strcmp(entry->key, key) == 0) {
      // printf("Key found! Returning value.\n");
      return entry->value;
    } 
    
    entry = entry->next;
    
  }

  // printf("Key not found. Returning NULL.\n");
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
Position* SetPlayerPosition(float x, float y, float z) {
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
  const Star *StarA = (Star *)a;
  const Star *StarB = (Star *)b;

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
  const Star *StarA = (Star *)a;
  const Star *StarB = (Star *)b;

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
  const Star *StarA = (Star *)a;
  const Star *StarB = (Star *)b;

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

// STAR PATH FUNCTIONS
  // In the future, replace 'radius' with player's jump distance
  // Remove the expanding radius search (*1.5) because player's can't jump outside of jump distance
  // Better idea: allow the expansion and with player jump distance, determine (based on count of expansions)
  // how many jumps will need to happen to reach next star in path
  // GAME NOTE: Players can opt to jump directly to destination but solar worm holes will be quicker which would
  // incentivize using star system navigation instead of interstellar jumps

StarArray* StarPath(const char *destination_key, KDNode *root, HashMap *map) {
  // Create path and visited map
  StarArray *star_path = CreateStarArray();
  HashMap *visited = CreateHashMap(star_path, 1024);

  // Convert destination_key to star node
  Star *destination = GetFromHashMap(map, destination_key);

  // Determine closest star
  Star *origin = NearestNeighbor(root, player_position);
  origin->path_cost = CalculateEuclideanDistance(origin, destination);

  StarPathBuild(origin, destination, star_path, root, visited);

  DeallocHashMap(visited);
  // star_path is freed in main
  return star_path;
}

void StarPathBuild(Star *current_star, Star *destination, StarArray *path_array, KDNode *root, HashMap *visited) {

//==============================================================================================//
// ADD TO ARRAY AND CHECK FOR BASE CASE
//==============================================================================================//
  AddStarToArray(path_array, current_star);

// Prevent revisiting the same star
  if (GetFromHashMap(visited, current_star->name)) {
      printf("Star %s already visited, skipping...\n", current_star->name);
      return;
  }

  AddToHashMap(visited, current_star->name, current_star);

  printf("Adding %s to star_path\n\n", current_star->name);
  printf("-=***   BUILD START   ***=-\n");
  printf("current star: %s\ndestination star: %s\n\n", current_star->name, destination->name);

  if (GetFromHashMap(visited, destination->name)) {
    // Base case
    printf("\nDestination reached!\n");
    PrintStarPath(path_array);
    return;
  }

//==============================================================================================//
// GET NEIGHBORS
//==============================================================================================//

  StarArray *neighbors = NULL;
  float radius = CalculateEuclideanDistance(current_star, destination) * 0.5;
    
  do {
    if (neighbors) {
      DeallocSubStarArray(neighbors);
    }
    neighbors = StarSearchRange(root, current_star, radius);
    radius += 1;
  } while (neighbors->size < 5);

//==============================================================================================//
// ITERATE THROUGH NEIGHBORS CHECKING FOR VISITED, CALCULATE PATH COST, UPDATE AS NECESSARY
//==============================================================================================//

  StarArray *neighbors_to_use = CreateStarArray();

  for (int i = 0; i < neighbors->size; i++) {
    Star *neighbor = &neighbors->stars[i];
    Star *visited_star = GetFromHashMap(visited, neighbor->name);

    float new_cost = CalculateEuclideanDistance(current_star, neighbor) + 
                     CalculateEuclideanDistance(neighbor, destination);

    if (visited_star) { 
      printf("Skipping star: %s (no better path found)\n", neighbor->name);
      continue;
    }
    
    neighbor->path_cost = new_cost;
    printf("Updated path cost for %s to %.2f\n", neighbor->name, new_cost);

    AddStarToArray(neighbors_to_use, neighbor);
  }

//==============================================================================================//
// CREATE UPDATED ARRAY WITH NON-VISITED STARS AND BEND IT TO CONFORM TO HEAP RULES
//==============================================================================================//

  Heapify(neighbors_to_use);

  // Print stars and costs for QA check of StarPathBuild()
  printf("\nHeap after neighbor updates:\n");
  for (int i = 0; i < neighbors_to_use->size; i++) {
    printf("%s: cost = %.2f\n", neighbors_to_use->stars[i].name, neighbors_to_use->stars[i].path_cost);
  }

//==============================================================================================//
// GET MIN NODE FOR INSERTION AS NEXT STAR; WILL BE ADDED AT THE TOP; PASSED INTO RECURSIVE CALL
//==============================================================================================//
  Star *next_star = PopMin(neighbors_to_use);
  
  if (next_star == NULL) {
    fprintf(stderr, "Dead end encountered, backtracking...\n");
    DeallocSubStarArray(neighbors_to_use);
    DeallocSubStarArray(neighbors);
    return;
  }

  printf("*** Passing %s to StarPathBuild ***\n\n", next_star->name);
  printf("Current star path:\n");
  PrintStarPath(path_array);

//==============================================================================================//
// MARK CURRENT STAR AS VISITED AFTER PROCESSING
//==============================================================================================//
  AddToHashMap(visited, current_star->name, current_star);

  // RECURSIVE CALL
  StarPathBuild(next_star, destination, path_array, root, visited);

  // CLEAN UP
  free(next_star);
  DeallocSubStarArray(neighbors_to_use);
  DeallocSubStarArray(neighbors);
}

void PrintStarPath(StarArray *array) {
  for (int i = 0; i < array->size; i++) {
      printf("%s", array->stars[i].name);
      if (i < array->size - 1) {
        printf(" -> ");
      }
    } printf("\n");
}

// Differs from 'CalculateDistance(...)' in that this compares the distance between stars, not a player position
float CalculateEuclideanDistance(Star *current, Star *goal) {
  float dx = goal->position->x - current->position->x;
  float dy = goal->position->y - current->position->y;
  float dz = goal->position->z - current->position->z;

  return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

// HEAP FUNCTIONS
void Heapify(StarArray *heap) {
  for (int i = (heap->size / 2) - 1; i >= 0; i--) {
    SiftDown(heap, i);
  }
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

  Star *min = calloc(1, sizeof(Star));
  if (min == NULL) {
    fprintf(stderr, "ERROR [PopMin()]: Memory allocation failed in PopMin()\n");
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
