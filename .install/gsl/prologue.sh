#!/bin/bash

LIB="gsl"
if [ $CLR == "true" ]; then rm -rf $CLR; fi
if [ ! -d "$SRC/$LIB" ]; then git clone --depth 1 "https://github.com/ampl/gsl" "$SRC/$LIB"; fi
