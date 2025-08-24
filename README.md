# TTWS: Teensy Tiny WebServer

A tiny epoll based webserver written in C.

Uses a trie tree for routing.

Definitely not safe to use as a library currently. Didn't start as a library so it has some process level things that should never happen, like _exiting the entire process_.

And for some reason I used global state (completetly unnecesarily). HTTP Parsing is laughable, and the epoll loop is not NON_BLOCKING lol.

I'll probably come back and do this up again because there's ground for a good little lib here. After it's nice and clean addomg SSE's and WebSocket's could make it fairly powerful. Oh, and threading! Don't forget the threading!
