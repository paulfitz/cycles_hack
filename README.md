Development moved to paulfitz/emcycles - this repo will be going away.

This is an ugly, egregiously hacked version of the Cycles renderer in
Blender 3D.  You don't want it.  I'm experimenting with stripping it
down beyond the point of sanity.  Current status:

    $ ldd app/cycles_test 
        linux-vdso.so.1 =>  (0x00007fffb26f5000)
        libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f13d19ed000)
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f13d176b000)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f13d1554000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f13d11cd000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f13d1d29000)

    $ ls -lh app/cycles_test 
    -rwxr-xr-x 1 paulfitz paulfitz 3.9M Apr 13 05:48 app/cycles_test

Running ./app/cycles_test will dump a series of ppm files into the
current directory, hopefully showing successively prettier elephants.