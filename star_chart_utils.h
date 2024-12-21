#ifndef STAR_CHART_UTILS_H
#define STAR_CHART_UTILS_H

#define PI 3.14159265358979323846

// Forward declaration of KDNode; driven by StarPath (StarSearchRange)
typedef struct KDNode KDNode;
typedef struct Position Position;

// Consider updating to include a 'Position *pos' element instead of the x, y, and z
// individual values can still be accessed: Star->pos->x, Star->pos->y, etc.
typedef struct Star {
	char* name;
	Position *position;
	KDNode* kd_node;
	float lightyears;
	float path_cost; // For star path
} Star;

typedef struct StarArray {
	Star* stars;
	int size;
	int capacity;
} StarArray;

typedef struct KDNode {
	Star* star;
	struct KDNode* left;
	struct KDNode* right;
} KDNode;

// Update this to double to match the Star struct position values
// Wait, Star Struct has x, y, z values... why doesn't it just have
// a 'Position *position' element?
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
	int size;
	int count;
} HashMap;

typedef struct MinHeap {
	Star* elements;
	int size;
	int capacity;
} MinHeap;

// typedef struct HeapNode {
// 	Star* star;
// 	float g_cost;
// 	float h_cost;
// } HeapNode;


// GLOBAL VARIABLE
extern Position player_position;

// READ DATABASE FILE
StarArray* ParseFile();

// CONVERSION MATH TO DETERMINE X, Y, Z, AND NAVIGATION VECTORS
double Sign(double value);
double ToDecimalRA(double hours, double minutes, double seconds);
double ToDecimalDec(double degrees, double minutes, double seconds);
void ConvertTo3DCoords(double A, double B, double C, double* x, double* y, double* z);

// DATA STRUCTURE CREATION
StarArray* CreateStarArray();
KDNode* CreateBalancedKDTree(Star* stars, int start, int end, int depth);
HashMap* CreateHashMap(StarArray* star_array, int size);

// ARRAY UTILITY FUNCTIONS
void AddStarToArray(StarArray* array, Star* star);
void OptimizeStarArraySize(StarArray* array);
void PrintStarValues(StarArray* array);
void DeallocSubStarArray(StarArray* array);
void DeallocMainStarArray(StarArray* array);

// KD-TREE UTILITY FUNCTIONS
StarArray* StarSearchRange(KDNode* root, Star *center, float radius);
void RadiusSearch(KDNode* node, Star *center, float radius, int depth, StarArray* result);
Star* NearestNeighbor(KDNode* root, const Position reference);
Star* NearestNeighborSearch(KDNode* root, const Position reference, int depth, Star* current_closest_star, double* current_best_distance);
void PrintKDTree(KDNode* node);
void DeallocKDTree(KDNode* node);

// HASHMAP UTILITY FUNCTIONS
unsigned long hash(const char* key);
void ResizeHashMap(HashMap* map);
void AddToHashMap(HashMap* map, const char* key, Star* value);
Star* GetFromHashMap(HashMap* map, const char* key);
void DeallocHashMap(HashMap* map);

// OTHER UTILITY FUNCTIONS
Position* SetPlayerPosition(float x, float y, float z);
double CalculateDistance(const Star* star, const Position reference);
int CompareNodeX(const void* a, const void* b);
int CompareNodeY(const void* a, const void* b);
int CompareNodeZ(const void* a, const void* b);


// STAR PATH FUNCTIONS
StarArray* StarPath(const char* destination_key, KDNode* root, HashMap* map);
void StarPathBuild(Star* origin, Star* destination, StarArray* array, KDNode* root, HashMap *visited);
void PrintStarPath(StarArray* array);
float CalculateEuclideanDistance(Star* current, Star* goal);

// HEAP FUNCTIONS
void Heapify(StarArray* heap);
void SiftDown(StarArray* heap, int index);
Star* PopMin(StarArray* heap);
void Peek(StarArray* heap);
// void AddToHeap(StarArray* heap, Star* node);
// void SiftUp(StarArray* heap);

#endif