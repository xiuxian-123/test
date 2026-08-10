CC=gcc
OUTPUT=./out
SRC=./src
MKDIR=mkdir

build:
	$(MKDIR) $(OUTPUT) -p
	$(CC) $(SRC)/tools.c       -o $(OUTPUT)/tools -I./include
	$(CC) $(SRC)/calculator.c -o $(OUTPUT)/calculator
	# $(CC) $(SRC)/bmp.c       -o $(OUTPUT)/bmp -I./include
	$(CC) $(SRC)/bmp_review.c -o $(OUTPUT)/bmp_review -I./include -pthread
	$(CC) $(SRC)/pthread.c       -o $(OUTPUT)/pthread -I./include

remove:
	rm $(OUTPUT)/calculator
	rm $(OUTPUT)/tools
	rm $(OUTPUT)/pthread
	rm $(OUTPUT)/bmp
	rm $(OUTPUT)/bmp_review

