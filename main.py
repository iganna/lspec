from lspec_db import LspecDatabase, normalize, calc_lspec_size
from lspec_signals import LspecSignals


def drange(start, stop, step):
    r = start
    while r < stop:
        yield r
        r += step
# --------------------------------------------------------------
# Normalize data
# --------------------------------------------------------------
# In this step you need to define a path with initial files
# then run the normalisation of initial libraries to a fixed numeber of
# sequences

path_to_initial = 'data/control_samples/'

path_to_db = []
n_norm_seqs = drange(1000, 10000, 2000)
for i, n_seq in enumerate(n_norm_seqs):
    path_to_normalized = path_to_initial + 'norm' + '{:02d}/'.format(i+1)
    path_to_db += [path_to_normalized]
    normalize(path_to_initial, path_to_normalized, n_seq)



# --------------------------------------------------------------
# Obtain LSPECs
# --------------------------------------------------------------
# For this procedure you need only to define a path with normalised samples
# db means database
# path_to_db = ...
# In this example  this path was defines in the previous section

#The path with UGENE scripts
path_to_ugene = 'ugene-spb/'

for path_db in path_to_db:
    print(path_db)
    d = LspecDatabase(path_db)
    #20 is a number of threads
    d.intersection(20)


# --------------------------------------------------------------
# Get number of sequences in LSPECs
# --------------------------------------------------------------

n_seqs = []
for path_db in path_to_db:
    path_to_lspecs = path_db + 'lspecs/'
    file_freq_table = path_db + 'database/db_table.txt'
    n_seqs += [calc_lspec_size(path_to_lspecs, file_freq_table)]

print(n_seqs)

# --------------------------------------------------------------
# Get signals of LSPECs in target samples
# --------------------------------------------------------------
# You need to set three paths:
# path_to_lspec - path to the folder with LSPECs
# path_to_test - path to the folder with target libraries
# path_to_signals - path with results

path_to_test = 'data/test_samples/'
for i, path_db in enumerate(path_to_db):
    path_to_lspecs = path_db + 'lspecs/'
    path_to_signals = path_to_test + 'result_' + str(i+1) + '/'
    signals = LspecSignals(path_to_lspecs, path_to_test, path_to_signals)
    signals.intersect_approxim(30)





