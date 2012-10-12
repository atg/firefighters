Codename: firefighters
============

2D multiplayer top-down flamethrowing game

## Server / Client Split

The server is in charge of most actions.

* Generating maps, new map supertiles to clients
* Working out who's dead and who's alive, and how much health they have

Both the server and client do collision detection. The s
