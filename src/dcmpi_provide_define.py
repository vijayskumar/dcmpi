#!/usr/bin/env python

reps = 99

n = 1
while n <= reps:
    name = 'dcmpi_provide'
    name += str(n)
    print '#define ' + name + '(',
    m = 1
    s = ''
    while m <= n:
        s += 'c' + `m` + ','
        m += 1
    s = s[0:-1]
    print s,
    print ') extern "C" DCFilter * dcmpi_get_filter(const char * fn) {',
    m = 1
    while m <= n:
        print 'if (!strcmp(fn,#c' + `m` + ')) { return new c' + `m` \
            + '; }',
        m += 1
    print 'return NULL;}',
    print 'extern "C" void dcmpi_get_contained_filters(std::vector<std::string> & o) {',
    print 'o.clear();',
    m = 1
    while m <= n:
        print 'o.push_back(#c' + `m` + ');',
        m += 1
    print '}'
    n += 1

    
        
        
