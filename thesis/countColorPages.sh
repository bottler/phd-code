#the printers need to know how many of the pages are in colour.
gs -o - -sDEVICE=inkcov j.pdf | grep -v "^ 0.00000  0.00000  0.00000" | grep "^ " | wc -l

