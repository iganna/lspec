# Identifying components of mixed and contaminated soil samples by detecting specific signatures of control 16S rRNA libraries



## Description

The proposed algorithm copes with the problem determining whether two soil samples — test and control — have the same origin or source.
It first extracts the Library-SPECific sets of sequences (LSPECs) for alternative control libraries and then quantifies signals of LSPECs in a test library. 

## Pipeline

The pipeline is presented in main.py script. In brief, it contains three major parts:

*  Normalisation of control samples
```
from lspec_db import normalize
# Directory with control samples
path_to_controls = 'data/control_samples/'
# The output directory
path_to_normalized = path_to_initial + 'norm/'
# Number of samples for normalisation
n_seq = 10 ** 4
normalize(path_to_controls, path_to_normalized, n_seq)
```

* Extraction of LSPECs
```
# Directory with normalised samples
path_db = path_to_normalized
# Create an auxiliary structure 
d = LspecDatabase(path_db)
# Extract LSPECs
n_thr = 4  # numer of threads
d.intersection(n_thr)  
```

* Intersection of LSPECs with a test samples
```
# Directory with test samples 
path_to_test = 'data/test_samples/'
# Directory with results
path_to_signals = path_to_test + 'result'
# Create an auxiliary structure 
signals = LspecSignals(path_to_lspecs, path_to_test, path_to_signals)
# Perform intersection and write results
signals.intersect_approxim(n_thr)
```


## Requirements

To run BernMix methods you need Python 3.4 or later. 

## Authors

**Anna Igolkina**  [e-mail](mailto:igolkinaanna11@gmail.com).    



## License information

Open-sourced scripts licensed under the [MIT license](https://opensource.org/licenses/MIT).