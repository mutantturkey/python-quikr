#!/usr/bin/python

from multiprocessing import Pool
from Bio import SeqIO
import multiprocessing
from subprocess import *
import os
import gzip
import quikr as q
import sys
import numpy as np
import argparse
import platform
import csv

# our defaults
kmer = 6
lamb = 10000
output_directory = ""
input_directory = ""


def main():

  global kmer
  global input_directory 
  global output_directory 
  global lamb
  global trained_matrix 

  parser = argparse.ArgumentParser(description="MultifastaOTU")

  parser.add_argument("-i", "--input-directory", help="directory containing fasta files", required=True)
  parser.add_argument("-o", "--otu-table", help="otu_table", required=True)
  parser.add_argument("-t", "--trained-matrix", help="your trained matrix ", required=True)
  parser.add_argument("-f", "--trained-fasta", help="the fasta file used to train your matrix", required=True)
  parser.add_argument("-d", "--output-directory", help="quikr output directory", required=True)
  parser.add_argument("-l", "--lamb", type=int, help="the default lambda value is 10,000")
  parser.add_argument("-k", "--kmer", type=int, help="specifies which kmer to use, default=6")
  parser.add_argument("-j", "--jobs", type=int, help="specifies how many jobs to run at once, default=number of CPUs")
  args = parser.parse_args()
    
  # our defaults
  jobs=multiprocessing.cpu_count()
  trained_matrix = args.trained_matrix
  input_directory = args.input_directory
  output_directory = args.output_directory

  # Make sure our input exist
  if not os.path.isdir(args.input_directory):
    parser.error("Input directory not found")

  if not os.path.isdir(args.output_directory):
    print "Output directory not found, creating directory"
    os.path.mkdir(args, output_directory)

  if not os.path.isfile(args.trained_matrix):
    parser.error("Custom trained matrix not found")
    
  if not os.path.isfile(args.trained_fasta):
    parser.error("Fasta file of trained matrix not found")

  # use alternative lambda
  if args.lamb is not None:
    lamb = args.lamb
    
  if args.jobs is not None:
    jobs = args.jobs

  if args.kmer is not None:
    kmer = args.kmer

  # Load trained matrix
  if qu.isCompressed(args.trained_matrix):
    trained_matrix_file = gzip.open(args.trained_matrix, "rb")
  else:
    trained_matrix_file = open(args.trained_matrix, "rb")
  
  trained_matrix = np.load(trained_matrix_file)

  # Return a list of the input directory
  fasta_list = os.listdir(args.input_directory)

  # Queue up and run our quikr functions.
  pool = Pool(processes=jobs)
  results = pool.map(quikr_call, fasta_list)

  # Create an array of headers
  headers = []
  trained_matrix_headers = open(args.trained_fasta, "rU")
  for header in SeqIO.parse(trained_matrix_headers, "fasta"):
    headers.append(header.id)
  trained_matrix_headers.close()

  # create our number of reads matrix
  number_of_reads = np.zeros((len(headers), len(fasta_list)))

  # load the keys with values from each fasta result
  for fasta, fasta_it in map(None, fasta_list, range(len(fasta_list))):

    count_sequences = Popen(["grep", "-c" , "^>", args.input_directory + fasta], stdout=PIPE) 
    number_of_sequences = int(count_sequences.stdout.readline())

    proportions = np.loadtxt(output_directory + fasta);
    
    for proportion, proportion_it in map(None, proportions, range(len(proportions))):
      number_of_reads[proportion_it, fasta_it] = round(proportion * number_of_sequences)

  # remove empty rows from our matrix
  final_headers = list()
  final_data = list()
  for row, header in map(None, number_of_reads, headers):
    if row.sum() != 0:
      final_headers.append(header)
      final_data.append(row)

  # convert from a list back into a numpy array
  final_data = np.array(final_data, dtype=int)

  # stack our final header and our number_of_reads matrix
  number_of_reads = np.column_stack((final_headers, final_data))

  # write our OTU table
  output_file = open(args.otu_table, "wb") 
  writer = csv.writer(output_file, delimiter="\t")

  #write out our fasta file row
  writer.writerow(['# QIIME vGail OTU table'])

  fasta_row = ['#OTU_ID']
  fasta_row.append(' '.join(fasta_list))
  fasta_row = [' '.join(fasta_row)]
  writer.writerow(fasta_row)

  # write out our results
  for i in range(0, np.shape(number_of_reads)[0]):
      writer.writerow(list(number_of_reads[i]))

  output_file.close()

  return 0


def quikr_call(fasta_file):
  input_location = input_directory + fasta_file
  output_location = output_directory + os.path.basename(fasta_file)

  xstar = q.quikr(input_location, trained_matrix, kmer, lamb)
  np.savetxt(output_location, xstar, delimiter=",", fmt="%f")
  return xstar

if __name__ == "__main__":
  sys.exit(main())
 