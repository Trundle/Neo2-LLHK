================
Neo2 LLHK Driver
================

An alternative `Neo2 keyboard layout`_ implementation for Windows.


The Problem
===========

Theoretically, there already exists a correctly-implemented Neo2 keyboard
layout for Windows, `kbdneo <https://github.com/neo-layout/neo-layout/tree/master/windows/kbdneo2>`_.
Unfortunately, Neo2 uses four modifiers. While Windows keyboard layouts
support an (more or less) unlimited number of modifiers, the Universal
Windows Platform (UWP) only supports alt, shift and control as modifiers
and hence it's not really possible to implement Neo2 as keyboard layout.
It's not really clear if that is intentional or a bug (`see also
kbdedit's comment on the same issue
<http://www.kbdedit.com/manual/low_level_modifiers.html#uwp_limitations>`_).


The (somewhat hacky) Solution
=============================

Windows provides the `Low-Level Keyboard Hook`_ (hence the project's name)
that allows a callback to get called every time a new keyboard input is
about to be processed.
Neo2-LLHK uses the low-level keyboard hook to suppress most keyboard inputs
and instead injects custom keyboard inputs via
`SendInput <https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput>`_


Limitations
===========

``SendInput()`` is subject to `User Interface Privilege Isolation`_ and
hence it's only possible to inject keyboard input to applications that are
at an equal or lesser integrity level.

Neo2-LLHK also doesn't implement all of Neo2 (for example none of layers 5
and 6). This is purely due to the author's laziness and not a general
limitation.


Prior Art
=========

This project is heavily influenced by `david0's neo2-llhk <https://github.com/david0/neo2-llkh>`_.


License
=======

Neo2-LLHK is released under the Apache License, Version 2.0. See ``LICENSE``
or http://www.apache.org/licenses/LICENSE-2.0.html for details.


.. _Low-Level Keyboard Hook: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644985(v%3Dvs.85)
.. _Neo2 keyboard layout: https://www.neo-layout.org/
.. _User Interface Privilege Isolation: https://en.wikipedia.org/wiki/User_Interface_Privilege_Isolation