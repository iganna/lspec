""" This file contain functions to gec contributions of lspecs """


__author__ = "Anna Igolkina"
__license__ = "MIT"
__maintainer__ = "Anna Igolkina"
__email__ = "igolkinaanna11@gmail.com"

from lspec_db import LspecDatabase
import numpy as np
import os
from functools import reduce, partial
from itertools import product
import glob
from multiprocess import Pool
from Bio import SeqIO
# from Bio.SeqRecord import SeqRecord
# from Bio.Seq import Seq


class LspecSignals:

    def __init__(self, path_to_lspecs, path_to_test, path_to_signals=None):

        self.path_to_lspecs = path_to_lspecs
        self.path_to_test = path_to_test

        # path to results, signals
        if path_to_signals is None:
            self.path_to_signals = path_to_test + 'results/'
        else:
            self.path_to_signals = path_to_signals
        if not os.path.exists(self.path_to_signals):
            os.makedirs(self.path_to_signals)

        # Create tables to quantify signals
        files_test = glob.glob(self.path_to_test + '*.fasta')
        files_lspec = glob.glob(self.path_to_lspecs + '*.fasta')
        n_tests = len(files_test)
        n_lspecs = len(files_lspec)

        self.signal_n = np.empty((n_tests, n_lspecs))
        self.signal_m = np.empty((n_tests, n_lspecs))
        self.signal_N = np.empty(n_tests)
        self.signal_M = np.empty(n_lspecs)
        self.signal_p = np.empty((n_tests, n_lspecs))

    def intersect(self, n_threads=10, acc=99):
        """
        This function performed intersection of a test sample with each LSPEC
        in a parallel mode
        :param n_threads: number of threads
        :param acc: accuracy of intersection
        :return: None
        """

        files_unique = glob.glob(self.path_to_test + '*.fasta')
        part_ugene_intersect = partial(self.ugene_intersect, acc=acc)
        with Pool(n_threads) as workers:
            pmap = workers.map
            pmap(part_ugene_intersect, files_unique)

    def ugene_intersect(self, file_test, acc=99):
        """
        This function evokes UGENE script to intersect one test samples
        with one LSPEC
        :param file_test: file with a test sample
        :param acc: accuracy of intersection
        :return: None
        """

        print('ugene-spb/ugene' + \
                             ' --task=intersections' + \
                             ' --sample=' + file_test + \
                             ' --markers-dir=' + self.path_to_lspecs + \
                             ' --report=' + self.path_to_signals + \
                             'report' + os.path.basename(file_test)[:-6] + '.txt' + \
                             ' --sample-reports-dir=' + self.path_to_signals + \
                             ' --acc=' + str(acc))

        retvalue = os.system('ugene-spb/ugene' + \
                             ' --task=intersections' + \
                             ' --sample=' + file_test + \
                             ' --markers-dir=' + self.path_to_lspecs + \
                             ' --report=' + self.path_to_signals + \
                             'report' + os.path.basename(file_test)[:-6] + '.txt' + \
                             ' --sample-reports-dir=' + self.path_to_signals + \
                             ' --acc=' + str(acc))

        print(retvalue)

    def intersect_approxim(self, n_threads=10, acc=99):
        """
        This function performed intersection of a test sample with each LSPEC
        in a parallel mode via the same procedure as the LSPEC extraction
        :param n_threads: number of threads
        :param acc: accuracy of intersection
        :return: None
        """
        files_test = glob.glob(self.path_to_test + '*.fasta')
        files_lspec = glob.glob(self.path_to_lspecs + '*.fasta')

        with Pool(n_threads) as workers:
            pmap = workers.map
            pmap(lambda x:
                 self.ugene_intersect_approxim(x[0], x[1], acc),
                 product(files_test, files_lspec))





    def ugene_intersect_approxim(self, file_test, file_lspec, acc=99):
        """
        This function evokes UGENE script to intersect one test samples
        with one LSPEC via the same procedure as the LSPEC extraction
        :param file_test: file with a test sample
        :param acc: accuracy of intersection
        :return: None
        """
        retvalue = os.system('ugene-spb/ugene' +
                             ' --task=reduce.uwl' +
                             ' --accuracy=' + str(acc) +
                             ' --in-seqs=' + file_test +
                             ' --in-db=' + file_lspec +
                             ' --out=' + self.path_to_signals +
                             os.path.basename(file_test)[:-6] + '_' +
                             os.path.basename(file_lspec)[:-6] +
                             '_Nsignal.fasta')
        print(retvalue)

        retvalue = os.system('ugene-spb/ugene' +
                             ' --task=reduce.uwl' +
                             ' --accuracy=' + str(acc) +
                             ' --in-seqs=' + file_lspec +
                             ' --in-db=' + file_test +
                             ' --out=' + self.path_to_signals +
                             os.path.basename(file_lspec)[:-6] + '_' +
                             os.path.basename(file_test)[:-6] +
                             '_Msignal.fasta')
        print(retvalue)


    def calc_signals_approxim(self, path_to_tables):
        """
        This function calculates the main parameters to find signal of a LSPEC
        in a test library
        :param path_to_tables: path to results
        :return: None
        """

        if not os.path.exists(path_to_tables):
            os.makedirs(path_to_tables)

        files_test = glob.glob(self.path_to_test + '*.fasta')
        files_lspec = glob.glob(self.path_to_lspecs + '*.fasta')

        files_test.sort()
        files_lspec.sort()

        # Count number of sequences in initial test files
        for i_test, file_test in enumerate(files_test):
            records = list(SeqIO.parse(file_test, "fasta"))
            self.signal_N[i_test] = len(records)

        # Count number of sequences in initialLSPEC files
        for i_lspec, file_lspec in enumerate(files_lspec):
            records = list(SeqIO.parse(file_lspec, "fasta"))
            self.signal_M[i_lspec] = len(records)


        for i_test, i_lspec in product(range(len(files_test)), range(len(files_lspec))):
            file_test = files_test[i_test]
            file_lspec = files_lspec[i_lspec]

            # get n-values
            file_n_signal = self.path_to_signals + \
                            os.path.basename(file_test)[:-6] + '_' + \
                            os.path.basename(file_lspec)[:-6] + \
                            '_Nsignal.fasta'

            tmp_records = list(SeqIO.parse(file_n_signal, "fasta"))
            self.signal_n[i_test, i_lspec] = self.signal_N[i_test] - len(tmp_records)

            # get m-values
            file_m_signal = self.path_to_signals + \
                            os.path.basename(file_lspec)[:-6] + '_' + \
                            os.path.basename(file_test)[:-6] + \
                            '_Msignal.fasta'
            tmp_records = list(SeqIO.parse(file_m_signal, "fasta"))
            self.signal_m[i_test, i_lspec] = self.signal_M[i_lspec] - len(tmp_records)

        np.savetxt(path_to_tables + 'signal_n_size.txt', self.signal_N.astype(int), fmt='%i')
        np.savetxt(path_to_tables + 'signal_m_size.txt', self.signal_M.astype(int), fmt='%i')
        np.savetxt(path_to_tables + 'signal_n.txt', self.signal_n.astype(int), fmt='%i')
        np.savetxt(path_to_tables + 'signal_m.txt', self.signal_m.astype(int), fmt='%i')

        values_of_signals = self.calc_contributions()
        print(values_of_signals)

    def calc_contributions(self):
        pass
