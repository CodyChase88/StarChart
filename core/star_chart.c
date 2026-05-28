#include <stdio.h>
#include <stdlib.h>
#include "star_chart_utils.h"

int main(void) {

    StarArray* star_array = ParseFile();

    Star** star_ptr_array = calloc(star_array->size, sizeof(Star*));
    for (int i = 0; i < star_array->size; i++) {
        star_ptr_array[i] = &star_array->stars[i];
    }
    KDNode* kd_tree = CreateBalancedKDTree(star_ptr_array, 0, star_array->size - 1, 0);
    // TODO: Apply this pattern to all deallocs (see SecureZero in utils)
    SecureZero(star_ptr_array, star_array->size * sizeof(Star*));
    free(star_ptr_array);

    HashMap* star_hash_map = CreateHashMap(star_array, star_array->size);

    Star* closest_star = NearestNeighbor(kd_tree, player_position);
    printf("Closest star: %s\n\n", closest_star->name);   
    
    // StarPath("* alf CMa", kd_tree, star_hash_map, star_array); // AKA Sirius
    // StarPath("V* BY Dra", kd_tree, star_hash_map, star_array); // AKA BY Draconis
    // StarPath("* tau Cet", kd_tree, star_hash_map, star_array); // AKA Tau Ceti
    // StarPath("* eps Eri", kd_tree, star_hash_map, star_array); // AKA Epsilon Eridani
    // StarPath("HD 1326", kd_tree, star_hash_map, star_array); // AKA Groombridge 34
    // StarPath("* del Pav", kd_tree, star_hash_map, star_array); // Used for long distance testing
    StarPath("G 182-36", kd_tree, star_hash_map, star_array); // Used to test unreachable fallback
    
    printf("-----------------------\n");
    // PrintStarValues(star_array);
    // PrintKDTree(kd_tree);
    // PrintStarValues(star_range);
    // PrintStarValues(star_path);


    // DeallocSubStarArray(star_path);
    DeallocHashMap(star_hash_map); 
    DeallocKDTree(kd_tree);

    /* -- WARNING -- */
    // ENSURE THIS IS CALLED LAST!!!
    // Star Array is the source of data for secondary data structures!
    DeallocMainStarArray(star_array);
}
