""" This file contain functions to gec contributions of lspecs """
from lspec_db import LspecDatabase
import numpy as np
import os
from functools import reduce, partial
import glob
from multiprocess import Pool


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

    def intersect(self, n_threads=10, acc=99):

        files_unique = glob.glob(self.path_to_test + '*.fasta')
        part_ugene_intersect = partial(self.ugene_intersect, acc=acc)
        with Pool(n_threads) as workers:
            pmap = workers.map
            pmap(part_ugene_intersect, files_unique)

    def ugene_intersect(self, file_test, acc=99):
        """

        :param file_test:
        :param acc:
        :return:
        """

        retvalue = os.system('ugene-spb/ugene' + \
                             ' --task=intersections' + \
                             ' --sample=' + file_test + \
                             ' --markers-dir=' + self.path_to_lspecs + \
                             ' --report=' + self.path_to_signals + \
                             'report' + os.path.basename(file_test)[:-6] + '.txt' + \
                             ' --sample-reports-dir=' + self.path_to_signals + \
                             ' --acc=' + str(acc))

        print(retvalue)

    def calc_contributions(self):
        pass
