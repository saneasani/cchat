CCC=gcc
CCFLAGS=-fsanitize=address,undefined
CLEAN=rm -f
PROGRAM_NAME=run_client
OBJECT_FILES=*.o
SOURCE_FILES=source_client.c tools.c events.c entry_map.c common.c connections.c 

$(PROGRAM_NAME): $(OBJECT_FILES)
	$(CCC) $(CCFLAGS) -o $@ $^

$(OBJECT_FILES): $(SOURCE_FILES)
	$(CCC) $(CCFLAGS) -c $^

clean:
	$(CLEAN) *.o $(PROGRAM_NAME)