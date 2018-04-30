Project by Alex Medos

Will run in Debug x86 configuration

AI related code:

Line 181 to 366

3D model path finding:

Line 381 to 425

A* Heuristic:

Line 560 to 570

Controls:

WASD - Spins camera around the maze
(don't test these controls too hard, they're not very good)

Up, Down Arrows - Zooms camera in and out

The maze is created by reading from MAZE.txt. It creates a 10x10 grid.
A maze is comprised of 1's (walls) and 0's (floor), as well as a single G (goal)
and an S (start)

If the maze is constructed improperly, the program will stop.
If there is no path to the goal, the maze will render, but nothing will animate

There are some other mazes in other maze ideas.txt, if you want to try other layouts