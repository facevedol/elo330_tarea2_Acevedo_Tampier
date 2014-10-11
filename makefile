SOURCE=csa.c
CC=gcc
OUTPUT=csa
FLAGS= -o $(OUTPUT)
RM=rm
default:
	$(CC) $(FLAGS) $(SOURCE)

clean:
	$(RM) $(OUTPUT)
