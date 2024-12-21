#include <stdio.h>
#include "star_chart_utils.h"

int main(void) {

    StarArray *star_array = ParseFile();
    KDNode *kd_tree = CreateBalancedKDTree(star_array->stars, 0, star_array->size - 1, 0);
    HashMap *star_hash_map = CreateHashMap(star_array, star_array->size);

    Star* closest_star = NearestNeighbor(kd_tree, player_position);
    // printf("Closest star: %s\n", closest_star->name);   
    
    // StarArray *star_range = StarSearchRange(kd_tree, 10.0);

    // StarArray *star_path = StarPath("Sirius", kd_tree, star_hash_map);
    // StarArray *star_path = StarPath("BY Draconis", kd_tree, star_hash_map);
    // StarArray *star_path = StarPath("Tau Ceti", kd_tree, star_hash_map);
    // StarArray *star_path = StarPath("Epsilon Eridani", kd_tree, star_hash_map);
    StarArray *star_path = StarPath("Groombridge 34", kd_tree, star_hash_map);
    
    printf("-----------------------\n");
    // PrintStarValues(star_array);
    // PrintKDTree(kd_tree);
    // PrintStarValues(star_range);
    // PrintStarValues(star_path);


    DeallocSubStarArray(star_path);
    // DeallocSubStarArray(star_range);
    DeallocHashMap(star_hash_map); 
    DeallocKDTree(kd_tree);

    /* -- WARNING -- */
    // ENSURE THIS IS CALLED LAST!!!
    // Star Array is the source of data for secondary data structures!
    DeallocMainStarArray(star_array);
}
