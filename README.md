# Neco

A very Burunyuu wayland app.

![Neco](https://github.com/gitRaiku/neco/blob/master/neco.gif?raw=true)

Creates a DVD-Logo style bouncing window using zwlr shells in wayland.

# Usage
```
Usage: neco [-s/--scale <1.0>] [path/to/gif]
```

# Configuration

As with any suckless software configuration is done through the ``src/config.h`` file. I added support for compilation using the ``#embed`` directive to embed the played gif into the binary for maximum portability. (This requires a C23 compliant compiler such as gcc15)

# Compilation

Compilation is just a simple 
```make```

Running 
```make install```
will also copy the resulting binary to ``/usr/local/bin/``.

# Important Notice
We are all birds looking at god's computer monitor.
