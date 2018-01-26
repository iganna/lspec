import numpy as np
import os
import glob
from Bio import SeqIO
from Bio.SeqRecord import SeqRecord
from Bio.Seq import Seq
from collections import Counter
from itertools import compress
from functools import reduce, partial
from multiprocess import Pool



class LspecDatabase:
    """ This Class describes the Database of reference samples """
    def __init__(self, path_to_db):
        """

        :param path_to_db:
        """
        if not os.path.exists(path_to_db):
            raise ValueError('Cannot find a path to database')
        self.path_to_db = path_to_db
        self.path_to_info = path_to_db + 'database/'
        self.path_to_unique = path_to_db + 'database/unique/'
        self.path_to_common = path_to_db + 'database/common/'
        self.path_to_resid = path_to_db + 'database/residuals/'
        self.path_to_anti = path_to_db + 'database/residuals_anti/'
        self.path_to_lspecs = path_to_db + 'lspecs/'



        # Read fasta files, divide unique and common
        self.samples = []
        self.seq_list = []
        self.seq_names = []  # names of all sequences in the analysis
        self.seq_freqs = []  # table of frequencies of sequences in the whole database

        self.create_db()


    def create_db(self):
        """
        This functio read all sequences
        :return:
        """

        # TODO make
        db_files = glob.glob(self.path_to_db + '*.fasta')
        # db_files = os.listdir(self.path_to_db)
        if len(db_files) < 2:
            raise ValueError('Database must contain at least two reference samples')

        self.samples = [os.path.basename(f)[:-6] for f in db_files]

        # Get all sequences
        seqs = set()
        for f in db_files:
            records = list(SeqIO.parse(f, "fasta"))
            seqs |= {str(rec.seq) for rec in records}
        self.seq_list = list(seqs)
        self.seq_freqs = np.zeros((len(self.seq_list), len(self.samples)))

        # Get frequences of sequences within samples
        for i, f in enumerate(db_files):
            records = list(SeqIO.parse(f, "fasta"))
            seqs = [str(rec.seq) for rec in records]
            counter = Counter(seqs)
            self.seq_freqs[:, i] = [counter[s] if s in counter.keys() else 0
                                    for s in self.seq_list]

        # Renaming of sequences
        cnt_seqs_in_samples = np.zeros(len(self.samples)).astype(int)
        cnt_common_seqs = 0
        for i, freqs in enumerate(self.seq_freqs):
            if sum(freqs != 0) == 1:
                i_sample = list(freqs != 0).index(True)
                self.seq_names += ['New' + self.samples[i_sample] + '_' + \
                                   '{:06}'.format(cnt_seqs_in_samples[i_sample])]
                cnt_seqs_in_samples[i_sample] += 1
            else:
                self.seq_names += ['Common' + '{:09}'.format(cnt_common_seqs)]
                cnt_common_seqs += 1
        if len(self.seq_names) != len(self.seq_freqs):
            raise ValueError('Unexpectable error during the renaming of seqs')

        # Save Database
        self.save_db()

    def save_db(self, path_to_info=None):
        """
        Save a table (ONLY table) of names, seqs and frequences
        :param path_to_info:
        :return:
        """

        if path_to_info is None:
            path_to_info = self.path_to_info
        else:
            self.path_to_info = path_to_info

        if not os.path.exists(path_to_info):
            os.makedirs(path_to_info)

        with open(path_to_info + 'db_table.txt', 'w') as f:
            for name, seq, freqs in zip(self.seq_names, self.seq_list, self.seq_freqs):
                print(name, seq, freqs, file=f)

    def split_common_unique(self, path_to_unique=None, path_to_common=None):
        """

        :param path_to_unique:
        :param path_to_common:
        :return:
        """
        if path_to_unique is None:
            path_to_unique = self.path_to_unique
        else:
            self.path_to_unique = path_to_unique

        if path_to_common is None:
            path_to_common = self.path_to_common
        else:
            self.path_to_common = path_to_common

        if not os.path.exists(path_to_unique):
            os.makedirs(path_to_unique)
        if not os.path.exists(path_to_common):
            os.makedirs(path_to_common)

        # Get unique sequences for each sample
        seq_from_one_smpl = [sum(freqs > 0) == 1 for freqs in self.seq_freqs]
        for i, smpl_name in enumerate(self.samples):
            mask = (self.seq_freqs[:, i] > 0) & seq_from_one_smpl
            seqs = list(compress(self.seq_list, mask))
            names = list(compress(self.seq_names, mask))
            record = [SeqRecord(seq=Seq(s), id=name, description=name) for s, name in zip(seqs, names)]

            with open(path_to_unique + 'Unique' + smpl_name + '.fasta', "w") as f_out:
                SeqIO.write(record, f_out, "fasta")

        # Get common sequences for all samples
        seq_is_common = [sum(freqs > 0) > 1 for freqs in self.seq_freqs]
        seqs = list(compress(self.seq_list,  seq_is_common))
        names = list(compress(self.seq_names,  seq_is_common))
        record = [SeqRecord(seq=Seq(s), id=name, description=name) for s, name in zip(seqs, names)]
        with open(path_to_common + 'Common' + '.fasta', "w") as f_out:
            SeqIO.write(record, f_out, "fasta")

    def intersection(self, n_threads=10):

        # # Split unique and common sequences
        # self.split_common_unique()
        #
        # # First intersection
        # files_unique = glob.glob(self.path_to_unique + '*.fasta')
        # file_common = glob.glob(self.path_to_common + '*.fasta')[0]
        #
        # part_ugene_intersect = partial(self.ugene_intersect, file_db=file_common)
        # with Pool(n_threads) as workers:
        #     pmap = workers.map
        #     pmap(part_ugene_intersect, files_unique)


        # Creat anti-libraries
        self.creare_anti_lib()

        # Get files with residuals and anti-residuals
        files_resid = glob.glob(self.path_to_resid + '*.fasta')
        files_anti = glob.glob(self.path_to_anti + '*.fasta')

        with Pool(n_threads) as workers:
            pmap = workers.map
            pmap(lambda x:
                 self.ugene_intersect(x[0], x[1],
                                      path_to_resid=self.path_to_lspecs),
                 zip(files_resid, files_anti))




    def ugene_intersect(self, file_in_seqs, file_db, path_to_resid=None, acc=97):
        """

        :param file_in_seqs:
        :param file_db:
        :param path_to_resid:
        :param acc:
        :return:
        """
        if path_to_resid is None:
            path_to_resid = self.path_to_resid
        else:
            self.path_to_resid = path_to_resid

        if not os.path.exists(path_to_resid):
            os.makedirs(path_to_resid)

        retvalue = os.system('ugene-spb/ugene' + \
                  ' --task=reduce.uwl' + \
                  ' --accuracy=' + str(acc) + \
                  ' --in-seqs=' + file_in_seqs + \
                  ' --in-db=' + file_db + \
                  ' --out=' + path_to_resid+os.path.basename(file_in_seqs)[:-6] + '_res.fasta')
        print(retvalue)


    def creare_anti_lib(self, path_to_libs=None, path_to_antilib=None):
        """

        :param path_to_libs:
        :param path_to_antilib:
        :return:
        """

        if path_to_libs is None:
            path_to_libs = self.path_to_resid

        if path_to_antilib is None:
            path_to_antilib = self.path_to_anti

        if not os.path.exists(path_to_antilib):
            os.makedirs(path_to_antilib)

        # Read all libraries
        f_libs = glob.glob(path_to_libs + '*.fasta')
        libs = []
        for f in f_libs:
            records = list(SeqIO.parse(f, "fasta"))
            libs.append(records)

        # Create anti-libraries
        for i, f in enumerate(f_libs):
            anti_lib = reduce(lambda lib1, lib2: lib1 + lib2, records[:i]+records[i+1:])
            SeqIO.write(anti_lib, path_to_antilib + os.path.basename(f)[:-6] + '_anti.fasta', "fasta")












