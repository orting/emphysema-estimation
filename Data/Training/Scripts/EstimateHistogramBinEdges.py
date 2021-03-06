#!/usr/bin/python3
'''See Data/Training/README.md'''

import sys, subprocess, datetime
from Util import intersperse

def main():
    scanLungListPath = 'FileLists/ScansLungs.csv'
    outfile = 'HistogramEdges_%s.txt' % datetime.datetime.now().timestamp()
    prog = '../../Build/Statistics/DetermineHistogramBinEdges'

    params = {
        'scales' : ['1.2', '2.4', '4.8'], 
        'samples' : '10000',
        'bins' : '49'
    }
    
    args = [
        prog,
        '--infile', scanLungListPath,
        '--outfile', outfile,
        '--samples', params['samples'],
        '--bins', params['bins'],
    ] + list(intersperse('--scale', (s for s in params['scales'])))
        
    print(' '.join(args))        
    if subprocess.call( args ) != 0:
        print( 'Error estimating histogram bin edges' )
        return 1
             
    return 0

if __name__ == '__main__':
    sys.exit( main() )



    
