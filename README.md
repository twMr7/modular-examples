modulize-example
================
Visual C++ example projects to demonstrate the building blocks of software modulization using *Poco C++ libraries*.

Prerequisites
-------------
- ***[Poco C++ libraries](https://github.com/pocoproject/poco)*** version 1.7.6
- ***[ZeroMQ](https://github.com/zeromq/libzmq)*** version 4.2.0

All projects here rely on the integration environment of [*vcpkg*](https://github.com/Microsoft/vcpkg) ports. Use of *vcpkg* is recommended for a quick start. Otherwise, it is necessary to modify the project settings for your own development environment.
 
Overview of The Examples
------------------------
- *AppLoadPlugin*: How a feature-rich application framework can provide without writing too many codes.
- *TaskStateControl*: Tasks and states coordinating in a higher level way.
- *MessageLink*: GUI and data logic are separate module. Two modules are communicated via network. 

