# Musical Pong
## Computer Architecture
## Erik Macik

Musical Pong is a program that looks a lot like regular pong. But, rather than
playing against another person, the goal is to work together to play a song
perfectly. Every time the ball (or in this case a star) hits a paddle, you, as
a team, score a point and play a note. Play all the notes to win. If you and
your partner have even one mistake, you have to start over!

# How it works:
* Button S1 will move the left paddle up.
* Button S2 will move the left paddle down.
* Button S3 will move the right paddle up.
* Button S4 will move the righe padde

# How to run the project on an MSP430:
1. Download the "project" repository.
2. From this directory, run the make file to install all of the necessary
libraries and dependencies. This will also run the makefile in the pong directory.
3. Go to the pong directory.
4. Make sure your MSP430 is plugged into your computer and connected to the Virtual Machine.
5. Run the command "make load."
