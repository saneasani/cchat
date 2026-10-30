CCC=gcc
CCFLAGS=-Wall -Wextra -I../ -fsanitize=address,undefined
CLEAN=rm -f
PROGRAM_NAME=run_server
OBJECT_FILES=*.o
SOURCE_FILES=source_server.c ../sys_server_msgs.c ../tools.c ../events.c ../entry_map.c ../common.c ../connections.c ../timers.c

$(PROGRAM_NAME): $(OBJECT_FILES)
	$(CCC) $(CCFLAGS) -o $@ $^

$(OBJECT_FILES): $(SOURCE_FILES)
	$(CCC) $(CCFLAGS) -c $^

clean:
	$(CLEAN) *.o $(PROGRAM_NAME)
