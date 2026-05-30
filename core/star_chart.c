#include <stdio.h>
#include <stdlib.h>
#include "star_chart_utils.h"

int main(void) {

    star_array_t *star_array = parse_file();

    star_t **star_ptr_array = calloc(star_array->size, sizeof(star_t*));
    for (size_t i = 0; i < star_array->size; i++) {
        star_ptr_array[i] = &star_array->stars[i];
    }
    kd_node_t *kd_tree = create_balanced_kd_tree(star_ptr_array, 0, star_array->size - 1, 0);
    // TODO: Apply this pattern to all deallocs (see SecureZero in utils)
    secure_zero(star_ptr_array, star_array->size * sizeof(star_t*));
    free(star_ptr_array);

    hash_map_t *star_hash_map = create_hash_map(star_array, star_array->size);

    star_t *closest_star = nearest_neighbor(kd_tree, player_position);
    printf("Closest star: %s\n\n", closest_star->name);   
    
    // star_path("* alf CMa", kd_tree, star_hash_map, star_array); // AKA Sirius
    // star_path("V* BY Dra", kd_tree, star_hash_map, star_array); // AKA BY Draconis
    // star_path("* tau Cet", kd_tree, star_hash_map, star_array); // AKA Tau Ceti
    // star_path("* eps Eri", kd_tree, star_hash_map, star_array); // AKA Epsilon Eridani
    // star_path("HD 1326", kd_tree, star_hash_map, star_array); // AKA Groombridge 34
    // star_path("* del Pav", kd_tree, star_hash_map, star_array); // Used for long distance testing
    star_path("G 182-36", kd_tree, star_hash_map, star_array); // Used to test unreachable fallback
    
    printf("-----------------------\n");
    // print_star_values(star_array);
    // print_kd_tree(kd_tree);
    // print_star_values(star_range);
    // print_star_values(star_path);


    // dealloc_sub_star_array(star_path);
    dealloc_hash_map(star_hash_map); 
    dealloc_kd_tree(kd_tree);

    /* -- WARNING -- */
    // ENSURE THIS IS CALLED LAST!!!
    // Star Array is the source of data for secondary data structures!
    dealloc_main_star_array(star_array);
}
