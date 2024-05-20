LIBS = -lglut -lGLU -lGL -lsndfile -lportaudio -lGLEW -lglfw -lfftw3

all: player player2

player: player.cpp
	g++ -o player player.cpp $(LIBS)

player2: player2.cpp
	g++ -o player2 player2.cpp $(LIBS)
