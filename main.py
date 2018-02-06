from lspec_db import LspecDatabase
from lspec_signals import LspecSignals


# n_ref_samples = ['05', '10', '15', '20', '25', '29']
# n_ref_samples = ['00']
# for s in n_ref_samples:
#
#

s = '00'
path_to_db = 'data_soil/pure_ref/dsgn' + s + '/'
path_to_ugene = 'ugene-spb/'
d = LspecDatabase(path_to_db)
d.intersection(29)


# path_to_lspec = 'data_simulation/out_13/'
# path_to_test = 'data_simulation/test/'
# signals = LspecSignals(path_to_lspec, path_to_test)
# signals.intersect()


