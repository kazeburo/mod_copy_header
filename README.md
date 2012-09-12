ABSTRACT
================

mod_copy_header is Apache 2.2 module for copy response header to note

INSTALLATION
================

download from http://github.com/kazeburo/mod_copy_header

    # apxs -c -i mod_copyheader.c


DOCUMENTATION
================

    Description: Enable CopyHeader module
    Syntax:      CopyHeaderActive on/off
    Context:     dir config
    Module:      mod_copy_header

    Description: set a header name to copy to note
    Syntax:      CopyHeader header
    Context:     dir config
    Module:      mod_bumpy_life

Example
----------------

    LoadModule copyheader_module   modules/mod_copyheader.so
    
    <Location /test>
      ProxyPass http://...
      <IfModule copyheader_module>
        CopyHeaderActive On
        CopyHeader X-Test
      </IfModule>
      Header unset X-Test
      LogFormat "... %{X-Testn}" xtest
      CustomLog "logs/access_log" xtest
    </Location>

COPYRIGHT & LICENSE
================

Copyright 2012 Masahiro Nagano

Apache License, Version 2.0
