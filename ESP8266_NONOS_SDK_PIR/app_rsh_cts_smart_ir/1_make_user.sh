#!/bin/bash

echo "gen flash=1 Mbytes"
touch user/user_main.c

boot=new
app=1
spi_speed=2
spi_mode=0
spi_size_map=2

make clean

rm ../bin/upgrade/*.dump 
rm ../bin/upgrade/*.bin
rm ../bin/upgrade/*.S

make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map
mv ../bin/upgrade/user1.1024.new.2.bin ../bin/upgrade/user1.bin

