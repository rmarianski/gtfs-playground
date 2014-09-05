#define _GNU_SOURCE
#include <assert.h>
#include <glib.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <csv.h>
#include <gdal/ogr_api.h>
#include <gdal/ogr_srs_api.h>
#include "shapes.h"

typedef struct current_shape_s {
    int field;
    char *shape_id;
    long sequence_id;
    double lat;
    double lng;
    shapes_s *shapes;
} current_shape_s;

double parse_double(char *s) {
    char *endptr;
    errno = 0;
    double d = strtod(s, &endptr);
    if (errno != 0) {
        perror("strtod");
        exit(EXIT_FAILURE);
    }
    if (endptr == s) {
        fprintf(stderr, "No value to parse: %s\n", s);
        exit(EXIT_FAILURE);
    }
    return d;
}

void csv_field_cb(void *s, size_t n, void *user_data) {
    current_shape_s *cur = user_data;
    char *t, *endptr;
    t = alloca(n + 1);
    memcpy(t, s, n);
    t[n] = '\0';

    switch (cur->field) {
        case 0:
            cur->shape_id = strdup(t);
            break;
        case 1:
            errno = 0;
            long sequence_id = strtol(t, &endptr, 10);
            if (errno != 0) {
                perror("strtol");
                exit(EXIT_FAILURE);
            }
            if (endptr == t) {
                fprintf(stderr, "No sequence id\n");
                exit(EXIT_FAILURE);
            }
            cur->sequence_id = sequence_id;
            break;
        case 2:
            cur->lat = parse_double(t);
            break;
        case 3:
            cur->lng = parse_double(t);
            break;
        default:
            fprintf(stderr, "Unexpected number of columns\n");
            fprintf(stderr, "%s - %d\n", t, cur->field);
            exit(EXIT_FAILURE);
    }
    cur->field++;
}

void csv_line_cb(int terminating_char, void *user_data) {
    current_shape_s *cur = user_data;
    if (cur->field < 4) {
        fprintf(stderr, "Not enough columns\n");
        exit(EXIT_FAILURE);
    }
    shapes_insert(cur->shapes, cur->shape_id, cur->sequence_id, cur->lat, cur->lng);
    cur->field = 0;
    free(cur->shape_id);
}

void csv_data_init(current_shape_s *cur) {
    memset(cur, 0, sizeof(current_shape_s));
    cur->shapes = malloc(sizeof(shapes_s));
    shapes_init(cur->shapes);
}

void csv_data_free(current_shape_s *cur) {
    shapes_free(cur->shapes);
    free(cur->shapes);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fputs("Missing path to csv\n", stderr);
        exit(EXIT_FAILURE);
    }
    char *csv_path = argv[1];
    FILE *csv_file = fopen(csv_path, "r");
    if (!csv_file) {
        perror("error reading csv");
        exit(EXIT_FAILURE);
    }

    struct csv_parser p;
    char *line = NULL;
    size_t buflen = 0;
    ssize_t linelen;

    csv_init(&p, 0);

    current_shape_s csv_data;
    csv_data_init(&csv_data);

    bool is_header = true;
    while ((linelen = getline(&line, &buflen, csv_file)) != -1) {
        if (is_header) {
            is_header = false;
            continue;
        }
        if (csv_parse(&p, line, linelen, csv_field_cb, csv_line_cb, &csv_data) != linelen) {
            fprintf(stderr, "Error parsing csv: %s\n", csv_strerror(csv_error(&p)));
            exit(EXIT_FAILURE);
        }
    }

    csv_fini(&p, csv_field_cb, csv_line_cb, &csv_data);

    free(line);
    csv_free(&p);
    fclose(csv_file);

    OGRRegisterAll();

    OGRSFDriverH shapefile_driver = OGRGetDriverByName("ESRI Shapefile");
    assert(shapefile_driver != NULL);

    OGRDataSourceH data_source = OGR_Dr_CreateDataSource(shapefile_driver, "out/gtfs.shp", NULL);
    assert(data_source != NULL);

    OGRSpatialReferenceH spatial_reference = OSRNewSpatialReference(NULL);
    assert(OSRImportFromEPSG(spatial_reference, 4326) == OGRERR_NONE);

    OGRLayerH layer = OGR_DS_CreateLayer(data_source, "shapes", spatial_reference, wkbLineString, NULL);
    assert(layer != NULL);

    OGRFieldDefnH field_definition = OGR_Fld_Create("shape_id", OFTString);

    OGR_Fld_SetWidth(field_definition, 32);

    assert(OGR_L_CreateField(layer, field_definition, TRUE) == OGRERR_NONE);

    OGR_Fld_Destroy(field_definition);

    OGRFeatureH feature;
    OGRGeometryH line_geom;

    shapes_s *shapes = csv_data.shapes;
    GHashTableIter iter;
    gpointer key, value;
    char *shape_id;
    shape_entries_s *shape_entries;
    g_hash_table_iter_init(&iter, shapes->by_id);

    while (g_hash_table_iter_next(&iter, &key, &value))  {
        shape_id = key;
        shape_entries = value;

        feature = OGR_F_Create(OGR_L_GetLayerDefn(layer));
        OGR_F_SetFieldString(feature, OGR_F_GetFieldIndex(feature, "shape_id"), shape_id);

        line_geom = OGR_G_CreateGeometry(wkbLineString);
        int n = shape_entries->n;
        shape_entry_s *entry;
        for (int i = 0; i < n; i++) {
            entry = &shape_entries->entries[i];
            OGR_G_AddPoint_2D(line_geom, entry->lng, entry->lat);
        }

        OGR_F_SetGeometry(feature, line_geom);
        OGR_G_DestroyGeometry(line_geom);

        assert(OGR_L_CreateFeature(layer, feature) == OGRERR_NONE);

        OGR_F_Destroy(feature);
    }

    OSRDestroySpatialReference(spatial_reference);
    OGR_DS_Destroy(data_source);

    csv_data_free(&csv_data);

    return 0;
}
