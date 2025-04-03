#!/bin/bash
#SBATCH --job-name=gtg
#SBATCH --partition=Centaurus
#SBATCH --time=01:00:00
#SBATCH --mem=32G

record="record.txt"

./gsc "Tom Hanks" 1 >> $record
./gsc "Tom Hanks" 2 >> $record
./gsc "Tom Hanks" 3 >> $record
