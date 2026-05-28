#ifndef STAR_CHART_UTILS_H
#define STAR_CHART_UTILS_H

#include <stddef.h> // for size_t

#define PI 3.14159265358979323846

// Forward declaration of KDNode; driven by StarPath (GetNeighbors)
typedef struct KDNode KDNode;
typedef struct Position Position;

typedef struct Star {
	char* name;
	KDNode* kd_node;
	Position* position;
    struct Star* came_from; // For A* path finding
    char* sp_type;
    float jump_range;
    float g_cost;
	float lightyears;
	float path_cost; // For star path
} Star;

typedef struct StarArray {
	Star* stars;
	int capacity;
	int size;
} StarArray;

typedef struct KDNode {
	Star* star;
	struct KDNode* left;
	struct KDNode* right;
} KDNode;

typedef struct Position {
	double x;
	double y;
	double z;
} Position;

typedef struct HashEntry {
	char* key;
	Star* value;
	struct HashEntry* next; // For chaining
} HashEntry;

typedef struct HashMap {
	HashEntry** buckets;
	int count;
	int size;
} HashMap;

typedef struct MinHeap {
	Star* elements;
	int capacity;
	int size;
} MinHeap;

// GLOBAL VARIABLE
extern Position player_position;

// READ DATABASE FILE
StarArray* ParseFile();

// CONVERT SPECTRAL TYPE TO "JUMP RANGE" EQUIVALENT
float GetJumpRange(const char *sp_type);

// CONVERSION MATH TO DETERMINE X, Y, Z, AND NAVIGATION VECTORS
double Sign(double value);
double ToDecimalRA(double hours, double minutes, double seconds);
double ToDecimalDec(double degrees, double minutes, double seconds);
void ConvertTo3DCoords(double A, double B, double C, double* x, double* y, double* z);

// DATA STRUCTURE CREATION
StarArray* CreateStarArray();
KDNode* CreateBalancedKDTree(Star** stars, int start, int end, int depth);
HashMap* CreateHashMap(StarArray* star_array, int size);

// ARRAY UTILITY FUNCTIONS
void AddStarToArray(StarArray* array, Star* star);
void OptimizeStarArraySize(StarArray* array);
void PrintStarValues(StarArray* array);
void DeallocSubStarArray(StarArray* array);
void DeallocMainStarArray(StarArray* array);

// KD-TREE UTILITY FUNCTIONS
//TODO: Notice that the GetNeighbors wrapper returns StarArray* but it's worker function returns void, because we're calling the worker function
//      as a variable in the wrapper. Mimic that structure for NearestNeighbor

StarArray* GetNeighbors(KDNode* root, Position *center, float radius);
void RadiusSearch(KDNode* node, Position *center, float radius, int depth, StarArray* result);
Star* NearestNeighbor(KDNode* root, const Position reference);
Star* NearestNeighborSearch(KDNode* root, const Position reference, int depth, Star* current_closest_star, double* current_best_distance);
Star* FindNearestReachableStar(Star* destination, KDNode* root, HashMap *map);
void PrintKDTree(KDNode* node);
void DeallocKDTree(KDNode* node);

// HASHMAP UTILITY FUNCTIONS
unsigned long hash(const char* key);
void ResizeHashMap(HashMap* map);
void AddToHashMap(HashMap* map, const char* key, Star* value);
Star* GetFromHashMap(HashMap* map, const char* key);
void DeallocHashMap(HashMap* map);

// OTHER UTILITY FUNCTIONS
void SetPlayerPosition(float x, float y, float z);
double CalculateDistance(const Star* star, const Position reference);
int CompareNodeX(const void* a, const void* b);
int CompareNodeY(const void* a, const void* b);
int CompareNodeZ(const void* a, const void* b);
double CalculateDistanceToPosition(Star* star, Position* pos);


// STAR PATH FUNCTIONS
void StarPath(const char* destination_key, KDNode* root, HashMap* map, StarArray* star_array);
StarArray* StarPathBuild(Star* destination, StarArray* array, KDNode* root, HashMap *map,  HashMap *visited);
void PrintStarPath(StarArray* array);
float CalculateEuclideanDistance(Star* current, Star* goal);
void ResetStarPathState(StarArray *array);

// HEAP FUNCTIONS
void Heapify(StarArray* heap);
void AddToHeap(StarArray* heap, Star* node);
void SiftUp(StarArray* heap, int index);
void SiftDown(StarArray* heap, int index);
Star* PopMin(StarArray* heap);
void Peek(StarArray* heap);

// SECURITY UTIL FUNCTIONS
void SecureZero(void *ptr, size_t size);

#endif
