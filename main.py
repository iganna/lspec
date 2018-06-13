from lspec_db import LspecDatabase, normalize
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

path_to_initial = 'data_init/'

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

# The path with UGENE scripts
path_to_ugene = 'ugene-spb/'
for path_db in path_to_db:
    d = LspecDatabase(path_db)
    # 20 is a number of threads
    d.intersection(20)

#
# n_ref_samples = ['01', '02', '03', '04', '05']
# for s in n_ref_samples:
#     path_to_db = 'data_simulation/norm01/design_' + s + '/'
#
#     d = LspecDatabase(path_to_db)
#     d.intersection(20)


#
# n_ref_samples = ['01', '02', '03', '04', '05']
# for s in n_ref_samples:
#     path_to_lspec = 'data_simulation/norm01/design_' + s + '/' + 'lspecs/'
#     path_to_test = 'data_simulation/norm01/a_artificial/'
#     path_to_signals = path_to_test + 'result_' + s + '/'
#     signals = LspecSignals(path_to_lspec, path_to_test, path_to_signals)
#     # signals.intersect()
#     signals.intersect_approxim(30)

# n_ref_samples = ['01', '02', '03', '04', '05']
# for s in n_ref_samples:
#     path_to_lspec = 'data_simulation/norm01/design_' + s + '/' + 'lspecs/'
#     path_to_test = 'data_simulation/norm01/a_artificial/'
#     path_to_signals = path_to_test + 'result_' + s + '/'
#     path_to_tables = path_to_test + 'signals_' + s + '/'
#     signals = LspecSignals(path_to_lspec, path_to_test, path_to_signals)
#     signals.calc_signals_approxim(path_to_tables)






