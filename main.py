from lspec_db import LspecDatabase
from lspec_signals import LspecSignals




n_ref_samples = ['01', '02', '03', '04', '05']
for s in n_ref_samples:
    path_to_db = 'data_simulation/norm01/design_' + s + '/'
    path_to_ugene = 'ugene-spb/'
    d = LspecDatabase(path_to_db)
    d.intersection(20)



#
#
# path_to_lspec = 'data_simulation/lspecs/'
# path_to_test = 'data_simulation/test/'
# signals = LspecSignals(path_to_lspec, path_to_test)
# # signals.intersect()
# signals.intersect_approxim(29)


