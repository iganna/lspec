from lspec_db import LspecDatabase

#
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


