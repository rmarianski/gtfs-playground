#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "shapes.h"

static void shapes_id_free(void *shape_id) {
    free(shape_id);
}

static void shapes_entries_free(void *void_shape_entries) {
    shape_entries_s *shape_entries = void_shape_entries;
    free(shape_entries->entries);
    free(shape_entries);
}

extern void shapes_init(shapes_s *shapes) {
    shapes->by_id = g_hash_table_new_full(g_str_hash, g_str_equal, shapes_id_free, shapes_entries_free);
}

extern void shapes_free(shapes_s *shapes) {
    g_hash_table_destroy(shapes->by_id);
}

extern void shape_entry_print(shape_entry_s *e, FILE *stream) {
    fprintf(stream, "%d|%f|%f", e->sequence_id, e->lat, e->lng);
}

extern void shape_entries_print(shape_entries_s *shape_entries, FILE *stream) {
    fprintf(stream, "%d [", shape_entries->n);
    for (int i = 0; i < shape_entries->n; i++) {
        shape_entry_print(&shape_entries->entries[i], stream);
        fputs(",", stream);
    }
    fprintf(stream, "]\n");
}

extern void shapes_insert(shapes_s *shapes, char *shape_id, int sequence_id, double lat, double lng) {
    gpointer result = g_hash_table_lookup(shapes->by_id, shape_id);
    shape_entries_s *shape_entries;
    if (result == NULL) {
        shape_entries = calloc(1, sizeof(shape_entries_s));
        g_hash_table_insert(shapes->by_id, strdup(shape_id), shape_entries);
    } else {
        shape_entries = result;
    }
    shape_entries->entries = realloc(
            shape_entries->entries, (shape_entries->n + 1) * sizeof(shape_entry_s));
    shape_entry_s *new_shape_entry = &shape_entries->entries[shape_entries->n++];
    *new_shape_entry = (shape_entry_s){.sequence_id = sequence_id, .lat = lat, .lng = lng};
}
