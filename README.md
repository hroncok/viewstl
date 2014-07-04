viewstl
=======

Simple lightview STL viewer.

Building
--------

To compile you'll need the admesh, opengl, glu, and glut libs and the gcc compiler for
your system. Then run `make`.

Using
-----

Using the program is simple! You type: `viewstl [filename] [option] [option]`.
If the file is valid, you will get the image on screen pretty quickly.  You may
choose one option from each set like: `-p -v`

Options and file name need to be given in the order shown if you run the
program with no arguments.  It won't crash if you break this rule, you just
won't get what you want.

Once the program is displaying something, you can manupulate the model by holding
the buttons (one at a time) and moving the mouse.  Left button is rotate, middle is
zoom, and right is pan.

You can choose different display modes using F4, F5, and F6.

Optionally you can try using the F1, F2, and F3 keys to emulate mouse clicks.
Because of the way GLUT works, this is not optimal, but it does appear to work.

Exit the program using the Esc key, or close the window.

If you have a GUI that supports drag and drop execution of files, you can put
the program or a link to it on the desktop, and drag files onto it.  The
default options are setup for this.

Run `./viewstl` to see all available options:

      viewstl: This is how you invoke the viewer... 
               Usage:  viewstl [file] (-o or -p) (-f or -v)
               Valid Options are: -o (Ortho View EXPEREMENTAL)
                                  -p (Perspective View)
                                  -f (Redraw only on view change)
                                  -v (Report debug info to STDOUT)
