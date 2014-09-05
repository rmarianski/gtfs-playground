#ifndef SHAPES_H
#define SHAPES_H

typedef struct shape_entry_s {
    int sequence_id;
    double lat;
    double lng;
} shape_entry_s;

typedef struct shape_entries_s {
    int n;
    shape_entry_s *entries;
} shape_entries_s;

typedef struct shapes_s {
    GHashTable *by_id;
} shapes_s;

void shapes_init(shapes_s *shapes);
void shapes_free(shapes_s *shapes);
void shapes_insert(shapes_s *shapes, char *shape_id, int sequence_id, double lat, double lng);

#endif
