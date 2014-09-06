# GTFS Playground

This is meant to be an experiment to work with csv's and ogr from a C context. At the moment it reads a gtfs shapes file (only tested with nyc) and outputs a shapefile with the geometries. The error checking bails the moment it receives something that is unexpected, so changes are it will blow up if you give it unclean data.

## Dependencies

* libcsv: http://sourceforge.net/projects/libcsv/
* glib: https://developer.gnome.org/glib/
* gdal: http://www.gdal.org/

Debian packages exist for glib and gdal, but I couldn't find one for libcsv. You can install that with the usual autotools cmmi dance.

    sudo apt-get install libgdal-dev libglib2.0-dev

## Usage

* Edit the Makefile to update the include/link flags for the locations of your libraries.
* Download gtfs data. Again, only tested with nyc. Put it in ./data/
  - http://web.mta.info/developers/
* Unzip the data into ./data
* `make clean run`
  - make run passes data/shapes.txt as a cli arg
* You should have a shapefile in ./out/
